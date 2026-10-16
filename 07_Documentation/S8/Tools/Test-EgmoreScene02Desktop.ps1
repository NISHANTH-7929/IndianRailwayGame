param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$project=Join-Path $workspace '03_UnrealProject/MyProject/MyProject.uproject'
$evidence=Join-Path $workspace '07_Documentation/S8/Evidence/EgmoreScene02'
$saved=Join-Path $workspace '03_UnrealProject/MyProject/Saved'
function SaveHashes {
 $result=@{}
 Get-ChildItem -LiteralPath $saved -Recurse -Filter 'checkpoint-*.irs' -ErrorAction SilentlyContinue | ForEach-Object {$result[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}
 return $result
}
$before=SaveHashes
$before|ConvertTo-Json|Set-Content (Join-Path $evidence 'desktop-checkpoints-before.json')
$log=Join-Path $evidence 'runtime-tour.log'
$arguments=@(('"'+$project+'"'),'/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study','-game','-nullrhi','-unattended','-nosplash','-S8EgmoreCameraTour','-ExecCmds="t.MaxFPS 30"',('-abslog="'+$log+'"'))
$process=Start-Process -FilePath "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" -ArgumentList $arguments -WindowStyle Hidden -PassThru
$started=Get-Date
$seen=@();$timedOut=$false
try {
 while(!$process.HasExited){
  Start-Sleep -Seconds 2
  if(Test-Path $log){$seen=@(Select-String -Path $log -Pattern 'S8_EGMORE_SCENE02_REVIEW (.+)' | ForEach-Object {$_.Matches[0].Groups[1].Value|ConvertFrom-Json})}
  if(@($seen.view|Sort-Object -Unique).Count -eq 6){break}
  if(((Get-Date)-$started).TotalSeconds -gt 130){$timedOut=$true;break}
  $process.Refresh()
 }
} finally {if(!$process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit()}}
$after=SaveHashes
$unchanged=($before.Count -eq $after.Count)
foreach($key in $before.Keys){if($after[$key] -ne $before[$key]){$unchanged=$false}}
$counts=($seen.Count -ge 6 -and @($seen|Where-Object {$_.placements -ne 1316 -or $_.modules -ne 40 -or $_.gameplayBound -ne $false}).Count -eq 0)
$passed=(!$timedOut -and @($seen.view|Sort-Object -Unique).Count -eq 6 -and $counts -and $unchanged)
@{passed=$passed;views=$seen;checkpointsUnchanged=$unchanged;before=$before;after=$after;timedOut=$timedOut;scope='NullRHI runtime camera/identity smoke; visual capture is separate';processStoppedAfterTour=$true}|ConvertTo-Json -Depth 7|Set-Content (Join-Path $evidence 'runtime-tour-result.json')
if(!$passed){throw 'Scene02 runtime camera smoke failed'}
