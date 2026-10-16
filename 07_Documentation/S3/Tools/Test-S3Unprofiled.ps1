$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S3Device.ps1')
$remote='/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject/UECommandLine.txt'
$out=Join-Path $PSScriptRoot '../Evidence/Device/unprofiled'
New-Item -ItemType Directory -Force $out | Out-Null
$probe=& $adb -s $serial shell ls $remote 2>&1
$existed=$LASTEXITCODE -eq 0
if($existed){& $adb -s $serial pull $remote (Join-Path $out 'original-commandline.txt');if($LASTEXITCODE){throw 'Backup failed'}}
'-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S3/L_TrainHandling -ExecCmds="r.SetFramePace 30" -log' | Set-Content (Join-Path $out 'commandline.txt')
try {
 & $adb -s $serial push (Join-Path $out 'commandline.txt') $remote
 if($LASTEXITCODE){throw 'Override failed'}
 & (Join-Path $PSScriptRoot 'Measure-S3.ps1') -Label unprofiled
} finally {
 if($existed){& $adb -s $serial push (Join-Path $out 'original-commandline.txt') $remote}
 else {& $adb -s $serial shell rm $remote}
 if($LASTEXITCODE){throw 'Restore failed'}
}

