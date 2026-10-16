"""Game-owned Unreal capture adapter. No invented timings or synthetic screenshots."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
run = Path(os.environ['GAME_DEV_RUN_DIR'])
run.mkdir(parents=True, exist_ok=True)
native = run / 'native'
saved = ROOT / '03_UnrealProject/MyProject/Saved'

def hashes():
    result = {}
    for family in ('S7','S8/Egmore/Service','S8/EgmoreTambaram/Service','S8/EgmoreTambaram/ServiceV5','S8/EgmoreTambaram/ServiceV6','S8/LHB/Service'):
        for slot in (0,1):
            p = saved/family/f'checkpoint-{slot}.irs'
            result[str(p)] = hashlib.sha256(p.read_bytes()).hexdigest() if p.exists() else None
    return result

before = hashes()
full = os.environ.get('TAMBARAM_FULL_JOURNEY') == '1'
args = [r'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe',
        str(ROOT/'03_UnrealProject/MyProject/MyProject.uproject'),
        '/Game/IndianRailway/S7/L_RailService','-game','-S8Tambaram','-S8AssetReview','-S8Cab',
        '-unattended','-nosplash','-RenderOffscreen','-ResX=1280','-ResY=720','-ForceRes',
        '-UseFixedTimeStep','-FPS=30',f'-TambaramEvidence={native}',
        '-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview',f'-abslog={run / "engine.log"}']
if os.environ.get('TAMBARAM_WHEEL_REVIEW') == '1':
    args.append('-TambaramWheelReview')
if os.environ.get('TAMBARAM_ART_REVIEW') == '1':
    args.append('-TambaramArtReview')
motion=os.environ.get('TAMBARAM_MOTION_REVIEW') == '1'
if motion:
    args.append('-TambaramMotionReview')
    if os.environ.get('TAMBARAM_AUDIO_STRESS') == '1':
        args.append('-TambaramAudioStressReview')
    if os.environ.get('TAMBARAM_PASSING_AUDIO') == '1':
        args.append('-TambaramPassingAudioReview')
    args[args.index('-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview')]='-ExecCmds=t.MaxFPS 0,r.VSync 0,Rail.TambaramDesktopReview'
    if os.environ.get('TAMBARAM_RECORD_AUDIO') == '1':
        # Offscreen review has no focused window. Override only this process's
        # engine config so the master recorder does not capture focus-muted zeroes.
        args.extend(['-TambaramRecordAudio','-AudioMixer','-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0'])
        if os.environ.get('TAMBARAM_AUDIO_CAB') == '1':
            args.append('-TambaramAudioCabReview')
        args[args.index('-ExecCmds=t.MaxFPS 0,r.VSync 0,Rail.TambaramDesktopReview')]='-ExecCmds=t.MaxFPS 30,r.VSync 0,Rail.TambaramDesktopReview'
if full:
    args.append('-TambaramFullJourney')
    if os.environ.get('TAMBARAM_RECORD_AUDIO') == '1':
        args.extend(['-TambaramRecordAudio','-AudioMixer','-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0'])
    # Three ordinary 30Hz physics steps per rendered frame, inside the runtime's
    # four-step bound. This reduces capture overhead without teleporting trains.
    args[args.index('-FPS=30')] = '-FPS=10'
    args[args.index('-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview')] = '-ExecCmds=t.MaxFPS 0,r.VSync 0,Rail.TambaramDesktopReview'
with (run/'process.log').open('w',encoding='utf-8') as log:
    proc = subprocess.Popen(args,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        exit_code = proc.wait(timeout=5700 if full else (650 if motion else 330))
    except subprocess.TimeoutExpired:
        proc.kill();proc.wait();raise RuntimeError('Bounded Unreal origin capture timed out')
after = hashes()
(run/'save-preservation.json').write_text(json.dumps(dict(before=before,after=after,unchanged=before==after),indent=2))
engine_log = (run/'engine.log').read_text(encoding='utf-8', errors='replace')
if 'Failed to compile Material for platform' in engine_log:
    raise RuntimeError('Native capture used fallback materials; inspect engine.log, not visual acceptance')
result_path=native/'result.json'
if exit_code or not result_path.exists():
    raise RuntimeError(f'Unreal exit={exit_code}; result exists={result_path.exists()}')
result=json.loads(result_path.read_text(encoding='utf-8-sig'))
frames=[dict(index=i,label=p.stem,attachments=[dict(kind='color',path=p.relative_to(run).as_posix(),encoding='png')])
        for i,p in enumerate(sorted(native.glob('*.png')))]
capture=dict(schema='game_dev.capture.v1',runId=os.environ['GAME_DEV_RUN_ID'],
             adapterId=os.environ['GAME_DEV_ADAPTER_ID'],scenarioId=os.environ['GAME_DEV_SCENARIO_ID'],
             sourceFormat='game-dev-capture-v1',frames=frames,measurements=[],
             adapterEvidence=dict(windowless=True,graphicsApi='Unreal default desktop RHI; inspect engine.log',
                 notes=['Native Unreal FScreenshotRequest output; full recorded encounter uses real-time cadence, with accelerated fixed steps outside the encounter.',
                        ('Full journey requested; inspect native result and streaming-reload evidence.' if full else ('Disposable forward/reverse/braking motion review.' if motion else 'Origin smoke check only.')),
                        'Audio-only synthetic 100 km/h, curve and repeated horn overlap fixture; not physical high-speed driving evidence.' if os.environ.get('TAMBARAM_AUDIO_STRESS') == '1' else 'Normal state-driven audio.',
                        'Synthetic AI pass-by audio trajectory using runtime spatial mixer; actual AI route unchanged.' if os.environ.get('TAMBARAM_PASSING_AUDIO') == '1' else 'No synthetic pass-by.',
                        'Not human visual acceptance or hardware performance certification.']))
(run/'capture.json').write_text(json.dumps(capture,indent=2))
if not result['passed'] or before!=after or (len(frames)<6 if full else len(frames)!=(8 if motion else 4)):
    raise RuntimeError('Native review, frame count or save preservation failed; inspect native/result.json')
if full:
    subprocess.run([sys.executable, str(Path(__file__).with_name('audit_station_capture_timing.py')), str(run)], check=True)
if motion and os.environ.get('TAMBARAM_RECORD_AUDIO') == '1':
    subprocess.run([sys.executable, str(Path(__file__).with_name('audit_audio_capture.py')), str(native/'motion-audio.wav')], check=True)
if full and os.environ.get('TAMBARAM_RECORD_AUDIO') == '1':
    subprocess.run([sys.executable, str(Path(__file__).with_name('audit_audio_capture.py')), str(native/'passing-service-audio.wav')], check=True)
    subprocess.run([sys.executable, str(Path(__file__).with_name('audit_opposing_service.py')), str(run), '--require-audio'], check=True)
print(json.dumps(dict(passed=True,frames=len(frames),normalSavesUnchanged=True)))
