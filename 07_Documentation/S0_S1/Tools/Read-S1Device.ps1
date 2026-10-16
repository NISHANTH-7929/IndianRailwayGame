param([string]$Serial, [string]$SdkRoot=$env:ANDROID_HOME)
$ErrorActionPreference='Stop'
$adb = Join-Path $SdkRoot 'platform-tools\adb.exe'
$outDir = Join-Path $PSScriptRoot '..\Evidence\Device'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$inventory = & $adb devices -l
$inventory | Set-Content (Join-Path $outDir 'adb-devices.txt')
$ready = @($inventory | Where-Object { $_ -match '^\S+\s+device\s' })
if (!$Serial) {
    if ($ready.Count -ne 1) { throw 'Exactly one authorized device required; no device facts inferred.' }
    $Serial = ($ready[0] -split '\s+')[0]
}
if ((& $adb -s $Serial get-state 2>$null) -ne 'device') { throw 'Selected device is not authorized/online.' }
$facts = [ordered]@{ capturedUtc=(Get-Date).ToUniversalTime().ToString('o'); serial=$Serial }
foreach ($item in @(@('model','ro.product.model'),@('manufacturer','ro.product.manufacturer'),
    @('androidVersion','ro.build.version.release'),@('api','ro.build.version.sdk'),
    @('soc','ro.soc.model'),@('hardware','ro.hardware'),@('abi','ro.product.cpu.abi'),
    @('buildFingerprint','ro.build.fingerprint'))) {
    $value = (& $adb -s $Serial shell getprop $item[1]) -join "`n"
    $facts[$item[0]] = $(if ($value.Trim()) {$value.Trim()} else {'UNVERIFIED'})
}
$facts | ConvertTo-Json | Set-Content (Join-Path $outDir 'device.json')
& $adb -s $Serial shell cat /proc/meminfo | Set-Content (Join-Path $outDir 'meminfo.txt')
& $adb -s $Serial shell cat /proc/cpuinfo | Set-Content (Join-Path $outDir 'cpuinfo.txt')
& $adb -s $Serial shell dumpsys SurfaceFlinger | Set-Content (Join-Path $outDir 'surfaceflinger.txt')
& $adb -s $Serial shell dumpsys thermalservice | Set-Content (Join-Path $outDir 'thermal-initial.txt')
& $adb -s $Serial shell dumpsys battery | Set-Content (Join-Path $outDir 'battery-initial.txt')
$facts | ConvertTo-Json
