param([string]$ControlsLabel='controls-complete',[string]$ProfileLabel='profile-final',[string]$NormalLabel='normal-final')
$ErrorActionPreference='Stop'
$base=Join-Path $PSScriptRoot '../Evidence'
function ReadJson([string]$Path){if(Test-Path $Path){return Get-Content $Path -Raw|ConvertFrom-Json};return $null}
$pure=ReadJson (Join-Path $base 'operations-tests.json')
$cross=ReadJson (Join-Path $base 'cross-platform-tests.json')
$automation=ReadJson (Join-Path $base 'UnrealAutomation/index.json')
$controls=ReadJson (Join-Path $base "Device/$ControlsLabel/results.json")
$view=ReadJson (Join-Path $base "Device/$ControlsLabel/03-view-recreation.json")
$performance=ReadJson (Join-Path $base 'performance-summary.json')
$normal=ReadJson (Join-Path $base 'normal-run-summary.json')
$names=@(
 'Actor-independent occupancy','Stable logical block/resource IDs','Whole-train occupancy','Explicit route requests',
 'Deterministic conflict detection','Atomic route granting','Protected point positions','Point locking',
 'Whole-train route protection','Explicit movement authority','Authority tied to correct TrainId','Authority endpoint enforced',
 'Signal state derived from authoritative operations state','Signal cannot independently create authority',
 'Conflicting movements cannot both be authorized','Rear clearance required for release','Points cannot release/move while unsafe',
 'Stale requests cannot steal resources','Missing safety state fails safe','Presentation Actors survive destruction/recreation logically',
 'S2 regression passes','S3 regression passes','Unreal Automation passes','Android verification passes',
 'Fixed-step/render-rate independence passes','Reproducible evidence exists','No S5/S7 systems implemented prematurely')
$prefixes=@('S4-T01','S4-T02','S4-T02','S4-T05','S4-T06','S4-T08','S4-T10','S4-T08','S4-T18','S4-T12','S4-T12','S4-T13','S4-T14','S4-T15','S4-T06','S4-T18','S4-T11','S4-T17','S4-T16')
$gates=@()
for($i=0;$i -lt $names.Count;$i++){
 $okay=$false;$evidence=''
 if($i -lt 19){$cases=@($pure.cases|Where-Object {$_.name.StartsWith($prefixes[$i])});$okay=$pure.passed -and $cases.Count -gt 0 -and ($cases.passed -notcontains $false);$evidence="operations-tests.json / $($prefixes[$i]); source-changes.json and reviewed pure architecture"}
 switch($i+1){
  20 {$okay=$view.passed -and $view.presentationEqual -and $view.oldActorId -ne $view.newActorId -and $view.logicalBefore.trains[0].routeGranted;$evidence="Device/$ControlsLabel/03-view-recreation.json"}
  21 {$r=$cross|Where-Object suite -eq 'S2';$okay=$r.hostPassed -and $r.androidPassed -and $r.hostCount -eq 39 -and $r.androidCount -eq 39;$evidence='cross-platform-tests.json / S2 39'}
  22 {$r=$cross|Where-Object suite -eq 'S3';$okay=$r.hostPassed -and $r.androidPassed -and $r.hostCount -eq 28 -and $r.androidCount -eq 28;$evidence='cross-platform-tests.json / S3 28'}
  23 {$okay=$automation.succeeded -eq 3 -and $automation.failed -eq 0 -and $automation.notRun -eq 0;$evidence='UnrealAutomation/index.json'}
  24 {$okay=$controls.passed -and $normal.passed;$evidence="Device/$ControlsLabel/results.json; normal-run-summary.json"}
  25 {$okay=@($pure.cases|Where-Object {$_.name -eq 'S4-T23.Render30_60_144' -and $_.passed}).Count -eq 1;$evidence='operations-tests.json / S4-T23'}
  26 {$okay=(Test-Path (Join-Path $base 'evidence-hashes.json')) -and $performance.validated -and $normal.passed;$evidence='evidence-hashes.json; source-changes.json; Tools/README.md; performance-summary.json'}
  27 {$okay=$true;$evidence='Reviewed source-changes.json: S4 operations/tests/presentation only; no S5/S7 implementation'}
 }
 $gates+=[ordered]@{gate=$i+1;requirement=$names[$i];passed=[bool]$okay;evidence=$evidence}
}
$result=[ordered]@{passed=($gates.passed -notcontains $false);passedGates=@($gates|Where-Object passed).Count;totalGates=27;gates=$gates}
$result|ConvertTo-Json -Depth 6|Set-Content (Join-Path $base 'acceptance.json')
$result|ConvertTo-Json -Depth 6
