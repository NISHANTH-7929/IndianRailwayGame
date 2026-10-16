param([string]$Label='operations-controls-verified',[int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1068,[int]$RowSpacing=75)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S4Device.ps1')
$package='com.indianrailway.s1harness'
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$saved="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S4"
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
function Foreground(){
 $focus=Adb @('shell','dumpsys window | grep mCurrentFocus')
 if(($focus -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw "Game is covered or not foreground; no touch sent: $focus"}
}
function Tap([int]$Column,[int]$Row){Foreground;Adb @('shell','input','tap',"$($FirstX+$Column*$Spacing)","$($TopY+$Row*$RowSpacing)") | Out-Null}
function State([string]$Name){$raw=Adb @('shell','cat',"$saved/telemetry.json");$raw|Set-Content (Join-Path $out "$Name.json");$s=$raw|ConvertFrom-Json;Write-Host "$Name tick=$($s.tick) valid=$($s.valid) A=$($s.trains[0].handling.headMetres) B=$($s.trains[1].handling.headMetres)";return $s}
function Screen([string]$Name){Foreground;Adb @('shell','screencap','-p','/data/local/tmp/ir-s4-screen.png')|Out-Null;Adb @('pull','/data/local/tmp/ir-s4-screen.png',(Join-Path $out "$Name.png"))|Out-Null}
$checks=[ordered]@{}
Tap 6 1
Start-Sleep -Seconds 5
$s=State '01-reset';$checks.initialHolding=$s.valid -and -not $s.trains[0].routeGranted -and -not $s.trains[1].routeGranted
Tap 2 2
Start-Sleep -Seconds 2
$s=State '02-route-a';$checks.routeA=$s.trains[0].routeGranted -and $s.routes[0].owner -eq 1 -and $s.signals[0].proceed
Tap 6 2
Start-Sleep -Seconds 1
Adb @('pull',"$saved/view-recreation.json",(Join-Path $out '03-view-recreation.json'))|Out-Null
$v=Get-Content (Join-Path $out '03-view-recreation.json') -Raw|ConvertFrom-Json
$checks.actorRecreation=$v.passed -and $v.logicalBefore.trains[0].routeGranted -and $v.oldActorId -ne $v.newActorId
Tap 1 2
Tap 3 2
Start-Sleep -Seconds 2
$s=State '04-conflict';$checks.conflictDenied=-not $s.trains[1].routeGranted -and -not $s.requests[1].granted -and $s.trains[0].routeGranted
Screen '04-conflict'
Tap 4 2
Start-Sleep -Seconds 2
$s=State '05-point-locked';$checks.pointLocked=$s.points[1].position -eq 1 -and $s.points[1].route -eq 1 -and -not $s.requests[1].granted
Tap 3 0
Start-Sleep -Seconds 22
$s=State '06-b-restrictive-stop';$checks.restrictiveStop=$s.trains[1].handling.headMetres -lt 130 -and $s.trains[1].handling.speedMps -eq 0 -and -not $s.faulted
Tap 0 2
Tap 2 0
Start-Sleep -Seconds 45
$s=State '07-a-moving';$checks.playerMovement=$s.trains[0].handling.speedMps -gt 0 -and $s.trains[0].handling.headMetres -gt 240 -and $s.trains[0].routeGranted
Screen '07-occupied'
Start-Sleep -Seconds 45
$s=State '08-front-passed';$checks.wholeTrainProtection=$s.trains[0].handling.headMetres -gt 501.327412 -and $s.trains[0].handling.headMetres -lt 682.327412 -and $s.routes[0].owner -eq 1 -and $s.blocks[4].occupants -contains 1
Start-Sleep -Seconds 60
$s=State '09-a-conservative-stop';$checks.heldBeforeRearClear=$s.routes[0].owner -eq 1 -and $s.trains[0].handling.speedMps -eq 0
Tap 4 0
Tap 3 0
Start-Sleep -Seconds 35
$s=State '10-rear-clear';$checks.rearRelease=$s.routes[0].owner -eq 0 -and $s.trains[0].handling.headMetres -gt 682.327412 -and $s.points[1].route -eq 0 -and $s.blocks[5].occupants.Count -eq 0 -and -not $s.faulted
Screen '10-rear-clear'
Tap 1 2
Tap 3 2
Start-Sleep -Seconds 2
$s=State '11-b-granted';$checks.bGrantAfterRelease=$s.trains[1].routeGranted -and $s.routes[1].owner -eq 2 -and $s.points[1].position -eq 0 -and $s.signals[1].proceed
if(-not $checks.bGrantAfterRelease){throw 'B route did not grant; stopped before driving'}
Tap 4 0
Tap 1 0
Start-Sleep -Seconds 22
$s=State '12-b-moved';$checks.bMovement=$s.trains[1].handling.headMetres -gt 130 -and $s.trains[1].handling.headMetres -le 165 -and -not $s.faulted
Tap 5 1
$before=State '13-paused'
Start-Sleep -Seconds 5
$after=State '14-pause-end';$checks.pause=$before.tick -eq $after.tick -and $after.suspended
Tap 5 1
Start-Sleep -Seconds 3
$s=State '15-resume';$checks.resume=$s.tick -gt $after.tick -and -not $s.suspended -and $s.trains[1].handling.throttle -eq 0 -and $s.trains[1].handling.brake -eq 1
$pidBefore=Adb @('shell','pidof',$package)
Adb @('shell','input','keyevent','KEYCODE_HOME')|Out-Null
Start-Sleep -Seconds 2
$background=State '16-background'
Start-Sleep -Seconds 6
$backgroundEnd=State '17-background-end';$checks.backgroundFrozen=($background|ConvertTo-Json -Depth 12 -Compress) -eq ($backgroundEnd|ConvertTo-Json -Depth 12 -Compress)
Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity")|Set-Content (Join-Path $out 'warm-launch.txt')
Start-Sleep -Seconds 3
$post=State '18-foreground';$pidAfter=Adb @('shell','pidof',$package)
$checks.lifecycle=$pidBefore -eq $pidAfter -and $post.tick -gt $backgroundEnd.tick -and -not $post.faulted -and $post.routes[1].owner -eq 2
Screen '18-final'
Adb @('shell','input','keyevent','KEYCODE_BACK')|Out-Null
Start-Sleep -Seconds 3
$remaining=& $adb -s $serial shell pidof $package
$checks.backExit=[string]::IsNullOrWhiteSpace(($remaining -join ' '))
[ordered]@{passed=($checks.Values -notcontains $false);checks=$checks;pidBefore=$pidBefore;pidAfter=$pidAfter}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $out 'results.json')
Get-Content (Join-Path $out 'results.json')
if($checks.Values -contains $false){exit 1};exit 0


