param(
    [string]$QtRoot = $env:QTDIR,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [string]$BuildDirectory = 'build',
    [ValidateSet('all', 'integrated', 'startup', 'admin', 'client')]
    [string]$Target = 'all'
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cmakeCandidates = @()
if ($QtRoot) {
    $cmakeCandidates += Join-Path $QtRoot 'Tools\CMake_64\bin\cmake.exe'
}
$cmakeCandidates += 'C:\Qt\Tools\CMake_64\bin\cmake.exe'
$cmakeFromPath = Get-Command cmake.exe -ErrorAction SilentlyContinue | Select-Object -First 1
if ($cmakeFromPath) {
    $cmakeCandidates += $cmakeFromPath.Source
}

$qtCandidates = @()
if ($QtRoot) {
    $qtCandidates += $QtRoot
}
$qtCandidates += 'C:\Qt\6.8.3\msvc2022_64'

$cmake = $cmakeCandidates |
    Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
    Select-Object -First 1
$resolvedQtRoot = $qtCandidates |
    Where-Object {
        $_ -and (
            (Test-Path -LiteralPath (Join-Path $_ 'lib\cmake\Qt6')) -or
            (Test-Path -LiteralPath (Join-Path $_ 'lib\cmake\Qt5'))
        )
    } |
    Select-Object -First 1

if (-not $cmake) {
    throw '未找到 cmake.exe。请安装 CMake，或把 Qt Tools CMake 加入 PATH。'
}
if (-not $resolvedQtRoot) {
    throw '未找到 Qt CMake 包。请通过 -QtRoot 或 QTDIR 指定 Qt 5/6 工具链根目录。'
}

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory
} else {
    Join-Path $projectRoot $BuildDirectory
}

& $cmake -S $projectRoot -B $buildPath -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_PREFIX_PATH=$resolvedQtRoot"
if ($LASTEXITCODE -ne 0) {
    throw "CMake 配置失败，退出码 $LASTEXITCODE"
}

$targetMap = @{
    integrated = 'tdlas_hydrogen_ui'
    startup = 'tdlas_startup_codex_template'
    admin = 'tdlas_admin_codex_template'
    client = 'tdlas_client_codex_template'
}
$buildArguments = @('--build', $buildPath, '--config', $Configuration, '--parallel')
if ($Target -ne 'all') {
    $buildArguments += @('--target', $targetMap[$Target])
}

& $cmake @buildArguments
if ($LASTEXITCODE -ne 0) {
    throw "构建失败，退出码 $LASTEXITCODE"
}

Write-Host "构建完成：$buildPath\bin\$Configuration"
