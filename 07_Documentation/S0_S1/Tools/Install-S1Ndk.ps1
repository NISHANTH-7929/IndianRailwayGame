param([string]$SdkRoot = $env:ANDROID_HOME)
$ErrorActionPreference = 'Stop'
$workRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$downloadRoot = Join-Path $workRoot '08_Builds\S1_ToolDownloads'
$downloadRoot = [IO.Path]::GetFullPath($downloadRoot)
New-Item -ItemType Directory -Path $downloadRoot -Force | Out-Null
$zip = Join-Path $downloadRoot 'commandlinetools-win-15859902_latest.zip'
$expected = '90ae805d20434428bffcb699c290860f19bb5f66a67e6b330067e3de801fb04a'
if (!(Test-Path -LiteralPath $zip)) {
    Invoke-WebRequest 'https://dl.google.com/android/repository/commandlinetools-win-15859902_latest.zip' -OutFile $zip
}
if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ne $expected) {
    throw 'Google command-line tools checksum mismatch; refusing execution.'
}
$toolRoot = Join-Path $downloadRoot 'sdk-tools'
if (!(Test-Path -LiteralPath (Join-Path $toolRoot 'cmdline-tools\bin\sdkmanager.bat'))) {
    Expand-Archive -LiteralPath $zip -DestinationPath $toolRoot
}
$manager = Join-Path $toolRoot 'cmdline-tools\bin\sdkmanager.bat'
& $manager "--sdk_root=$SdkRoot" '--install' 'ndk;27.2.12479018'
if ($LASTEXITCODE -ne 0) { throw "sdkmanager failed with exit $LASTEXITCODE" }
Get-Content -LiteralPath (Join-Path $SdkRoot 'ndk\27.2.12479018\source.properties')
