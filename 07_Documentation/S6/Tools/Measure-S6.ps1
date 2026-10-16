param([switch]$Profile,[string]$Label='sustained',[int]$Seconds=450,[int]$TopY=1068)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S6Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out|Out-Null
$package='com.indianrailway.s1harness'
$base="/sdcard/Android/data/$package/files/UnrealGame/MyProject"
$remote="$base/UECommandLine.txt"
$saved="$base/MyProject/Saved/S6"
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
function Foreground(){if(((Adb @('shell','dumpsys window | grep mCurrentFocus')) -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw 'Game not foreground; measurement stopped'}}
function ReadState(){
 for($attempt=0;$attempt -lt 5;$attempt++){
  $raw=Adb @('shell','cat',"$saved/telemetry.json")
  try{return ($raw|ConvertFrom-Json -ErrorAction Stop)}catch{
   $raw|Set-Content "$out/partial-telemetry-$((Get-Date).ToString('HHmmssfff')).txt"
   if($attempt -eq 4){throw};Start-Sleep -Milliseconds 80
  }
 }
}
$existed=$false;$changed=$false
if($Profile){
 & $adb -s $serial shell ls $remote 2>&1|Out-Null;$existed=$LASTEXITCODE -eq 0
 if($existed){Adb @('pull',$remote,"$out/original-commandline.txt")|Out-Null}
 '-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S6/L_RailContinuity -csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log'|Set-Content "$out/profile-commandline.txt"
 Adb @('push',"$out/profile-commandline.txt",$remote)|Out-Null;$changed=$true;$Seconds=245
}
try {
 & (Join-Path $PSScriptRoot 'Device-S6.ps1') -Action Launch -Label "$Label/launch"|Out-Null
 Start-Sleep -Seconds 8
 $events=@()
 foreach($offset in @(0,140,280)){
  if($Profile -and $offset -gt 0){break}
  $events+=,@(($offset+2),6,1);$events+=,@(($offset+5),5,1);$events+=,@(($offset+6),3,0)
  $events+=,@(($offset+30),5,1);$events+=,@(($offset+31),1,2)
  foreach($cycle in @(35,50,65)){$events+=,@(($offset+$cycle),6,2)}
  $events+=,@(($offset+75),5,1)
 }
 if($Profile){foreach($cycle in @(110,135,160)){$events+=,@($cycle,6,2);$events+=,@(($cycle+7),5,1)};$events+=,@(175,5,1)}
 $events=@($events|Sort-Object {$_[0]})
 $timer=[Diagnostics.Stopwatch]::StartNew();$nextEvent=0;$nextSample=0;$nextMemory=0;$samples=0
 while($timer.Elapsed.TotalSeconds -lt $Seconds){
  Foreground
  if($nextEvent -lt $events.Count -and $timer.Elapsed.TotalSeconds -ge $events[$nextEvent][0]){
   $e=$events[$nextEvent];Adb @('shell','input','tap',"$(220+$e[1]*393)","$($TopY+$e[2]*75)")|Out-Null
   [ordered]@{seconds=$timer.Elapsed.TotalSeconds;column=$e[1];row=$e[2]}|ConvertTo-Json -Compress|Add-Content "$out/touches.jsonl";$nextEvent++
  }
  if($timer.Elapsed.TotalSeconds -ge $nextSample){
   $s=ReadState
   if(-not $s.logical.valid -or $s.logical.faulted -or $s.logicalFingerprint -eq 'INVALID'){throw 'Railway fault during streaming measurement'}
   [ordered]@{seconds=$timer.Elapsed.TotalSeconds;state=$s}|ConvertTo-Json -Depth 40 -Compress|Add-Content "$out/samples.jsonl"
   $samples++;$nextSample=$timer.Elapsed.TotalSeconds+2
  }
  if($timer.Elapsed.TotalSeconds -ge $nextMemory){
   $stamp=[int]$timer.Elapsed.TotalSeconds
   Adb @('shell','dumpsys','meminfo',$package)|Set-Content "$out/memory-$stamp.txt"
   Adb @('shell','dumpsys','thermalservice')|Set-Content "$out/thermal-$stamp.txt"
   Write-Output "S6 measurement $stamp / $Seconds seconds; $samples logical samples"
   $nextMemory=$timer.Elapsed.TotalSeconds+30
  }
  Start-Sleep -Milliseconds 200
 }
 & (Join-Path $PSScriptRoot 'Device-S6.ps1') -Action Pull -Label $Label|Out-Null
 [ordered]@{complete=$true;seconds=$timer.Elapsed.TotalSeconds;samples=$samples;events=$nextEvent}|ConvertTo-Json|Set-Content "$out/completion.json"
}finally{
 if($changed){
  if($existed){Adb @('push',"$out/original-commandline.txt",$remote)|Out-Null}else{Adb @('shell','rm',$remote)|Out-Null}
  [ordered]@{restored=$true;previouslyExisted=$existed}|ConvertTo-Json|Set-Content "$out/commandline-restore.json"
 }
}

