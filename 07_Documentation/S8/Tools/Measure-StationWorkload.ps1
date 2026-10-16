param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid evidence label'}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$adbExecutable=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$deviceSerial='10BE1Y2EL40003J'
$previousWake=(& $adbExecutable -s $deviceSerial shell settings get global stay_on_while_plugged_in).Trim()
if($LASTEXITCODE -ne 0 -or $previousWake -notmatch '^[0-7]$'){throw 'Authorized device unavailable or wake setting unreadable'}
$evidence=Join-Path $workspace "07_Documentation/S8/Evidence/Station/Device/$Label"
if(Test-Path -LiteralPath $evidence){throw 'Evidence already exists; choose a new label'}
$completed=$false
try {
 & $adbExecutable -s $deviceSerial shell settings put global stay_on_while_plugged_in 3
 if($LASTEXITCODE -ne 0){throw 'Temporary wake setting failed'}
 & python (Join-Path $PSScriptRoot 'profile_lhb.py') WAP7 --station --arrangement A --frames 6000 --label $Label --apk (Join-Path $workspace '08_Builds/S8_EgmoreReview/Android_ASTC/MyProject-arm64.apk')
 if($LASTEXITCODE -ne 0){throw 'Workload capture failed; evidence preserved'}
 $completed=$true
} finally {
 & $adbExecutable -s $deviceSerial shell settings put global stay_on_while_plugged_in $previousWake
 $restoredWake=(& $adbExecutable -s $deviceSerial shell settings get global stay_on_while_plugged_in).Trim()
 if(Test-Path -LiteralPath $evidence){
  [ordered]@{completed=$completed;wakeBefore=$previousWake;wakeDuring=3;wakeAfter=$restoredWake;wakeRestored=($previousWake -eq $restoredWake)} | ConvertTo-Json | Set-Content (Join-Path $evidence 'wake-restoration.json')
 }
 & $adbExecutable -s $deviceSerial shell am start -W -n com.indianrailway.s1harness/com.epicgames.unreal.SplashActivity | Out-Null
 if($restoredWake -ne $previousWake){throw 'Wake restoration needs attention'}
}
