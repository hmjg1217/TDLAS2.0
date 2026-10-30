param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$config = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'local-env.json') -Raw | ConvertFrom-Json
$env:PATH = (Join-Path $config.qtRoot 'bin') + ';' + $env:PATH
$env:TDLAS_DATA_ROOT = Join-Path $projectDirectory 'data'
$executable = Join-Path $projectDirectory "build/msvc-daqmx/$Configuration/tdlas_app.exe"
if (-not (Test-Path -LiteralPath $executable)) { throw "Build first: $projectDirectory/Build-$Configuration.cmd" }
& $executable
