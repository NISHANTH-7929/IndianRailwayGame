param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$Fixtures,
    [switch]$Scenery,
    [ValidateSet('urban','industrial','airport','riverside','suburban','crossover','opposing-3','opposing-4','opposing-5')]
    [string[]]$Scenes=@('urban','crossover','suburban','riverside')
)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
if($Scenes.Count -eq 0 -or @($Scenes | Select-Object -Unique).Count -ne $Scenes.Count){throw 'Specify at least one scene, without duplicates'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
if(!$Fixtures){$Fixtures=Join-Path $root '07_Documentation/S9/Evidence/SceneryFixtures-20261023-03'}
$fixtureRoot=(Resolve-Path -LiteralPath $Fixtures).Path
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $e){throw 'Use a fresh review folder'}
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if(!(Test-Path -LiteralPath $editor)){throw "Missing editor: $editor"}
foreach($scene in $Scenes){
    foreach($suffix in @('-isolated-saves/checkpoint-0.irs','-isolated-saves/checkpoint-1.irs','-logical.json')){
        if(!(Test-Path -LiteralPath (Join-Path $fixtureRoot "$scene$suffix"))){throw "Missing fixture: $scene$suffix"}
    }
}
New-Item -ItemType Directory -Path $e | Out-Null
$rows=@()
foreach($scene in $Scenes){
    $source=Join-Path $fixtureRoot $scene
    $out=Join-Path $e $scene
    $sourceHashes=@{}
    foreach($suffix in @('-isolated-saves/checkpoint-0.irs','-isolated-saves/checkpoint-1.irs','-logical.json')){
        $sourceHashes[$suffix]=(Get-FileHash -LiteralPath "$source$suffix" -Algorithm SHA256).Hash
    }
    Copy-Item -LiteralPath "$source-isolated-saves" -Destination "$out-isolated-saves" -Recurse
    Copy-Item -LiteralPath "$source-logical.json" -Destination "$out-logical.json"
    $argsList=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S7/L_RailService',
        '-game','-RenderOffscreen','-unattended','-nosplash','-windowed','-ResX=1600','-ResY=900','-ForceRes',
        '-S8Tambaram','-S8Cab','-S9LiveJourney','-S8AssetReview','-S8AssetReviewResume',
        "-TambaramEvidence=$out",'-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview',"-abslog=$e/$scene.log")
    if($scene -eq 'crossover'){$argsList+='-TambaramTrackCrossover'}
    $kind=if($Scenery){'scenery'}else{'track'}
    $argsList+=if($Scenery){'-TambaramSceneryReview'}else{'-TambaramTrackReview'}
    $argsList | ConvertTo-Json | Set-Content -LiteralPath "$e/$scene-arguments.json"
    & $editor @argsList *> "$e/$scene-stdout.log"
    $code=$LASTEXITCODE
    $result=if(Test-Path -LiteralPath "$out/result.json"){Get-Content -LiteralPath "$out/result.json" -Raw | ConvertFrom-Json}else{$null}
    $scope=if(Test-Path -LiteralPath "$out/review-scope.json"){Get-Content -LiteralPath "$out/review-scope.json" -Raw | ConvertFrom-Json}else{$null}
    $assets=if(Test-Path -LiteralPath "$out/track-assets.json"){Get-Content -LiteralPath "$out/track-assets.json" -Raw | ConvertFrom-Json}else{$null}
    $sourceUnchanged=$true
    foreach($suffix in $sourceHashes.Keys){
        $sourceUnchanged=$sourceUnchanged -and $sourceHashes[$suffix] -eq (Get-FileHash -LiteralPath "$source$suffix" -Algorithm SHA256).Hash
    }
    $captures=@(Get-ChildItem -LiteralPath $out -Filter '*.png' -ErrorAction SilentlyContinue).Count
    $passed=$code -eq 0 -and $result.passed -and $result.captures -eq 4 -and $captures -eq 4 -and $scope.reviewKind -eq $kind -and $scope.stateUnchanged -and $assets.passed -and $assets.natureAssetsPassed -and $sourceUnchanged
    $rows+=@{scene=$scene;exitCode=$code;passed=$passed;staticReviewOnly=$true;sourceFixturesUnchanged=$sourceUnchanged;sourceSHA256=$sourceHashes;captures=$captures;assignedTrackAssetsPassed=$assets.passed}
    $rows | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$e/render-results.json"
    if(!$passed){throw "Track review failed: $scene (see $e/$scene.log and $out/result.json)"}
}
