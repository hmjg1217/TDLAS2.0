param(
    [string]$QtRoot = $env:QTDIR,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [string]$BuildDirectory = 'build',
    [string]$OutputDirectory = 'artifacts\captures'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
if (-not $QtRoot) {
    $QtRoot = 'C:\Qt\6.8.3\msvc2022_64'
}
$qtBinPath = Join-Path $QtRoot 'bin'
if (-not (Test-Path -LiteralPath (Join-Path $qtBinPath 'Qt6Core.dll')) -and
    -not (Test-Path -LiteralPath (Join-Path $qtBinPath 'Qt5Core.dll'))) {
    throw "未找到 Qt 运行库目录：$qtBinPath"
}
$env:PATH = "$qtBinPath;$env:PATH"

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory
} else {
    Join-Path $projectRoot $BuildDirectory
}
$outputPath = if ([System.IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory
} else {
    Join-Path $projectRoot $OutputDirectory
}
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null

$captureMatrix = @(
    @{ App = 'integrated'; Executable = 'tdlas_hydrogen_ui.exe'; Screen = 'login'; PassScreen = $false },
    @{ App = 'startup'; Executable = 'tdlas_startup_codex_template.exe'; Screen = 'splash'; PassScreen = $true },
    @{ App = 'startup'; Executable = 'tdlas_startup_codex_template.exe'; Screen = 'login'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'main'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'parameters'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'debug'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'peak'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'camera'; PassScreen = $true },
    @{ App = 'admin'; Executable = 'tdlas_admin_codex_template.exe'; Screen = 'camera-motion'; PassScreen = $true },
    @{ App = 'client'; Executable = 'tdlas_client_codex_template.exe'; Screen = 'main'; PassScreen = $true },
    @{ App = 'client'; Executable = 'tdlas_client_codex_template.exe'; Screen = 'parameters'; PassScreen = $true },
    @{ App = 'client'; Executable = 'tdlas_client_codex_template.exe'; Screen = 'camera'; PassScreen = $true },
    @{ App = 'client'; Executable = 'tdlas_client_codex_template.exe'; Screen = 'camera-motion'; PassScreen = $true }
)

$results = foreach ($entry in $captureMatrix) {
    $executable = Join-Path $buildPath "bin\$Configuration\$($entry.Executable)"
    if (-not (Test-Path -LiteralPath $executable)) {
        throw "未找到构建产物：$executable"
    }

    $capturePath = Join-Path $outputPath "$($entry.App)-$($entry.Screen).png"
    $argumentList = @('--capture', $capturePath)
    if ($entry.PassScreen) {
        $argumentList = @('--screen', $entry.Screen) + $argumentList
    }
    $process = Start-Process -FilePath $executable -ArgumentList $argumentList -WindowStyle Hidden -Wait -PassThru

    if ($process.ExitCode -ne 0) {
        throw "$($entry.App)/$($entry.Screen) 截图失败，退出码 $($process.ExitCode)"
    }
    if (-not (Test-Path -LiteralPath $capturePath)) {
        throw "$($entry.App)/$($entry.Screen) 未生成截图：$capturePath"
    }

    $file = Get-Item -LiteralPath $capturePath
    [pscustomobject]@{
        App = $entry.App
        Screen = $entry.Screen
        ExitCode = $process.ExitCode
        Bytes = $file.Length
        Path = $file.FullName
    }
}

$results | Format-Table -AutoSize
