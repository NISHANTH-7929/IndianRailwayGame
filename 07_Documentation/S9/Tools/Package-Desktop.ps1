param([Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
$out=Join-Path $root "08_Builds/S9_$RunId"
if((Test-Path $e) -or (Test-Path $out)){throw 'Evidence/build exists; use new label'}
New-Item -ItemType Directory $e | Out-Null
$uat='C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat'
$argsList=@('BuildCookRun',"-project=$root/03_UnrealProject/MyProject/MyProject.uproject",'-noP4','-platform=Win64','-clientconfig=Shipping','-build','-cook','-stage','-pak','-iostore','-package','-archive',"-archivedirectory=$out",'-map=/Game/IndianRailway/S7/L_RailService+/Game/IndianRailway/S6/L_ChunkA+/Game/IndianRailway/S6/L_ChunkB','-unattended','-utf8output')
$argsList | ConvertTo-Json | Set-Content "$e/arguments.json"
& $uat @argsList *> "$e/package.log"
$code=$LASTEXITCODE
@{exitCode=$code;archive=$out;configuration='Shipping';platform='Win64'} | ConvertTo-Json | Set-Content "$e/result.json"
exit $code
