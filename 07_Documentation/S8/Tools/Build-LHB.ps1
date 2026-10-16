param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[string]$SdkRoot=$env:ANDROID_HOME,[int]$CaptureFrames=0,[switch]$Station,[switch]$EgmoreReview,[switch]$EgmoreScene02,[switch]$EgmoreService)
$ErrorActionPreference='Stop'
if($EgmoreService){$EgmoreScene02=$true}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project=Join-Path $workspace '03_UnrealProject\MyProject\MyProject.uproject'
$evidence=Join-Path $PSScriptRoot '..\Evidence\LHB'
if($Station){$evidence=Join-Path $PSScriptRoot '..\Evidence\Station'}
if($EgmoreReview){$evidence=Join-Path $PSScriptRoot '..\Evidence\Egmore'}
if($EgmoreScene02){$evidence=Join-Path $PSScriptRoot '..\Evidence\EgmoreScene02'; & python (Join-Path $PSScriptRoot 'validate_egmore_scene02.py'); if($LASTEXITCODE -ne 0){throw 'Scene02 import boundary validation failed'}}
& python (Join-Path $PSScriptRoot 'validate_s8.py') --output (Join-Path $evidence 'validation-results.json')
if($LASTEXITCODE -ne 0){throw 'S8 catalog validation failed; packaging stopped'}
& python (Join-Path $PSScriptRoot 'validate_lhb.py')
if($LASTEXITCODE -ne 0){throw 'LHB validation failed'} 
& python (Join-Path $PSScriptRoot 'validate_wap7_cab.py') --output (Join-Path $evidence ('cab-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.json'))
if($LASTEXITCODE -ne 0){throw 'WAP-7 cab source/runtime validation failed'}
if($Station){
 & python (Join-Path $PSScriptRoot 'validate_station_kit.py')
 if($LASTEXITCODE -ne 0){throw 'Station source/import validation failed'}
}
if($EgmoreReview){
 & python (Join-Path $PSScriptRoot 'validate_egmore_kit.py')
 if($LASTEXITCODE -ne 0){throw 'Egmore source/import validation failed'}
 & python (Join-Path $PSScriptRoot 'validate_egmore_sign.py')
 if($LASTEXITCODE -ne 0){throw 'Egmore signage validation failed'}
}
$buildId=Get-Date -Format 'yyyyMMdd-HHmmss'
$env:NDKROOT=Join-Path $SdkRoot 'ndk\27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$env:ANDROID_HOME=$SdkRoot
$env:DOTNET_ROOT=Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64'
$env:DOTNET_MULTILEVEL_LOOKUP='0'
$env:DOTNET_ROLL_FORWARD='LatestMajor'
$dotnet=Join-Path $env:DOTNET_ROOT 'dotnet.exe'
$uat=Join-Path $EngineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
$runtime='-cmdline=/Game/IndianRailway/S8/LHB/L_RailService_LHB -S8LHB -ExecCmds="r.SetFramePace 30" -log'
if($CaptureFrames -gt 0){$runtime="-cmdline=/Game/IndianRailway/S8/LHB/L_RailService_LHB -S8LHB -csvCaptureFrames=$CaptureFrames -csvGpuStats -csvCompression=0 -ExecCmds=`"r.SetFramePace 30`" -log"}
$arguments=@($uat,'BuildCookRun',"-project=$project",'-noP4','-platform=Android','-cookflavor=ASTC',
 '-clientconfig=Development','-build','-cook','-stage','-pak','-iostore','-package','-archive',
 "-archivedirectory=$workspace\08_Builds\S8_LHB",'-map=/Game/IndianRailway/S8/LHB/L_RailService_LHB+/Game/IndianRailway/S7/L_RailService+/Game/IndianRailway/S6/L_RailContinuity+/Game/IndianRailway/S6/L_ChunkA+/Game/IndianRailway/S6/L_ChunkB+/Game/IndianRailway/S5/L_RailAI+/Game/IndianRailway/S4/L_RailOperations+/Game/IndianRailway/S3/L_TrainHandling+/Game/IndianRailway/S2/L_RailKernel+/Game/IndianRailway/Validation/L_AndroidHarness',
 $runtime,'-unattended','-utf8output')
if($Station){
 $arguments=$arguments | ForEach-Object {
  if($_ -like '-archivedirectory=*'){"-archivedirectory=$workspace\08_Builds\S8_Station"}
  elseif($_ -like '-map=*'){$_ -replace '^-map=','-map=/Game/IndianRailway/S8/Stations/L_RailService_Station+'}
  elseif($_ -like '-cmdline=*'){($_ -replace '/Game/IndianRailway/S8/LHB/L_RailService_LHB','/Game/IndianRailway/S8/Stations/L_RailService_Station') -replace '-S8LHB','-S8LHB -S8Station'}
  else {$_}
 }
}
$log=Join-Path $evidence "package-$buildId.log"
if($EgmoreReview){
 $arguments=$arguments | ForEach-Object {
  if($_ -like '-archivedirectory=*'){"-archivedirectory=$workspace\08_Builds\S8_EgmoreReview"}
  elseif($_ -like '-map=*'){$_ -replace '^-map=','-map=/Game/IndianRailway/S8/Review/L_EgmoreHeritage_Study+'}
  else {$_}
 }
}
if($EgmoreScene02){
 $arguments=$arguments | ForEach-Object {
  if($_ -like '-archivedirectory=*'){"-archivedirectory=$workspace\08_Builds\S8_EgmoreScene02_$buildId"}
  elseif($_ -like '-map=*'){$_ -replace '^-map=','-map=/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study+/Game/IndianRailway/S8/Stations/L_RailService_Station+'}
  elseif($_ -like '-cmdline=*'){'-cmdline=/Game/IndianRailway/S8/Review/L_EgmoreScene02_Study -S8EgmoreCameraTour -ExecCmds="r.SetFramePace 30" -log'}
  else {$_}
 }
}
if($EgmoreService){
 $arguments=$arguments | ForEach-Object {
  if($_ -like '-archivedirectory=*'){"-archivedirectory=$workspace\08_Builds\S8_EgmoreService_$buildId"}
  elseif($_ -like '-map=*'){$_ -replace '^-map=','-map=/Game/IndianRailway/S8/Egmore/L_RailService_Egmore+/Game/IndianRailway/S8/Egmore/L_EgmoreChunkA+/Game/IndianRailway/S8/Egmore/L_EgmoreChunkB+'}
  elseif($_ -like '-cmdline=*'){'-cmdline=/Game/IndianRailway/S8/Egmore/L_RailService_Egmore -S8Egmore -S8Station -S8LHB -ExecCmds="r.SetFramePace 30" -log'}
  else {$_}
 }
}
Push-Location (Split-Path $uat)
try { & $dotnet @arguments *> $log; $buildExit=$LASTEXITCODE } finally { Pop-Location }
[ordered]@{buildId=$buildId;exitCode=$buildExit;arguments=$arguments;log=$log} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $evidence "package-$buildId-result.json")
exit $buildExit










