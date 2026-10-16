"""Analyze UE CSV with duplicate column names and footer metadata preserved.
Usage: python analyze_csv.py capture.csv [warmup_seconds=10]
No invented GPU samples: zero-only counters are marked UNVALIDATED.
"""
import csv
import json
import math
from pathlib import Path
import statistics
import sys

source = Path(sys.argv[1])
csv.field_size_limit(16 * 1024 * 1024)
warmup = float(sys.argv[2]) if len(sys.argv) > 2 else 10.0
end_seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 170.0
with source.open(encoding='utf-8-sig', newline='') as handle:
    contents = list(csv.reader(handle))
    assert contents[-1] and contents[-1][0] == '[HasHeaderRowAtEnd]', 'Capture is incomplete; wait for CSV finalization'
    headers = [row for row in contents if row and row[0] == 'EVENTS']
    header = headers[-1]
    assert all(header[:len(h)] == h for h in headers), 'Unexpected CSV column reordering'
    frame_index = header.index('FrameTime')
    rows = []
    for row in contents:
        try:
            frame_ms = float(row[frame_index])
        except (ValueError, IndexError):
            continue
        if math.isfinite(frame_ms) and frame_ms > 0:
            rows.append(row + ['0'] * (len(header) - len(row)))

def summary(values):
    ordered = sorted(values)
    def percentile(p):
        return ordered[max(0, math.ceil(len(ordered) * p) - 1)]
    return dict(count=len(values), mean=statistics.fmean(values),
                p50=percentile(.50), p95=percentile(.95), p99=percentile(.99),
                min=min(values), max=max(values))

elapsed = 0.0
steady = []
for row in rows:
    elapsed += float(row[frame_index]) / 1000
    if warmup < elapsed <= end_seconds:
        steady.append(row)
assert steady, 'No post-warmup data'
result = dict(source=str(source), total_frames=len(rows), total_frame_seconds=elapsed,
              excluded_warmup_seconds=warmup, active_window_end_seconds=end_seconds, percentile_method='nearest rank',
              interpretation='Engine frame intervals; CPU lanes overlap and can include waits. Not compositor presentation timing or final-game performance.',
              counters={})
for name in ('FrameTime', 'GameThreadTime', 'RenderThreadTime', 'RHIThreadTime',
             'GameThreadTime_CriticalPath', 'RenderThreadTime_CriticalPath', 'GPUTime',
             'RailS7/ServiceUpdateMs', 'RailS6/AIUpdateMs', 'RailS6/SimulationStepMs', 'PhysicalUsedMB', 'AndroidCPU/CPUTemp', 'AndroidCPU/ThermalStatus'):
    if name not in header:
        result['counters'][name] = dict(status='UNVALIDATED', reason='Counter absent')
        continue
    values = [float(row[header.index(name)]) for row in steady]
    if not any(values) and name in ('GPUTime', 'AndroidCPU/CPUTemp'):
        result['counters'][name] = dict(status='UNVALIDATED', reason='Counter contains only zeros')
    else:
        result['counters'][name] = dict(status='CAPTURED', **summary(values))
frames = [float(row[frame_index]) for row in steady]
result['average_fps'] = 1000 / statistics.fmean(frames)
result['frames_over_50ms'] = sum(value > 50 for value in frames)
result['frames_over_100ms'] = sum(value > 100 for value in frames)
result['steady_frame_seconds'] = sum(frames) / 1000
for name in ('View/PosX', 'View/PosY', 'View/PosZ', 'View/ForwardX', 'View/ForwardY', 'View/ForwardZ'):
    index = header.index(name)
    values = [float(row[index]) for row in rows]
    result[name] = dict(first=values[0], last=values[-1], min=min(values), max=max(values))
output = source.with_suffix('.summary.json')
output.write_text(json.dumps(result, indent=2), encoding='utf-8')
print(json.dumps(result, indent=2))




