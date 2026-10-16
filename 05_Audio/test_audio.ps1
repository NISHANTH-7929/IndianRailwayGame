param([string]$EvidenceDirectory='')
$ErrorActionPreference='Stop'
$workspace=Split-Path $PSScriptRoot
$project=Join-Path $workspace '03_UnrealProject/MyProject/MyProject.uproject'
$saved=Join-Path $workspace '03_UnrealProject/MyProject/Saved'
$evidence=if($EvidenceDirectory){$EvidenceDirectory}else{Join-Path $PSScriptRoot 'Pixabay_20261009'}
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
function SaveHashes {
 $result=@{}
 Get-ChildItem -LiteralPath $saved -Recurse -Filter 'checkpoint-*.irs' | Where-Object {$_.FullName -notlike '*\S8\AudioReview\*'} | ForEach-Object {$result[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName).Hash}
 return $result
}
$before=SaveHashes
$log=Join-Path $evidence 'runtime.log'
$arguments=@(('"'+$project+'"'),'/Game/IndianRailway/S7/L_RailService','-game','-RenderOffscreen','-windowed','-ResX=640','-ResY=360','-unattended','-nosplash','-S8AssetReview','-S8AudioReview','-S8DisableLocomotive','-ExecCmds="t.MaxFPS 30"',('-abslog="'+$log+'"'))
$process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
$started=Get-Date
try {
 while(!$process.HasExited){
  Start-Sleep -Seconds 2
  if(Test-Path $log){
   $content=Get-Content -LiteralPath $log -Raw
   if($content -match 'S8_REVIEW_PHASE phase=3 '){break}
  }
  if(((Get-Date)-$started).TotalSeconds -gt 180){throw 'Audio driving test timed out'}
  $process.Refresh()
 }
} finally {
 if(!$process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit()}
 $after=SaveHashes
 $unchanged=$before.Count -eq $after.Count
 foreach($key in $before.Keys){if($after[$key] -ne $before[$key]){$unchanged=$false}}
 $content=Get-Content -LiteralPath $log -Raw
 $result=[ordered]@{
  assetsLoaded=([regex]::Matches($content,'RAIL_AUDIO loaded ').Count -eq 5)
  hornTriggered=($content -match 'RAIL_AUDIO horn')
  brakeTriggered=($content -match 'RAIL_AUDIO brake')
  railPlaying=($content -match 'RAIL_AUDIO_STATE running=1 rail=1')
  stationPlaying=($content -match 'RAIL_AUDIO_STATE running=1 rail=\d station=1')
  normalCheckpointsUnchanged=$unchanged
  auditoryApproval='Pending human listening; playback state is not acoustic validation'
  log=$log
 }
 $result|ConvertTo-Json|Set-Content (Join-Path $evidence 'runtime-result.json')
 $result|ConvertTo-Json
}
if(!$result.assetsLoaded -or !$result.hornTriggered -or !$result.brakeTriggered -or !$result.railPlaying -or !$result.stationPlaying -or !$unchanged){throw 'Audio runtime checks failed'}
