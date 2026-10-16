param(
 [Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$Label,
 [switch]$FreshFixture,
 [ValidateRange(20,180)][int]$TimeoutSeconds=100,
 [string]$Editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
)
$ErrorActionPreference='Stop'
$workspace=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../../..'))
$saved=Join-Path $workspace '03_UnrealProject/MyProject/Saved'
$isolated=Join-Path $saved 'S8/Egmore/AssetReview'
$out=Join-Path $workspace "07_Documentation/S8/Workstreams/StationArt/Device/$Label"
if(Test-Path -LiteralPath $out){throw 'Evidence directory already exists'}
if(!(Test-Path -LiteralPath $Editor)){throw 'Editor executable missing'}
if(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue){throw 'Close existing editor processes before this isolated test'}
New-Item -ItemType Directory -Path $out | Out-Null
$backup=Join-Path $out 'isolated-original'
$telemetry=Join-Path $isolated 'telemetry.json'
$last=Join-Path $isolated 'last-checkpoint.json'
$results=[Collections.Generic.List[object]]::new()
function NormalHashes {
 $h=@{}
 foreach($family in @('S7','S8/LHB/Service','S8/Station/Service','S8/Egmore/Service')){
  foreach($slot in 0,1){$p=Join-Path $saved "$family/checkpoint-$slot.irs";$h[$p]=if(Test-Path -LiteralPath $p){(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash}else{$null}}
 }
 return $h
}
function SlotHashes {
 $h=@{};foreach($slot in 0,1){$p=Join-Path $isolated "checkpoint-$slot.irs";$h[[string]$slot]=if(Test-Path -LiteralPath $p){(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash}else{$null}};return $h
}
function SameHashes($a,$b){foreach($key in $a.Keys){if($a[$key] -ne $b[$key]){return $false}};return $true}
function TreeHashes([string]$root){
 $h=@{};if(Test-Path -LiteralPath $root){foreach($file in Get-ChildItem -LiteralPath $root -Recurse -File -Force){$h[$file.FullName.Substring($root.Length)]=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}};return $h
}
function ClearIsolated {
 $resolved=[IO.Path]::GetFullPath($isolated)
 if($resolved -ne [IO.Path]::GetFullPath((Join-Path $saved 'S8/Egmore/AssetReview'))){throw 'Unexpected isolated target'}
 if(Test-Path -LiteralPath $resolved){
  foreach($item in Get-ChildItem -LiteralPath $resolved -Force){
   if($item.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Refusing isolated reparse point'}
   Remove-Item -LiteralPath $item.FullName -Recurse -Force
  }
 }else{New-Item -ItemType Directory -Path $resolved | Out-Null}
}
function RunCase([string]$name,[string]$mode,$expected){
 $caseDir=Join-Path $out $name;New-Item -ItemType Directory -Path $caseDir | Out-Null
 $log=Join-Path $caseDir 'engine.log'
 if(Test-Path -LiteralPath $telemetry){Remove-Item -LiteralPath $telemetry}
 $beforeSlots=SlotHashes
 $args=@(('"'+(Join-Path $workspace '03_UnrealProject/MyProject/MyProject.uproject')+'"'),'/Game/IndianRailway/S8/Egmore/L_RailService_Egmore','-game','-nullrhi','-unattended','-S8Egmore','-S8Station','-S8LHB','-S8AssetReview','-ExecCmds="t.MaxFPS 30"',('-abslog="'+$log+'"'))
 if($mode -ne 'seed'){$args+='-S8AssetReviewResume'}
 $proc=$null;$samples=0;$lastStamp=[datetime]::MinValue;$signature=$null;$ok=$false
 try{
  $proc=Start-Process -FilePath $Editor -ArgumentList $args -WindowStyle Hidden -PassThru
  $deadline=[datetime]::UtcNow.AddSeconds($TimeoutSeconds)
  while([datetime]::UtcNow -lt $deadline){
   Start-Sleep -Milliseconds 1200;$proc.Refresh();if($proc.HasExited){throw "$name exited before verification"}
   if(!(Test-Path -LiteralPath $telemetry)){continue}
   $stamp=(Get-Item -LiteralPath $telemetry).LastWriteTimeUtc;if($stamp -le $lastStamp){continue}
   try{$raw=[IO.File]::ReadAllText($telemetry);$s=$raw|ConvertFrom-Json}catch{continue}
   $lastStamp=$stamp
   if(!(Test-Path -LiteralPath $log)){continue}
   try{$logText=Get-Content -LiteralPath $log -Raw}catch{continue}
   if($logText -notmatch 'Bringing World /Game/IndianRailway/S8/Egmore/L_RailService_Egmore'){continue}
   $ready=$s.content -eq 'IR-S8-EGMORE-1' -and $s.logical.valid -and !$s.logical.faulted -and !$s.blocked -and $s.egmoreScene.ready -and $s.locomotive.ready -and $s.trafficLocomotive.ready
   if(!$ready){continue}
   if($mode -eq 'seed'){
    if($s.generation -lt 1 -or !(Test-Path -LiteralPath $last)){continue}
    $checkpoint=Get-Content -LiteralPath $last -Raw|ConvertFrom-Json
    if($checkpoint.generation -lt 1 -or $checkpoint.service.serviceId -ne 'S8-Egmore-01'){continue}
    $ok=$true;break
   }
   if(!$s.paused){throw "$name was not paused"}
   if($mode -eq 'resume'){
    if($logText -notmatch 'S8_ASSET_REVIEW_RESUME isolated=1 recovered=1 blocked=0'){throw 'Recovery did not succeed'}
    if($s.logicalFingerprint -ne $expected.logicalFingerprint -or $s.generation -ne $expected.generation -or ($s.service|ConvertTo-Json -Depth 40 -Compress) -ne ($expected.service|ConvertTo-Json -Depth 40 -Compress)){throw 'Restored fingerprint, generation or service differs from checkpoint evidence'}
   }else{
    if($logText -notmatch 'S8_ASSET_REVIEW_RESUME isolated=1 recovered=0 blocked=1'){throw "$name did not fail closed"}
    if($s.generation -ne 0 -or $s.logical.tick -ne 0 -or $s.service.phase -ne 'Selection'){throw "$name started a fresh run"}
   }
   $nextSignature="$($s.logicalFingerprint)/$($s.generation)/$($s.logical.tick)"
   if($signature -and $signature -ne $nextSignature){throw "$name changed while paused"}
   $signature=$nextSignature;++$samples
   [IO.File]::WriteAllText((Join-Path $caseDir "sample-$samples.json"),$raw)
   if($samples -ge 2){$ok=$true;break}
  }
  if(!$ok){throw "$name timed out"}
 }finally{
  if($proc){$proc.Refresh();if(!$proc.HasExited){Stop-Process -Id $proc.Id -Force;if(!$proc.WaitForExit(10000)){throw 'Test process did not stop'}}}
  $afterSlots=SlotHashes
  $slotsUnchanged=SameHashes $beforeSlots $afterSlots
  $normalUnchanged=SameHashes $normalBefore (NormalHashes)
  $results.Add([ordered]@{case=$name;passed=($ok -and $normalUnchanged -and ($mode -eq 'seed' -or $slotsUnchanged));freshStableSamples=$samples;checkpointWritesObserved=(!$slotsUnchanged);normalSavesUnchanged=$normalUnchanged;before=$beforeSlots;after=$afterSlots})
  if($mode -ne 'seed' -and !$slotsUnchanged){throw "$name changed checkpoint files"}
  if(!$normalUnchanged){throw "$name changed protected normal saves"}
 }
}
$normalBefore=NormalHashes
$existed=Test-Path -LiteralPath $isolated
$failure=$null;$restored=$false;$backupComplete=$false;$isolatedBefore=TreeHashes $isolated
try{
 if($existed){
  if(Get-ChildItem -LiteralPath $isolated -Recurse -Force|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Refusing reparse point in isolated directory'}
  Copy-Item -LiteralPath $isolated -Destination $backup -Recurse
  $backupHashes=TreeHashes $backup
  if($backupHashes.Count -ne $isolatedBefore.Count -or !(SameHashes $isolatedBefore $backupHashes)){throw 'Isolated backup verification failed'}
 }
 $backupComplete=$true
 if($FreshFixture){ClearIsolated;RunCase 'seed' 'seed' $null}
 if(!(Test-Path -LiteralPath $last)){throw 'No checkpoint evidence. Run explicitly with -FreshFixture to generate a temporary fixture.'}
 $expected=Get-Content -LiteralPath $last -Raw|ConvertFrom-Json
 if($expected.content -ne 'IR-S8-EGMORE-1' -or $expected.service.serviceId -ne 'S8-Egmore-01' -or $expected.generation -lt 1 -or $expected.logicalFingerprint -notmatch '^[0-9a-f]{16}$'){throw 'Existing checkpoint evidence is inconsistent; use -FreshFixture'}
 Copy-Item -LiteralPath $last -Destination (Join-Path $out 'expected-checkpoint.json')
 RunCase 'resume' 'resume' $expected
 foreach($slot in 0,1){$p=Join-Path $isolated "checkpoint-$slot.irs";if(Test-Path -LiteralPath $p){Remove-Item -LiteralPath $p}}
 RunCase 'missing' 'blocked' $null
 foreach($slot in 0,1){[IO.File]::WriteAllBytes((Join-Path $isolated "checkpoint-$slot.irs"),[byte[]]@(0,1,2,3))}
 RunCase 'corrupt' 'blocked' $null
}catch{$failure=$_.Exception.Message}
finally{
 # Backup remains in evidence even after successful restoration.
 if($backupComplete){
  ClearIsolated
  if($existed){foreach($item in Get-ChildItem -LiteralPath $backup -Force){Copy-Item -LiteralPath $item.FullName -Destination $isolated -Recurse}}
  else{Remove-Item -LiteralPath $isolated}
  $isolatedAfter=TreeHashes $isolated
  $restored=$isolatedBefore.Count -eq $isolatedAfter.Count -and (SameHashes $isolatedBefore $isolatedAfter)
 }
 $normalAfter=NormalHashes;$unchanged=SameHashes $normalBefore $normalAfter
 [ordered]@{passed=(!$failure -and $restored -and $unchanged);failure=$failure;isolatedRestored=$restored;normalSavesUnchanged=$unchanged;freshFixture=[bool]$FreshFixture;cases=@($results.ToArray());before=$normalBefore;after=$normalAfter;scope='Desktop hidden NullRHI cold recovery; no Android or visual claim'}|ConvertTo-Json -Depth 20|Set-Content -LiteralPath (Join-Path $out 'result.json')
}
if($failure){throw $failure}
if(!$restored -or !$unchanged){throw 'Restoration or normal checkpoint invariant failed'}
