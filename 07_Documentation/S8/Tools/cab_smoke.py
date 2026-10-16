import subprocess,time,json,sys,hashlib,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
ADB='C:/Users/nisha/AppData/Local/Android/Sdk/platform-tools/adb.exe'
inventory=subprocess.run([ADB,'devices'],capture_output=True,text=True,check=True).stdout
endpoints=[line.split()[0] for line in inventory.splitlines() if '10BE1Y2EL40003J' in line and line.endswith('\tdevice')]
if not endpoints:raise RuntimeError('Expected phone is not connected')
SERIAL='10BE1Y2EL40003J' if '10BE1Y2EL40003J' in endpoints else endpoints[0]
PKG='com.indianrailway.s1harness'
BASE='/sdcard/Android/data/'+PKG+'/files/UnrealGame/MyProject'
SAVED=BASE+'/MyProject/Saved'
out=ROOT/'07_Documentation/S8/Evidence/Cab'/sys.argv[1];out.mkdir(exist_ok=False)
apk=Path(os.environ.get('RAIL_CAB_TEST_APK',str(ROOT/'08_Builds/S8_WAP7Cab/Android_ASTC/MyProject-arm64.apk')))
(out/'build.json').write_text(json.dumps({'apkSha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'apkBytes':apk.stat().st_size},indent=2))
def adb(*args,binary=False,check=True):
 r=subprocess.run([ADB,'-s',SERIAL,*args],capture_output=True,text=not binary,timeout=60)
 if check and r.returncode:raise RuntimeError(r.stderr)
 return r
installed=adb('shell','pm','path',PKG).stdout.strip().splitlines()
if len(installed)!=1 or not installed[0].startswith('package:'):raise RuntimeError('Expected a single installed base APK')
installed_hash=adb('shell','sha256sum',installed[0].removeprefix('package:')).stdout.split()[0]
if installed_hash.lower()!=hashlib.sha256(apk.read_bytes()).hexdigest():raise RuntimeError('Installed APK does not match the selected evidence artifact')
(out/'installed-apk.json').write_text(json.dumps({'path':installed[0],'sha256':installed_hash,'matches':True},indent=2))
def hashes():return adb('shell','sha256sum',*[SAVED+'/'+folder+f'/checkpoint-{slot}.irs' for folder in ['S7','S8/LHB/Service','S8/Station/Service'] for slot in [0,1]]).stdout
adb('shell','am','force-stop',PKG)
before=hashes();previous=adb('shell','cat',BASE+'/UECommandLine.txt',check=False)
(out/'saves-before.txt').write_text(before)
(out/'old-commandline.txt').write_text(previous.stdout)
flags=' '.join(sys.argv[2:])
(out/'commandline.txt').write_text('-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S7/L_RailService -S8AssetReview -S8Cab -S8CabReview '+flags+' -log')
try:
 adb('push',str(out/'commandline.txt'),BASE+'/UECommandLine.txt')
 adb('shell','am','start','-W','-n',PKG+'/com.epicgames.unreal.SplashActivity')
 for attempt in range(45):
  time.sleep(2)
  r=adb('shell','cat',SAVED+'/S8/AssetReview/telemetry.json',check=False)
  if r.returncode:continue
  state=json.loads(r.stdout)
  if state.get('locomotive',{}).get('cabTransitions')==1 and state.get('logical',{}).get('tick',999999)<300:break
 else:raise RuntimeError('No fresh cab startup; exclude this run')
 last_tick=-1
 for second in range(0,90,3):
  time.sleep(3)
  r=adb('shell','cat',SAVED+'/S8/AssetReview/telemetry.json',check=False)
  if r.returncode:continue
  state=json.loads(r.stdout)
  tick=state.get('logical',{}).get('tick',-1)
  if tick<=last_tick:raise RuntimeError('Stale gameplay telemetry; exclude this run')
  last_tick=tick
  (out/f'state-{second+3}.json').write_text(json.dumps(state,indent=2))
  (out/f'screen-{second+3}.png').write_bytes(adb('exec-out','screencap','-p',binary=True).stdout)
  loco=state.get('locomotive',{})
  print(second+3,'tick',tick,'cab',loco.get('cabCamera'),'transitions',loco.get('cabTransitions'),flush=True)
 (out/'game-log.txt').write_text(adb('shell','cat',SAVED+'/Logs/MyProject.log',check=False).stdout)
finally:
 restoration={'normalSaveUnchanged':False,'commandlineRestored':False,'completed':False}
 try:
  adb('shell','am','force-stop',PKG)
  if previous.returncode==0:adb('push',str(out/'old-commandline.txt'),BASE+'/UECommandLine.txt')
  else:adb('shell','rm',BASE+'/UECommandLine.txt')
  check=adb('shell','cat',BASE+'/UECommandLine.txt',check=False)
  restoration['commandlineRestored']=(check.returncode==0 and check.stdout==previous.stdout) if previous.returncode==0 else check.returncode!=0
  restoration['normalSaveUnchanged']=hashes()==before
  restoration['completed']=restoration['commandlineRestored'] and restoration['normalSaveUnchanged']
  if not restoration['completed']:raise RuntimeError('Save/commandline restoration verification failed')
 except Exception as error:
  restoration['error']=str(error);raise
 finally:(out/'restoration.json').write_text(json.dumps(restoration,indent=2))
