param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [switch]$Deploy)
$ErrorActionPreference = 'Stop'
$projectDirectory = [IO.Path]::GetFullPath((Split-Path $PSScriptRoot -Parent))
$config = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'local-env.json') -Raw | ConvertFrom-Json
$cmake = Join-Path $config.cmakeRoot 'bin/cmake.exe'
$ctest = Join-Path $config.cmakeRoot 'bin/ctest.exe'
$vs = & $config.vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vs) { throw 'Visual Studio 2022 C++ tools not found.' }
foreach ($component in @('Core','Widgets','Charts','Multimedia','Test')) {
    $required = Join-Path $config.qtRoot "lib/cmake/Qt6$component/Qt6${component}Config.cmake"
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing Qt component: $required" }
}
# SUBST changes only the compiler-visible path; all bytes stay inside TDLAS2.0.
# Never replace another drive/mapping. Reuse a mapping only if its target matches.
$drive = $null
# Query Unicode paths directly: subst.exe's console output may use the OEM codepage.
Add-Type -Namespace TdlasBuild -Name DriveMapping -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("kernel32.dll", CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern uint QueryDosDevice(string name, System.Text.StringBuilder target, int capacity);
'@
foreach ($letter in @('T','U','V','W','X','Y','Z','S','R')) {
    $target = New-Object Text.StringBuilder 4096
    if ([TdlasBuild.DriveMapping]::QueryDosDevice(($letter + ':'), $target, $target.Capacity) -gt 0) {
        if ($target.ToString() -ieq ('\??\' + $projectDirectory.TrimEnd('\'))) { $drive = $letter + ':'; break }
    }
}
if (-not $drive) {
    foreach ($letter in @('T','U','V','W','X','Y','Z','S','R')) {
        if (-not (Test-Path ($letter + ':\')) -and -not (Get-PSDrive -Name $letter -ErrorAction SilentlyContinue)) {
            $drive = $letter + ':'; & subst.exe $drive $projectDirectory
            if ($LASTEXITCODE -ne 0) { throw 'Could not create ASCII project mapping.' }
            break
        }
    }
}
if (-not $drive) { throw 'No free drive letter for ASCII project mapping.' }
$source = $drive + '/'
$buildVariant = 'msvc-daqmx'
$artifactVariant = $Configuration.ToLowerInvariant() + '-daqmx'
$build = $drive + '/build/' + $buildVariant
$logs = Join-Path $projectDirectory ('artifacts/' + $artifactVariant)
New-Item -ItemType Directory -Force -Path $logs, (Join-Path $projectDirectory 'temp'), (Join-Path $projectDirectory 'data') | Out-Null
$env:PATH = (Join-Path $config.qtRoot 'bin') + ';' + $env:PATH
$env:TEMP = Join-Path $projectDirectory 'temp'; $env:TMP = $env:TEMP
$env:TDLAS_TEST_ARTIFACTS = Join-Path $logs 'ui'
function Invoke-Checked([string]$Tool, [string[]]$Arguments, [string]$Log) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Tool @Arguments 2>&1 | Tee-Object -FilePath (Join-Path $logs $Log) | Out-Host
        $toolExitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($toolExitCode -ne 0) { throw "$Tool failed: $toolExitCode; see $logs/$Log" }
}
$daqArguments = @("-DDAQMX_INCLUDE_DIR=$($config.daqmxInclude)", "-DDAQMX_LIBRARY=$($config.daqmxLibrary)")
Invoke-Checked $cmake (@('-S', $source, '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_GENERATOR_INSTANCE=$vs", "-DCMAKE_SYSTEM_VERSION=$($config.windowsSdkVersion)", "-DCMAKE_PREFIX_PATH=$($config.qtRoot)", '-DTDLAS_BUILD_TESTS=ON') + $daqArguments) 'configure.log'
Invoke-Checked $cmake @('--build', $build, '--config', $Configuration, '--parallel', '4') 'build.log'
Invoke-Checked $ctest @('--test-dir', $build, '-C', $Configuration, '--output-on-failure') 'test.log'
$executable = Join-Path $projectDirectory "build/$buildVariant/$Configuration/tdlas_app.exe"
$package = $null
$zipPath = $null
if ($Deploy) {
    if ($Configuration -ne 'Release') { throw 'Deploy requires Release configuration.' }
    $packageName = 'TDLAS2.0-NI-DAQmx-Portable'
    $package = Join-Path $projectDirectory ('artifacts/' + $packageName)
    $stageName = $packageName + '-stage'
    $stage = Join-Path $projectDirectory ('temp/' + $stageName)
    $artifactsRoot = [IO.Path]::GetFullPath((Join-Path $projectDirectory 'artifacts')).TrimEnd('\') + '\'
    $tempRoot = [IO.Path]::GetFullPath((Join-Path $projectDirectory 'temp')).TrimEnd('\') + '\'
    function Assert-GeneratedPath([string]$Candidate, [string]$AllowedRoot) {
        $resolved = [IO.Path]::GetFullPath($Candidate)
        if (-not $resolved.StartsWith($AllowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Generated path escaped its allowed root: $resolved"
        }
    }
    Assert-GeneratedPath $package $artifactsRoot
    Assert-GeneratedPath $stage $tempRoot
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $stage, (Join-Path $stage 'data') | Out-Null
    Copy-Item -LiteralPath $executable -Destination $stage
    $env:VCINSTALLDIR = Join-Path $vs 'VC/'
    # Force refresh after any interrupted deployment; use ASCII paths for Qt tools.
    $mappedPackage = $drive + '/temp/' + $stageName
    Invoke-Checked (Join-Path $config.qtRoot 'bin/windeployqt.exe') @('--release', '--force', '--compiler-runtime', '--dir', $mappedPackage, ($mappedPackage + '/tdlas_app.exe')) 'deploy.log'
    $redist = Get-ChildItem -LiteralPath (Join-Path $vs 'VC/Redist/MSVC') -Directory | Sort-Object Name -Descending | Where-Object { Test-Path (Join-Path $_.FullName 'x64/Microsoft.VC143.CRT/msvcp140.dll') } | Select-Object -First 1
    if (-not $redist) { throw 'MSVC x64 runtime DLLs not found.' }
    Get-ChildItem -LiteralPath (Join-Path $redist.FullName 'x64/Microsoft.VC143.CRT') -Filter '*.dll' | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $stage }
    $redistInstaller = Join-Path $redist.FullName 'vc_redist.x64.exe'
    if (-not (Test-Path -LiteralPath $redistInstaller)) { throw "Missing VC++ runtime installer: $redistInstaller" }
    Copy-Item -LiteralPath $redistInstaller -Destination $stage
    foreach ($dll in @('dxcompiler.dll', 'dxil.dll')) {
        $sdkDll = Join-Path $config.windowsSdkRoot "Redist/D3D/x64/$dll"
        if (-not (Test-Path -LiteralPath $sdkDll)) { throw "Missing Windows SDK runtime: $sdkDll" }
        Copy-Item -LiteralPath $sdkDll -Destination $stage
    }
    Set-Content -LiteralPath (Join-Path $stage 'qt.conf') -Value "[Paths]`nPrefix=." -Encoding ASCII
    # Windows PowerShell 5 may parse a UTF-8 script without BOM using the
    # current ANSI code page. Construct the Chinese source name from code
    # points and keep the deployed file name ASCII-only.
    $guideName = (-join @([char]0x7528, [char]0x6237, [char]0x4F7F, [char]0x7528, [char]0x8BF4, [char]0x660E)) + '.md'
    Copy-Item -LiteralPath (Join-Path $projectDirectory ('docs/' + $guideName)) -Destination (Join-Path $stage 'USER_GUIDE.zh-CN.md')
    Set-Content -LiteralPath (Join-Path $stage 'VERSION.txt') -Value 'TDLAS2 2.0.0 Portable Release (NI-DAQmx enabled)' -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $stage 'PORTABLE_INFO.txt') -Value @(
        'Copy this entire directory; do not copy tdlas_app.exe by itself.',
        'The target PC must be 64-bit Windows with a compatible NI-DAQmx driver installed.',
        'Qt and Microsoft Visual C++ runtime files are included. vc_redist.x64.exe is included for repair/install.',
        'The application does not discover devices, acquire input, drive AO, or open a camera until the user requests it.',
        'Generated data is stored under the data directory unless the user selects another location.'
    ) -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $stage 'data/README.txt') -Value 'Application data, settings, raw runs, exports, images, and recordings are stored below this directory.' -Encoding ASCII
    # Do not report success for truncated/preallocated DLLs left by a failed copy.
    foreach ($runtimeDll in (Get-ChildItem -LiteralPath $stage -Filter '*.dll' -Recurse)) {
        $stream = [IO.File]::OpenRead($runtimeDll.FullName)
        try {
            if ($stream.ReadByte() -ne 77 -or $stream.ReadByte() -ne 90) { throw "Invalid PE header: $($runtimeDll.FullName)" }
        } finally { $stream.Dispose() }
    }
    foreach ($qtDll in (Get-ChildItem -LiteralPath $stage -Filter 'Qt6*.dll')) {
        if ($qtDll.Name -eq 'Qt6Core.dll') { continue } # windeployqt patches its prefix.
        $installed = Join-Path $config.qtRoot ('bin/' + $qtDll.Name)
        $sha = [Security.Cryptography.SHA256]::Create()
        try {
            $originalHash = [Convert]::ToBase64String($sha.ComputeHash([IO.File]::ReadAllBytes($installed)))
            $deployedHash = [Convert]::ToBase64String($sha.ComputeHash([IO.File]::ReadAllBytes($qtDll.FullName)))
            if ($originalHash -ne $deployedHash) { throw "Qt runtime checksum mismatch: $($qtDll.Name)" }
        } finally { $sha.Dispose() }
    }
    $hashLines = Get-ChildItem -LiteralPath $stage -File -Recurse |
        Where-Object Name -ne 'SHA256SUMS.txt' |
        Sort-Object FullName |
        ForEach-Object {
            $packagePrefix = $stage.TrimEnd('\') + '\'
            if (-not $_.FullName.StartsWith($packagePrefix, [StringComparison]::OrdinalIgnoreCase)) { throw "Package inventory escaped root: $($_.FullName)" }
            $relative = $_.FullName.Substring($packagePrefix.Length).Replace('\', '/')
            $fileSha = [Security.Cryptography.SHA256]::Create()
            $fileStream = [IO.File]::OpenRead($_.FullName)
            try { $hash = [BitConverter]::ToString($fileSha.ComputeHash($fileStream)).Replace('-', '').ToLowerInvariant() }
            finally { $fileStream.Dispose(); $fileSha.Dispose() }
            '{0}  {1}' -f $hash, $relative
        }
    Set-Content -LiteralPath (Join-Path $stage 'SHA256SUMS.txt') -Value $hashLines -Encoding ASCII

    if (Test-Path -LiteralPath $package) { Remove-Item -LiteralPath $package -Recurse -Force }
    Move-Item -LiteralPath $stage -Destination $package
    $zipPath = Join-Path $projectDirectory ('artifacts/' + $packageName + '.zip')
    Assert-GeneratedPath $zipPath $artifactsRoot
    if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }
    Compress-Archive -LiteralPath $package -DestinationPath $zipPath -CompressionLevel Optimal
    $zipSha = [Security.Cryptography.SHA256]::Create()
    $zipStream = [IO.File]::OpenRead($zipPath)
    try { $zipHash = [BitConverter]::ToString($zipSha.ComputeHash($zipStream)).Replace('-', '').ToLowerInvariant() }
    finally { $zipStream.Dispose(); $zipSha.Dispose() }
    Set-Content -LiteralPath ($zipPath + '.sha256.txt') -Value ($zipHash + '  ' + [IO.Path]::GetFileName($zipPath)) -Encoding ASCII

    # Remove only the two superseded generated package directories after the
    # unified package and its ZIP have both been created successfully.
    foreach ($legacyName in @('release-package', 'release-package-daqmx')) {
        $legacyPackage = Join-Path $projectDirectory ('artifacts/' + $legacyName)
        Assert-GeneratedPath $legacyPackage $artifactsRoot
        if (Test-Path -LiteralPath $legacyPackage) { Remove-Item -LiteralPath $legacyPackage -Recurse -Force }
    }
}
[ordered]@{Configuration=$Configuration; DaqmxEnabled=$true; TestsPassed=$true; Source=$projectDirectory; Mapping=$drive; Executable=$executable; Package=$package; Zip=$zipPath; Logs=$logs} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $logs 'build-result.json') -Encoding UTF8
Write-Host "Build and tests passed. Executable: $executable"
