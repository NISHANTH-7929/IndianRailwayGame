$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$evidence=Join-Path $PSScriptRoot '../Evidence'
function ReadJson([string]$Relative){Get-Content (Join-Path $evidence $Relative) -Raw | ConvertFrom-Json}
$hostHandling=ReadJson 'host-tests.json'
$hostKernel=ReadJson 's2-host-tests.json'
$deviceHandling=ReadJson 'Device/handoff-sustained/Saved/S3/handling-tests.json'
$deviceKernel=ReadJson 'Device/handoff-sustained/Saved/S3/kernel-tests.json'
$controls=ReadJson 'Device/handoff-controls/results.json'
$automation=ReadJson 'UnrealAutomation/index.json'
$state=ReadJson 'Device/handoff-sustained/Saved/S3/telemetry.json'
$performance=ReadJson 'final-performance.json'
$apk=ReadJson 'final-apk.json'
$log=Get-Content (Join-Path $evidence 'Device/handoff-sustained/Saved/Logs/MyProject.log') -Raw
$package=Get-Content (Join-Path $evidence 'Device/handoff-launch/package.txt') -Raw
$report=Get-Content (Join-Path $PSScriptRoot '../S3_IMPLEMENTATION_REPORT.md') -Raw
$nativeBefore=ReadJson 'profiled-native-hash.json'
$nativeAfter=ReadJson 'final-native-hash.json'
$checks=[ordered]@{
 hostHandling=$hostHandling.passed -and $hostHandling.cases.Count -eq 28
 hostKernel=$hostKernel.passed -and $hostKernel.cases.Count -eq 39
 androidHandling=$deviceHandling.passed -and $deviceHandling.cases.Count -eq 28
 androidKernel=$deviceKernel.passed -and $deviceKernel.cases.Count -eq 39
 androidControls=$controls.passed -and @($controls.checks.psobject.Properties).Count -eq 16
 unrealAutomation=$automation.succeeded -eq 2 -and $automation.failed -eq 0 -and $automation.succeededWithWarnings -eq 0
 normalPlayBeyond6000Ticks=$state.tick -gt 6000 -and $state.speedMps -eq 0 -and $state.rejected -eq 0 -and $state.brake -eq 1
 noNormalPlayFault=$log -notmatch 'PAUSED:|Fixed-step overload|Fatal error|LogRailKernel: Error'
 profilerDisabledByDefault=$log -notmatch 'csvCaptureFrames|Capture Ending'
 finalApkHash=(Get-FileHash (Join-Path $root '08_Builds/S3_Android/Android_ASTC/MyProject-arm64.apk')).Hash -eq $apk.sha256
 sameNativeLibrary=$nativeBefore.Hash -eq $nativeAfter.Hash -and (Get-FileHash $nativeAfter.Path).Hash -eq $nativeAfter.Hash
 offlineManifest=$package.Length -gt 500 -and $package.Contains('com.indianrailway.s1harness') -and $package -notmatch 'android.permission.INTERNET'
 completePerformanceCapture=$performance.total_frames -eq 6000
 reportSections=[regex]::Matches($report,'(?m)^## \d+\.').Count -eq 20
 documentedAcceptanceGates=[regex]::Matches($report,'(?m)^\| \d+\..*\| PASS \|').Count -eq 21
}
$summary=[ordered]@{
 status=if($checks.Values -contains $false){'FAIL'}else{'PASS'}
 milestone='S3';checkedUtc=(Get-Date).ToUniversalTime().ToString('o');checks=$checks
 acceptanceGates=21;gateAssessment='Source/architecture review and measured evidence mapped individually in report section19; not all gates are machine-testable.'
 tests=[ordered]@{hostHandling=28;hostKernel=39;androidHandling=28;androidKernel=39;androidControls=16;unrealSuites=2}
 apk=$apk;normalPlayFinalState=$state;profilingVariantPerformance=$performance
 knownProfilerLimitation='Export after6000 captured frames triggered the preserved overload pause. Normal handoff launch disables automatic profiling and passes215s driving beyond that point. Native library identical.'
 next='STOP. Owner review before S4 occupancy/protected infrastructure/route requests/point locking/movement authority/signal presentation.'
}
$summary | ConvertTo-Json -Depth 12 | Set-Content (Join-Path $evidence 'acceptance-summary.json')
[pscustomobject]$summary | Select-Object status,tests,checks | ConvertTo-Json -Depth 5
if($checks.Values -contains $false){exit 1}
exit 0
