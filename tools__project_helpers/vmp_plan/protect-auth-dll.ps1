param(
    [Parameter(Mandatory = $true)]
    [string]$VMProtectPath,

    [Parameter(Mandatory = $false)]
    [string]$OutputDir = "C:\NB\dist__vmp_auth_dll",

    [Parameter(Mandatory = $false)]
    [switch]$SkipSmoke
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$inputDll = Join-Path $root 'build__auth_dll_cache\NoteBotAuth.dll'
$projectFile = Join-Path $root 'build__auth_dll_cache\NoteBotAuth.dll.vmp'
$outputDll = Join-Path $OutputDir 'NoteBotAuth.dll'
$applyPlan = Join-Path $PSScriptRoot 'apply-vmp-plan.ps1'

function Resolve-VMProtectCon {
    param([string]$Path)

    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        if ([System.IO.Path]::GetFileName($Path) -ieq 'VMProtect_Con.exe') {
            return (Resolve-Path -LiteralPath $Path).Path
        }
        throw "Need VMProtect_Con.exe, got: $Path"
    }

    if (Test-Path -LiteralPath $Path -PathType Container) {
        $direct = Join-Path $Path 'VMProtect_Con.exe'
        if (Test-Path -LiteralPath $direct -PathType Leaf) {
            return (Resolve-Path -LiteralPath $direct).Path
        }
        $found = Get-ChildItem -LiteralPath $Path -Recurse -Filter 'VMProtect_Con.exe' -File -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    throw "VMProtect_Con.exe not found under: $Path"
}

function Get-Dumpbin {
    $cmd = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $vsRoot = Join-Path $env:ProgramFiles 'Microsoft Visual Studio'
    if (Test-Path -LiteralPath $vsRoot) {
        $found = Get-ChildItem -LiteralPath $vsRoot -Recurse -Filter dumpbin.exe -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -match '\\bin\\Hostx64\\x64\\dumpbin\.exe$' } |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    throw 'dumpbin.exe not found'
}

function Invoke-Smoke {
    param([string]$DllPath)

    $code = @'
using System;
using System.Runtime.InteropServices;

public static class NbAuthVmpSmoke {
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    static extern IntPtr LoadLibraryW(string path);

    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Ansi)]
    static extern IntPtr GetProcAddress(IntPtr module, string procName);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int NbGetVersion(IntPtr buf, int size);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int NbGetSimpleInt();

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate int NbCall([MarshalAs(UnmanagedType.LPStr)] string action,
                        [MarshalAs(UnmanagedType.LPStr)] string json,
                        IntPtr outJson,
                        int outSize);

    public static string Run(string path) {
        IntPtr h = LoadLibraryW(path);
        if (h == IntPtr.Zero) throw new Exception("LoadLibraryW failed: " + Marshal.GetLastWin32Error());

        IntPtr pVer = GetProcAddress(h, "nb_get_version");
        IntPtr pProto = GetProcAddress(h, "nb_get_protocol_version");
        IntPtr pAbi = GetProcAddress(h, "nb_get_abi_version");
        IntPtr pCall = GetProcAddress(h, "nb_call");
        if (pVer == IntPtr.Zero || pProto == IntPtr.Zero || pAbi == IntPtr.Zero || pCall == IntPtr.Zero) {
            throw new Exception("GetProcAddress failed");
        }

        var getVersion = (NbGetVersion)Marshal.GetDelegateForFunctionPointer(pVer, typeof(NbGetVersion));
        var getProtocolVersion = (NbGetSimpleInt)Marshal.GetDelegateForFunctionPointer(pProto, typeof(NbGetSimpleInt));
        var getAbiVersion = (NbGetSimpleInt)Marshal.GetDelegateForFunctionPointer(pAbi, typeof(NbGetSimpleInt));
        var nbCall = (NbCall)Marshal.GetDelegateForFunctionPointer(pCall, typeof(NbCall));

        IntPtr verBuf = Marshal.AllocHGlobal(64);
        IntPtr outBuf = Marshal.AllocHGlobal(4096);
        try {
            int verLen = getVersion(verBuf, 64);
            string ver = Marshal.PtrToStringAnsi(verBuf) ?? "";
            int proto = getProtocolVersion();
            int abi = getAbiVersion();
            int pingRc = nbCall("ping", "{}", outBuf, 4096);
            string ping = Marshal.PtrToStringAnsi(outBuf) ?? "";
            int statusRc = nbCall("get_status_snapshot", "{}", outBuf, 4096);
            string status = Marshal.PtrToStringAnsi(outBuf) ?? "";
            int protoInfoRc = nbCall("get_protocol_info", "{}", outBuf, 4096);
            string protoInfo = Marshal.PtrToStringAnsi(outBuf) ?? "";

            if (proto != 3) throw new Exception("unexpected protocol version: " + proto);
            if (abi != 1) throw new Exception("unexpected abi version: " + abi);
            if (pingRc <= 0 || !ping.Contains("\"message\":\"pong\"")) {
                throw new Exception("ping smoke failed: " + ping);
            }
            if (statusRc <= 0 || !status.Contains("\"status_snapshot\"")) {
                throw new Exception("status snapshot smoke failed: " + status);
            }
            if (protoInfoRc <= 0 || !protoInfo.Contains("\"protocol_version\":3") || !protoInfo.Contains("\"abi_version\":1")) {
                throw new Exception("protocol info smoke failed: " + protoInfo);
            }

            return "version_rc=" + verLen +
                   "\nversion=" + ver +
                   "\nprotocol=" + proto +
                   "\nabi=" + abi +
                   "\nping_rc=" + pingRc +
                   "\nping=" + ping +
                   "\nstatus_rc=" + statusRc +
                   "\nstatus=" + status +
                   "\nprotocol_info_rc=" + protoInfoRc +
                   "\nprotocol_info=" + protoInfo;
        }
        finally {
            Marshal.FreeHGlobal(verBuf);
            Marshal.FreeHGlobal(outBuf);
        }
    }
}
'@

    Add-Type -TypeDefinition $code
    [NbAuthVmpSmoke]::Run($DllPath)
}

if (-not (Test-Path -LiteralPath $inputDll -PathType Leaf)) {
    throw "Input DLL not found: $inputDll"
}

$vmpCon = Resolve-VMProtectCon -Path $VMProtectPath
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Remove-Item -LiteralPath $outputDll -Force -ErrorAction SilentlyContinue

& powershell -NoProfile -ExecutionPolicy Bypass -File $applyPlan -Target Auth

if (-not (Test-Path -LiteralPath $projectFile -PathType Leaf)) {
    throw "VMP project not found: $projectFile"
}

& $vmpCon $inputDll $outputDll -pf $projectFile
if ($LASTEXITCODE -ne 0) {
    throw "VMProtect_Con failed with exit code $LASTEXITCODE"
}

if (-not (Test-Path -LiteralPath $outputDll -PathType Leaf)) {
    throw "Protected DLL was not created: $outputDll"
}

$dumpbin = Get-Dumpbin
$exports = & $dumpbin /exports $outputDll
$required = @(
    'nb_activate',
    'nb_call',
    'nb_check_update',
    'nb_diagnose',
    'nb_download',
    'nb_download_secure',
    'nb_get_abi_version',
    'nb_get_protocol_version',
    'nb_get_version',
    'nb_init',
    'nb_inject',
    'nb_inject_async',
    'nb_verify'
)
foreach ($name in $required) {
    if (-not ($exports | Select-String -SimpleMatch $name -Quiet)) {
        throw "Missing export after protection: $name"
    }
}

if (-not $SkipSmoke) {
    Invoke-Smoke -DllPath $outputDll
}

$item = Get-Item -LiteralPath $outputDll
$sha256 = Get-FileHash -LiteralPath $outputDll -Algorithm SHA256
$md5 = Get-FileHash -LiteralPath $outputDll -Algorithm MD5

[pscustomobject]@{
    Output = $outputDll
    Size = $item.Length
    MD5 = $md5.Hash.ToLowerInvariant()
    SHA256 = $sha256.Hash.ToLowerInvariant()
}
