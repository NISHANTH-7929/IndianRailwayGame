param([string]$Label='compatibility-final',[int]$TopY=1068)
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S6Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out|Out-Null
$package='com.indianrailway.s1harness'
$saved="/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved/S6"
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
function StopGame(){Adb @('shell','am','force-stop',$package)|Out-Null}
function Foreground(){if(((Adb @('shell','dumpsys window | grep mCurrentFocus')) -join ' ') -notmatch 'com\.indianrailway\.s1harness'){throw 'Game not foreground; no input sent'}}
function Launch([string]$Name){Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity")|Set-Content "$out/$Name-launch.txt";Start-Sleep -Seconds 8;Foreground}
function State([string]$Name){$raw=Adb @('shell','cat',"$saved/telemetry.json");$raw|Set-Content "$out/$Name.json";return ($raw|ConvertFrom-Json)}
function RestoreFiles(){foreach($i in 0..1){Adb @('push',"$out/original-$i.irs","$saved/checkpoint-$i.irs")|Out-Null}}
function RemoteHash([int]$Index){return ((Adb @('shell','sha256sum',"$saved/checkpoint-$Index.irs")) -join ' ').Trim().Split(' ')[0]}
StopGame
foreach($i in 0..1){Adb @('pull',"$saved/checkpoint-$i.irs","$out/original-$i.irs")|Out-Null}
$records=0..1|ForEach-Object {$b=[IO.File]::ReadAllBytes("$out/original-$_.irs");[pscustomobject]@{slot=$_;generation=[BitConverter]::ToUInt64($b,12)}}
$newest=$records|Sort-Object generation -Descending|Select-Object -First 1
$older=$records|Sort-Object generation|Select-Object -First 1
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/ThirdParty/Python3/Win64/python.exe' (Join-Path $PSScriptRoot 'make_invalid_records.py') "$out/original-$($newest.slot).irs" "$out/variants"
if($LASTEXITCODE){throw 'Cannot prepare controlled record copies'}
$checks=[ordered]@{}
try {
 foreach($variant in @('checksum','truncated','schema-zero','schema-future','content-missing')){
  StopGame;RestoreFiles
  Adb @('push',"$out/variants/$variant.irs","$saved/checkpoint-$($newest.slot).irs")|Out-Null
  Launch $variant
  $state=State $variant
  $checks["$variant-fallback"]=$state.paused -and $state.logical.valid -and -not $state.logical.faulted -and $state.recovery.StartsWith("Restored generation $($older.generation) paused")
  if($variant -in @('schema-zero','schema-future','content-missing')){
   Foreground;Adb @('shell','input','tap','220',"$($TopY+150)")|Out-Null
   Start-Sleep -Seconds 2
   $after=State "$variant-save-attempt"
   $expected=(Get-FileHash "$out/variants/$variant.irs" -Algorithm SHA256).Hash
   $backup=(Get-FileHash "$out/original-$($older.slot).irs" -Algorithm SHA256).Hash
   $checks["$variant-retained"]=(RemoteHash $newest.slot) -ieq $expected -and (RemoteHash $older.slot) -ieq $backup -and $after.save -match 'Incompatible checkpoint retained'
  }
  Write-Output "Checked $variant"
 }
 StopGame
 foreach($i in 0..1){Adb @('push',"$out/variants/schema-future.irs","$saved/checkpoint-$i.irs")|Out-Null}
 Launch 'no-compatible-checkpoint'
 $before=State 'no-compatible-before-resume'
 Foreground;Adb @('shell','input','tap','2185',"$($TopY+75)")|Out-Null
 Start-Sleep -Seconds 3
 $after=State 'no-compatible-after-resume'
 $checks.noCompatibleRemainsPaused=$before.paused -and $after.paused -and $after.logical.tick -eq 0 -and -not $after.logical.trains[0].routeGranted -and -not $after.logical.trains[1].routeGranted
}finally{
 StopGame;RestoreFiles;Launch 'restored-originals'
}
$final=State 'restored-originals'
$checks.originalsRecover=$final.paused -and $final.logical.valid -and $final.recovery.StartsWith("Restored generation $($newest.generation) paused")
[ordered]@{passed=($checks.Values -notcontains $false);checks=$checks;olderGeneration=$older.generation;newerGeneration=$newest.generation}|ConvertTo-Json -Depth 5|Set-Content "$out/results.json"
Get-Content "$out/results.json"
if($checks.Values -contains $false){exit 1}
