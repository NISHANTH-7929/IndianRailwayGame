$ErrorActionPreference='Stop'
$base=Join-Path $PSScriptRoot '../Evidence'
$hostTests=Get-Content (Join-Path $base 'ai-tests.json') -Raw|ConvertFrom-Json
$automation=Get-Content (Join-Path $base 'UnrealAutomation/index.json') -Raw|ConvertFrom-Json
$controls=Get-Content (Join-Path $base 'Device/ai-controls/results.json') -Raw|ConvertFrom-Json
$cross=Get-Content (Join-Path $base 'cross-platform-tests.json') -Raw|ConvertFrom-Json
$normal=Get-Content (Join-Path $base 'normal-run-summary.json') -Raw|ConvertFrom-Json
$performance=Get-Content (Join-Path $base 'performance-summary.json') -Raw|ConvertFrom-Json
$scope=Get-Content (Join-Path $base 'source-review.json') -Raw|ConvertFrom-Json
function Test([string]$Prefix){$cases=@($hostTests.cases|Where-Object {$_.name.StartsWith($Prefix)});return $cases.Count -gt 0 -and ($cases.passed -notcontains $false)}
function Regression([string]$Name,[int]$Count){$r=Get-Content (Join-Path $base "$Name-regression.json") -Raw|ConvertFrom-Json;return $r.passed -and $r.cases.Count -eq $Count -and ($r.cases.passed -notcontains $false)}
$gates=[ordered]@{
 '01.LogicalAIEntities'=(Test 'S5-T01')
 '02.SemanticTrainCommands'=(Test 'S5-T02')
 '03.S4RouteRequests'=(Test 'S5-T04')
 '04.CannotBypassAuthority'=(Test 'S5-T14') -and (Test 'S5.InvalidInfrastructure')
 '05.S3Handling'=(Test 'S5-T15')
 '06.NoDirectTransformMutation'=(Test 'S5-T03')
 '07.ObeysMovementAuthority'=(Test 'S5-T09') -and (Test 'S5-T11')
 '08.StopAndRestrictionResponse'=(Test 'S5-T10') -and (Test 'S5-T12') -and (Test 'S5-T23')
 '09.SafeRouteDenial'=(Test 'S5-T05') -and (Test 'S5-T06') -and (Test 'S5-T07')
 '10.SharedPlayerInfrastructure'=(Test 'S5-T19') -and $controls.checks.autonomousMovement
 '11.ConflictsPrevented'=(Test 'S5-T14')
 '12.WholeTrainOccupancy'=(Test 'S5-T17')
 '13.RearClearanceRelease'=(Test 'S5-T18') -and $controls.checks.rearStillProtects -and $controls.checks.rearRelease
 '14.RealActorRecreation'=$controls.checks.actorRecreation -and $controls.checks.recreateAfterMovement
 '15.Determinism'=(Test 'S5-T20')
 '16.RenderIndependence'=(Test 'S5-T21')
 '17.S2Regression'=(Regression 's2' 39)
 '18.S3Regression'=(Regression 's3' 28)
 '19.S4Regression'=(Regression 's4' 39)
 '20.UnrealAutomation'=$automation.succeeded -eq 4 -and $automation.failed -eq 0
 '21.AndroidIntegration'=$controls.passed -and $cross.passed -and $normal.passed -and $performance.validated
 '22.S5OnlyScope'=$scope.scopeS5Only -and $scope.driverForbiddenMutableCalls -eq 0 -and (Test-Path (Join-Path $base 'source-changes.json'))
}
# Scope is manually reviewed against the manifest/report and recorded in source-review.json.
$all=$gates.Values -notcontains $false
[ordered]@{passed=$all;passedCount=@($gates.Values|Where-Object {$_}).Count;total=22;gates=$gates;scopeReview='Source manifest manually reviewed: S5 driver, adapter, views/tests/tools only; no S6/S7 systems.'}|ConvertTo-Json -Depth 5|Set-Content (Join-Path $base 'acceptance.json')
Get-Content (Join-Path $base 'acceptance.json')
if(-not $all){exit 1}

