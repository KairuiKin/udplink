param(
    [string]$RunId,
    [string]$PlatformIoEnv = 'megaatmega2560_w5100',
    [string]$Board = 'Arduino Mega 2560',
    [string]$Shield = 'W5100-compatible shield',
    [string]$HostIp = '192.168.1.100',
    [int]$HostPort = 8889,
    [string]$BoardIp = '192.168.1.177',
    [int]$BoardPort = 8888,
    [string]$UploadPort
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Quote-PowerShell([string]$Value) {
    return "'" + $Value.Replace("'", "''") + "'"
}

function Write-Utf8NoBom([string]$Path, [string]$Content) {
    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $Content, $utf8NoBom)
}

function Write-HelperScript([string]$Path, [string[]]$BodyLines) {
    $lines = @(
        'Set-StrictMode -Version Latest',
        '$ErrorActionPreference = ''Stop'''
    ) + $BodyLines
    $content = ($lines -join [Environment]::NewLine) + [Environment]::NewLine
    Write-Utf8NoBom -Path $Path -Content $content
}

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $RunId) {
    $RunId = Get-Date -Format 'yyyyMMdd-HHmmss'
}

Push-Location $repoRoot
try {
    $runDir = Join-Path $repoRoot (Join-Path 'logs\board-runs' $RunId)
    python scripts/init_board_run.py --run-id $RunId --board $Board --shield $Shield --platformio-env $PlatformIoEnv | Out-Null

    cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
    cmake --build build --config Release --target rudp_example_udp_peer

    $peerExe = Join-Path $repoRoot 'build\Release\rudp_example_udp_peer.exe'
    $peerLog = Join-Path $runDir 'host-peer.log'
    $exampleDir = Join-Path $repoRoot 'examples\arduino\arduino_udp'

    $peerCmd = "python scripts/run_peer_capture.py --log `"$peerLog`" -- `"$peerExe`" $HostIp $HostPort $BoardIp $BoardPort"

    $pioBuildCmd = "pio run -e $PlatformIoEnv"
    $pioUploadCmd = if ($UploadPort) {
        "pio run -e $PlatformIoEnv -t upload --upload-port $UploadPort"
    } else {
        "pio run -e $PlatformIoEnv -t upload"
    }
    $pioMonitorCmd = if ($UploadPort) {
        "pio device monitor -b 115200 -p $UploadPort"
    } else {
        'pio device monitor -b 115200'
    }

    $hostPeerScript = Join-Path $runDir 'start-host-peer.ps1'
    $boardBuildScript = Join-Path $runDir 'build-board.ps1'
    $boardUploadScript = Join-Path $runDir 'upload-board.ps1'
    $boardMonitorScript = Join-Path $runDir 'monitor-board.ps1'
    $renderReportScript = Join-Path $runDir 'render-report.ps1'
    $validateRunScript = Join-Path $runDir 'validate-run.ps1'
    $finalizeReportScript = Join-Path $runDir 'finalize-report.ps1'
    $fileIssueScript = Join-Path $runDir 'file-issue.ps1'

    Write-HelperScript -Path $hostPeerScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $repoRoot)),
        ('python scripts/run_peer_capture.py --log ' + (Quote-PowerShell $peerLog) + ' -- ' + (Quote-PowerShell $peerExe) + " $HostIp $HostPort $BoardIp $BoardPort")
    )
    Write-HelperScript -Path $boardBuildScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $exampleDir)),
        $pioBuildCmd
    )
    Write-HelperScript -Path $boardUploadScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $exampleDir)),
        $pioUploadCmd
    )
    Write-HelperScript -Path $boardMonitorScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $exampleDir)),
        $pioMonitorCmd
    )
    Write-HelperScript -Path $renderReportScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $repoRoot)),
        ('python scripts/render_board_run_report.py --run-id ' + (Quote-PowerShell $RunId))
    )
    Write-HelperScript -Path $validateRunScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $repoRoot)),
        ('python scripts/validate_board_run.py --run-id ' + (Quote-PowerShell $RunId) + ' --mode preflight')
    )
    Write-HelperScript -Path $finalizeReportScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $repoRoot)),
        ('python scripts/finalize_board_run.py --run-id ' + (Quote-PowerShell $RunId))
    )
    Write-HelperScript -Path $fileIssueScript -BodyLines @(
        ('Set-Location ' + (Quote-PowerShell $repoRoot)),
        ('python scripts/file_board_run_issue.py --run-id ' + (Quote-PowerShell $RunId))
    )

    $summaryPath = Join-Path $runDir 'run-summary.txt'
    if (Test-Path $summaryPath) {
        $summary = Get-Content $summaryPath -Raw
        $summary = $summary.Replace('Host OS:', "Host OS: Windows")
        $summary = $summary.Replace('Host peer command:', "Host peer command: $peerCmd")
        $summary = $summary.Replace('Board IP / port:', "Board IP / port: $BoardIp / $BoardPort")
        $summary = $summary.Replace('Host IP / port:', "Host IP / port: $HostIp / $HostPort")
        $summary = $summary.Replace(
            'Notes:',
            "Host peer helper script: $hostPeerScript`r`nBoard build helper script: $boardBuildScript`r`nBoard upload helper script: $boardUploadScript`r`nBoard monitor helper script: $boardMonitorScript`r`nReport render helper script: $renderReportScript`r`nReport finalize helper script: $finalizeReportScript`r`nIssue filing helper script: $fileIssueScript`r`nNotes:"
        )
        Set-Content $summaryPath $summary -Encoding utf8
    }

    $serialPorts = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)

    Write-Host "Run directory: $runDir"
    Write-Host "Host peer log: $peerLog"
    Write-Host ''
    Write-Host 'Generated helper scripts:'
    Write-Host "- $hostPeerScript"
    Write-Host "- $boardBuildScript"
    Write-Host "- $boardUploadScript"
    Write-Host "- $boardMonitorScript"
    Write-Host "- $renderReportScript"
    Write-Host "- $validateRunScript"
    Write-Host "- $finalizeReportScript"
    Write-Host "- $fileIssueScript"
    Write-Host ''
    if ($serialPorts.Count -gt 0) {
        Write-Host 'Detected serial ports:'
        foreach ($port in $serialPorts) {
            Write-Host "- $port"
        }
    } else {
        Write-Host 'Detected serial ports: none'
    }
    Write-Host ''
    Write-Host 'Host peer command:'
    Write-Host $peerCmd
    Write-Host ''
    Write-Host 'Board build/upload commands:'
    Write-Host "cd examples/arduino/arduino_udp"
    Write-Host $pioBuildCmd
    Write-Host $pioUploadCmd
    Write-Host $pioMonitorCmd
    Write-Host ''
    Write-Host 'Suggested execution order:'
    Write-Host '1. Run start-host-peer.ps1 from the generated run directory in one PowerShell window.'
    Write-Host '2. Run build-board.ps1 and upload-board.ps1 in another PowerShell window.'
    Write-Host '3. Run monitor-board.ps1 and paste key serial lines into the run directory notes.'
    Write-Host '4. Run validate-run.ps1 to confirm the prepared run pack is coherent before hardware execution.'
    Write-Host '5. Run finalize-report.ps1 after updating the notes to render and validate board-bringup-report.md.'
    Write-Host '6. Run file-issue.ps1 after reviewing the report to open the GitHub issue.'
}
finally {
    Pop-Location
}
