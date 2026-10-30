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
foreach($slot in @('checkpoint-0.irs','checkpoint-1.irs')){Copy-Item -LiteralPath "$fixture-isolated-saves/$slot" -Destination $saveDir}
$expected=Get-Content -LiteralPath "$fixture-logical.json" -Raw | ConvertFrom-Json
$expectedJson=ConvertTo-Json -InputObject $expected -Depth 100 -Compress
$assetRoot='/Game/IndianRailway/S9/TrackEnvironmentV1/'
function Asset([string]$folder,[string]$name){return "$assetRoot$folder/$name.$name"}
$expectedFinishes=@('M_RailHead','M_RailRust','M_SleeperConcrete','M_TrackStone') | ForEach-Object {Asset 'MaterialsV4' $_}
$expectedTurnout=@('M_RailHead','M_RailRust','M_SleeperConcrete','M_Fastener') | ForEach-Object {Asset 'MaterialsV4' $_}
$expectedTextures=@('T_TrackBallast_BC','T_DryGround_BC','T_DryVerge_BC') | ForEach-Object {Asset 'TexturesV2' $_}
$natureRoot='/Game/IndianRailway/S9/CorridorNatureV1/'
$expectedNature=@('SM_CorridorLeafClusterV2','SM_CorridorVerge12m','SM_CorridorGrassPatchV1','SM_CorridorBranchV1') | ForEach-Object {"${natureRoot}Meshes/$_.$_"}
$expectedNature+=@('M_CorridorLeaf','M_CorridorNatureGround','M_CorridorDryGrass','M_CorridorBark','M_CorridorOuterGround') | ForEach-Object {"${natureRoot}Materials/$_.$_"}
$rows=@()
foreach($attempt in 1..2){
    $telemetry=Join-Path $saveDir 'telemetry.json'
    # Require fresh process telemetry on both cold launches.
    if(Test-Path -LiteralPath $telemetry){Move-Item -LiteralPath $telemetry -Destination "$out/previous-launch-telemetry.json"}
    $argsList=@('/Game/IndianRailway/S7/L_RailService','-S8Tambaram','-S8Cab','-S9LiveJourney',
        '-RenderOffscreen','-windowed','-ResX=1280','-ResY=720','-ForceRes','-unattended','-nosplash',
        "-UserDir=$($userDir.Replace('\','/'))/",'-ExecCmds=t.MaxFPS 30')
    $nativeArgs=($argsList | ForEach-Object {'"'+$_+'"'}) -join ' '
    $process=Start-Process -FilePath $exe -ArgumentList $nativeArgs -PassThru -WindowStyle Hidden
    @{processId=$process.Id;executable=$exe;arguments=$argsList} | ConvertTo-Json | Set-Content "$out/launch-$attempt.json"
    try{
        $deadline=[DateTime]::UtcNow.AddSeconds(120);$state=$null
        do{
            if($process.HasExited){throw 'Packaged game exited before scene readiness'}
            if(Test-Path -LiteralPath $telemetry){try{$state=Get-Content -LiteralPath $telemetry -Raw | ConvertFrom-Json}catch{$state=$null}}
            $views=@($state.tambaramViews)
            $ready=$state -and $views.Count -gt 0 -and @($views | Where-Object {!$_.ready -or !$_.station.ready}).Count -eq 0 -and $state.locomotive.ready
            if($ready){break}
            Start-Sleep -Milliseconds 500
        }while([DateTime]::UtcNow -lt $deadline)
        if(!$state){throw 'No runtime telemetry'}
        $unchanged=(ConvertTo-Json -InputObject $state.logical -Depth 100 -Compress) -ceq $expectedJson
        $assets=$views.Count -gt 0
        foreach($v in $views){
            $a=$v.trackAssets
            $assets=$assets -and (($a.nature -join '|') -ceq ($expectedNature -join '|'))
            $assets=$assets -and $a.rails -ceq (Asset 'Meshes' 'SM_Track6m_RealismV2')
            $assets=$assets -and $a.bed -ceq (Asset 'Meshes' 'SM_Ballast6m_RealismV3') -and $a.stone -ceq (Asset 'Meshes' 'SM_AngularBallastStoneV1') -and $a.ground -ceq (Asset 'MaterialsV4' 'M_DryGround')
            $assets=$assets -and (($a.finishes -join '|') -ceq ($expectedFinishes -join '|')) -and (($v.turnoutFinishes -join '|') -ceq ($expectedTurnout -join '|'))
            $textures=@($a.textures)
            $assets=$assets -and $textures.Count -eq 3 -and (($textures.asset -join '|') -ceq ($expectedTextures -join '|'))
            foreach($t in $textures){$assets=$assets -and $t -and $t.width -eq 1024 -and $t.height -eq 1024 -and $t.mips -eq 11 -and $t.streamable}
        }
        $cab=$state.locomotive.cabCoreMesh -ceq '/Game/IndianRailway/S9/CabRealismV1/Meshes/SM_WAP7_Cab_StaticCore_HardwareV1.SM_WAP7_Cab_StaticCore_HardwareV1' -and $state.locomotive.cabFinishMaterials -eq 15
        $passed=$ready -and $unchanged -and $assets -and $cab -and $state.paused -and !$state.blocked
        $rows+=@{attempt=$attempt;passed=[bool]$passed;checkpointStateUnchanged=$unchanged;sceneReady=[bool]$ready;
            trackAssetsLoaded=[bool]$assets;cabRetained=[bool]$cab;tick=$state.logical.tick;fingerprint=$state.logicalFingerprint;
            paused=$state.paused;blocked=$state.blocked;views=$views}
        Copy-Item -LiteralPath $telemetry -Destination "$out/verified-telemetry-$attempt.json"
        @{passed=($rows.Count -eq 2 -and @($rows | Where-Object {!$_.passed}).Count -eq 0);attempts=$rows;
            executableSha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash;
            mobileTested=$false;fullJourneyTested=$false;realTimePerformanceClaimed=$false} | ConvertTo-Json -Depth 10 | Set-Content "$out/result.json"
        if(!$passed){throw 'Packaged track/recovery verification failed; inspect result.json'}
    }finally{
        if(!$process.HasExited){Stop-Process -Id $process.Id; $process.WaitForExit(10000) | Out-Null}
    }
}
