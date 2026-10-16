$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial='10BE1Y2EL40003J'
$package='com.indianrailway.s1harness'
$remote="/sdcard/Android/data/$package/files/UnrealGame/MyProject/UECommandLine.txt"
$out=Join-Path $PSScriptRoot '../Evidence/Device/s1-smoke'
New-Item -ItemType Directory -Force $out | Out-Null
function Invoke-Adb([string]$Name,[string[]]$Arguments){
 $value=& $adb -s $serial @Arguments 2>&1;$code=$LASTEXITCODE
 $value | Set-Content (Join-Path $out "$Name.txt")
 [ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');arguments=$Arguments;exitCode=$code} | ConvertTo-Json -Compress | Add-Content (Join-Path $out 'smoke-commands.jsonl')
 if($code){throw "ADB $Name failed: $value"};return $value
}
$probe=& $adb -s $serial shell ls $remote 2>&1
$existed=$LASTEXITCODE -eq 0
if($existed){Invoke-Adb 'backup-commandline' @('pull',$remote,(Join-Path $out 'original-commandline.txt')) | Out-Null}
try {
 Invoke-Adb 'set-map' @('push',(Join-Path $PSScriptRoot 'S1-smoke-commandline.txt'),$remote) | Out-Null
 & (Join-Path $PSScriptRoot 'Test-S2Device.ps1') -Action Launch -Label 's1-smoke'
 Start-Sleep -Seconds 4
 & (Join-Path $PSScriptRoot 'Test-S2Device.ps1') -Action Snapshot -Label 's1-smoke'
 Invoke-Adb 'move' @('shell','input','swipe','400','1000','550','1000','1200') | Out-Null
 & (Join-Path $PSScriptRoot 'Test-S2Device.ps1') -Action Snapshot -Label 's1-smoke-moved'
 Invoke-Adb 'home' @('shell','input','keyevent','3') | Out-Null
 Start-Sleep -Seconds 5
 Invoke-Adb 'resume' @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity") | Out-Null
 Start-Sleep -Seconds 2
 & (Join-Path $PSScriptRoot 'Test-S2Device.ps1') -Action Snapshot -Label 's1-smoke-resumed'
 & (Join-Path $PSScriptRoot 'Test-S2Device.ps1') -Action Pull -Label 's1-smoke-resumed'
 Invoke-Adb 'back' @('shell','input','keyevent','4') | Out-Null
 Start-Sleep -Seconds 3
 $remaining=& $adb -s $serial shell pidof $package
 [ordered]@{remainingProcesses=$remaining;exited=($LASTEXITCODE -eq 1)} | ConvertTo-Json | Set-Content (Join-Path $out 'back-exit.json')
} finally {
 if($existed){Invoke-Adb 'restore-commandline' @('push',(Join-Path $out 'original-commandline.txt'),$remote) | Out-Null}
 else {Invoke-Adb 'remove-smoke-override' @('shell','rm',$remote) | Out-Null}
}
