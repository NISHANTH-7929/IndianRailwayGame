param([string]$Label='DrivingUI-01',[int]$Width=1600,[int]$Height=900)
$ErrorActionPreference='Stop'
if($Label -notmatch '^[A-Za-z0-9-]+$'){throw 'Invalid evidence label'}
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$saved=Join-Path $root '03_UnrealProject/MyProject/Saved'
$out=Join-Path $root "07_Documentation/S8/Evidence/StabilizationDesktop-20261011/$Label"
if(Test-Path $out){throw 'Evidence directory already exists'}
function NormalHashes {
 $rows=@{}
 foreach($family in 'S7','S8/LHB/Service','S8/Station/Service','S8/Egmore/Service'){
  foreach($slot in 0,1){$file=Join-Path $saved "$family/checkpoint-$slot.irs";$rows[$file]=if(Test-Path $file){(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash}else{$null}}
 }
 return $rows
}
$before=NormalHashes
$arguments=@("$root/03_UnrealProject/MyProject/MyProject.uproject",'/Game/IndianRailway/S8/Egmore/L_RailService_Egmore','-game','-S8Egmore','-S8Station','-S8LHB','-S8AssetReview','-S8Cab','-unattended','-nosplash','-windowed',"-ResX=$Width","-ResY=$Height",'-ForceRes','-UseFixedTimeStep','-FPS=30',"-RailDesktopEvidence=$out",'"-ExecCmds=t.MaxFPS 30,Rail.S8DesktopReview"',"-abslog=$out-engine.log")
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
$timedOut=-not $process.WaitForExit(420000)
if($timedOut){Stop-Process -Id $process.Id;$process.WaitForExit()}
$after=NormalHashes
$unchanged=$true
foreach($path in $before.Keys){if($before[$path] -ne $after[$path]){$unchanged=$false}}
$record=[ordered]@{normalSavesUnchanged=$unchanged;before=$before;after=$after;timedOut=$timedOut;exitCode=$process.ExitCode;resolution=@($Width,$Height);fixedSimulationStepSeconds=(1.0/30);androidVerified=$false;realTimePerformanceVerified=$false}
$record|ConvertTo-Json -Depth 5|Set-Content -LiteralPath "$out-preservation.json"
if(!$unchanged -or $timedOut){throw 'Desktop preservation or time bound failed'}
if(!(Test-Path "$out/result.json")){throw 'No desktop review result'}
$result=Get-Content "$out/result.json" -Raw|ConvertFrom-Json
if(!$result.passed){throw 'Desktop checks failed: inspect result.json'}
Write-Output "Desktop review passed: $out"
