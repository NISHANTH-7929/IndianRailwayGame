param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid evidence label'}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$adbExecutable=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$deviceRows=@(& $adbExecutable devices | Where-Object {$_ -match '^10BE1Y2EL40003J\s+device$'})
if($deviceRows.Count -ne 1){throw 'This wrapper requires the authorized phone on wired USB'}
$deviceSerial=($deviceRows[0] -split '\s+')[0]
$evidence=Join-Path $workspace "07_Documentation/S8/Evidence/Egmore/$Label-device-validation"
if(Test-Path -LiteralPath $evidence){throw 'Evidence directory already exists; use a new label'}
New-Item -ItemType Directory -Path $evidence | Out-Null
$previousWake=(& $adbExecutable -s $deviceSerial shell settings get global stay_on_while_plugged_in).Trim()
if($previousWake -notmatch '^[0-7]$'){throw 'Cannot safely preserve the existing wake setting'}
& $adbExecutable -s $deviceSerial shell dumpsys battery > (Join-Path $evidence 'battery-before.txt')
$completed=$false
try {
 # This phone can classify charging on wired USB as AC. Both plugged-in bits
 # are temporary for this validation; the user's original setting is restored.
 & $adbExecutable -s $deviceSerial shell settings put global stay_on_while_plugged_in 3
 if($LASTEXITCODE -ne 0){throw 'Unable to set temporary plugged-in wake'}
 & $adbExecutable -s $deviceSerial install -r (Join-Path $workspace '08_Builds/S8_EgmoreReview/Android_ASTC/MyProject-arm64.apk') *> (Join-Path $evidence 'install.txt')
 if($LASTEXITCODE -ne 0){throw 'APK installation failed'}
 & python (Join-Path $PSScriptRoot 'capture_egmore_android.py') *> (Join-Path $evidence 'capture.txt')
 if($LASTEXITCODE -ne 0){throw 'Egmore device capture failed'}
 & python (Join-Path $PSScriptRoot 'verify_station_v6_recovery.py') --prefix $Label *> (Join-Path $evidence 'recovery.txt')
 if($LASTEXITCODE -ne 0){throw 'Cold recovery validation failed'}
 $completed=$true
} finally {
 & $adbExecutable -s $deviceSerial shell settings put global stay_on_while_plugged_in $previousWake
 $restoredWake=(& $adbExecutable -s $deviceSerial shell settings get global stay_on_while_plugged_in).Trim()
 [ordered]@{completed=$completed;serial=$deviceSerial;wakeBefore=$previousWake;wakeDuring=3;wakeAfter=$restoredWake;wakeRestored=($restoredWake -eq $previousWake)} | ConvertTo-Json | Set-Content (Join-Path $evidence 'result.json')
 if($restoredWake -ne $previousWake){throw 'Wake setting restoration needs attention'}
}
