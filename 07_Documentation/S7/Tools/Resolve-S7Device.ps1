$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$devices=& $adb devices
if($LASTEXITCODE){throw 'ADB inventory failed'}
$matches=@($devices | Where-Object {$_ -match '^\S*10BE1Y2EL40003J\S*\s+device\s*$'} | ForEach-Object {($_ -split '\s+')[0]})
if($matches.Count -ne 1){throw "Expected one authorized iQOO endpoint, found $($matches.Count). Disconnect duplicate USB/wireless endpoints or reconnect the phone."}
$matches[0]


