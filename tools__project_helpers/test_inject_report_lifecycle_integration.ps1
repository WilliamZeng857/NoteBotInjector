param(
    [string]$DllPath = 'C:\NB\build__auth_dll_cache\NoteBotAuth.dll',
    [int]$TargetPid = 1234
)

$ErrorActionPreference = 'Stop'

$code = @"
using System;
using System.Runtime.InteropServices;

public static class NbLifecycleNative {
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
  public static extern IntPtr LoadLibraryW(string lpFileName);
  [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
  public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);
  [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int NbInit();
  [UnmanagedFunctionPointer(CallingConvention.Cdecl)] public delegate int NbCall(IntPtr action, IntPtr json, IntPtr output, int outputSize);
}
"@

Add-Type $code

function New-Utf8Ptr([string]$text) {
    $bytes = [Text.Encoding]::UTF8.GetBytes($text + [char]0)
    $ptr = [Runtime.InteropServices.Marshal]::AllocHGlobal($bytes.Length)
    [Runtime.InteropServices.Marshal]::Copy($bytes, 0, $ptr, $bytes.Length)
    return $ptr
}

function Get-Delegate([IntPtr]$module, [string]$name, [Type]$type) {
    $proc = [NbLifecycleNative]::GetProcAddress($module, $name)
    if ($proc -eq [IntPtr]::Zero) { throw "missing export $name" }
    return [Runtime.InteropServices.Marshal]::GetDelegateForFunctionPointer($proc, $type)
}

function Invoke-NbCall([string]$action, [hashtable]$body = @{}) {
    $json = if ($body.Count -eq 0) { '{}' } else { $body | ConvertTo-Json -Compress -Depth 8 }
    $actionPtr = New-Utf8Ptr $action
    $jsonPtr = New-Utf8Ptr $json
    $outSize = 262144
    $outPtr = [Runtime.InteropServices.Marshal]::AllocHGlobal($outSize)
    try {
        $rcLen = $script:nbCall.Invoke($actionPtr, $jsonPtr, $outPtr, $outSize)
        $bytes = New-Object byte[] $outSize
        [Runtime.InteropServices.Marshal]::Copy($outPtr, $bytes, 0, $outSize)
        $length = [Array]::IndexOf($bytes, [byte]0)
        if ($length -lt 0) { $length = $outSize }
        $raw = [Text.Encoding]::UTF8.GetString($bytes, 0, $length)
        $obj = $raw | ConvertFrom-Json
        [pscustomobject]@{ action = $action; rcLen = $rcLen; raw = $raw; obj = $obj }
    } finally {
        [Runtime.InteropServices.Marshal]::FreeHGlobal($actionPtr)
        [Runtime.InteropServices.Marshal]::FreeHGlobal($jsonPtr)
        [Runtime.InteropServices.Marshal]::FreeHGlobal($outPtr)
    }
}

function Require([bool]$condition, [string]$message) {
    if (!$condition) { throw "FAILED: $message" }
}

Require (Test-Path -LiteralPath $DllPath) "DLL not found: $DllPath"
$module = [NbLifecycleNative]::LoadLibraryW($DllPath)
Require ($module -ne [IntPtr]::Zero) "LoadLibrary failed: $DllPath"
$nbInit = Get-Delegate $module 'nb_init' ([NbLifecycleNative+NbInit])
$script:nbCall = Get-Delegate $module 'nb_call' ([NbLifecycleNative+NbCall])
Require ($nbInit.Invoke() -eq 0) 'nb_init failed'

$status = Invoke-NbCall 'get_status_snapshot'
Require ($status.obj.rc -eq 0 -and $status.obj.data.status_snapshot.active -eq $true) 'active V3 license is required'
$policy = Invoke-NbCall 'dll_policy_v3'
Require ($policy.obj.rc -eq 0) 'dll_policy_v3 failed'

$ticketBody = @{
    target_pid = $TargetPid
    dll_name = [string]$policy.obj.data.dll_name
    dll_sha256 = [string]$policy.obj.data.dll_sha256
    exe_version = 'integration-lifecycle-test'
}
$firstIssue = Invoke-NbCall 'issue_inject_ticket_v3' $ticketBody
Require ($firstIssue.obj.rc -eq 0) "first ticket issue failed: $($firstIssue.obj.message)"

try {
    $lockedIssue = Invoke-NbCall 'issue_inject_ticket_v3' $ticketBody
    Require ($lockedIssue.obj.rc -eq 1007) "duplicate issue did not return 1007: $($lockedIssue.raw)"
    Require ($lockedIssue.obj.message -match 'inject_session_locked') "duplicate issue lost inject_session_locked: $($lockedIssue.raw)"

    $finalize = Invoke-NbCall 'finalize_inject_failure_v3' @{
        status = 'inject_failed'
        reason = 'inject_failed:integration_lifecycle_probe'
    }
    Require ($finalize.obj.rc -eq 0) "finalize failed: $($finalize.raw)"

    $report = Invoke-NbCall 'report_inject_result_v3'
    Require ($report.obj.rc -eq 0) "report failed: $($report.raw)"
    Require ($report.obj.data.accepted -eq $true) "report was not accepted: $($report.raw)"
} finally {
    $cleanup = Invoke-NbCall 'report_inject_result_v3'
    if ($cleanup.obj.rc -eq 0 -and $cleanup.obj.data.accepted -ne $true) {
        throw "FAILED: cleanup report was not accepted: $($cleanup.raw)"
    }
}

Write-Host 'PASS: live issue-lock-finalize-report lifecycle'
