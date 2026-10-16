param([Parameter(Mandatory=$true)][string]$Label)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[a-zA-Z0-9-]+$'){throw 'Invalid label'}
$workspace='D:/Project/IndianRailwayGame'
$out=Join-Path $workspace "07_Documentation/S8/Workstreams/StationArt/Device/$Label"
if(Test-Path -LiteralPath $out){throw 'Evidence already exists'}
New-Item -ItemType Directory -Path $out | Out-Null
$saved=Join-Path $workspace '03_UnrealProject/MyProject/Saved'
function NormalHashes {
 $hashes=@{}
 foreach($family in @('S7','S8/LHB/Service','S8/Station/Service','S8/Egmore/Service')){
  foreach($slot in 0,1){$path=Join-Path $saved "$family/checkpoint-$slot.irs"; $hashes[$path]=if(Test-Path -LiteralPath $path){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}else{$null}}
 }
 return $hashes
}
$before=NormalHashes
$telemetry=Join-Path $saved 'S8/Egmore/AssetReview/telemetry.json'
$initial=if(Test-Path -LiteralPath $telemetry){[IO.File]::ReadAllBytes($telemetry)}else{$null}
$log=Join-Path $out 'engine.log'
$arguments=@(('"'+$workspace+'/03_UnrealProject/MyProject/MyProject.uproject"'),'/Game/IndianRailway/S8/Egmore/L_RailService_Egmore','-game','-nullrhi','-unattended','-S8Egmore','-S8Station','-S8LHB','-S8AssetReview','-ExecCmds="t.MaxFPS 30"',('-abslog="'+$log+'"'))
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
$deadline=(Get-Date).AddSeconds(100)
$passed=$false
try {
 while(!$process.HasExited -and (Get-Date) -lt $deadline){
  Start-Sleep -Seconds 2
  if(Test-Path -LiteralPath $telemetry){
   $bytes=[IO.File]::ReadAllBytes($telemetry)
   if($initial -and [Convert]::ToBase64String($initial) -eq [Convert]::ToBase64String($bytes)){continue}
   try{$state=Get-Content -LiteralPath $telemetry -Raw | ConvertFrom-Json}catch{continue}
   $loaded=(Test-Path -LiteralPath $log) -and (Select-String -LiteralPath $log -SimpleMatch 'LogWorld: Bringing World /Game/IndianRailway/S8/Egmore/L_RailService_Egmore.L_RailService_Egmore up for play' -Quiet)
   if($loaded -and $state.content -eq 'IR-S8-EGMORE-1' -and $state.service.serviceId -eq 'S8-Egmore-01' -and $state.logical.valid -and !$state.logical.faulted -and !$state.blocked -and $state.egmoreScene.ready -and $state.egmoreScene.placements -eq 2594 -and $state.egmoreScene.modules -eq 39 -and $state.trafficLocomotive.ready -and $state.locomotive.ready -and $state.logical.tick -ge 120){
    Copy-Item -LiteralPath $telemetry -Destination (Join-Path $out 'telemetry.json');$passed=$true;break
   }
  }
  $process.Refresh()
 }
} finally {
 if(!$process.HasExited){Stop-Process -Id $process.Id;$process.WaitForExit()}
 if(Test-Path -LiteralPath $telemetry){Copy-Item -LiteralPath $telemetry -Destination (Join-Path $out 'final-telemetry.json')}
 $after=NormalHashes;$unchanged=$true
 foreach($path in $before.Keys){if($before[$path] -ne $after[$path]){$unchanged=$false}}
 [ordered]@{passed=($passed -and $unchanged);normalSavesUnchanged=$unchanged;before=$before;after=$after;scope='NullRHI readiness smoke only, not visual or completed service';processStopped=$true}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $out 'result.json')
}
if(!$passed -or !$unchanged){throw 'Playable readiness smoke failed'}
