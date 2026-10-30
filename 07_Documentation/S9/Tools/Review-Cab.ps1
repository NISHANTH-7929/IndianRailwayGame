param([Parameter(Mandatory=$true)][string]$RunId)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $e){throw 'Use a fresh review folder'}
$fixture=Join-Path $root '07_Documentation/S9/Evidence/SceneryFixtures-20261023-03/urban'
New-Item -ItemType Directory -Path $e | Out-Null
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$rows=@()
foreach($variant in @('before','after')){
    $out=Join-Path $e $variant
    Copy-Item -LiteralPath "$fixture-isolated-saves" -Destination "$out-isolated-saves" -Recurse
    Copy-Item -LiteralPath "$fixture-logical.json" -Destination "$out-logical.json"
    $argsList=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S7/L_RailService',
        '-game','-RenderOffscreen','-unattended','-nosplash','-windowed','-ResX=1600','-ResY=900','-ForceRes',
        '-S8Tambaram','-S8Cab','-S9LiveJourney','-S8AssetReview','-S8AssetReviewResume','-TambaramCabReview',
        "-TambaramEvidence=$out",'-ExecCmds=t.MaxFPS 30,Rail.TambaramDesktopReview',"-abslog=$e/$variant.log")
    if($variant -eq 'before'){$argsList+='-OriginalCabReview'}
    $argsList | ConvertTo-Json | Set-Content -LiteralPath "$e/$variant-arguments.json"
    & $editor @argsList *> "$e/$variant-stdout.log"
    $code=$LASTEXITCODE
    $result=if(Test-Path "$out/result.json"){Get-Content "$out/result.json" -Raw | ConvertFrom-Json}else{$null}
    $rows+=@{variant=$variant;exitCode=$code;passed=($code -eq 0 -and $result.passed);staticReviewOnly=$true}
    $rows | ConvertTo-Json | Set-Content -LiteralPath "$e/render-results.json"
    if($code -ne 0 -or !$result.passed){throw "Cab review failed: $variant"}
}
