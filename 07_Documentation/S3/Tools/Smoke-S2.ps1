$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S3Device.ps1')
$remote='/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/UECommandLine.txt'
$out=Join-Path $PSScriptRoot '../Evidence/Device/s2-smoke'
New-Item -ItemType Directory -Force $out | Out-Null
$probe=& $adb -s $serial shell ls $remote 2>&1
$existed=$LASTEXITCODE -eq 0
if($existed){& $adb -s $serial pull $remote (Join-Path $out 'original-commandline.txt');if($LASTEXITCODE){throw 'Backup failed'}}
'-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S2/L_RailKernel -ExecCmds="r.SetFramePace 30" -log' | Set-Content (Join-Path $out 'commandline.txt')
try {
 & $adb -s $serial push (Join-Path $out 'commandline.txt') $remote
 if($LASTEXITCODE){throw 'Override failed'}
 & (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Launch -Label 's2-smoke'
 Start-Sleep -Seconds 16
 & (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Snapshot -Label 's2-smoke'
 & (Join-Path $PSScriptRoot 'Test-S3Device.ps1') -Action Pull -Label 's2-smoke'
} finally {
 if($existed){& $adb -s $serial push (Join-Path $out 'original-commandline.txt') $remote}
 else {& $adb -s $serial shell rm $remote}
 if($LASTEXITCODE){throw 'Restore failed'}
}

