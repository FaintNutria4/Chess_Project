param(
    [switch]$v,
    [string]$ServerPath = (Join-Path $PSScriptRoot "..\chess_server.exe"),
    [string]$Address = "127.0.0.1",
    [int]$Port = 8080,
    [int]$ConnectTimeoutMs = 5000,
    [int]$ReceiveTimeoutMs = 2000
)

$ErrorActionPreference = "Stop"

$script:checks = 0
$script:failures = 0

function Info([string]$msg) {
    if ($v) { Write-Host $msg }
}

function Check([string]$label, [bool]$cond) {
    $script:checks++
    if (-not $cond) {
        $script:failures++
        Write-Host "FAIL: $label"
    }
}

function Connect-WebSocket([string]$address, [int]$port, [int]$timeoutMs) {
    $ws = New-Object System.Net.WebSockets.ClientWebSocket
    $uri = New-Object System.Uri("ws://${address}:${port}/")
    $task = $ws.ConnectAsync($uri, [System.Threading.CancellationToken]::None)
    if (-not $task.Wait($timeoutMs)) {
        $ws.Dispose()
        return $null
    }
    if ($ws.State -ne [System.Net.WebSockets.WebSocketState]::Open) {
        $ws.Dispose()
        return $null
    }
    return $ws
}

function Send-Message($ws, [string]$text) {
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($text)
    $segment = New-Object System.ArraySegment[byte] -ArgumentList @(,$bytes)
    $task = $ws.SendAsync($segment, [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [System.Threading.CancellationToken]::None)
    $task.Wait($ReceiveTimeoutMs) | Out-Null
}

function Read-Message($ws, [int]$timeoutMs) {
    $buffer = New-Object byte[] 4096
    $segment = New-Object System.ArraySegment[byte] -ArgumentList @(,$buffer)
    $task = $ws.ReceiveAsync($segment, [System.Threading.CancellationToken]::None)
    if (-not $task.Wait($timeoutMs)) { return "" }
    if ($task.IsFaulted -or $task.Result.MessageType -eq [System.Net.WebSockets.WebSocketMessageType]::Close) { return "" }
    return [System.Text.Encoding]::UTF8.GetString($buffer, 0, $task.Result.Count)
}

if (-not (Test-Path $ServerPath)) {
    Write-Host "FAIL: $ServerPath not found - run 'scons' in server/ first"
    exit 1
}

$occupied = $false
$probePort = New-Object System.Net.Sockets.TcpClient
try {
    $probePort.Connect($Address, $Port)
    $occupied = $true
} catch {
} finally {
    $probePort.Close()
}
if ($occupied) {
    Write-Host "FAIL: port $Port already in use - stop the running chess_server.exe first"
    exit 1
}

$stdoutFile = [System.IO.Path]::GetTempFileName()
$stderrFile = [System.IO.Path]::GetTempFileName()

Info ("starting " + (Split-Path $ServerPath -Leaf) + " (port $Port)")
$sw = [System.Diagnostics.Stopwatch]::StartNew()
$server = Start-Process -FilePath $ServerPath -WorkingDirectory (Split-Path $ServerPath -Parent) -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile
$sockets = @()

try {
    $ws1 = $null
    $deadline = [DateTime]::UtcNow.AddMilliseconds($ConnectTimeoutMs)
    while (-not $ws1 -and [DateTime]::UtcNow -lt $deadline) {
        if ($server.HasExited) { break }
        $ws1 = Connect-WebSocket $Address $Port 500
    }
    $sw.Stop()

    if ($ws1) {
        Info ("listening after " + [int]$sw.Elapsed.TotalMilliseconds + "ms")
    }
    Check "server process alive after startup" (-not $server.HasExited)
    Check "websocket connect within ${ConnectTimeoutMs}ms" ($null -ne $ws1)

    if ($server.HasExited) {
        $outText = ""
        if (Test-Path $stdoutFile) { $outText = (Get-Content $stdoutFile -Raw -ErrorAction SilentlyContinue) }
        $errText = ""
        if (Test-Path $stderrFile) { $errText = (Get-Content $stderrFile -Raw -ErrorAction SilentlyContinue) }
        Write-Host "server exited with code $($server.ExitCode)"
        if ($outText) { Write-Host $outText.TrimEnd() }
        if ($errText) { Write-Host $errText.TrimEnd() }
    }

    if ($ws1) {
        $sockets += $ws1
        Info ("client #1 connected to " + $Address + ":" + $Port)

        Send-Message $ws1 "e2e4"
        $reply = Read-Message $ws1 $ReceiveTimeoutMs
        Info ('send "e2e4" -> got "' + $reply + '"')
        Check "round-trip 1: e2e4 -> ACK: e2e4" ($reply -ceq "ACK: e2e4")

        Send-Message $ws1 "d7d5"
        $reply = Read-Message $ws1 $ReceiveTimeoutMs
        Info ('send "d7d5" -> got "' + $reply + '"')
        Check "round-trip 2 on same socket: d7d5" ($reply -ceq "ACK: d7d5")

        $ws2 = Connect-WebSocket $Address $Port $ConnectTimeoutMs
        if ($ws2) { $sockets += $ws2 }
        Info "client #2 connected (concurrent)"

        Send-Message $ws2 "g1f3"
        $reply = Read-Message $ws2 $ReceiveTimeoutMs
        Info ('send "g1f3" -> got "' + $reply + '"')
        Check "second concurrent client gets ACK" ($reply -ceq "ACK: g1f3")

        Info "closing both clients"
        foreach ($ws in $sockets) {
            try { $ws.Dispose() } catch {}
        }
        Start-Sleep -Milliseconds 100
        Check "server survives client disconnect" (-not $server.HasExited)
        Info "server survived disconnect"
    }
} finally {
    foreach ($ws in $sockets) {
        try { $ws.Dispose() } catch {}
    }
    if (-not $server.HasExited) {
        $server.Kill()
        $server.WaitForExit(3000) | Out-Null
    }
    Remove-Item $stdoutFile, $stderrFile -ErrorAction SilentlyContinue
}

Write-Host "$($script:checks) checks, $($script:failures) failures"
if ($script:failures -eq 0) { exit 0 }
exit 1
