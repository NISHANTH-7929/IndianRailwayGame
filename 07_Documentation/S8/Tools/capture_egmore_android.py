"""Render the separate Egmore study on-device; preserve normal saves/launch."""
import subprocess,os,time,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
adb_exe=str(Path(os.environ['ANDROID_HOME'])/'platform-tools/adb.exe')
inventory=subprocess.run([adb_exe,'devices'],capture_output=True,text=True,check=True).stdout
devices=[l.split()[0] for l in inventory.splitlines() if '10BE1Y2EL40003J' in l and l.endswith('\tdevice')]
if len(devices)!=1:raise RuntimeError('Expected the authorized phone')
package='com.indianrailway.s1harness'
base=f'/sdcard/Android/data/{package}/files/UnrealGame/MyProject'
saved=base+'/MyProject/Saved';remote=base+'/UECommandLine.txt'
out=ROOT/'07_Documentation/S8/Evidence/Egmore/Device'/time.strftime('review-%Y%m%d-%H%M%S')
out.mkdir(parents=True,exist_ok=False)
def adb(*args,check=True):
    return subprocess.run([adb_exe,'-s',devices[0],*args],capture_output=True,check=check,timeout=90)
def hashes():
    paths=[saved+'/'+folder+f'/checkpoint-{slot}.irs' for folder in ['S7','S8/LHB/Service','S8/Station/Service'] for slot in [0,1]]
    return adb('shell','sha256sum',*paths).stdout
before=hashes();(out/'saves-before.txt').write_bytes(before)
existed=adb('shell','test','-f',remote,check=False).returncode==0
if existed:adb('pull',remote,str(out/'original-commandline.txt'))
command='-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study -ExecCmds="r.SetFramePace 30" -log'
(out/'commandline.txt').write_text(command)
try:
    adb('shell','am','force-stop',package)
    adb('push',str(out/'commandline.txt'),remote)
    adb('shell','am','start','-W','-n',package+'/com.epicgames.unreal.SplashActivity')
    for delay in [20,20,20]:
        time.sleep(delay)
        focus=adb('shell','dumpsys','window').stdout
        if not any(b'mCurrentFocus' in l and package.encode() in l for l in focus.splitlines()):raise RuntimeError('Game left foreground; no capture')
        stamp=time.strftime('%H%M%S')
        (out/(stamp+'.png')).write_bytes(adb('exec-out','screencap','-p').stdout)
        (out/(stamp+'-memory.txt')).write_bytes(adb('shell','dumpsys','meminfo',package).stdout)
    apk=ROOT/'08_Builds/S8_EgmoreReview/Android_ASTC/MyProject-arm64.apk'
    (out/'result.json').write_text(json.dumps({'captured':True,'scope':'static Egmore art review; not a WAP7-LHB gameplay benchmark',
        'apkSha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'visualAccepted':False},indent=2))
except Exception as error:
    (out/'failure.json').write_text(json.dumps({'reason':str(error)}));raise
finally:
    (out/'engine.log').write_bytes(adb('shell','cat',saved+'/Logs/MyProject.log',check=False).stdout)
    adb('shell','am','force-stop',package,check=False)
    if existed:adb('push',str(out/'original-commandline.txt'),remote)
    else:adb('shell','rm',remote)
    after=hashes();(out/'saves-after.txt').write_bytes(after)
    (out/'restoration.json').write_text(json.dumps({'commandlineRestored':True,'savesUnchanged':after==before}))
    adb('shell','am','start','-W','-n',package+'/com.epicgames.unreal.SplashActivity')
    if after!=before:raise RuntimeError('Normal checkpoint hashes changed')
print(out)
