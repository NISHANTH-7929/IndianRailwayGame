"""Read-only capture and explicit launch helper for LHB Android validation."""
import argparse,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
ADB=str(Path(os.environ['ANDROID_HOME'])/'platform-tools/adb.exe')
PACKAGE='com.indianrailway.s1harness'
BASE=f'/sdcard/Android/data/{PACKAGE}/files/UnrealGame/MyProject'
endpoints=subprocess.run([ADB,'devices'],capture_output=True,text=True,check=True).stdout.splitlines()
devices=[line.split()[0] for line in endpoints if '10BE1Y2EL40003J' in line and line.endswith('\tdevice')]
if len(devices)!=1:raise RuntimeError('Expected one authorized iQOO endpoint')
SERIAL=devices[0]
def adb(*args,check=True):
 return subprocess.run([ADB,'-s',SERIAL,*args],capture_output=True,check=check)
p=argparse.ArgumentParser();p.add_argument('action',choices=['capture','launch']);p.add_argument('label');p.add_argument('--legacy',action='store_true');p.add_argument('--station',action='store_true');a=p.parse_args()
if a.legacy and a.station:p.error('legacy and station are distinct modes')
out=ROOT/('07_Documentation/S8/Evidence/Station/Device' if a.station else '07_Documentation/S8/Evidence/LHB/Device')/a.label
out.mkdir(parents=True,exist_ok=False)
if a.action=='launch':
 map_path='/Game/IndianRailway/S7/L_RailService' if a.legacy else '/Game/IndianRailway/S8/LHB/L_RailService_LHB'
 if a.station:map_path='/Game/IndianRailway/S8/Stations/L_RailService_Station'
 command='-project="../../../MyProject/MyProject.uproject" '+map_path+' '+('' if a.legacy else '-S8LHB ')+'-ExecCmds="r.SetFramePace 30" -log'
 if a.station:command+=' -S8Station'
 path=out/'commandline.txt';path.write_text(command)
 adb('shell','am','force-stop',PACKAGE)
 adb('push',str(path),BASE+'/UECommandLine.txt')
 (out/'launch.txt').write_bytes(adb('shell','am','start','-W','-n',PACKAGE+'/com.epicgames.unreal.SplashActivity').stdout)
else:
 focus=adb('shell','dumpsys','window').stdout
 if not any(b'mCurrentFocus' in line and PACKAGE.encode() in line for line in focus.splitlines()):
  raise RuntimeError('Game is not foreground; no screenshot captured')
 (out/'screen.png').write_bytes(adb('exec-out','screencap','-p').stdout)
 (out/'focus.txt').write_bytes(adb('shell','dumpsys','window').stdout)
 directory='/S7' if a.legacy else '/S8/LHB/Service'
 if a.station:directory='/S8/Station/Service'
 (out/'telemetry.json').write_bytes(adb('shell','cat',BASE+'/MyProject/Saved'+directory+'/telemetry.json').stdout)
 adb('pull',BASE+'/MyProject/Saved'+directory,str(out/'Saved'))
 (out/'engine.log').write_bytes(adb('shell','cat',BASE+'/MyProject/Saved/Logs/MyProject.log',check=False).stdout)
 state=json.loads((out/'telemetry.json').read_text(encoding='utf-8-sig'))
 print(json.dumps({k:state.get(k) for k in ['content','paused','blocked','recovery','logicalFingerprint','lhbVehicles','lhbComponents']}))
 print(json.dumps(state.get('service',{}))[:1500])
