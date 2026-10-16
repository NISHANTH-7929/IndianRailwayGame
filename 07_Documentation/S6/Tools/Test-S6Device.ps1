param([string]$Label='controls',[int]$FirstX=220,[int]$Spacing=393,[int]$TopY=1068,[int]$RowSpacing=75)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S6Device.ps1')
$package='com.indianrailway.s1harness'
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$saved="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S6"
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
function Foreground(){if(((Adb @('shell','dumpsys window | grep mCurrentFocus')) -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw 'Game not foreground; no touch sent'}}
function Tap([int]$Column,[int]$Row){Foreground;[ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');column=$Column;row=$Row}|ConvertTo-Json -Compress|Add-Content "$out/touches.jsonl";Adb @('shell','input','tap',"$($FirstX+$Column*$Spacing)","$($TopY+$Row*$RowSpacing)")|Out-Null}
function State([string]$Name){$raw=Adb @('shell','cat',"$saved/telemetry.json");$raw|Set-Content "$out/$Name.json";$s=$raw|ConvertFrom-Json;if($s.logical.faulted -or -not $s.logical.valid){throw "Invalid railway at $Name"};Write-Host "$Name tick=$($s.logical.tick) head=$($s.logical.trains[0].handling.headMetres) levels=$($s.loadedLevels) paused=$($s.paused)";return $s}
function Screen([string]$Name){Foreground;Adb @('shell','screencap','-p','/data/local/tmp/ir-s6-screen.png')|Out-Null;Adb @('pull','/data/local/tmp/ir-s6-screen.png',"$out/$Name.png")|Out-Null}
function Logical($s){return $s.logical|ConvertTo-Json -Depth 30 -Compress}
function Launch([string]$Name){Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity")|Set-Content "$out/$Name-launch.txt";Start-Sleep -Seconds 8;Foreground}
function Slots([string]$Name){Adb @('pull',"$saved/checkpoint-0.irs","$out/$Name-0.irs")|Out-Null;Adb @('pull',"$saved/checkpoint-1.irs","$out/$Name-1.irs")|Out-Null}
$checks=[ordered]@{}
Tap 6 1 # Explicit reset preserves existing checkpoints until a verified save.
Start-Sleep -Seconds 3
Tap 5 1 # Resume initial paused fixture.
Start-Sleep -Seconds 3
$s=State '01-ai-wait'
$checks.aiWait=$s.logical.ai[0].state -in @('WaitingForRoute','RequestingRoute') -and $s.logical.trains[1].handling.speedMps -eq 0 -and -not $s.logical.trains[1].routeGranted -and -not $s.logical.requests[1].granted
Screen '01-ai-wait'
Tap 3 0
Start-Sleep -Seconds 23
Tap 5 1
Start-Sleep -Seconds 2
$moving=State '02-moving-boundary-paused'
$head=$moving.logical.trains[0].handling.headMetres
$checks.boundary=$head -gt 240 -and $head -lt 421 -and $moving.logical.trains[0].handling.speedMps -gt 0
$checks.overlap=$moving.loadedLevels -eq 2 -and $moving.chunkActors -eq 2 -and $moving.visualVehicles -eq 10
Screen '02-boundary'
Tap 1 2
Start-Sleep -Seconds 2
$look=State '03-look-back';$checks.lookBack=$look.lookBack -and (Logical $look) -eq (Logical $moving)
Screen '03-look-back'
Tap 6 2
Start-Sleep -Milliseconds 800
$unload=State '04-unloading'
Start-Sleep -Seconds 6
$reload=State '05-reloaded'
$checks.streamingLogicalContinuity=(Logical $moving) -eq (Logical $reload) -and $moving.logicalFingerprint -eq $reload.logicalFingerprint -and $unload.logicalFingerprint -eq $moving.logicalFingerprint
$checks.realUnloadReload=$reload.loads -ge 4 -and $reload.unloads -ge 2 -and $reload.loadedLevels -eq 2
$checks.retiredActorsReleased=$reload.retiredActorsAlive -eq 0
$checks.retiredAssetsReleased=$reload.retiredMeshesAlive -eq 0
Screen '05-reloaded'
Tap 0 2
Start-Sleep -Seconds 2
$beforeKill=State '06-before-kill'
Slots 'moving'
Adb @('shell','am','force-stop',$package)|Out-Null
Launch 'moving-restore'
$afterKill=State '07-moving-restored'
$checks.offlineMovingRestore=$afterKill.paused -and (Logical $beforeKill) -eq (Logical $afterKill) -and $beforeKill.logicalFingerprint -eq $afterKill.logicalFingerprint
Screen '07-moving-restored'
Tap 5 1
Start-Sleep -Seconds 1
Tap 0 2
Start-Sleep -Milliseconds 250
$live=State '07b-running-before-kill'
Adb @('shell','am','force-stop',$package)|Out-Null
$raw=Adb @('shell','cat',"$saved/last-checkpoint.json");$raw|Set-Content "$out/07c-last-committed.json";$committed=$raw|ConvertFrom-Json
Launch 'live-moving-restore'
$liveRestored=State '07d-live-moving-restored'
$checks.killWhileRunning=-not $live.paused -and $live.logical.trains[0].handling.speedMps -gt 0 -and $liveRestored.paused -and $liveRestored.logicalFingerprint -eq $committed.logicalFingerprint
Tap 5 1
Tap 5 0
Start-Sleep -Seconds 3
Adb @('shell','input','keyevent','KEYCODE_HOME')|Out-Null
Start-Sleep -Seconds 3
$braking=State '08-background-braking'
Start-Sleep -Seconds 3
$frozen=State '09-background-frozen'
$checks.backgroundFrozen=(Logical $braking) -eq (Logical $frozen) -and $frozen.paused
$checks.brakingSaved=$braking.logical.trains[0].handling.brake -gt 0
$pidBefore=(Adb @('shell','pidof',$package)) -join ' '
Launch 'warm-foreground'
$warm=State '09b-warm-foreground'
$pidAfter=(Adb @('shell','pidof',$package)) -join ' '
$checks.warmForeground=$pidBefore -eq $pidAfter -and $warm.paused -and $warm.logicalFingerprint -eq $braking.logicalFingerprint
Adb @('shell','am','force-stop',$package)|Out-Null
Launch 'braking-restore'
$restored=State '10-braking-restored'
$checks.brakingRestore=$restored.paused -and (Logical $braking) -eq (Logical $restored) -and $restored.logicalFingerprint -eq $braking.logicalFingerprint
$checks.waitingRestore=$restored.logical.ai[0].state -eq $braking.logical.ai[0].state
Screen '10-braking-restored'
Slots 'braking'
# Preserve both originals before corrupting only the newer inactive-on-restart slot.
$records=0..1|ForEach-Object {$bytes=[IO.File]::ReadAllBytes("$out/braking-$_.irs");[pscustomobject]@{slot=$_;generation=[BitConverter]::ToUInt64($bytes,12);bytes=$bytes}}
$newest=$records|Sort-Object generation -Descending|Select-Object -First 1
$older=$records|Sort-Object generation|Select-Object -First 1
Adb @('shell','am','force-stop',$package)|Out-Null
$corrupt=[byte[]]$newest.bytes.Clone();$corrupt[$corrupt.Length-1]=$corrupt[$corrupt.Length-1] -bxor 1
[IO.File]::WriteAllBytes("$out/corrupt-newest.irs",$corrupt)
Adb @('push',"$out/corrupt-newest.irs","$saved/checkpoint-$($newest.slot).irs")|Out-Null
Launch 'corruption-restore'
$fallback=State '11-corruption-fallback'
$checks.corruptFallback=$fallback.paused -and $fallback.recovery.StartsWith("Restored generation $($older.generation) paused") -and $fallback.logicalFingerprint -eq $braking.logicalFingerprint -and $fallback.recovery -match 'checksum mismatch'
Screen '11-corruption-fallback'
Tap 5 1
Start-Sleep -Seconds 4
$continued=State '12-continued';$checks.continued=$continued.logical.tick -gt $fallback.logical.tick
Tap 0 0
Tap 6 0
Start-Sleep -Seconds 15
$stopped=State '12b-boundary-stopped'
$checks.boundaryStop=$stopped.logical.trains[0].handling.speedMps -eq 0 -and $stopped.logical.trains[0].handling.headMetres -gt 240 -and $stopped.logical.trains[0].handling.headMetres -lt 421 -and $stopped.logical.routes[0].owner -eq 1
Screen '12b-boundary-stopped'
Foreground;Adb @('shell','input','keyevent','KEYCODE_BACK')|Out-Null
Start-Sleep -Seconds 4
$remaining=& $adb -s $serial shell pidof $package
$checks.backExit=[string]::IsNullOrWhiteSpace(($remaining -join ' '))
Launch 'back-restore'
$back=State '13-back-restored';$checks.backRecovery=$back.paused -and $back.generation -ge $fallback.generation
Adb @('shell','dumpsys','package',$package)|Set-Content "$out/package-permissions.txt"
$permissions=Get-Content -Raw "$out/package-permissions.txt"
$checks.noInternetPermission=$permissions -notmatch 'android.permission.INTERNET'
[ordered]@{passed=($checks.Values -notcontains $false);checks=$checks}|ConvertTo-Json -Depth 6|Set-Content "$out/results.json"
Get-Content "$out/results.json"
if($checks.Values -contains $false){exit 1}

