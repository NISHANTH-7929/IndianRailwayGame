param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[string]$SdkRoot=$env:ANDROID_HOME,[int]$CaptureFrames=0)
$ErrorActionPreference='Stop'
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project=Join-Path $workspace '03_UnrealProject\MyProject\MyProject.uproject'
$evidence=Join-Path $PSScriptRoot '..\Evidence\Cab'
& python (Join-Path $PSScriptRoot 'validate_s8.py') --output (Join-Path $evidence 'validation-results.json')
if($LASTEXITCODE -ne 0){throw 'S8 catalog validation failed; packaging stopped'}
$buildId=Get-Date -Format 'yyyyMMdd-HHmmss'
$env:NDKROOT=Join-Path $SdkRoot 'ndk\27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$env:ANDROID_HOME=$SdkRoot
$env:DOTNET_ROOT=Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64'
$env:DOTNET_MULTILEVEL_LOOKUP='0'
$env:DOTNET_ROLL_FORWARD='LatestMajor'
$dotnet=Join-Path $env:DOTNET_ROOT 'dotnet.exe'
$uat=Join-Path $EngineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
$runtime='-cmdline=-ExecCmds="r.SetFramePace 30" -log'
if($CaptureFrames -gt 0){$runtime="-cmdline=-csvCaptureFrames=$CaptureFrames -csvGpuStats -csvCompression=0 -ExecCmds=`"r.SetFramePace 30`" -log"}
$arguments=@($uat,'-WaitForUATMutex','BuildCookRun',"-project=$project",'-noP4','-platform=Android','-cookflavor=ASTC',
 '-clientconfig=Development','-build','-cook','-stage','-pak','-iostore','-package','-archive',
 "-archivedirectory=$workspace\08_Builds\S8_WAP7Cab",'-map=/Game/IndianRailway/S7/L_RailService+/Game/IndianRailway/S6/L_RailContinuity+/Game/IndianRailway/S6/L_ChunkA+/Game/IndianRailway/S6/L_ChunkB+/Game/IndianRailway/S5/L_RailAI+/Game/IndianRailway/S4/L_RailOperations+/Game/IndianRailway/S3/L_TrainHandling+/Game/IndianRailway/S2/L_RailKernel+/Game/IndianRailway/Validation/L_AndroidHarness',
 $runtime,'-unattended','-utf8output')
$log=Join-Path $evidence "package-$buildId.log"
Push-Location (Split-Path $uat)
try { & $dotnet @arguments *> $log; $buildExit=$LASTEXITCODE } finally { Pop-Location }
[ordered]@{buildId=$buildId;exitCode=$buildExit;arguments=$arguments;log=$log} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $evidence "package-$buildId-result.json")
exit $buildExit







