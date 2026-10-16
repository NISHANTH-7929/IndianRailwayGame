$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$evidence=Join-Path $root '07_Documentation/S8/Evidence/StabilizationDesktop-20261011'
$checks=@()
foreach($run in 'DrivingUI-02','DrivingUI-PhoneAspect-01'){
 $result=Get-Content "$evidence/$run/result.json" -Raw|ConvertFrom-Json
 foreach($step in $result.steps){
  $l=$step.locomotive
  $gauge=@($l.controlPoses|Where-Object role -eq 'gauge_speed')[0]
  $expectedGauge=-130+260*[Math]::Min([Math]::Abs($step.speedMps)*3.6/180,1)
  $powerMatch=$step.hud -match 'Applied Power (\d+)% / Brake (\d+)%'
  $hudPower=if($powerMatch){[int]$Matches[1]}else{-999}
  $hudBrake=if($powerMatch){[int]$Matches[2]}else{-999}
  $row=[ordered]@{
   run=$run;step=$step.name
   cabBrake=([Math]::Abs($step.appliedBrake-$l.appliedBrake) -lt .00001)
   cabPower=([Math]::Abs($step.appliedPower-$l.appliedPower) -lt .00001)
   cabEmergency=($step.emergency -eq $l.emergency)
   hudBrake=($powerMatch -and [Math]::Abs($hudBrake-$step.appliedBrake*100) -le .501)
   hudPower=($powerMatch -and [Math]::Abs($hudPower-$step.appliedPower*100) -le .501)
   hudEmergency=($step.hud.Contains('EMERGENCY') -eq $step.emergency)
   speedReadout=([Math]::Abs($l.speedKmh-[Math]::Abs($step.speedMps)*3.6) -lt .001)
   speedNeedle=($null -ne $gauge -and [Math]::Abs($gauge.rotation[2]-$expectedGauge) -lt .03)
  }
  $checks+=[pscustomobject]$row
 }
}
$failed=@($checks|Where-Object {!$_.cabBrake -or !$_.cabPower -or !$_.cabEmergency -or !$_.hudBrake -or !$_.hudPower -or !$_.hudEmergency -or !$_.speedReadout -or !$_.speedNeedle})
[ordered]@{passed=($failed.Count -eq 0);sampleCount=$checks.Count;failed=$failed;checks=$checks;scope='Read-only comparison of recorded authoritative handling, cab telemetry, HUD text and actual speed-needle rotation. Does not verify physical input or all other gauges.'}|ConvertTo-Json -Depth 7|Set-Content "$evidence/presentation-crosscheck.json"
if($failed.Count){$failed|Format-Table;throw 'Presentation evidence mismatch'}
Write-Output "$($checks.Count) presentation samples passed all eight consistency checks."
