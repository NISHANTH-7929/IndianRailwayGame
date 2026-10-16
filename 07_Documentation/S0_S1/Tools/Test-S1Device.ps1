param(
    [ValidateSet('Install','Launch','Snapshot','Background','Resume','Back','Stop','Pull')][string]$Action,
    [string]$Label = 'manual',
    [string]$Serial = '10BE1Y2EL40003J'
)
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$adb = Join-Path $env:ANDROID_HOME 'platform-tools\adb.exe'
$package = 'com.indianrailway.s1harness'
$evidence = Join-Path $PSScriptRoot "..\Evidence\Runs\$Label"
New-Item -ItemType Directory -Force -Path $evidence | Out-Null
function Record-Adb([string]$Name, [string[]]$AdbArgs) {
    $output = & $adb -s $Serial @AdbArgs 2>&1
    $code = $LASTEXITCODE
    $output | Set-Content -LiteralPath (Join-Path $evidence "$Name.txt")
    [ordered]@{utc=(Get-Date).ToUniversalTime().ToString('o');args=$AdbArgs;exitCode=$code} |
        ConvertTo-Json -Compress | Add-Content -LiteralPath (Join-Path $evidence 'commands.jsonl')
    if ($code -ne 0) { throw "adb $Name failed: $output" }
    return $output
}
switch ($Action) {
    'Install' {
        $apk = Join-Path $workspace '08_Builds\S1_Android\Android_ASTC\MyProject-arm64.apk'
        Get-FileHash -LiteralPath $apk -Algorithm SHA256 | ConvertTo-Json | Set-Content (Join-Path $evidence 'apk-hash.json')
        Record-Adb 'install' @('install','-r',$apk)
        Record-Adb 'package' @('shell','dumpsys','package',$package) | Out-Null
    }
    'Launch' {
        Record-Adb 'force-stop' @('shell','am','force-stop',$package)
        Record-Adb 'airplane' @('shell','settings','get','global','airplane_mode_on')
        Record-Adb 'wifi' @('shell','settings','get','global','wifi_on')
        Record-Adb 'data' @('shell','settings','get','global','mobile_data')
        Record-Adb 'launch' @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity")
    }
    'Snapshot' {
        Record-Adb 'pid' @('shell','pidof',$package)
        Record-Adb 'memory' @('shell','dumpsys','meminfo',$package) | Out-Null
        Record-Adb 'thermal' @('shell','dumpsys','thermalservice') | Out-Null
        Record-Adb 'battery' @('shell','dumpsys','battery') | Out-Null
        Record-Adb 'screencap' @('shell','screencap','-p','/data/local/tmp/ir-s1.png') | Out-Null
        Record-Adb 'screenshot-pull' @('pull','/data/local/tmp/ir-s1.png',(Join-Path $evidence 'screen.png'))
    }
    'Background' { Record-Adb 'home' @('shell','input','keyevent','3') }
    'Resume' { Record-Adb 'resume' @('shell','am','start','-W','-n',"$package/com.epicgames.unreal.SplashActivity") }
    'Back' { Record-Adb 'back' @('shell','input','keyevent','4') }
    'Stop' { Record-Adb 'force-stop' @('shell','am','force-stop',$package) }
    'Pull' {
        Record-Adb 'app-files' @('pull',"/sdcard/Android/data/$package/files/UnrealGame/Engine/Saved",(Join-Path $evidence 'Saved'))
        $appProcess = ((& $adb -s $Serial shell pidof $package).Trim() -split '\s+')[0]
        if ($appProcess) { Record-Adb 'logcat' @('logcat','-d','-v','threadtime',"--pid=$appProcess") | Out-Null }
    }
}
