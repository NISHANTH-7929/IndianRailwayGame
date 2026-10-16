"""Stage/revert unsupported-content records only in the isolated LHB save slots."""
import argparse,subprocess,os,json,hashlib,zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'07_Documentation/S8/Evidence/LHB/Device/incompatible-slots'
ADB=str(Path(os.environ['ANDROID_HOME'])/'platform-tools/adb.exe')
BASE='/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/MyProject/Saved/S8/LHB/Service'
def adb(*args):return subprocess.run([ADB,'-s','10BE1Y2EL40003J',*args],check=True,capture_output=True)
p=argparse.ArgumentParser();p.add_argument('action',choices=['stage','verify','restore']);a=p.parse_args()
if a.action=='stage':
 OUT.mkdir(parents=True,exist_ok=False)
 adb('shell','am','force-stop','com.indianrailway.s1harness')
 for i in range(2):
  name=f'checkpoint-{i}.irs'; original=OUT/('original-'+name)
  adb('pull',BASE+'/'+name,str(original));data=bytearray(original.read_bytes())
  start=data.find(b'IR-S8-LHB-3A-1');assert start>=0
  data[start:start+len(b'IR-S8-LHB-3A-1')]=b'IR-S8-LHB-3A-X'
  data[-4:]=zlib.crc32(data[:-4]).to_bytes(4,'little')
  future=OUT/('future-'+name);future.write_bytes(data);adb('push',str(future),BASE+'/'+name)
else:
 if a.action=='restore':adb('shell','am','force-stop','com.indianrailway.s1harness')
 checks={}
 for i in range(2):
  name=f'checkpoint-{i}.irs'
  if a.action=='restore':adb('push',str(OUT/('original-'+name)),BASE+'/'+name)
  current=adb('exec-out','cat',BASE+'/'+name).stdout
  expected=(OUT/(('original-' if a.action=='restore' else 'future-')+name)).read_bytes()
  checks[name]=current==expected
 (OUT/(a.action+'.json')).write_text(json.dumps(checks,indent=2))
 assert all(checks.values()),checks
 print(checks)
