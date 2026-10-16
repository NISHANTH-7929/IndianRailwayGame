param([ValidateSet('Install','Launch','Snapshot','Pull','Stop')][string]$Action,[string]$Label='s8',[string]$Serial='')
$ErrorActionPreference='Stop'
if(-not $Serial){$Serial=& (Join-Path $PSScriptRoot '../../S7/Tools/Resolve-S7Device.ps1')}
$workspace=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$adb=Join-Path $env:ANDROID_HOME 'platform-tools\adb.exe'
$package='com.indianrailway.s1harness'
$out=Join-Path $PSScriptRoot "..\Evidence\Device\$Label"
New-Item -ItemType Directory -Force $out | Out-Null
function Record([string]$Name,[string[]]$Arguments){
 $value=& $adb -s $Serial @Arguments 2>&1;$code=$LASTEXITCODE
 $value | Set-Content (Join-Path $out "$Name.txt")
 [ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');arguments=$Arguments;exitCode=$code} | ConvertTo-Json -Compress | Add-Content (Join-Path $out 'commands.jsonl')
 if($code -ne 0){throw "ADB $Name failed: $value"};return $value
}
switch($Action){
 'Install'{
  $apk=Join-Path $workspace '08_Builds\S8_Android\Android_ASTC\MyProject-arm64.apk'
  Get-FileHash $apk | ConvertTo-Json | Set-Content (Join-Path $out 'apk-hash.json')
  Record 'install' @('install','-r',$apk)
  Record 'package' @('shell','dumpsys','package',$package) | Out-Null
 }
 'Launch'{
  Record 'stop' @('shell','am','force-stop',$package)
  Record 'airplane' @('shell','settings','get','global','airplane_mode_on')
  Record 'mobile-data' @('shell','settings','get','global','mobile_data')
  Record 'launch' @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity")
 }
 'Snapshot'{
  Record 'pid' @('shell','pidof',$package)
  Record 'memory' @('shell','dumpsys','meminfo',$package) | Out-Null
  Record 'thermal' @('shell','dumpsys','thermalservice') | Out-Null
  Record 'capture' @('shell','screencap','-p','/data/local/tmp/ir-s8.png') | Out-Null
  Record 'screen' @('pull','/data/local/tmp/ir-s8.png',(Join-Path $out 'screen.png'))
 }
 'Pull'{
  Record 'saved' @('pull',"/sdcard/Android/data/$package/files/UnrealGame/MyProject/MyProject/Saved",(Join-Path $out 'Saved'))
  $processText=(& $adb -s $Serial shell pidof $package) -join ' '
  $appProcess=($processText.Trim() -split '\s+')[0]
  if($appProcess){Record 'logcat' @('logcat','-d','-v','threadtime',"--pid=$appProcess") | Out-Null}
 }
 'Stop'{Record 'stop' @('shell','am','force-stop',$package)}
}







