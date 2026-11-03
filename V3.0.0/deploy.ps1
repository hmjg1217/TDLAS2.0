param(
    [string]$QtRoot = $env:QTDIR,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo', 'MinSizeRel')]
    [string]$Configuration = 'Release',
    [string]$BuildDirectory = 'build',
    [string]$PackageDirectory = 'dist',
    [switch]$IncludePreviewTargets
)

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $QtRoot) {
    $QtRoot = 'C:\Qt\6.8.3\msvc2022_64'
}

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory
} else {
    Join-Path $projectRoot $BuildDirectory
}
$packagePath = if ([System.IO.Path]::IsPathRooted($PackageDirectory)) {
    $PackageDirectory
} else {
    Join-Path $projectRoot $PackageDirectory
}
$deployTool = Join-Path $QtRoot 'bin\windeployqt.exe'
$previewExecutableNames = @(
    'tdlas_startup_codex_template.exe',
    'tdlas_admin_codex_template.exe',
    'tdlas_client_codex_template.exe'
)
$executableNames = @('tdlas_hydrogen_ui.exe')
if ($IncludePreviewTargets) {
    $executableNames += $previewExecutableNames
}

if (-not (Test-Path -LiteralPath $deployTool)) {
    throw "未找到 windeployqt.exe：$deployTool"
}

$sourceExecutables = foreach ($name in $executableNames) {
    $path = Join-Path $buildPath "bin\$Configuration\$name"
    if (-not (Test-Path -LiteralPath $path)) {
        throw "未找到构建产物：$path。请先运行 build.ps1。"
    }
    $path
}

New-Item -ItemType Directory -Path $packagePath -Force | Out-Null
if (-not $IncludePreviewTargets) {
    foreach ($previewName in $previewExecutableNames) {
        $stalePreview = Join-Path $packagePath $previewName
        if (Test-Path -LiteralPath $stalePreview) {
            Remove-Item -LiteralPath $stalePreview -Force
        }
    }
}
$deployMode = if ($Configuration -eq 'Debug') { '--debug' } else { '--release' }
foreach ($sourceExecutable in $sourceExecutables) {
    $deployedExecutable = Join-Path $packagePath (Split-Path -Leaf $sourceExecutable)
    Copy-Item -LiteralPath $sourceExecutable -Destination $deployedExecutable -Force
    & $deployTool $deployMode --compiler-runtime --no-translations --dir $packagePath $deployedExecutable
    if ($LASTEXITCODE -ne 0) {
        throw "Qt 运行库部署失败，退出码 $LASTEXITCODE"
    }
}

$runtimeNames = @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
$missingRuntime = $runtimeNames | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $packagePath $_))
}
if ($missingRuntime) {
    $redistRoot = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC'
    $versionDirectories = if (Test-Path -LiteralPath $redistRoot) {
        Get-ChildItem -LiteralPath $redistRoot -Directory |
            Where-Object { $_.Name -match '^\d+(\.\d+)+$' } |
            Sort-Object { [version]$_.Name } -Descending
    }
    $crtDirectory = $versionDirectories |
        ForEach-Object { Join-Path $_.FullName 'x64\Microsoft.VC143.CRT' } |
        Where-Object { Test-Path -LiteralPath $_ } |
        Select-Object -First 1

    if ($crtDirectory) {
        Get-ChildItem -LiteralPath $crtDirectory -Filter '*.dll' -File | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $packagePath $_.Name) -Force
        }
    }
}

$requiredFiles = @($executableNames | ForEach-Object { Join-Path $packagePath $_ })
$requiredFiles += Join-Path $packagePath 'platforms\qwindows.dll'
$requiredFiles += $runtimeNames | ForEach-Object { Join-Path $packagePath $_ }
$missingFiles = $requiredFiles | Where-Object { -not (Test-Path -LiteralPath $_) }
if ($missingFiles) {
    throw "部署目录缺少必要文件：$($missingFiles -join ', ')"
}

Write-Host "可运行部署包已生成：$packagePath"
