"""Build an evidence ledger without inventing human observations."""
import json
import pathlib
import re
root=pathlib.Path(__file__).resolve().parents[3]
stage=root/'07_Documentation/S7'
ev=stage/'Evidence'
brief=pathlib.Path('C:/Users/nisha/.codex/attachments/a32596b0-0ee2-42a4-b634-34247a14bed3/Pasted text.txt').read_text(encoding='utf-8-sig')
host=json.loads((ev/'service-tests.json').read_text(encoding='utf-8-sig'))
assert host['passed'] and len(host['cases'])==46
automation=json.loads((ev/'UnrealAutomation/index.json').read_text(encoding='utf-8-sig'))
assert automation['succeeded']==6 and automation['failed']==0
names={c['name']:c['passed'] for c in host['cases']}
mapping={1:'definition',2:'selection',3:'confirm-formation',4:'start',5:'input',6:'existing-AI',7:'real-traffic-hold',8:'real-traffic-hold',9:'rear-clear-allows-player',10:'station-state-contract',11:'platform-too-short',12:'berth',13:'overshoot-failure',14:'dwell',15:'departure',16:'objective-events',17:'delay-attribution',18:'forward-service-safety-failure',19:'retry',20:'complete-itinerary',21:'explained-result',22:'settlement',23:'duplicate-settlement',24:'interrupted-settlement-recover',27:'cold-restore-full-state',29:'replay-preserves-profile'}
tests=[]
for number,title in re.findall(r'S7-T(\d+)\s*\n([^\n]+)',brief):
    n=int(number)
    evidence=['Evidence/service-tests.json#S7-'+mapping[n]] if n in mapping else []
    if n in mapping: assert names['S7-'+mapping[n]]
    if n in (25,26,27): evidence.append('Evidence/Device/initial-lifecycle-results.json')
    if n in (28,36,37,38,39,40,41): evidence+=['Evidence/Device/delivery-loop/analysis.json','Evidence/paid-result-recovery.json','Evidence/apk-permissions.txt']
    if n==24: evidence+=['Evidence/pending-result-recovery.json','Evidence/paid-result-recovery.json']
    if 30<=n<=34: evidence.append('Evidence/host-results.txt')
    if n==35: evidence.append('Evidence/UnrealAutomation/index.json')
    if n==42: evidence+=['source-change-manifest.json','../../03_UnrealProject/MyProject/Source/MyProject/Simulation/RailService.cpp']
    tests.append(dict(id='S7-T'+number,title=title.strip(),status='PASS',evidence=evidence))
assert len(tests)==42
(stage/'required-tests.json').write_text(json.dumps(tests,indent=2),encoding='utf-8')
acceptance=brief.split('S7 is PASS only if:')[1].split('RESOURCE / CREDIT DISCIPLINE')[0]
gates=[]
for number,title in re.findall(r'^(\d+)\. (.+)$',acceptance,re.M):
    n=int(number)
    gates.append(dict(id=n,title=title.strip(),status='PENDING' if n in (7,33) else 'PASS',reason='Real participant comprehension/observations not supplied' if n in (7,33) else 'See required-tests.json and implementation report'))
assert len(gates)==36
(stage/'acceptance.json').write_text(json.dumps(dict(result='FAIL',accepted=34,total=36,gates=gates),indent=2),encoding='utf-8')
report=(stage/'S7_IMPLEMENTATION_REPORT.md').read_text(encoding='utf-8-sig')
report=report.replace('# S7 implementation report — verification in progress','# S7 implementation report')
start=report.index('**S7 FAIL')
end=report.index('## 1.')
report=report[:start]+'''**S7 FAIL — Acceptance 34/36.** Implementation and technical verification are complete: 238 host cases,
238 final Android cases and six Unreal Automation suites pass. Gates 7 (real-player comprehension of the restriction)
and 33 (six-person usability) remain pending; no participant feedback is fabricated.
The stage is not accepted and S8 has not begun.

'''+report[end:]
def section(number,title,body):
    global report
    report=re.sub(r'## '+str(number)+r'\..*?(?=\n## |\Z)',f'## {number}. {title}\n{body.strip()}\n',report,flags=re.S)
section(13,'Offline behaviour','''The packaged APK has no INTERNET permission (`Evidence/apk-permissions.txt`).
The complete phone loop, save/recovery, settlement and replay ran with this
restriction. The phone hotspot remained on; airplane mode was not required.
There is no account, online entitlement or network-dependent objective.''')
section(14,'UI/tutorial','''Selection, formation confirmation, mode choice, pause, result, retry and replay
use touch widgets. Throttle/brake presets, emergency/reset, horn and look-back
send ordinary commands. HUD labels explain speed, authority, traffic, stop and
dwell. Final corrections remove clipped button labels and menu debug overlap,
and show “no valid station stop recorded” after a missed berth. Results include
the domain-calculated score components. Screenshots are retained under Evidence/Device.
Unaided comprehension still needs real-player observations.''')
section(17,'Android verification','''Target: iQOO Neo9 Pro / I2304, Android 16, ARM64 Vulkan/ASTC Development build.
Device evidence includes real AI restriction/release, two successful Assisted
loops, a Standard overshoot failure, retry/profile preservation, complete-state
pause/warm/cold recovery and pending/paid result recovery. The fresh recorded
run finished at grade A, score 98.534885, berth error -0.195349 m, 300 dwell ticks,
all nine objectives, zero player lateness and 175.833333 s traffic hold.
Host tests also complete a successful Standard loop through ordinary commands.
`required-tests.json` maps all 42 requested tests. A truncated-write settlement
is fault-injected in the shared transaction host test; phone tests verify its
pending and paid recovery endpoints, not a precisely timed physical write tear.
Final APK identity is in Evidence/apk-hash-release-candidate.json. Installation
and final cold launch are verified. All nine regenerated Android reports parse
strictly and pass 238 cases (Evidence/android-tests-summary.json). The corrected
report writer also passes strict parsing across all nine host reports. Final
selection screenshot and telemetry preserve reputation 20, two awards and the
unlocked badge. The temporary profiling override is absent. Evidence is under
Device/final-selection and final-profile-override.txt.''')
section(18,'Usability findings','''Pending. No six-person observations have been received. The anonymous CSV is
an empty template, not evidence. USABILITY_PROTOCOL.md requires three casual
players and three enthusiasts, excluding the developer; it records hints,
speed/traffic/station/result understanding, resume and voluntary replay. Audio
audibility/comprehension must be recorded by those listeners. Automated ADB
runs and synthetic-sound generation do not establish these human findings.''')
section(19,'Performance results','''A finalized 12,000-frame capture contains 407.75 s. The selected 100–320 s
window is within the fresh running service (after initial menus and before its
result). It averages approximately 29.965 FPS at the 30 FPS cap, with five frames
over 50 ms and none over 100 ms. See the CSV summary for exact CPU/GPU and service
update distributions. Device PSS samples were approximately 446.47–452.93 MiB;
Android thermal status was consistently 2 while the hotspot remained active.
These are engine frame intervals, not compositor presentation times or proof of
production-content performance. The capture APK is recorded in apk-hash-delivery.json;
the later final APK changes only paused-menu text/label presentation in
RailServiceApplication.cpp, plus JSON escaping in the test-report writer. Active driving, simulation, content and rendering
workload are unchanged. This provenance is explicit rather than attributing the
capture to a different binary.''')
section(20,'Problems encountered','''The resumed source lacked its widget. Initial reverse-breach testing showed
S4 correctly prevented the breach. Overload debt had no player recovery action.
Phone inspection exposed auto-wrapped/clipped button text, menu/debug overlap,
a technical fresh-save diagnostic in normal UI and a misleading zero berth error
after an invalid stop. Foreground guards stopped touches when Android Settings
was open. The evidence audit also found unescaped JSON in embedded test details; the shared writer was corrected and all reports strictly parsed. Sparse phone monitoring missed the brief Arrival state; fixed-step host
tests verify all seven transitions. Build caches required access outside the workspace.''')
section(21,'Fixes','''Implemented the missing widget and Automation wrapper; preserved S4 protection
and added a forward-only service rule. Added explicit checkpoint restoration
after clock overload, mode and score-weight validation, Standard completion
coverage, emergency reset and background movement-audio stop. UI fixes use
unwrapped button labels, a menu background, hidden debug labels and clear invalid
berth wording. Scoring components come from the same domain calculation as the grade.''')
section(22,'Known limitations','''One short fixture-based service, one station, provisional train/platform meshes
and three synthetic cues; no claim of production visuals, broad hardware support
or future-route scale. Expert remains a reserved mode boundary. Full usability
and subjective audio evaluation are outstanding. Development startup executes
regression tests. Sparse device samples do not replace fixed-step transition tests.
The save compatibility key includes the exact authored service configuration;
future content migrations are not implemented.''')
table='\n'.join(f"| {g['id']} | {g['title']} | {g['status']} |" for g in gates)
section(23,'Acceptance checklist',f'''46/46 S7 host cases, 192/192 earlier regression cases, and 6/6 Unreal
Automation suites pass. These counts are separate from the 36 acceptance gates.
All 42 named test mappings and evidence references are in required-tests.json.
Human understanding is not inferred from visible labels or automated success.

| Gate | Requirement | Status |
|---|---|---|
{table}

**S7 FAIL — Acceptance 34/36**, pending real-player gates 7 and 33.''')
performance=json.loads((ev/'Device/delivery-profile/Saved/Profiling/CSV/Profile(20260928_122055).summary.json').read_text())
rows=[]
for name in ['FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RailS7/ServiceUpdateMs']:
    counter=performance['counters'][name]
    rows.append(f"| {name} | {counter['mean']:.3f} | {counter['p95']:.3f} | {counter['p99']:.3f} |")
table='\n| Counter (ms) | Mean | p95 | p99 |\n|---|---:|---:|---:|\n'+'\n'.join(rows)+'\n\nCPU lanes overlap and may include waits; do not sum them.\n'
report=report.replace('\n## 20.',table+'\n## 20.')
(stage/'S7_IMPLEMENTATION_REPORT.md').write_text(report,encoding='utf-8')
print('Report and test ledger generated; S7 remains FAIL, 34/36, pending human observations.')

