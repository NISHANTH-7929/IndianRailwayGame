param([string]$Label='observation',[int]$TapX=-1,[int]$TapY=-1,[switch]$Screenshot)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot '../../S7/Tools/Resolve-S7Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$package='com.indianrailway.s1harness'
$saved="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S7"
$focus=& $adb -s $serial shell dumpsys window
if($LASTEXITCODE){throw 'Cannot inspect foreground app'}
$focused=@($focus | Where-Object {$_ -match 'mCurrentFocus|mFocusedApp'})
$focused | Set-Content (Join-Path $out 'focus.txt')
if(-not ($focused -match 'mCurrentFocus.*com.indianrailway.s1harness')){throw 'Indian Railway is not foreground; no touch sent'}
if($TapX -ge 0 -and $TapY -ge 0){
 & $adb -s $serial shell input tap $TapX $TapY
 if($LASTEXITCODE){throw 'Touch failed'}
 [ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');x=$TapX;y=$TapY;foreground=$focused} | ConvertTo-Json | Set-Content (Join-Path $out 'touch.json')
 Start-Sleep -Milliseconds 700
}
& $adb -s $serial pull "$saved/telemetry.json" (Join-Path $out 'telemetry.json')
if($LASTEXITCODE){throw 'Telemetry pull failed'}
$telemetry=Get-Content (Join-Path $out 'telemetry.json') -Raw | ConvertFrom-Json
[ordered]@{generation=$telemetry.generation;paused=$telemetry.paused;blocked=$telemetry.blocked;fingerprint=$telemetry.logicalFingerprint;service=$telemetry.service;tick=$telemetry.logical.tick;trains=$telemetry.logical.trains;ai=$telemetry.logical.ai} | ConvertTo-Json -Depth 8
if($Screenshot){
 & $adb -s $serial shell screencap -p /data/local/tmp/ir-s7.png
 if($LASTEXITCODE){throw 'Screenshot failed'}
 & $adb -s $serial pull /data/local/tmp/ir-s7.png (Join-Path $out 'screen.png')
 if($LASTEXITCODE){throw 'Screenshot pull failed'}
}



