$ErrorActionPreference='Stop'
$tools=$PSScriptRoot
$clock=[Diagnostics.Stopwatch]::StartNew()
for($i=0;$i -le 7;$i++){
 $target=$i*30
 while($clock.Elapsed.TotalSeconds -lt $target){Start-Sleep -Milliseconds 250}
 & (Join-Path $tools 'Test-S2Device.ps1') -Action Snapshot -Label "final-sustained/sample-$i"
 if($LASTEXITCODE){throw 'Snapshot failed'}
 Write-Output "Sample $i at $([math]::Round($clock.Elapsed.TotalSeconds,2)) seconds"
}
& (Join-Path $tools 'Test-S2Device.ps1') -Action Pull -Label 'final-sustained'
