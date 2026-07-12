$ErrorActionPreference = 'Stop'

function Require-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Description
    )

    if ($Text -notmatch $Pattern) {
        throw "FAILED: $Description"
    }
}

$root = Split-Path -Parent $PSScriptRoot
$backend = [System.IO.File]::ReadAllText((Join-Path $root 'src__injector_exe\backend.cpp'))
$rpcHeader = [System.IO.File]::ReadAllText((Join-Path $root 'src__auth_dll\src\v3\v3_rpc_client.h'))
$rpcClient = [System.IO.File]::ReadAllText((Join-Path $root 'src__auth_dll\src\v3\v3_rpc_client.cpp'))
$state = [System.IO.File]::ReadAllText((Join-Path $root 'src__auth_dll\src\v3\v3_state.cpp'))

$injectFailureOffset = $backend.IndexOf('if (!injected)')
if ($injectFailureOffset -lt 0) {
    throw 'FAILED: missing local injection failure branch'
}
$injectFailureBody = $backend.Substring($injectFailureOffset, [Math]::Min(1800, $backend.Length - $injectFailureOffset))
Require-Match $injectFailureBody 'finalize_inject_failure_v3' 'local injection failure finalizes its issued ticket'
Require-Match $injectFailureBody 'report_inject_result_v3' 'local injection failure reports its finalized ticket'
Require-Match $backend 'reportAccepted' 'inject success requires an accepted result report'
Require-Match $rpcHeader 'struct RpcIssueTicketResponse \{[\s\S]*?QString message;' 'issue response retains the server message'
Require-Match $rpcHeader 'struct RpcReportResultResponse \{[\s\S]*?QString message;' 'report response retains the server message'
Require-Match $rpcClient 'response\.message = obj\.value\(QStringLiteral\("msg"\)\)\.toString\(\);' 'RPC parser reads the server msg field'
Require-Match $state 'pending_inject_report_v3\.dat' 'pending reports have durable storage'
Require-Match $state 'retryPendingReportLocked' 'pending reports are retried before another ticket'

Write-Host 'PASS: inject report lifecycle contracts'
