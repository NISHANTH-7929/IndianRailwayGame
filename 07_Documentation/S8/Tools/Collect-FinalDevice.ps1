param([string]$Label='final-launch')
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot '../../S7/Tools/Resolve-S7Device.ps1')
$out=Join-Path $PSScriptRoot "../Evidence/Device/$Label"
New-Item -ItemType Directory -Force $out | Out-Null
$base='/sdcard/Android/data/com.indianrailway.s1harness/files/UnrealGame/MyProject'
function Adb([string[]]$Arguments){$value=& $adb -s $serial @Arguments 2>&1;if($LASTEXITCODE){throw "ADB failed: $value"};return $value}
$override=Adb @('shell',"if [ -e $base/UECommandLine.txt ]; then echo PRESENT; else echo ABSENT; fi")
$override | Set-Content "$out/override.txt"
if(($override -join '').Trim() -ne 'ABSENT'){throw 'Profiling override still present; inspect before normal-launch check'}
& (Join-Path $PSScriptRoot 'Device-S8.ps1') -Action Launch -Label $Label | Out-Null
Start-Sleep -Seconds 12
& (Join-Path $PSScriptRoot 'Observe-S8.ps1') -Label $Label -Screenshot | Out-Null
Adb @('pull',"$base/MyProject/Saved/S7","$out/S7") | Out-Null
Adb @('pull',"$base/MyProject/Saved/S8","$out/S8") | Out-Null
Adb @('pull',"$base/MyProject/Saved/Logs/MyProject.log","$out/engine.log") | Out-Null
[ordered]@{model=(Adb @('shell','getprop','ro.product.model')).Trim();api=(Adb @('shell','getprop','ro.build.version.sdk')).Trim();abi=(Adb @('shell','getprop','ro.product.cpu.abi')).Trim();serial=$serial} | ConvertTo-Json | Set-Content "$out/device.json"
Adb @('shell','dumpsys','package','com.indianrailway.s1harness') | Set-Content "$out/package.txt"
Write-Output 'Fresh normal-launch S7/S8 evidence collected.'
