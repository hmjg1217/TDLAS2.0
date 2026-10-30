param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [switch]$Package)
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$config = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'local-env.json') -Raw | ConvertFrom-Json
$variant = $Configuration.ToLowerInvariant() + '-daqmx'
$logs = Join-Path $projectDirectory ('artifacts/' + $variant)
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$buildVariant = 'msvc-daqmx'
$executable = Join-Path $projectDirectory "build/$buildVariant/$Configuration/tdlas_app.exe"
if ($Package) {
    $packageName = 'TDLAS2.0-NI-DAQmx-Portable'
    $executable = Join-Path $projectDirectory "artifacts/$packageName/tdlas_app.exe"
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    # Keep startup verification state out of the immutable portable package.
    $env:TDLAS_DATA_ROOT = Join-Path $logs 'portable-smoke-data'
} else { $env:PATH = (Join-Path $config.qtRoot 'bin') + ';' + $env:PATH }
if (-not $Package) { $env:TDLAS_DATA_ROOT = Join-Path $projectDirectory 'data' }
$screenshot = Join-Path $logs 'startup-native.png'
$process = Start-Process -FilePath $executable -ArgumentList @('--smoke-test', ('"' + $screenshot + '"')) -WindowStyle Hidden -PassThru
if (-not $process.WaitForExit(20000)) {
    Stop-Process -Id $process.Id
    throw 'UI startup smoke test timed out; stopped this test process (no acquisition was requested).'
}
$process.Refresh()
if ($process.ExitCode -ne 0) { throw "UI startup failed: $($process.ExitCode)" }
if (-not (Test-Path -LiteralPath $screenshot)) { throw 'UI screenshot was not created.' }
[ordered]@{Executable=$executable; ExitCode=$process.ExitCode; DaqmxEnabled=$true; Maximized=$true; AutomaticDiscovery=$false; AutomaticAcquisition=$false; QtRemovedFromPath=[bool]$Package; Screenshot=$screenshot} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'startup-result.json') -Encoding UTF8
Write-Host "Startup passed: $screenshot"
