param([int]$DurationSeconds=180,[string]$Serial='10BE1Y2EL40003J',[string]$Label='sustained')
$ErrorActionPreference='Stop'
$adb=Join-Path $env:ANDROID_HOME 'platform-tools\adb.exe'
$folder=Join-Path $PSScriptRoot "..\Evidence\Runs\$Label"
New-Item -ItemType Directory -Force -Path $folder | Out-Null
$watch=[Diagnostics.Stopwatch]::StartNew()
$samples=@()
for ($index=0; $index -le [math]::Ceiling($DurationSeconds/30); $index++) {
    $prefix=Join-Path $folder ('sample-{0:D2}' -f $index)
    $processIds=(((& $adb -s $Serial shell pidof com.indianrailway.s1harness).Trim()) -split '\s+')
    if (!$processIds[0]) { throw 'Harness exited during sustained measurement' }
    & $adb -s $Serial shell dumpsys meminfo com.indianrailway.s1harness | Set-Content "$prefix-memory.txt"
    & $adb -s $Serial shell dumpsys thermalservice | Set-Content "$prefix-thermal.txt"
    & $adb -s $Serial shell dumpsys battery | Set-Content "$prefix-battery.txt"
    $samples += [ordered]@{index=$index;elapsedSeconds=$watch.Elapsed.TotalSeconds;utc=(Get-Date).ToUniversalTime().ToString('o');processIds=$processIds}
    $samples | ConvertTo-Json | Set-Content (Join-Path $folder 'samples.json')
    Write-Output "Sample ${index}: processes=$($processIds -join ',') elapsed=$([math]::Round($watch.Elapsed.TotalSeconds,1))s"
    if ($watch.Elapsed.TotalSeconds -ge $DurationSeconds) { break }
    Start-Sleep -Seconds 30
}
& (Join-Path $PSScriptRoot 'Test-S1Device.ps1') -Action Pull -Label $Label -Serial $Serial
