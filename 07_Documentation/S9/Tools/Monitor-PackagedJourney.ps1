param([Parameter(Mandatory=$true)][int]$GameProcessId,[Parameter(Mandatory=$true)][string]$Evidence,[int]$Seconds=3600,[string]$Label='journey-observations',[switch]$LiveJourney)
$ErrorActionPreference='Stop'
$relative=if($LiveJourney){'UserData/Saved/S9/EgmoreTambaram/LiveServiceV1/telemetry.json'}else{'UserData/Saved/S8/EgmoreTambaram/ServiceV6/telemetry.json'}
$telemetry=Join-Path $Evidence $relative
if($Label -notmatch '^[A-Za-z0-9_-]+$'){throw 'Use a simple label'}
$output=Join-Path $Evidence ($Label+'.jsonl')
if(Test-Path $output){throw 'Use fresh observation file'}
$watch=[Diagnostics.Stopwatch]::StartNew()
$lastReport=-60
while($watch.Elapsed.TotalSeconds -lt $Seconds){
 $process=Get-Process -Id $GameProcessId -ErrorAction SilentlyContinue
 if(!$process){Write-Output 'Game exited';break}
 try {$s=Get-Content -LiteralPath $telemetry -Raw | ConvertFrom-Json} catch {Start-Sleep -Seconds 2;continue}
 $row=[ordered]@{wallSeconds=$watch.Elapsed.TotalSeconds;tick=$s.service.ticks;phase=$s.service.phase;success=$s.service.success;settled=$s.service.settled;paused=$s.paused;blocked=$s.blocked;faulted=$s.logical.faulted;fingerprint=$s.logicalFingerprint;head=$s.logical.trains[1].handling.headMetres;speedMps=$s.logical.trains[1].handling.speedMps;workingSetBytes=$process.WorkingSet64;privateBytes=$process.PrivateMemorySize64}
 $row.trains=@($s.logical.trains | ForEach-Object { @{id=$_.id;edge=$_.handling.edge;head=$_.handling.headMetres;speed=$_.handling.speedMps;route=$_.route;authority=$_.authorityValid} })
 $row.points=$s.logical.points
 $row.streaming=$s.streaming
 $row.views=$s.tambaramViews
 $row.loadedLevels=$s.loadedLevels
 $row.retiredActorsAlive=$s.retiredActorsAlive
 $row.retiredMeshesAlive=$s.retiredMeshesAlive
 $row.saveMs=$s.saveMs
 $row | ConvertTo-Json -Depth 8 -Compress | Add-Content -LiteralPath $output
 if($watch.Elapsed.TotalSeconds-$lastReport -ge 60){$row | ConvertTo-Json -Depth 8 -Compress; $lastReport=$watch.Elapsed.TotalSeconds}
 if($s.service.phase -eq 'Result' -or $s.logical.faulted){Copy-Item -LiteralPath $telemetry -Destination (Join-Path $Evidence 'terminal-telemetry.json');break}
 Start-Sleep -Seconds 5
}
