param([switch]$Profile,[string]$Label='normal-sustained',[int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1068,[int]$RowSpacing=75)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S5Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out|Out-Null
$remote='/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/UECommandLine.txt'
$existed=$false;$changed=$false
if($Profile){
 $probe=& $adb -s $serial shell ls $remote 2>&1;$existed=$LASTEXITCODE -eq 0
 if($existed){& $adb -s $serial pull $remote (Join-Path $out 'original-commandline.txt');if($LASTEXITCODE){throw 'Cannot back up commandline'}}
 '-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S5/L_RailAI -csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log'|Set-Content (Join-Path $out 'profile-commandline.txt')
 & $adb -s $serial push (Join-Path $out 'profile-commandline.txt') $remote
 if($LASTEXITCODE){throw 'Cannot set profiling override'};$changed=$true
}
try {
 & (Join-Path $PSScriptRoot 'Test-S5Device.ps1') -Action Launch -Label "$Label/launch"
 # Finite host touch trace, no in-game driver logic.
 $events=@(@(15,4,0),@(16,2,0),@(166,4,0),@(167,3,0))
 if($Profile){$events+=,@(190,5,1)}
 $duration=if($Profile){245}else{215}
 $clock=[Diagnostics.Stopwatch]::StartNew();$eventIndex=0;$sampleIndex=0;$lastFocusCheck=-1
 while($clock.Elapsed.TotalSeconds -lt $duration){
  if($clock.Elapsed.TotalSeconds -ge $lastFocusCheck+1){
   $focus=& $adb -s $serial shell 'dumpsys window | grep mCurrentFocus'
   if($LASTEXITCODE -or ($focus -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw "Game not foreground; measurement stopped: $focus"}
   $lastFocusCheck=$clock.Elapsed.TotalSeconds
  }
  if($eventIndex -lt $events.Count -and $clock.Elapsed.TotalSeconds -ge $events[$eventIndex][0]){
   $e=$events[$eventIndex];& $adb -s $serial shell input tap ($FirstX+$e[1]*$Spacing) ($TopY+$e[2]*$RowSpacing)
   if($LASTEXITCODE){throw 'Touch injection failed'}
   [ordered]@{seconds=$clock.Elapsed.TotalSeconds;column=$e[1];row=$e[2]}|ConvertTo-Json -Compress|Add-Content (Join-Path $out 'touch-events.jsonl');$eventIndex++
  }
  if($sampleIndex -le [math]::Floor($duration/30) -and $clock.Elapsed.TotalSeconds -ge $sampleIndex*30){
   & (Join-Path $PSScriptRoot 'Test-S5Device.ps1') -Action Snapshot -Label "$Label/sample-$sampleIndex"
   & $adb -s $serial shell cat '/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/MyProject/Saved/S5/telemetry.json'|Set-Content (Join-Path $out "sample-$sampleIndex/telemetry.json")
   if($sampleIndex -gt 0){$state=Get-Content (Join-Path $out "sample-$sampleIndex/telemetry.json") -Raw|ConvertFrom-Json;if(-not $state.valid -or $state.faulted -or ($state.suspended -and (-not $Profile -or $clock.Elapsed.TotalSeconds -lt 190))){throw "Unexpected fault or pause during active measurement"}}
   Write-Output "Sample $sampleIndex at $([math]::Round($clock.Elapsed.TotalSeconds,2)) seconds";$sampleIndex++
  }
  Start-Sleep -Milliseconds 100
 }
 & (Join-Path $PSScriptRoot 'Test-S5Device.ps1') -Action Pull -Label $Label
} finally {
 if($changed){
  if($existed){& $adb -s $serial push (Join-Path $out 'original-commandline.txt') $remote}else{& $adb -s $serial shell rm $remote}
  if($LASTEXITCODE){throw 'Profiling override restore failed'}
 }
}




