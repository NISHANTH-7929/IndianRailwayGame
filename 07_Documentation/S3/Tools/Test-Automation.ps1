param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$workspace/03_UnrealProject/MyProject/MyProject.uproject" -nullrhi -unattended -nosplash '-ExecCmds=Automation RunTests IndianRailway' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$workspace/07_Documentation/S3/Evidence/UnrealAutomation" "-abslog=$workspace/07_Documentation/S3/Evidence/automation.log" *> "$workspace/07_Documentation/S3/Evidence/automation-console.log"
exit $LASTEXITCODE
