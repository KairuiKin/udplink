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

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $RunId) {
    $RunId = Get-Date -Format 'yyyyMMdd-HHmmss'
}

Push-Location $repoRoot
try {
    $runDir = Join-Path $repoRoot (Join-Path 'logs\board-runs' $RunId)
    python scripts/init_board_run.py --run-id $RunId --board $Board --shield $Shield --platformio-env $PlatformIoEnv | Out-Null

    if (-not (Test-Path 'build')) {
        cmake -S . -B build -DRUDP_BUILD_EXAMPLES=ON -DRUDP_BUILD_TESTS=ON
    }
    cmake --build build --config Release --target rudp_example_udp_peer

    $peerExe = Join-Path $repoRoot 'build\Release\rudp_example_udp_peer.exe'
    $peerLog = Join-Path $runDir 'host-peer.log'
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

    $summaryPath = Join-Path $runDir 'run-summary.txt'
    if (Test-Path $summaryPath) {
        $summary = Get-Content $summaryPath -Raw
        $summary = $summary.Replace('Host OS:', "Host OS: Windows")
        $summary = $summary.Replace('Host peer command:', "Host peer command: $peerCmd")
        $summary = $summary.Replace('Board IP / port:', "Board IP / port: $BoardIp / $BoardPort")
        $summary = $summary.Replace('Host IP / port:', "Host IP / port: $HostIp / $HostPort")
        Set-Content $summaryPath $summary -Encoding utf8
    }

    $serialPorts = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)

    Write-Host "Run directory: $runDir"
    Write-Host "Host peer log: $peerLog"
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
    Write-Host '1. Start the host peer command in one PowerShell window.'
    Write-Host '2. Run the build and upload commands in another PowerShell window.'
    Write-Host '3. Open the monitor command and paste key serial lines into the run directory notes.'
}
finally {
    Pop-Location
}
