param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[string]$SdkRoot=$env:ANDROID_HOME)
$ErrorActionPreference='Stop'
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project=Join-Path $workspace '03_UnrealProject\MyProject\MyProject.uproject'
$evidence=Join-Path $PSScriptRoot '..\Evidence'
$buildId=Get-Date -Format 'yyyyMMdd-HHmmss'
$env:NDKROOT=Join-Path $SdkRoot 'ndk\27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$env:ANDROID_HOME=$SdkRoot
$env:DOTNET_ROOT=Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64'
$env:DOTNET_MULTILEVEL_LOOKUP='0'
$env:DOTNET_ROLL_FORWARD='LatestMajor'
$dotnet=Join-Path $env:DOTNET_ROOT 'dotnet.exe'
$uat=Join-Path $EngineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
$arguments=@($uat,'BuildCookRun',"-project=$project",'-noP4','-platform=Android','-cookflavor=ASTC',
 '-clientconfig=Development','-build','-cook','-stage','-pak','-iostore','-package','-archive',
 "-archivedirectory=$workspace\08_Builds\S2_Android",'-map=/Game/IndianRailway/S2/L_RailKernel+/Game/IndianRailway/Validation/L_AndroidHarness',
 '-cmdline=-csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log','-unattended','-utf8output')
$log=Join-Path $evidence "package-$buildId.log"
Push-Location (Split-Path $uat)
try { & $dotnet @arguments *> $log; $buildExit=$LASTEXITCODE } finally { Pop-Location }
[ordered]@{buildId=$buildId;exitCode=$buildExit;arguments=$arguments;log=$log} | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $evidence "package-$buildId-result.json")
exit $buildExit
