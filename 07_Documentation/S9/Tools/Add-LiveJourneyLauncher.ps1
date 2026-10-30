param([Parameter(Mandatory=$true)][string]$Archive)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $Archive).Path
if(!(Test-Path -LiteralPath (Join-Path $root 'MyProject.exe'))){throw 'Expected Windows archive root'}
$launcher=Join-Path $root 'Start Live Railway Journey.cmd'
if(Test-Path -LiteralPath $launcher){throw 'Launcher already exists'}
@'
@echo off
start "" "%~dp0MyProject.exe" -S8Tambaram -S8Cab -S9LiveJourney
'@ | Set-Content -LiteralPath $launcher -Encoding ascii
@'
LIVE EGMORE TO TAMBARAM JOURNEY

Start Live Railway Journey.cmd selects the new five-train service. Wait for the
priority departure, cross to your protected line at the throat restriction,
then encounter three northbound services along the corridor.

Choose Assisted or Standard in the briefing before starting. Assisted regulates
cruise and braking after you release the brake and apply power. Standard leaves
power, coasting and braking to you. Both modes obey the same route protection.

New service progress uses Saved/S9/EgmoreTambaram/LiveServiceV1. Your original
journey saves are retained separately.
Keep the entire Windows folder together. No network connection is required.

Representative scenario and scenery, not a current operating timetable.
Existing generic/synthesized audio remains; no authentic recording claim.
'@ | Set-Content -LiteralPath (Join-Path $root 'LIVE-JOURNEY-README.txt') -Encoding ascii
