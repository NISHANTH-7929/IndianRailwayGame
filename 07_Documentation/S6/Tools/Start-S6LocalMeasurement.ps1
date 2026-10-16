param([switch]$SustainedOnly)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S6Device.ps1')
$root=if($SustainedOnly){'/data/local/tmp/ir-s6-residency-final'}else{'/data/local/tmp/ir-s6-final-measurement'}
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
Adb @('shell','command -v nohup; command -v sh')|Out-Null
& $adb -s $serial shell test -e $root
if($LASTEXITCODE -eq 0){throw 'Measurement directory already exists; inspect its status before starting another run'}
$out=Join-Path $PSScriptRoot $(if($SustainedOnly){'../Evidence/local-residency-launch'}else{'../Evidence/local-measurement-launch'})
New-Item -ItemType Directory -Force $out|Out-Null
foreach($phase in @('profile-final','sustained-final')){
 $events=@()
 foreach($offset in @(0,140,280)){
  if($phase -eq 'profile-final' -and $offset -gt 0){break}
  $events+=,@(($offset+2),6,1);$events+=,@(($offset+5),5,1);$events+=,@(($offset+6),3,0)
  $events+=,@(($offset+30),5,1);$events+=,@(($offset+31),1,2)
  foreach($cycle in @(35,50,65)){$events+=,@(($offset+$cycle),6,2)}
  $events+=,@(($offset+75),5,1)
 }
 if($phase -eq 'profile-final'){
  foreach($cycle in @(110,135,160)){$events+=,@($cycle,6,2);$events+=,@(($cycle+7),5,1)}
  $events+=,@(175,5,1)
 }
 $lines=@($events|Sort-Object {$_[0]}|ForEach-Object {$_ -join ' '})
 [IO.File]::WriteAllText((Join-Path $out "$phase-events.txt"),($lines -join "`n")+"`n",[Text.UTF8Encoding]::new($false))
}
Adb @('shell','mkdir','-p',$root)|Out-Null
Adb @('push',(Join-Path $PSScriptRoot 'measure_s6_device.sh'),"$root/measure.sh")|Out-Null
foreach($phase in @('profile-final','sustained-final')){Adb @('push',(Join-Path $out "$phase-events.txt"),"$root/$phase-events.txt")|Out-Null}
# The earlier failed profile created this exact override; it was absent before testing.
Adb @('shell','rm','-f','/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/UECommandLine.txt')|Out-Null
Adb @('shell','sh','-n',"$root/measure.sh")|Out-Null
$mode=if($SustainedOnly){'sustained'}else{'all'}
Adb @('shell',('nohup sh '+$root+'/measure.sh '+$root+' '+$mode+' > '+$root+'/runner.log 2>&1 < /dev/null & echo $!'))|Set-Content (Join-Path $out 'pid.txt')
Write-Output "Started bounded phone measurement: $mode."
