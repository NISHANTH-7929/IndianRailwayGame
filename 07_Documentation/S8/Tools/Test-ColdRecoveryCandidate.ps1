param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid evidence label'}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial='10BE1Y2EL40003J'
$package='com.indianrailway.s1harness'
$base="/sdcard/Android/data/$package/files/UnrealGame/MyProject"
$remote="$base/UECommandLine.txt"
$out=Join-Path $workspace "07_Documentation/S8/Evidence/Station/$Label-wrapper"
if(Test-Path -LiteralPath $out){throw 'Evidence already exists'}
New-Item -ItemType Directory -Path $out | Out-Null
$wake=(& $adb -s $serial shell settings get global stay_on_while_plugged_in).Trim()
if($wake -notmatch '^[0-7]$'){throw 'Wake setting unavailable'}
$paths=@();foreach($folder in @('S7','S8/LHB/Service','S8/Station/Service')){foreach($slot in 0,1){$paths+="$base/MyProject/Saved/$folder/checkpoint-$slot.irs"}}
$before=@(& $adb -s $serial shell sha256sum @paths)
if($LASTEXITCODE -ne 0){throw 'Cannot preserve all six save hashes'}
$before | Set-Content (Join-Path $out 'saves-before.txt')
& $adb -s $serial shell test -f $remote
$existed=$LASTEXITCODE -eq 0
if($existed){& $adb -s $serial pull $remote (Join-Path $out 'commandline-before.txt') | Out-Null;if($LASTEXITCODE -ne 0){throw 'Commandline backup failed'}}
[ordered]@{wakeBefore=$wake;commandlineExisted=$existed} | ConvertTo-Json | Set-Content (Join-Path $out 'before.json')
$passed=$false
try {
 & $adb -s $serial shell settings put global stay_on_while_plugged_in 3
 if($LASTEXITCODE -ne 0){throw 'Temporary wake setting failed'}
 & python (Join-Path $PSScriptRoot 'verify_station_v6_recovery.py') --prefix $Label *> (Join-Path $out 'recovery.txt')
 if($LASTEXITCODE -ne 0){throw 'Cold recovery check failed'}
 $passed=$true
} finally {
 & $adb -s $serial shell am force-stop $package
 if($existed){& $adb -s $serial push (Join-Path $out 'commandline-before.txt') $remote | Out-Null}else{& $adb -s $serial shell rm -f $remote}
 $commandRestored=$LASTEXITCODE -eq 0
 if($existed -and $commandRestored){
  & $adb -s $serial pull $remote (Join-Path $out 'commandline-after.txt') | Out-Null
  $commandRestored=$LASTEXITCODE -eq 0 -and (Get-FileHash (Join-Path $out 'commandline-before.txt')).Hash -eq (Get-FileHash (Join-Path $out 'commandline-after.txt')).Hash
 }
 & $adb -s $serial shell settings put global stay_on_while_plugged_in $wake
 $response=& $adb -s $serial shell settings get global stay_on_while_plugged_in
 $wakeAfter=if($null -ne $response){("$response").Trim()}else{'unavailable'}
 $after=@(& $adb -s $serial shell sha256sum @paths)
 $saveOk=$LASTEXITCODE -eq 0 -and ($before -join "`n") -eq ($after -join "`n")
 $after | Set-Content (Join-Path $out 'saves-after.txt')
 [ordered]@{passed=$passed;commandlineRestored=$commandRestored;sixSavesUnchanged=$saveOk;wakeBefore=$wake;wakeAfter=$wakeAfter;wakeRestored=($wakeAfter -eq $wake)} | ConvertTo-Json | Set-Content (Join-Path $out 'result.json')
 & $adb -s $serial shell am start -W -n "$package/com.epicgames.unreal.SplashActivity" *> (Join-Path $out 'normal-launch.txt')
 if(-not $commandRestored -or -not $saveOk -or $wakeAfter -ne $wake){throw 'Restoration incomplete; inspect recorded evidence'}
}
