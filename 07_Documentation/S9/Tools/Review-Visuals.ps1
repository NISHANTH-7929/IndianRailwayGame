param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string[]]$Scenes=@('urban','suburban','riverside','opposing-4'),
    [switch]$Cab,
    [string]$FixtureSet='SceneryFixtures-20261023-03'
)
# Renders scenery (and optionally cab) review frames from isolated fixture saves into a fresh
# Evidence/<RunId> folder. Fixture saves are copied; originals are never written.
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$fixtures=Join-Path $root "07_Documentation/S9/Evidence/$FixtureSet"
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $e){throw 'Use a fresh review folder'}
New-Item -ItemType Directory -Path $e | Out-Null
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$jobs=@($Scenes | ForEach-Object {@{scene=$_;mode='-TambaramSceneryReview'}})
if($Cab){$jobs+=@{scene='cab';fixture='urban';mode='-TambaramCabReview'}}
$rows=@()
foreach($job in $jobs){
    $scene=$job.scene;$fixture=if($job.fixture){$job.fixture}else{$scene}
    $out=Join-Path $e $scene
    Copy-Item -LiteralPath "$fixtures/$fixture-isolated-saves" -Destination "$out-isolated-saves" -Recurse
    Copy-Item -LiteralPath "$fixtures/$fixture-logical.json" -Destination "$out-logical.json"
    $argsList=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S7/L_RailService',
        '-game','-RenderOffscreen','-unattended','-nosplash','-windowed','-ResX=1600','-ResY=900','-ForceRes',
        '-S8Tambaram','-S8Cab','-S9LiveJourney','-S8AssetReview','-S8AssetReviewResume',$job.mode,
        "-TambaramEvidence=$out",'-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview',"-abslog=$e/$scene.log")
    & $editor @argsList *> "$e/$scene-stdout.log"
    $code=$LASTEXITCODE
    $result=if(Test-Path "$out/result.json"){Get-Content "$out/result.json" -Raw | ConvertFrom-Json}else{$null}
    $rows+=@{scene=$scene;exitCode=$code;passed=($code -eq 0 -and $result.passed);staticReviewOnly=$true}
    $rows | ConvertTo-Json | Set-Content "$e/render-results.json"
}
$rows | ForEach-Object { "{0}: exit {1} passed {2}" -f $_.scene,$_.exitCode,$_.passed }
