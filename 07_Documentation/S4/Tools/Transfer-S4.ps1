$ErrorActionPreference='Stop'
$root=(Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$adb=Join-Path $env:ANDROID_HOME 'platform-tools/adb.exe'
$serial=& (Join-Path $PSScriptRoot 'Resolve-S4Device.ps1')
$apk=Join-Path $root '08_Builds/S4_Android/Android_ASTC/MyProject-arm64.apk'
$part=Join-Path $root '07_Documentation/S4/Evidence/transfer-chunk.bin'
$expected=(Get-FileHash $apk -Algorithm SHA256).Hash
$remote="/data/local/tmp/ir-s4-$($expected.ToLowerInvariant()).payload"
& $adb -s $serial shell "touch $remote"
if($LASTEXITCODE){throw 'Cannot access phone'}
$offset=[long]((& $adb -s $serial shell "stat -c %s $remote").Trim())
if($LASTEXITCODE){throw 'Cannot read transfer offset'}
$stream=[IO.File]::OpenRead($apk)
try {
 if($offset -gt $stream.Length){throw 'Remote transfer larger than APK'}
 [void]$stream.Seek($offset,[IO.SeekOrigin]::Begin)
 $buffer=New-Object byte[] (1MB)
 while(($count=$stream.Read($buffer,0,$buffer.Length)) -gt 0){
  if($count -eq $buffer.Length){[IO.File]::WriteAllBytes($part,$buffer)}else{[IO.File]::WriteAllBytes($part,$buffer[0..($count-1)])}
  $transfer=& $adb -s $serial push -Z $part /data/local/tmp/ir-s4-part 2>&1
  if($LASTEXITCODE){throw "Transfer stopped at $offset bytes; rerun to resume. $transfer"}
  & $adb -s $serial shell "cat /data/local/tmp/ir-s4-part >> $remote"
  if($LASTEXITCODE){throw 'Append interrupted; rerun to resume from actual remote length'}
  $offset+=$count
  if($offset % (16MB) -eq 0){Write-Output "Transferred $offset / $($stream.Length) bytes"}
  Start-Sleep -Milliseconds 150
 }
}finally{$stream.Dispose()}
$actual=(& $adb -s $serial shell "sha256sum $remote").Trim().Split(' ')[0]
if($actual -ine $expected){throw "Hash mismatch: $actual"}
Write-Output "Verified SHA256 $actual"
& $adb -s $serial shell pm install -r $remote
if($LASTEXITCODE){throw 'Package installation failed'}



