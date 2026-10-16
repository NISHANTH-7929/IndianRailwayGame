param([string]$ProfileLabel='profile-final',[string]$NormalLabel='normal-final',[string]$CsvSummary='')
$ErrorActionPreference='Stop'
$base=Join-Path $PSScriptRoot '../Evidence'
function Samples([string]$Label){
 $items=@()
 Get-ChildItem (Join-Path $base "Device/$Label") -Directory -Filter 'sample-*'|Sort-Object Name|ForEach-Object {
  $index=[int]($_.Name -replace 'sample-','');if($index -eq 0){return}
  $memory=Get-Content (Join-Path $_.FullName 'memory.txt') -Raw
  $thermal=Get-Content (Join-Path $_.FullName 'thermal.txt') -Raw
  $pss=[regex]::Match($memory,'TOTAL PSS:\s+(\d+)');$status=[regex]::Match($thermal,'Thermal Status:\s+(\d+)')
  $s=Get-Content (Join-Path $_.FullName 'telemetry.json') -Raw|ConvertFrom-Json
  $items+=[ordered]@{sample=$index;tick=$s.tick;valid=$s.valid;faulted=$s.faulted;suspended=$s.suspended;pssKiB=if($pss.Success){[long]$pss.Groups[1].Value}else{$null};thermalStatus=if($status.Success){[int]$status.Groups[1].Value}else{$null};trainAHead=$s.trains[0].handling.headMetres;trainBHead=$s.trains[1].handling.headMetres}
 }
 return $items
}
if($CsvSummary){
 $csv=Get-Content $CsvSummary -Raw|ConvertFrom-Json;$samples=@(Samples $ProfileLabel)
 $result=[ordered]@{validated=$csv.total_frames -eq 6000 -and $csv.average_fps -gt 0 -and $samples.Count -eq 8 -and ($samples.valid -notcontains $false) -and ($samples.faulted -notcontains $true);capture=$csv;samples=$samples;pssMinKiB=($samples.pssKiB|Measure-Object -Minimum).Minimum;pssMaxKiB=($samples.pssKiB|Measure-Object -Maximum).Maximum;thermalStatuses=@($samples.thermalStatus|Sort-Object -Unique);note='Same final APK with temporary CSV commandline. Active10–180s window excludes warmup and deliberate190s simulation pause before export. Engine timings only, fixture/device only.'}
 $result|ConvertTo-Json -Depth 12|Set-Content (Join-Path $base 'performance-summary.json')
}
if(Test-Path (Join-Path $base "Device/$NormalLabel/sample-7/telemetry.json")){
 $samples=@(Samples $NormalLabel)
 $logPath=Join-Path $base "Device/$NormalLabel/Saved/Logs/MyProject.log"
 $faults=@();if(Test-Path $logPath){$faults=@(Select-String -Path $logPath -Pattern 'S4 FAULT|Fixed-step overload|Fatal error|Assertion failed')}
 $valid=$samples.Count -eq 7 -and ($samples.valid -notcontains $false) -and ($samples.faulted -notcontains $true) -and ($samples.suspended -notcontains $true) -and $samples[-1].tick -gt $samples[0].tick -and $faults.Count -eq 0
 [ordered]@{passed=$valid;durationSeconds=215;profilingDefault='off';samples=$samples;faultLogMatches=$faults.Count;pssMinKiB=($samples.pssKiB|Measure-Object -Minimum).Minimum;pssMaxKiB=($samples.pssKiB|Measure-Object -Maximum).Maximum;thermalStatuses=@($samples.thermalStatus|Sort-Object -Unique)}|ConvertTo-Json -Depth 10|Set-Content (Join-Path $base 'normal-run-summary.json')
}
