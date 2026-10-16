"""Validate collected device evidence; missing evidence is a failure, never a pass."""
import json
import re
from validate_s8 import DOC,load

E=DOC/'Evidence';D=E/'Device';checks={};details={}
def record(name,condition): checks[name]=bool(condition)
try:
    before=load(D/'pre-install/Saved/S7/telemetry.json')
    first=load(D/'first-launch/telemetry.json')
    final=load(D/'final-launch/telemetry.json')
    runtime=load(D/'final-launch/S8/foundation-runtime.json')
    profile=load(D/'foundation-profile/measurement.json')
    completion=load(D/'foundation-profile/completion.json')
    restoration=load(D/'foundation-profile/commandline-restore.json')
    artifact=load(E/'artifact.json')
    install=(E/'install.log').read_text(encoding='utf-8-sig')
    record('installHashVerified',artifact['sha256'].lower() in install.lower() and re.search(r'(?m)^Success\s*$',install))
    record('packagedRuntime',runtime['passed'] and not runtime['failures'])
    record('S7SaveAcrossInstall',first['logicalFingerprint']==before['logicalFingerprint'] and first['paused'])
    record('S7SaveAfterProfile',final['logicalFingerprint']==before['logicalFingerprint'] and final['paused'])
    record('progressionPreserved',before['service']['profile']==first['service']['profile']==final['service']['profile'])
    record('logicalValidity',all(s['logical']['valid'] and not s['logical']['faulted'] and not s['blocked'] for s in [first,final]))
    reports=[]
    expected={'kernel-tests':39,'handling-tests':28,'operations-tests':39,'ai-tests':36,'content-tests':7,'record-tests':15,'world-snapshot-tests':19,'checkpoint-tests':9,'service-tests':46}
    for name,count in expected.items():
        report=load(D/f'final-launch/S7/{name}.json')
        passed=report['passed'] and len(report['cases'])==count and all(c['passed'] for c in report['cases'])
        record(name,passed);reports.append(dict(name=name,count=len(report['cases']),passed=passed))
    details['androidRegressionCases']=sum(r['count'] for r in reports);details['reports']=reports
    permissions=(E/'apk-permissions.txt').read_text(encoding='utf-8-sig')
    record('offlineNoInternetPermission','package: com.indianrailway.s1harness' in permissions and 'android.permission.INTERNET' not in permissions)
    record('profileComplete',completion['complete'] and profile['frames']==6000 and profile['analysisFrames']>0)
    record('profileMachineReadable',profile['metrics']['FrameTime']['status']=='CAPTURED' and profile['metrics']['FPS']['status']=='CAPTURED' and not profile['finalBudgetClaim'])
    record('overrideRestored',restoration['restored'] and (D/'final-launch/override.txt').read_text(encoding='utf-8-sig').strip()=='ABSENT')
    device=load(D/'final-launch/device.json')
    record('namedDevice',device['model']=='I2304' and device['api']=='36' and device['abi']=='arm64-v8a')
    engine=(D/'final-launch/engine.log').read_text(encoding='utf-8-sig',errors='replace')
    record('VulkanRuntime','Vulkan' in engine and 'LogVulkanRHI' in engine)
    command_lines=[line for line in engine.splitlines() if 'Command Line:' in line]
    record('normalLaunchNoCapture',bool(command_lines) and '-csvCaptureFrames=' not in '\n'.join(command_lines))
    details.update(fingerprint=final['logicalFingerprint'],tick=final['logical']['tick'],profileScope=completion['scope'],metrics=profile['metrics'],device=device)
except (OSError,ValueError,KeyError,TypeError) as error:
    checks['completeEvidence']=False;details['error']=str(error)
passed=all(checks.values()) and bool(checks)
summary=('iQOO Neo 9 Pro/I2304 API36 ARM64 Vulkan/ASTC: verified install, cold launch, packaged S8 checks, '
         '238 fresh S2-S7 startup cases, exact paused-save/progression preservation across installation and profiling, '
         'app-level offline operation (no INTERNET permission), and a finalized 6000-frame paused-fixture harness capture. '
         'Temporary override restored; normal launch rechecked. This is not the representative S8 workload.') if passed else 'Android verification incomplete or failed; inspect checks and retained raw evidence.'
result=dict(passed=passed,summary=summary,checks=checks,details=details)
(E/'android-verification.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps(dict(passed=passed,checks=checks),indent=2));raise SystemExit(not passed)
