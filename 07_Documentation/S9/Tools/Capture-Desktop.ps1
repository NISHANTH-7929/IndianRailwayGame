param([Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'RunId must be a simple unique label'}
$run=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $run){throw 'Evidence directory exists; use a fresh RunId'}
$env:GAME_DEV_RUN_DIR=$run
$env:GAME_DEV_RUN_ID=$RunId
$env:GAME_DEV_ADAPTER_ID='indian-railway-desktop'
$env:GAME_DEV_SCENARIO_ID='tambaram-full'
$env:TAMBARAM_FULL_JOURNEY='1'
$env:TAMBARAM_RECORD_AUDIO='1'
& python "$root/07_Documentation/S8/Tools/capture_tambaram_desktop.py"
exit $LASTEXITCODE
