import subprocess,time,json,sys,hashlib
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
apk=ROOT/'08_Builds/S8_WAP7Cab/Android_ASTC/MyProject-arm64.apk'
(out/'build.json').write_text(json.dumps({'apkSha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'apkBytes':apk.stat().st_size},indent=2))
def adb(*args,binary=False,check=True):
 r=subprocess.run([ADB,'-s',SERIAL,*args],capture_output=True,text=not binary,timeout=60)
 if check and r.returncode:raise RuntimeError(r.stderr)
 return r
def hashes():return adb('shell','sha256sum',SAVED+'/S7/checkpoint-0.irs',SAVED+'/S7/checkpoint-1.irs').stdout
adb('shell','am','force-stop',PKG)
before=hashes();previous=adb('shell','cat',BASE+'/UECommandLine.txt',check=False)
(out/'old-commandline.txt').write_text(previous.stdout)
flags=' '.join(sys.argv[2:])
(out/'commandline.txt').write_text('-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S7/L_RailService -S8AssetReview -S8Cab -S8CabManual '+flags+' -log')
try:
 adb('push',str(out/'commandline.txt'),BASE+'/UECommandLine.txt')
 adb('shell','am','start','-W','-n',PKG+'/com.epicgames.unreal.SplashActivity')
 for attempt in range(45):
  time.sleep(2)
  ready=adb('shell','cat',SAVED+'/S8/AssetReview/telemetry.json',check=False)
  if ready.returncode==0:
   state=json.loads(ready.stdout)
   if state.get('locomotive',{}).get('parts')==len(json.loads((ROOT/'03_UnrealProject/MyProject/Content/IndianRailway/Production/locomotives/WAP7.json').read_text())['parts']) and state['locomotive'].get('cabTransitions')==1 and state['locomotive'].get('interactions')==0:break
 else:raise RuntimeError('Fresh final cab did not become ready')
 def capture(label):
  state=json.loads(adb('shell','cat',SAVED+'/S8/AssetReview/telemetry.json').stdout)
  (out/(label+'.json')).write_text(json.dumps(state,indent=2))
  (out/(label+'.png')).write_bytes(adb('exec-out','screencap','-p',binary=True).stdout)
  print(label,state.get('locomotive'),flush=True)
  return state
 state=capture('initial')
 # Android MobileContentScaleFactor 1 uses a 720-pixel short axis, rounded to eight.
 from PIL import Image
 width,height=Image.open(out/'initial.png').size
 scale=height/720
 for role in ['train_brake','throttle','throttle','throttle','throttle','train_brake','train_brake','train_brake','reverser','reverser','reverser','switch_emergency','switch_emergency','switch_wipers','switch_cablight']:
  state=capture('before-'+role+str(time.time_ns()))
  pose=next(x for x in state['locomotive']['controlPoses'] if x['role']==role)
  x,y=[round(v*scale) for v in pose['screen']]
  if x>width*.95:
   adb('shell','input','swipe',str(width//2),str(height//3),str(width//2+350),str(height//3),'500')
   time.sleep(2)
   state=capture('look-right-'+role)
   pose=next(x for x in state['locomotive']['controlPoses'] if x['role']==role)
   x,y=[round(v*scale) for v in pose['screen']]
  adb('shell','input','swipe',str(x),str(y),str(x),str(y),'160')
  time.sleep(2)
  after=capture('after-'+role+str(time.time_ns()))
  if after['locomotive']['interactions']!=state['locomotive']['interactions']+1:raise RuntimeError('Unexpected input count; exclude run and inspect screenshots')
 (out/'game-log.txt').write_text(adb('shell','cat',SAVED+'/Logs/MyProject.log',check=False).stdout)
finally:
 adb('shell','am','force-stop',PKG)
 if previous.returncode==0:adb('push',str(out/'old-commandline.txt'),BASE+'/UECommandLine.txt')
 else:adb('shell','rm',BASE+'/UECommandLine.txt')
 (out/'restoration.json').write_text(json.dumps({'normalSaveUnchanged':hashes()==before,'commandlineRestored':True},indent=2))
