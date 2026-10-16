$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$evidence=Join-Path $PSScriptRoot '../Evidence'
$baseline=Get-Content (Join-Path $evidence 'Baseline/source-hashes.json') -Raw|ConvertFrom-Json
$old=@{};foreach($entry in $baseline){$old[$entry.path]=$entry.sha256}
$files=@(Get-ChildItem "$root/03_UnrealProject/MyProject/Source","$root/03_UnrealProject/MyProject/Config" -File -Recurse)
$changes=@();foreach($file in $files){$hash=(Get-FileHash $file.FullName -Algorithm SHA256).Hash;if(-not $old.ContainsKey($file.FullName) -or $old[$file.FullName] -ne $hash){$changes+=[ordered]@{path=$file.FullName;change=if($old.ContainsKey($file.FullName)){'modified'}else{'added'};before=$old[$file.FullName];sha256=$hash}}}
[ordered]@{baseline='Evidence/Baseline/source-hashes.json';sourceAndConfig=$changes;newMap='03_UnrealProject/MyProject/Content/IndianRailway/S5/L_RailAI.umap';toolsAndReports='07_Documentation/S5';updatedHistoricalRunner='07_Documentation/S4/Tools/Test-Operations.cmd now links the new adapter dependencies; accepted S4 evidence is unchanged';git='Workspace is not a Git repository; comparison uses the saved pre-edit baseline.'}|ConvertTo-Json -Depth 7|Set-Content (Join-Path $evidence 'source-changes.json')
$files+=Get-ChildItem "$root/03_UnrealProject/MyProject/Content/IndianRailway/S5" -File -Recurse
$files+=Get-ChildItem "$root/07_Documentation/S5/Tools" -File -Recurse
$files+=Get-ChildItem $evidence -File -Recurse|Where-Object {$_.FullName -notmatch '\\Baseline\\|evidence-hashes\.json$|acceptance\.json$|report-hash\.json$|\.obj$|\.exe$|transfer-chunk\.bin$'}
$files+=Get-Item "$root/08_Builds/S5_Android/Android_ASTC/MyProject-arm64.apk","$root/07_Documentation/S4/Tools/Test-Operations.cmd"
$hashes=@($files|Sort-Object FullName -Unique|ForEach-Object {[ordered]@{path=$_.FullName;bytes=$_.Length;sha256=(Get-FileHash $_.FullName -Algorithm SHA256).Hash}})
[ordered]@{algorithm='SHA256';files=$hashes}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $evidence 'evidence-hashes.json')

