param([int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1153,[int]$BottomY=1228,[string]$Label='final-sustained')
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S3Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
# Bounded touch-test sequence, not an in-game driver or AI.
$events=@(@(5,3,0),@(40,0,0),@(50,6,0),@(70,0,1),@(71,4,0),@(72,2,0),
 @(100,0,0),@(101,6,0),@(120,2,1),@(121,4,0),@(122,2,0),@(155,0,0),@(156,6,0),
 @(175,0,1),@(176,4,0),@(177,1,0),@(199,0,0),@(200,6,0))
& (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Launch -Label "$Label/launch"
$clock=[Diagnostics.Stopwatch]::StartNew()
$eventIndex=0;$sampleIndex=0
while($clock.Elapsed.TotalSeconds -lt 215){
 if($eventIndex -lt $events.Count -and $clock.Elapsed.TotalSeconds -ge $events[$eventIndex][0]){
  $e=$events[$eventIndex];$y=if($e[2] -eq 0){$TopY}else{$BottomY}
  & $adb -s $serial shell input tap ($FirstX+$e[1]*$Spacing) $y
  if($LASTEXITCODE){throw 'Touch test injection failed'}
  [ordered]@{seconds=$clock.Elapsed.TotalSeconds;column=$e[1];row=$e[2]} | ConvertTo-Json -Compress | Add-Content (Join-Path $out 'touch-events.jsonl')
  $eventIndex++
 }
 if($sampleIndex -le 7 -and $clock.Elapsed.TotalSeconds -ge $sampleIndex*30){
  & (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Snapshot -Label "$Label/sample-$sampleIndex"
  & $adb -s $serial shell cat '/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/MyProject/Saved/S3/telemetry.json' | Set-Content (Join-Path $out "sample-$sampleIndex/telemetry.json")
  Write-Output "Sample $sampleIndex at $([math]::Round($clock.Elapsed.TotalSeconds,2)) seconds"
  $sampleIndex++
 }
 Start-Sleep -Milliseconds 100
}
& (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Pull -Label $Label


