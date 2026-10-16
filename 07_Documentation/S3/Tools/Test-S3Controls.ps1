param([string]$Label='controls',[int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1153,[int]$BottomY=1228)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S3Device.ps1')
$package='com.indianrailway.s1harness'
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$remote="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S3/telemetry.json"
function Adb([string[]]$Arguments){
 $value=& $adb -s $serial @Arguments 2>&1
 if($LASTEXITCODE){throw "ADB failed: $value"}
 return $value
}
function Tap([int]$Column,[int]$Row){
 $y=if($Row -eq 0){$TopY}else{$BottomY}
 Adb @('shell','input','tap',"$($FirstX+$Column*$Spacing)","$y") | Out-Null
}
function State([string]$Name){
 $raw=Adb @('shell','cat',$remote)
 $raw | Set-Content (Join-Path $out "$Name.json")
 $value=$raw | ConvertFrom-Json
 Write-Output "STATE $Name tick=$($value.tick) head=$($value.headMetres) speed=$($value.speedMps) throttle=$($value.throttle) brake=$($value.brake) reverse=$($value.direction)"
 return $value
}
function SaveState([string]$Name){$raw=Adb @('shell','cat',$remote);$raw | Set-Content (Join-Path $out "$Name.json");return ($raw | ConvertFrom-Json)}
$checks=[ordered]@{}
Tap 6 1
Start-Sleep -Seconds 4
$s=SaveState '01-reset';$checks.stationary=$s.speedMps -eq 0
Tap 3 0
Start-Sleep -Seconds 15
$s=SaveState '02-accelerate';$checks.acceleration=$s.speedMps -gt 3 -and $s.throttle -eq 1
Tap 0 0
Start-Sleep -Seconds 2
$coast=SaveState '03-coast-start'
Start-Sleep -Seconds 4
$s=SaveState '04-coast-end';$checks.coasting=$s.speedMps -lt $coast.speedMps -and $s.speedMps -gt 0 -and $s.tractionN -eq 0
Tap 0 1
Start-Sleep -Seconds 2
$s=SaveState '05-unsafe-reverse';$checks.safeReverser=$s.direction -eq 1 -and $s.rejected -gt 0
Tap 5 0
Start-Sleep -Seconds 3
$s=SaveState '06-brake-half';$checks.braking=$s.speedMps -lt $coast.speedMps -and $s.brake -eq .5
Tap 6 0
Start-Sleep -Seconds 10
$s=SaveState '07-stopped';$checks.stop=$s.speedMps -eq 0
Tap 0 1
Tap 4 0
Tap 3 0
Start-Sleep -Seconds 12
$s=SaveState '08-reverse';$checks.reverse=$s.speedMps -lt -2 -and $s.direction -eq -1
Tap 0 0
Tap 6 0
Start-Sleep -Seconds 8
$s=SaveState '09-reverse-stop';$checks.reverseStop=$s.speedMps -eq 0
Tap 4 0
Tap 2 0
Start-Sleep -Seconds 8
Tap 3 1
Start-Sleep -Seconds 5
$s=SaveState '10-emergency';$checks.emergency=$s.emergency -and $s.speedMps -eq 0
Tap 0 0
Tap 4 1
Start-Sleep -Seconds 2
$s=SaveState '11-emergency-reset';$checks.emergencyReset=-not $s.emergency
Tap 5 1
$before=SaveState '12-paused-before'
Start-Sleep -Seconds 5
$after=SaveState '13-paused-after';$checks.pause=$before.tick -eq $after.tick -and $after.paused -and $before.headMetres -eq $after.headMetres
Tap 5 1
Start-Sleep -Seconds 3
$s=SaveState '14-resumed';$checks.resume=$s.tick -gt $after.tick -and $s.throttle -eq 0 -and $s.brake -eq 1
Tap 2 1
Tap 4 0
Tap 2 0
Start-Sleep -Seconds 7
$pre=SaveState '15-before-home'
$pidBefore=Adb @('shell','pidof',$package)
Adb @('shell','input','keyevent','KEYCODE_HOME') | Out-Null
Start-Sleep -Seconds 2
$background=SaveState '16-background-start'
Start-Sleep -Seconds 6
$backgroundEnd=SaveState '17-background-end'
$checks.backgroundFrozen=$background.tick -eq $backgroundEnd.tick -and $background.headMetres -eq $backgroundEnd.headMetres
Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity") | Set-Content (Join-Path $out 'warm-launch.txt')
Start-Sleep -Seconds 3
$post=SaveState '18-after-home'
$pidAfter=Adb @('shell','pidof',$package)
$checks.lifecycle=$pidBefore -eq $pidAfter -and $post.tick -gt $backgroundEnd.tick -and $post.throttle -eq 0 -and $post.brake -eq 1
Tap 4 0
Tap 2 0
Start-Sleep -Seconds 6
$s=SaveState '19-input-after-resume';$checks.inputAfterResume=$s.throttle -eq .5 -and $s.brake -eq 0 -and $s.speedMps -gt 0
Tap 0 0
Tap 6 0
Start-Sleep -Seconds 6
Adb @('shell','screencap','-p','/data/local/tmp/ir-s3-controls.png') | Out-Null
Adb @('pull','/data/local/tmp/ir-s3-controls.png',(Join-Path $out 'screen.png')) | Out-Null
Adb @('shell','input','keyevent','KEYCODE_BACK') | Out-Null
Start-Sleep -Seconds 3
$remaining=& $adb -s $serial shell pidof $package
$checks.backExit=[string]::IsNullOrWhiteSpace(($remaining -join ' '))
[ordered]@{passed=($checks.Values -notcontains $false);checks=$checks;pidBefore=$pidBefore;pidAfter=$pidAfter;beforeHome=$pre;afterHome=$post} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $out 'results.json')
Get-Content (Join-Path $out 'results.json')
# pidof intentionally returns 1 after successful Back exit; do not leak that as test failure.
if($checks.Values -contains $false){exit 1}
exit 0


