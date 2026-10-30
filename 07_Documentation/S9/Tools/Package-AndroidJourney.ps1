param([Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
$out=Join-Path $root "08_Builds/S9_$RunId"
if((Test-Path $e) -or (Test-Path $out)){throw 'Evidence/build exists; use new label'}
New-Item -ItemType Directory $e | Out-Null
$env:ANDROID_HOME="$env:LOCALAPPDATA/Android/Sdk"
$env:NDKROOT=Join-Path $env:ANDROID_HOME 'ndk/27.2.12479018'
$env:NDK_ROOT=$env:NDKROOT
$uat='C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat'
$argsList=@('BuildCookRun',"-project=$root/03_UnrealProject/MyProject/MyProject.uproject",'-noP4','-platform=Android','-cookflavor=ASTC','-clientconfig=Development','-build','-cook','-stage','-pak','-iostore','-package','-archive',"-archivedirectory=$out",'-map=/Game/IndianRailway/S7/L_RailService+/Game/IndianRailway/S6/L_ChunkA+/Game/IndianRailway/S6/L_ChunkB','-cmdline=-S8Tambaram -S8Cab -S9LiveJourney -log','-unattended','-utf8output')
# The cooker needs the editor plugin classes, but not their local HTTP server.
# Override only this process, preserving the user's interactive editor settings.
$argsList += '-AdditionalCookerOptions=-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
$argsList | ConvertTo-Json | Set-Content "$e/arguments.json"
& $uat @argsList *> "$e/package.log"
$code=$LASTEXITCODE
@{exitCode=$code;archive=$out;configuration='Development';platform='Android_ASTC'} | ConvertTo-Json | Set-Content "$e/result.json"
exit $code
