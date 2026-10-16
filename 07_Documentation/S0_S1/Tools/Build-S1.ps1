param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$SdkRoot = $env:ANDROID_HOME
)
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$project = Join-Path $workspace '03_UnrealProject\MyProject\MyProject.uproject'
$evidence = Join-Path $PSScriptRoot '..\Evidence'
$buildId = Get-Date -Format 'yyyyMMdd-HHmmss'
$log = Join-Path $evidence "package-$buildId.log"
$result = Join-Path $evidence "package-$buildId-result.json"
$ndk = Join-Path $SdkRoot 'ndk\27.2.12479018'
if (!(Test-Path -LiteralPath (Join-Path $ndk 'source.properties'))) { throw 'NDK r27c not ready.' }
if (!(Test-Path -LiteralPath (Join-Path $EngineRoot 'Engine\Binaries\Android\UnrealGame.target'))) { throw 'Engine Android target files missing.' }
if (!(Test-Path -LiteralPath (Join-Path (Split-Path $project) 'Content\IndianRailway\Validation\L_AndroidHarness.umap'))) { throw 'S1 map missing.' }
# Process-local paths only: preserve other projects' machine/user toolchain selections.
$env:ANDROID_HOME = $SdkRoot
$env:NDKROOT = $ndk
$env:NDK_ROOT = $ndk
$dotnetRoot = Join-Path $EngineRoot 'Engine\Binaries\ThirdParty\DotNet\10.0\win-x64'
$dotnet = Join-Path $dotnetRoot 'dotnet.exe'
$uat = Join-Path $EngineRoot 'Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll'
if (!(Test-Path -LiteralPath $dotnet) -or !(Test-Path -LiteralPath $uat)) { throw 'Bundled AutomationTool runtime missing.' }
$env:DOTNET_ROOT = $dotnetRoot
$env:DOTNET_MULTILEVEL_LOOKUP = '0'
$env:DOTNET_ROLL_FORWARD = 'LatestMajor'
$arguments = @($uat, 'BuildCookRun', "-project=$project", '-noP4', '-platform=Android', '-cookflavor=ASTC',
    '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-iostore', '-package', '-archive',
    "-archivedirectory=$workspace\08_Builds\S1_Android", '-map=/Game/IndianRailway/Validation/L_AndroidHarness',
    '-cmdline=/Game/IndianRailway/Validation/L_AndroidHarness -csvCaptureFrames=6000 -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log', '-unattended', '-utf8output')
$started = (Get-Date).ToUniversalTime().ToString('o')
Push-Location (Split-Path $uat)
try { & $dotnet @arguments 2>&1 | Tee-Object -FilePath $log } finally { Pop-Location }
$buildExit = $LASTEXITCODE
[ordered]@{ buildId=$buildId; startedUtc=$started; finishedUtc=(Get-Date).ToUniversalTime().ToString('o');
    project=$project; tool=$dotnet; automationTool=$uat; arguments=$arguments; ndk=$ndk; exitCode=$buildExit;
    status=$(if ($buildExit -eq 0) {'PACKAGE_COMMAND_SUCCEEDED_DEVICE_UNVALIDATED'} else {'FAILED'});
    log=$log } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $result
exit $buildExit
