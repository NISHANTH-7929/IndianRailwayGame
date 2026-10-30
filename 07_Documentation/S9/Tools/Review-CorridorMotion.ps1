param(
    [Parameter(Mandatory=$true)][string]$RunId,
    [string]$Fixtures,
    [ValidateSet('urban','suburban')][string[]]$Scenes=@('urban'),
    [ValidateRange(60,90)][int]$DurationSeconds=90
)
$ErrorActionPreference='Stop'
if($RunId -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple unique run label'}
if($Scenes.Count -eq 0 -or @($Scenes | Select-Object -Unique).Count -ne $Scenes.Count){throw 'Specify scenes without duplicates'}
$root=(Resolve-Path "$PSScriptRoot/../../..").Path
if(!$Fixtures){$Fixtures=Join-Path $root '07_Documentation/S9/Evidence/SceneryFixtures-20261023-03'}
$fixtureRoot=(Resolve-Path -LiteralPath $Fixtures).Path
$e=Join-Path $root "07_Documentation/S9/Evidence/$RunId"
if(Test-Path -LiteralPath $e){throw 'Use a fresh review folder'}
$editor='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
if(!(Test-Path -LiteralPath $editor)){throw "Missing editor: $editor"}
$suffixes=@('-isolated-saves/checkpoint-0.irs','-isolated-saves/checkpoint-1.irs','-logical.json')
foreach($scene in $Scenes){foreach($suffix in $suffixes){if(!(Test-Path -LiteralPath (Join-Path $fixtureRoot "$scene$suffix"))){throw "Missing fixture: $scene$suffix"}}}
New-Item -ItemType Directory -Path $e | Out-Null
$rows=@()
foreach($scene in $Scenes){
    $source=Join-Path $fixtureRoot $scene
    $out=Join-Path $e $scene
    $sourceHashes=@{}
    foreach($suffix in $suffixes){$sourceHashes[$suffix]=(Get-FileHash -LiteralPath "$source$suffix" -Algorithm SHA256).Hash}
    Copy-Item -LiteralPath "$source-isolated-saves" -Destination "$out-isolated-saves" -Recurse
    Copy-Item -LiteralPath "$source-logical.json" -Destination "$out-logical.json"
    $argsList=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S7/L_RailService',
        '-game','-RenderOffscreen','-unattended','-nosplash','-windowed','-ResX=1600','-ResY=900','-ForceRes',
        '-S8Tambaram','-S8Cab','-S9LiveJourney','-S8AssetReview','-S8AssetReviewResume','-CorridorMotionReview',
        "-CorridorMotionSeconds=$DurationSeconds","-TambaramEvidence=$out",'-ExecCmds=t.MaxFPS 30,Rail.CorridorMotionReview',"-abslog=$e/$scene.log")
    $argsList | ConvertTo-Json | Set-Content -LiteralPath "$e/$scene-arguments.json"
    & $editor @argsList *> "$e/$scene-stdout.log"
    $code=$LASTEXITCODE
    $result=if(Test-Path -LiteralPath "$out/result.json"){Get-Content -LiteralPath "$out/result.json" -Raw | ConvertFrom-Json}else{$null}
    $sourceUnchanged=$true
    foreach($suffix in $sourceHashes.Keys){$sourceUnchanged=$sourceUnchanged -and $sourceHashes[$suffix] -eq (Get-FileHash -LiteralPath "$source$suffix" -Algorithm SHA256).Hash}
    $captures=@(Get-ChildItem -LiteralPath $out -Filter '*.png' -ErrorAction SilentlyContinue).Count
    $passed=$code -eq 0 -and $result.passed -and $result.captures -eq 3 -and $captures -eq 3 -and $sourceUnchanged
    $rows+=@{scene=$scene;exitCode=$code;passed=$passed;functionalPassed=$result.functionalPassed;frameBudgetPassed=$result.frameBudgetPassed;
        sourceFixturesUnchanged=$sourceUnchanged;sourceSHA256=$sourceHashes;captures=$captures;durationSeconds=$DurationSeconds;
        scope='Editor real-time RenderOffscreen; no Shipping, mobile, manual journey or automatic flicker qualification'}
    $rows | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath "$e/motion-results.json"
    if(!$passed){throw "Motion review failed: $scene (see $out/result.json and $e/$scene.log)"}
}
