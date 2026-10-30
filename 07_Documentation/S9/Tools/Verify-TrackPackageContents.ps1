param([Parameter(Mandatory=$true)][string]$Archive,[Parameter(Mandatory=$true)][string]$PackageRunId)
$ErrorActionPreference='Stop'
if($PackageRunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple package run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$archiveRoot=(Resolve-Path -LiteralPath $Archive).Path
$evidence=Join-Path $root "07_Documentation/S9/Evidence/$PackageRunId"
$result=Get-Content -LiteralPath "$evidence/result.json" -Raw | ConvertFrom-Json
if($result.exitCode -ne 0 -or $result.configuration -cne 'Shipping' -or $result.platform -cne 'Win64'){throw 'A successful Win64 Shipping package is required'}
$expectedArchive=Join-Path (Resolve-Path -LiteralPath $result.archive).Path 'Windows'
if($archiveRoot -ine $expectedArchive){throw 'Archive must belong to the supplied package run'}
$started=(Get-Item -LiteralPath "$evidence/arguments.json").LastWriteTimeUtc
$metadata=Join-Path $root '03_UnrealProject/MyProject/Saved/Cooked/Windows/MyProject/Metadata/ShaderLibrarySource'
$copyRoot=Join-Path $evidence 'verified-shader-metadata'
if(Test-Path -LiteralPath $copyRoot){throw 'Shader metadata receipt already exists'}
New-Item -ItemType Directory -Path $copyRoot | Out-Null
$expected=@('M_TrackBallast','M_TrackStone','M_DryGround','M_RailHead','M_RailRust','M_SleeperConcrete','M_Fastener') | ForEach-Object {"/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV4/$_"}
$expected+=@('M_CorridorLeaf','M_CorridorNatureGround','M_CorridorDryGrass','M_CorridorBark','M_CorridorOuterGround') | ForEach-Object {"/Game/IndianRailway/S9/CorridorNatureV1/Materials/$_"}
$shaders=@()
foreach($format in @('SM5','SM6')){
    $name="ShaderAssetInfo-MyProject-PCD3D_$format-PCD3D_$format.assetinfo.json"
    $source=Get-Item -LiteralPath (Join-Path $metadata $name)
    if($source.LastWriteTimeUtc -lt $started){throw "Stale shader metadata: $name"}
    $destination=Join-Path $copyRoot $name
    Copy-Item -LiteralPath $source.FullName -Destination $destination
    $data=Get-Content -LiteralPath $destination -Raw | ConvertFrom-Json
    $assets=@($data.ShaderCodeToAssets | ForEach-Object {$_.Assets})
    $missing=@($expected | Where-Object {$assets -cnotcontains $_})
    $shaders+=@{format=$format;passed=($missing.Count -eq 0);missing=$missing;expectedMaterials=$expected;
        file=$destination;sha256=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash}
}
$files=@(Get-ChildItem -LiteralPath $archiveRoot -Recurse -File | Where-Object {$_.Extension -in @('.exe','.pak','.ucas','.utoc')})
foreach($extension in @('.exe','.pak','.ucas','.utoc')){
    if(@($files | Where-Object {$_.Extension -eq $extension}).Count -eq 0){throw "Missing packaged $extension files"}
}
$hashes=@($files | Sort-Object FullName | ForEach-Object {@{path=$_.FullName;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash}})
$passed=@($shaders | Where-Object {!$_.passed}).Count -eq 0
@{passed=$passed;archive=$archiveRoot;shaderLibraries=$shaders;packagedFiles=$hashes;
    scope='Cooked shader membership and archive identity; runtime loading is checked separately'} |
    ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$evidence/content-verification.json"
if(!$passed){throw 'Required track material shader entries are missing'}
