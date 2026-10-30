param([ValidateSet('Debug','Release')][string]$Configuration = 'Debug', [ValidateSet('3','4','5','6','6.5','7','8','9','10')][string]$Step = '3')
$ErrorActionPreference = 'Stop'
$projectDirectory = Split-Path $PSScriptRoot -Parent
$config = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'local-env.json') -Raw | ConvertFrom-Json
$variant = $Configuration.ToLowerInvariant() + '-daqmx'
$logs = Join-Path $projectDirectory ('artifacts/' + $variant + '/ui-native-step' + $Step)
New-Item -ItemType Directory -Force -Path $logs | Out-Null
$env:PATH = (Join-Path $config.qtRoot 'bin') + ';' + $env:PATH
$env:QT_QPA_PLATFORM = 'windows'
$env:TDLAS_TEST_ARTIFACTS = $logs
$env:TEMP = Join-Path $projectDirectory 'temp'; $env:TMP = $env:TEMP
$buildVariant = 'msvc-daqmx'
$executable = Join-Path $projectDirectory "build/$buildVariant/$Configuration/ui_test.exe"
$report = Join-Path $logs 'test.xml'
# The executable uses only synthetic fixtures; step 9 does not enumerate or open a real camera.
$cases = @('nativePages', 'fitLegendAndThemePersistence', 'commonResultsAndNullBreaks')
if ($Step -eq '4') { $cases = @('nativePages', 'simulationControlsAndConfirmations', 'unpaintedDataRequiresConfirmation', 'stopDuringStartingFromUi', 'simulationAndSafeClose') }
if ($Step -eq '5') { $cases = @('nativePages', 'slowProcessingCloseRemainsResponsive', 'simulationAndSafeClose') }
if ($Step -eq '6') { $cases = @('nativePages', 'algorithmMigrationIsNotMeasurement', 'simulationAndSafeClose') }
if ($Step -eq '6.5') { $cases = @('nativePages', 'loginAndRoleWorkspaces', 'simulationAndSafeClose') }
if ($Step -eq '7') { $cases = @('nativePages', 'chartInteractionDisablesAndRestoresFollow', 'commonResultsAndNullBreaks', 'simulationAndSafeClose') }
if ($Step -eq '8') { $cases = @('nativePages', 'asynchronousDaqDiscoveryAndCapabilities', 'simulationAndSafeClose') }
if ($Step -eq '9') { $cases = @('nativePages', 'cameraControlsDoNotAutoConnect', 'simulationAndSafeClose') }
if ($Step -eq '10') { $cases = @(
    'nativePages',
    'finalAcceptanceChecklist',
    'keyboardScrollAndValidation',
    'fitLegendAndThemePersistence',
    'chartInteractionDisablesAndRestoresFollow',
    'commonResultsAndNullBreaks',
    'cameraControlsDoNotAutoConnect',
    'simulationAndSafeClose'
) }
$process = Start-Process -FilePath $executable -ArgumentList ($cases + @('-o', ('"' + $report + ',junitxml"'))) -WindowStyle Hidden -PassThru
if (-not $process.WaitForExit($(if ($Step -eq '10') { 90000 } else { 45000 }))) {
    Stop-Process -Id $process.Id
    throw 'Native UI fixture test timed out; stopped this hardware-free test process.'
}
$process.Refresh()
if ($process.ExitCode -ne 0) { throw "Native UI tests failed: $($process.ExitCode); see $report" }
Write-Host "Native UI checks passed: $logs"
