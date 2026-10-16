param([Parameter(Mandatory=$true)][string]$Label,[Parameter(Mandatory=$true)][string]$Apk,[switch]$SkipInstall)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid evidence label'}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$selectedApk=(Resolve-Path -LiteralPath $Apk).Path
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial='10BE1Y2EL40003J'
$receipt=Join-Path $workspace "07_Documentation/S8/Evidence/Cab/$Label-device"
if(Test-Path -LiteralPath $receipt){throw 'Evidence already exists'}
New-Item -ItemType Directory -Path $receipt | Out-Null
$wake=(& $adb -s $serial shell settings get global stay_on_while_plugged_in).Trim()
if($wake -notmatch '^[0-7]$'){throw 'Unable to preserve wake setting'}
[ordered]@{wakeBefore=$wake;serial=$serial;apk=$selectedApk} | ConvertTo-Json | Set-Content (Join-Path $receipt 'before.json')
$passed=$false
try {
 & $adb -s $serial shell settings put global stay_on_while_plugged_in 3
 if($LASTEXITCODE -ne 0){throw 'Cannot set temporary wake'}
 if(-not $SkipInstall){
  & $adb -s $serial install -r $selectedApk *> (Join-Path $receipt 'install.txt')
  if($LASTEXITCODE -ne 0){throw 'Installation failed'}
 }
 $env:RAIL_CAB_TEST_APK=$selectedApk
 & python (Join-Path $PSScriptRoot 'cab_smoke.py') "$Label-day" *> (Join-Path $receipt 'day.txt')
 if($LASTEXITCODE -ne 0){throw 'Daylight cab smoke failed'}
 & python (Join-Path $PSScriptRoot 'cab_smoke.py') "$Label-night" '-S8LowLight' *> (Join-Path $receipt 'night.txt')
 if($LASTEXITCODE -ne 0){throw 'Low-light cab smoke failed'}
 $passed=$true
} finally {
 & $adb -s $serial shell settings put global stay_on_while_plugged_in $wake
 $wakeResponse=& $adb -s $serial shell settings get global stay_on_while_plugged_in
 $after=if($null -ne $wakeResponse){("$wakeResponse").Trim()}else{'unavailable'}
 & $adb -s $serial shell am start -W -n 'com.indianrailway.s1harness/com.epicgames.unreal.SplashActivity' *> (Join-Path $receipt 'normal-launch.txt')
 [ordered]@{passed=$passed;apk=$selectedApk;sha256=(Get-FileHash -LiteralPath $selectedApk).Hash;wakeBefore=$wake;wakeAfter=$after;wakeRestored=($wake -eq $after)} | ConvertTo-Json | Set-Content (Join-Path $receipt 'result.json')
 if($after -ne $wake){throw 'Wake restoration needs attention'}
}
