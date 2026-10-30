"""Summarize finalized Unreal CSV captures with explicit driving markers.

Includes scenery-loading frames during travel. Refuses legacy mixed captures
without markers and unfinished files. Timing scopes overlap: do not sum them.
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path


def summarize(path):
    csv.field_size_limit(64 * 1024 * 1024)
    with Path(path).open(encoding='utf-8-sig', newline='') as f:
        rows = [r for r in csv.reader(f) if r]
    if not rows or rows[-1][0] != '[HasHeaderRowAtEnd]':
        raise ValueError('Capture is not finalized; no driving qualification produced')
    headers = [r for r in rows if r[0] == 'EVENTS']
    if not headers:
        raise ValueError('Missing Unreal CSV header')
    header = headers[-1]
    marker = 'RailS7/DrivingFrame'
    if marker not in header:
        raise ValueError('Missing driving marker; legacy mixed captures are not comparable')
    if 'FrameTime' not in header:
        raise ValueError('Missing FrameTime')
    indices = {name: i for i, name in enumerate(header)}
    frames = []
    invalid = 0
    excluded = 0
    for row in rows:
        if row[0] == 'EVENTS' or row[0].startswith('['):
            continue
        try:
            driving = float(row[indices[marker]])
            if driving not in (0, 1):
                raise ValueError('Invalid marker')
            if driving == 0:
                excluded += 1
                continue
            frame = float(row[indices['FrameTime']])
            if not math.isfinite(frame) or frame <= 0:
                raise ValueError('Invalid frame time')
            frames.append(row)
        except (ValueError, IndexError):
            invalid += 1
    if invalid:
        raise ValueError(f'{invalid} malformed frame rows; refusing a partial summary')
    if not frames:
        raise ValueError('No driving frames recorded')
    result = {'scope': 'Driving including loading transitions; excludes menus, explicit pauses and background',
              'finalized': True, 'drivingFrames': len(frames), 'excludedFrames': excluded,
              'notes': 'CPU scopes overlap; correlation is not proof of causation. Compare only matched device, build settings, route, camera and capture interval.',
              'metrics': {}}
    names = ['FrameTime', 'GameThreadTime', 'RenderThreadTime', 'RHIThreadTime', 'GPUTime',
             'RailS7/GameThread/ContinuityMs', 'RailS7/GameThread/CheckpointMs',
             'RailS7/GameThread/TelemetryMs', 'RailScenery/GameThread/BuildCellMs',
             'RailScenery/GameThread/RemoveCellMs',
             'RailStations/GameThread/PresentMs', 'RailStations/GameThread/RebuildMs',
             'RailStations/GameThread/BuildBatchMs', 'RailChunkLoading/GameThread/CoachLoadMs']
    # Unreal continuous CSV writing appends columns as stats first occur.
    # Earlier rows omit those trailing values; they are zero, not lost frames.
    def value(row, index):
        return float(row[index]) if index < len(row) else 0.0
    for name in names:
        if name not in indices:
            continue
        try:
            values = sorted(value(row, indices[name]) for row in frames)
        except (ValueError, IndexError) as exc:
            raise ValueError(f'Malformed {name} samples') from exc
        if any(not math.isfinite(v) or v < 0 for v in values):
            raise ValueError(f'Invalid {name} samples')
        result['metrics'][name] = {'meanMs': statistics.fmean(values),
                                  'p95Ms': values[math.ceil(len(values)*.95)-1],
                                  'p99Ms': values[math.ceil(len(values)*.99)-1],
                                  'maxMs': values[-1], 'over100ms': sum(v > 100 for v in values)}
    result['hitchRows'] = []
    for row in sorted(frames, key=lambda r: float(r[indices['FrameTime']]), reverse=True)[:20]:
        result['hitchRows'].append({name: value(row, i) for name, i in indices.items()
                                   if name in names or name in ('RailS7/PresentationBlocked',
                                   'RailS7/PresentationTransition', 'RailScenery/CellsBuilt',
                                   'RailStations/Rebuilds', 'RailStations/ComponentsBuilt',
                                   'RailStations/InstancesBuilt', 'RailStations/ComponentsRetired',
                                   'RailChunkLoading/CoachPartsBuilt')})
    return result


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('capture', type=Path)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    report = summarize(a.capture)
    # Refuse overwriting earlier evidence.
    with a.output.open('x', encoding='utf-8') as f:
        json.dump(report, f, indent=2)
    print(json.dumps(report, indent=2))
