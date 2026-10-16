param([string]$Label='paused-lifecycle')
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S7Device.ps1')
$package='com.indianrailway.s1harness'
$root=Join-Path $PSScriptRoot '../Evidence/Device'
function Observe([string]$Suffix){
 & (Join-Path $PSScriptRoot 'Observe-S7.ps1') -Label "$Label-$Suffix" | Out-Null
 return Get-Content "$root/$Label-$Suffix/telemetry.json" -Raw | ConvertFrom-Json
}
function Adb([string[]]$Arguments){$output=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $output"};return $output}
$before=Observe 'before'
if(-not $before.paused -or $before.service.phase -ne 'Running'){throw 'Requires a paused running service'}
Start-Sleep -Seconds 5
$frozen=Observe 'frozen'
Adb @('shell','input','keyevent','KEYCODE_HOME') | Out-Null
Start-Sleep -Seconds 3
Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity") | Out-Null
Start-Sleep -Seconds 4
$warm=Observe 'warm'
Adb @('shell','am','force-stop',$package) | Out-Null
Adb @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity") | Out-Null
Start-Sleep -Seconds 10
$cold=Observe 'cold'
$checks=[ordered]@{
 pauseFreezesCompleteState=$before.logicalFingerprint -eq $frozen.logicalFingerprint
 backgroundForeground=$warm.paused -and $warm.logicalFingerprint -eq $before.logicalFingerprint
 coldResume=$cold.paused -and $cold.logicalFingerprint -eq $before.logicalFingerprint
 railwayValid=$cold.logical.valid -and -not $cold.logical.faulted
}
$checks | ConvertTo-Json | Set-Content "$root/$Label-results.json"
$checks | ConvertTo-Json
if($checks.Values -contains $false){throw 'Lifecycle check failed; inspect retained evidence'}
