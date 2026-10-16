param([string]$Label=('foundation-profile-'+(Get-Date -Format 'yyyyMMdd-HHmmss')))
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot '../../S7/Tools/Resolve-S7Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$package='com.indianrailway.s1harness'
$base="/sdcard/Android/data/$package/files/UnrealGame/MyProject"
$saved="$base/MyProject/Saved"
$remote="$base/UECommandLine.txt"
function Adb([string[]]$Arguments){
 $value=& $adb -s $serial @Arguments 2>&1
 if($LASTEXITCODE){throw "ADB failed: $value"}
 return $value
}
function ReadState(){
 for($attempt=0;$attempt -lt 5;$attempt++){
  $raw=Adb @('shell','cat',"$saved/S7/telemetry.json")
  try{return ($raw | ConvertFrom-Json -ErrorAction Stop)}catch{if($attempt -eq 4){throw};Start-Sleep -Milliseconds 100}
 }
}
$profile=Get-Content (Join-Path $PSScriptRoot '../../../03_UnrealProject/MyProject/Content/IndianRailway/Production/profile.json') -Raw | ConvertFrom-Json
if((Adb @('shell','getprop','ro.product.model')).Trim() -ne 'I2304'){throw 'Wrong device'}
if((Adb @('shell','getprop','ro.build.version.sdk')).Trim() -ne '36'){throw 'Wrong Android API'}
$before=ReadState
if(-not $before.paused){throw 'Foundation harness smoke requires paused S7 state; never pause or resume it blindly'}
$before | ConvertTo-Json -Depth 50 | Set-Content "$out/before.json"
& $adb -s $serial shell ls $remote 2>&1 | Out-Null
$existed=$LASTEXITCODE -eq 0
if($existed){Adb @('pull',$remote,"$out/original-commandline.txt") | Out-Null}
$oldCsv=@(Adb @('shell','ls',"$saved/Profiling/CSV"))
$changed=$false
try {
 $command='-project="../../../MyProject/MyProject.uproject" /Game/IndianRailway/S7/L_RailService -csvCaptureFrames='+$profile.captureFrames+' -csvGpuStats -csvCompression=0 -ExecCmds="r.SetFramePace 30" -log'
 $command | Set-Content "$out/profile-commandline.txt"
 Adb @('push',"$out/profile-commandline.txt",$remote) | Out-Null
 $changed=$true
 & (Join-Path $PSScriptRoot 'Device-S8.ps1') -Action Launch -Label "$Label/launch" | Out-Null
 $timer=[Diagnostics.Stopwatch]::StartNew()
 $samples=@();$pss=@();$thermal=@();$latency=@();$residency=@();$lastLoads=-1
 while($timer.Elapsed.TotalSeconds -lt 225){
  Start-Sleep -Seconds 15
  $focus=(Adb @('shell','dumpsys','window')) -join "`n"
  if($focus -notmatch 'mCurrentFocus.*com\.indianrailway\.s1harness'){throw 'Game left foreground; profile excluded'}
  $state=ReadState
  if(-not $state.paused -or $state.logicalFingerprint -ne $before.logicalFingerprint -or -not $state.logical.valid -or $state.logical.faulted){throw 'Paused fixture changed; profile excluded'}
  $stamp=[int]$timer.Elapsed.TotalSeconds
  $samples+=,[ordered]@{seconds=$timer.Elapsed.TotalSeconds;state=$state}
  $memory=(Adb @('shell','dumpsys','meminfo',$package)) -join "`n"
  $memory | Set-Content "$out/memory-$stamp.txt"
  if($memory -match 'TOTAL PSS:\s*(\d+)'){$pss+=[double]$Matches[1]}
  elseif($memory -match '(?m)^\s*TOTAL\s+(\d+)'){$pss+=[double]$Matches[1]}
  $temp=(Adb @('shell','dumpsys','thermalservice')) -join "`n"
  $temp | Set-Content "$out/thermal-$stamp.txt"
  if($temp -match 'Thermal Status:\s*(\d+)'){$thermal+=[int]$Matches[1]}
  $residency+=[int]$state.loadedLevels
  if($state.loads -ne $lastLoads){foreach($ms in $state.loadMs){if($ms -gt 0){$latency+=[double]$ms}};$lastLoads=$state.loads}
  $samples | ConvertTo-Json -Depth 50 | Set-Content "$out/samples.json"
  Write-Output "S8 paused-fixture harness: $stamp seconds; tick $($state.logical.tick)"
 }
 $newCsv=@(Adb @('shell','ls',"$saved/Profiling/CSV") | Where-Object {$_ -notin $oldCsv -and $_ -match '\.csv$'})
 if($newCsv.Count -ne 1){throw "Expected one fresh CSV, got $($newCsv.Count)"}
 Adb @('pull',"$saved/Profiling/CSV/$($newCsv[0])","$out/capture.csv") | Out-Null
 Adb @('pull',"$saved/S8/foundation-runtime.json","$out/foundation-runtime.json") | Out-Null
 $runtime=Get-Content "$out/foundation-runtime.json" -Raw | ConvertFrom-Json
 if(-not $runtime.passed){throw 'Packaged S8 runtime tests failed'}
 [ordered]@{PSS=$pss;thermalStatus=$thermal;loadLatency=$latency;residency=$residency;activationCounts=@();scope='paused S7 fixture; asset activation instrumentation unavailable';loadLatencyInterpretation='latest positive per-chunk latency sampled on load-count change, not exhaustive event distribution'} | ConvertTo-Json -Depth 8 | Set-Content "$out/device-metrics.json"
 $apk=Join-Path $PSScriptRoot '../../../08_Builds/S8_Android/Android_ASTC/MyProject-arm64.apk'
 & python (Join-Path $PSScriptRoot 'measure_s8.py') "$out/capture.csv" --device "$out/device-metrics.json" --apk $apk --output "$out/measurement.json"
 if($LASTEXITCODE){throw 'Measurement normalization failed'}
 [ordered]@{complete=$true;scope='foundation paused-fixture harness only';representativeWorkload=$false;seconds=$timer.Elapsed.TotalSeconds;samples=$samples.Count} | ConvertTo-Json | Set-Content "$out/completion.json"
}finally{
 if($changed){
  if($existed){Adb @('push',"$out/original-commandline.txt",$remote) | Out-Null}else{Adb @('shell','rm',$remote) | Out-Null}
  [ordered]@{restored=$true;previouslyExisted=$existed} | ConvertTo-Json | Set-Content "$out/commandline-restore.json"
 }
}
