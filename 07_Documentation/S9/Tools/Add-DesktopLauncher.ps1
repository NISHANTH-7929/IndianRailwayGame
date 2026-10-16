param([Parameter(Mandatory=$true)][string]$Archive)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $Archive).Path
if(!(Test-Path -LiteralPath (Join-Path $root 'MyProject.exe'))){throw 'Expected Windows archive root'}
$launcher=Join-Path $root 'Start Railway Journey.cmd'
if(Test-Path -LiteralPath $launcher){throw 'Launcher already exists'}
@'
@echo off
start "" "%~dp0MyProject.exe" -S8Tambaram -S8Cab
'@ | Set-Content -LiteralPath $launcher -Encoding ascii
@'
INDIAN RAILWAY - REPRESENTATIVE EGMORE TO TAMBARAM JOURNEY

Launch Start Railway Journey.cmd. This selects the WAP-7/LHB corridor and cab.
Uses the game's normal local profile; existing compatible saves open paused.
Keep the entire Windows folder together. This desktop candidate is offline.

Select the service, confirm the formation and start. Release Brake, then apply
Power. Assisted mode regulates speed and braking; Standard requires manual
driving. Wait for the conflicting train to clear the Egmore junction.

Intermediate stations are passing locations. Stop at Tambaram and wait through
the dwell, then save the result. Pause and Save / Exit retain your journey.
HUD toggles route detail while retaining speed/authority and safety notices.
Sound settings are available in the service and pause menus.

This is representative scenery and generic licensed audio, not an exact dated
timetable, stock allocation or certified locomotive training simulator.
This candidate is not a production-qualified release. No Android claim.
'@ | Set-Content -LiteralPath (Join-Path $root 'JOURNEY-README.txt') -Encoding ascii
