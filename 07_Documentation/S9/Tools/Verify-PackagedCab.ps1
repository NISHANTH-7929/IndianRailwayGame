param([Parameter(Mandatory=$true)][string]$Archive,[Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$archiveRoot=(Resolve-Path -LiteralPath $Archive).Path
$exe=Join-Path $archiveRoot 'MyProject/Binaries/Win64/MyProject-Win64-Shipping.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'Shipping executable missing'}
$out=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $out){throw 'Use a fresh evidence directory'}
$fixture=Join-Path $root '07_Documentation/S9/Evidence/SceneryFixtures-20261023-03/urban'
$userDir=Join-Path $out 'User'
$saveDir=Join-Path $userDir 'Saved/S9/EgmoreTambaram/LiveServiceV1'
New-Item -ItemType Directory -Path $saveDir | Out-Null
foreach($slot in @('checkpoint-0.irs','checkpoint-1.irs')){
    Copy-Item -LiteralPath "$fixture-isolated-saves/$slot" -Destination $saveDir
}
$expected=Get-Content -LiteralPath "$fixture-logical.json" -Raw | ConvertFrom-Json
$expectedJson=ConvertTo-Json -InputObject $expected -Depth 100 -Compress
$argsList=@('/Game/IndianRailway/S7/L_RailService','-S8Tambaram','-S8Cab','-S9LiveJourney',
    '-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-ForceRes','-unattended','-nosplash',
    "-UserDir=$($userDir.Replace('\','/'))/",'-ExecCmds=t.MaxFPS 30')
# Start-Process passes one native command line; quote each controlled argument.
$nativeArgs=($argsList | ForEach-Object {'"'+$_+'"'}) -join ' '
$process=Start-Process -FilePath $exe -ArgumentList $nativeArgs -PassThru -WindowStyle Hidden
@{processId=$process.Id;executable=$exe;arguments=$argsList} | ConvertTo-Json | Set-Content "$out/launch.json"
try{
    $deadline=[DateTime]::UtcNow.AddSeconds(120)
    $state=$null
    do{
        if($process.HasExited){throw 'Packaged game exited before scene readiness'}
        if(Test-Path "$saveDir/telemetry.json"){
            try{$state=Get-Content "$saveDir/telemetry.json" -Raw | ConvertFrom-Json}catch{$state=$null}
        }
        if($state.tambaramViews.ready -and $state.tambaramViews.station.ready -and $state.locomotive.ready){break}
        Start-Sleep -Milliseconds 500
    }while([DateTime]::UtcNow -lt $deadline)
    if(!$state){throw 'No runtime telemetry'}
    $unchanged=(ConvertTo-Json -InputObject $state.logical -Depth 100 -Compress) -ceq $expectedJson
    $candidate=$state.locomotive.cabCoreMesh -eq '/Game/IndianRailway/S9/CabRealismV1/Meshes/SM_WAP7_Cab_StaticCore_HardwareV1.SM_WAP7_Cab_StaticCore_HardwareV1'
    $ready=$state.tambaramViews.ready -and $state.tambaramViews.station.ready -and $state.locomotive.ready
    $passed=$ready -and $unchanged -and $candidate -and $state.paused -and !$state.blocked -and $state.locomotive.cabFinishMaterials -eq 15 -and $state.locomotive.cabTrianglesLOD0 -lt 50000
    $result=@{passed=[bool]$passed;checkpointStateUnchanged=$unchanged;sceneReady=[bool]$ready;
        candidateMeshLoaded=$candidate;cabFinishMaterials=$state.locomotive.cabFinishMaterials;
        cabTrianglesLOD0=$state.locomotive.cabTrianglesLOD0;parts=$state.locomotive.parts;
        tick=$state.logical.tick;fingerprint=$state.logicalFingerprint;paused=$state.paused;blocked=$state.blocked;
        executableSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;
        mobileTested=$false;fullJourneyTested=$false;realTimePerformanceClaimed=$false}
    $result | ConvertTo-Json | Set-Content "$out/result.json"
    Copy-Item "$saveDir/telemetry.json" "$out/verified-telemetry.json"
    if(!$passed){throw 'Packaged cab/recovery verification failed; inspect result.json'}
}finally{
    if(!$process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit(10000) | Out-Null}
}
