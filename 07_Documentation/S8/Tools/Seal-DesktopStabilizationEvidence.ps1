$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$evidence=Join-Path $root '07_Documentation/S8/Evidence/StabilizationDesktop-20261011'
$manifest=Join-Path $evidence 'evidence-hashes.json'
$files=@(Get-ChildItem -LiteralPath $evidence -File -Recurse|Where-Object FullName -ne $manifest)
foreach($relative in @(
 '03_UnrealProject/MyProject/Source/MyProject/RailDesktopStabilizationReview.cpp',
 '03_UnrealProject/MyProject/Source/MyProject/RailDiskRecoveryTests.cpp',
 '07_Documentation/S8/Tools/Test-StabilizationDesktop.ps1',
 '07_Documentation/S8/Tools/Test-DesktopPresentationEvidence.ps1',
 '07_Documentation/S8/Tools/diagnose_stabilization_skylights.py',
 '07_Documentation/S8/Tools/diagnose_stabilization_lighting_multiview.py',
 '07_Documentation/S8/Tools/verify_stabilization_preservation.py',
 '08_Builds/S8_EgmoreService_20261010-214817/Android_ASTC/MyProject-arm64.apk'
)){$files+=Get-Item -LiteralPath (Join-Path $root $relative)}
$rows=@($files|Sort-Object FullName -Unique|ForEach-Object{[ordered]@{path=$_.FullName;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}})
[ordered]@{generatedUtc=[DateTime]::UtcNow.ToString('o');scope='Evidence, selected test tools/source, and unchanged Android package; manifest excludes itself';files=$rows}|ConvertTo-Json -Depth 5|Set-Content -LiteralPath $manifest
Write-Output "Recorded $($rows.Count) file hashes."
