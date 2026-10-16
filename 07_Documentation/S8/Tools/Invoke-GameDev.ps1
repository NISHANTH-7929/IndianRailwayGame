# Pinned user installation; uses the already-installed project Blender.
$ErrorActionPreference='Stop'
$env:BLENDER_PATH='C:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
$gameDevExecutable=Join-Path $env:LOCALAPPDATA 'GameDevelopmentStudio\cli\game-dev.cmd'
if (-not (Test-Path -LiteralPath $gameDevExecutable)) { throw 'Game Development Studio CLI is not installed' }
& $gameDevExecutable @args
exit $LASTEXITCODE
