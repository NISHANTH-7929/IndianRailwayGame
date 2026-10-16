$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$base=Join-Path $root '07_Documentation/S4/Evidence'
$paths=@()
foreach($folder in @('03_UnrealProject/MyProject/Source','03_UnrealProject/MyProject/Config','03_UnrealProject/MyProject/Content/IndianRailway/S4','07_Documentation/S4/Tools')){
 $paths+=Get-ChildItem (Join-Path $root $folder) -Recurse -File
}
$paths+=Get-ChildItem $base -Recurse -File|Where-Object {$_.Name -notin @('evidence-hashes.json','acceptance.json','acceptance-audit.log','transfer-chunk.bin') -and $_.Extension -notin @('.obj','.exe','.pdb')}
$paths+=Get-Item (Join-Path $root '08_Builds/S4_Android/Android_ASTC/MyProject-arm64.apk')
$records=@($paths|Sort-Object FullName -Unique|ForEach-Object {[ordered]@{path=[IO.Path]::GetRelativePath($root,$_.FullName);bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}})
[ordered]@{generatedUtc=(Get-Date).ToUniversalTime().ToString('o');algorithm='SHA256';files=$records}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $base 'evidence-hashes.json')
Write-Output "Hashed $($records.Count) source, config, map, tool, APK and raw evidence files"
