param([Parameter(Mandatory=$true)][string]$Fixtures)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$fixtureRoot=(Resolve-Path $Fixtures).Path
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$rows=@()
foreach($scene in @('urban','industrial','airport','riverside','suburban','crossover','opposing-3','opposing-4','opposing-5')) {
    $out=Join-Path $fixtureRoot $scene
    if(Test-Path $out){throw "Review exists: $out"}
    if(!(Test-Path "$out-isolated-saves/checkpoint-0.irs")){throw "Missing fixture: $scene"}
    $argsList=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S7/L_RailService',
        '-game','-RenderOffscreen','-unattended','-nosplash','-windowed','-ResX=1600','-ResY=900',
        '-S8Tambaram','-S8Cab','-S9LiveJourney','-S8AssetReview','-S8AssetReviewResume','-TambaramSceneryReview',
        "-TambaramEvidence=$out",'-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview',"-abslog=$fixtureRoot/$scene.log")
    & $editor @argsList *> "$fixtureRoot/$scene-stdout.log"
    $code=$LASTEXITCODE
    $result=if(Test-Path "$out/result.json"){Get-Content "$out/result.json" -Raw | ConvertFrom-Json}else{$null}
    $rows+=@{scene=$scene;exitCode=$code;passed=($code -eq 0 -and $result.passed);staticReviewOnly=$true}
    $rows | ConvertTo-Json | Set-Content "$fixtureRoot/render-results.json"
    if($code -ne 0 -or !$result.passed){throw "Review failed: $scene"}
}
