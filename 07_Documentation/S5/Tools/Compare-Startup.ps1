param([string]$Label='initial-state')
$ErrorActionPreference='Stop'
$base=Join-Path $PSScriptRoot '../Evidence'
$device=Join-Path $base "Device/$Label/Saved/S5"
$sets=@(@('S2','s2-regression.json','kernel-tests.json',39),@('S3','s3-regression.json','handling-tests.json',28),@('S4','s4-regression.json','operations-tests.json',39),@('S5','ai-tests.json','ai-tests.json',36))
$results=@()
foreach($set in $sets){
 $h=Get-Content (Join-Path $base $set[1]) -Raw|ConvertFrom-Json
 $d=Get-Content (Join-Path $device $set[2]) -Raw|ConvertFrom-Json
 $same=@(Compare-Object @($h.cases.name|Sort-Object) @($d.cases.name|Sort-Object)).Count -eq 0
 $results+=[ordered]@{stage=$set[0];hostCases=$h.cases.Count;androidCases=$d.cases.Count;namesEqual=$same;passed=$h.passed -and $d.passed -and $same -and $d.cases.Count -eq $set[3] -and ($d.cases.passed -notcontains $false)}
}
[ordered]@{passed=($results.passed -notcontains $false);suites=$results;note='Identical named test inventories pass independently on Windows and Android. Does not claim bitwise floating-point equality across compilers.'}|ConvertTo-Json -Depth 6|Set-Content (Join-Path $base 'cross-platform-tests.json')
Get-Content (Join-Path $base 'cross-platform-tests.json')
