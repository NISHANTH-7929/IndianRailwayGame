param([string]$Label='ai-controls',[int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1068,[int]$RowSpacing=75)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S5Device.ps1')
$package='com.indianrailway.s1harness'
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$saved="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S5"
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
function Foreground(){
 $focus=Adb @('shell','dumpsys window | grep mCurrentFocus')
 if(($focus -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw "Game is covered or not foreground; no touch sent: $focus"}
}
function Tap([int]$Column,[int]$Row){Foreground;[ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');column=$Column;row=$Row;x=$FirstX+$Column*$Spacing;y=$TopY+$Row*$RowSpacing}|ConvertTo-Json -Compress|Add-Content (Join-Path $out 'touch-events.jsonl');Adb @('shell','input','tap',"$($FirstX+$Column*$Spacing)","$($TopY+$Row*$RowSpacing)") | Out-Null}
function State([string]$Name){$raw=Adb @('shell','cat',"$saved/telemetry.json");$raw|Set-Content (Join-Path $out "$Name.json");$s=$raw|ConvertFrom-Json;if($Name -match '^(0[1-9]|10)-' -and $s.suspended){throw "Unexpected pause before scheduled lifecycle checks: $Name"};if($s.faulted -or -not $s.valid){throw "Invalid simulation state: $Name"};Write-Host "$Name tick=$($s.tick) valid=$($s.valid) A=$($s.trains[0].handling.headMetres) B=$($s.trains[1].handling.headMetres)";return $s}
function Screen([string]$Name){Foreground;Adb @('shell','screencap','-p','/data/local/tmp/ir-s5-screen.png')|Out-Null;Adb @('pull','/data/local/tmp/ir-s5-screen.png',(Join-Path $out "$Name.png"))|Out-Null}
$checks=[ordered]@{}
Tap 6 1
Start-Sleep -Seconds 6
$s=State '01-reset';$checks.initialPlayerRoute=$s.valid -and $s.trains[0].routeGranted -and -not $s.trains[1].routeGranted -and $s.ai[0].train -eq 2
Tap 1 2
Start-Sleep -Seconds 3
$s=State '02-ai-wait';$checks.deniedAndStopped=-not $s.requests[1].granted -and $s.trains[1].handling.speedMps -eq 0 -and $s.trains[1].handling.headMetres -eq 100
Screen '02-ai-wait'
Tap 3 0
Start-Sleep -Seconds 2
$s=State '03-manual-b-rejected';$checks.aiOwnership=$s.trains[1].handling.throttle -eq 0 -and $s.trains[1].handling.headMetres -eq 100
Tap 6 2
Start-Sleep -Seconds 1
Adb @('pull',"$saved/view-recreation.json",(Join-Path $out '04-view-recreation.json'))|Out-Null
Adb @('pull',"$saved/train-view-recreation.json",(Join-Path $out '04-train-view-recreation.json'))|Out-Null
$v=Get-Content (Join-Path $out '04-train-view-recreation.json') -Raw|ConvertFrom-Json
$checks.actorRecreation=$v.passed -and $v.oldActorId -ne $v.newActorId -and ($v.logicalBefore.ai|ConvertTo-Json -Compress) -eq ($v.logicalAfter.ai|ConvertTo-Json -Compress)
Tap 0 2
Tap 4 0
Tap 2 0
Start-Sleep -Seconds 45
$s=State '05-player-moving';$checks.playerMoves=$s.trains[0].handling.headMetres -gt 240 -and $s.trains[0].handling.speedMps -gt 0 -and $s.trains[1].handling.headMetres -eq 100
Start-Sleep -Seconds 45
$s=State '06-front-passed';$checks.rearStillProtects=$s.trains[0].handling.headMetres -gt 501.327412 -and $s.trains[0].handling.headMetres -lt 682.327412 -and $s.routes[0].owner -eq 1 -and $s.trains[1].handling.headMetres -eq 100
Screen '06-rear-protection'
Start-Sleep -Seconds 60
$s=State '07-player-stop';$checks.waitBeforeClear=$s.routes[0].owner -eq 1 -and $s.trains[0].handling.speedMps -eq 0 -and $s.trains[1].handling.speedMps -eq 0
Tap 4 0
Tap 3 0
Start-Sleep -Seconds 30
$s=State '08-handoff';$checks.rearRelease=$s.routes[0].owner -eq 0 -and $s.trains[0].handling.headMetres -gt 682.327412
$checks.aiGrant=$s.trains[1].routeGranted -and $s.routes[1].owner -eq 2 -and $s.points[1].position -eq 0
Start-Sleep -Seconds 30
Tap 1 2
$s=State '09-ai-endpoint';$checks.autonomousMovement=$s.trains[1].handling.headMetres -gt 130 -and $s.trains[1].handling.headMetres -lt 165
$checks.endpointStop=$s.trains[1].handling.speedMps -eq 0 -and $s.ai[0].state -eq 'Stopped' -and $s.routes[1].owner -eq 2 -and -not $s.faulted
Screen '09-ai-endpoint'
Tap 6 2
Start-Sleep -Seconds 1
Adb @('pull',"$saved/train-view-recreation.json",(Join-Path $out '10-final-recreation.json'))|Out-Null
$v=Get-Content (Join-Path $out '10-final-recreation.json') -Raw|ConvertFrom-Json
$checks.recreateAfterMovement=$v.passed -and $v.logicalAfter.ai[0].state -eq 'Stopped'
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





