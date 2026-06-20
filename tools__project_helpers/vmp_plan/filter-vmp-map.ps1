param(
    [Parameter(Mandatory = $true)]
    [string]$MapPath,

    [Parameter(Mandatory = $true)]
    [string]$SelectionPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $MapPath)) {
    throw "Map file not found: $MapPath"
}
if (-not (Test-Path -LiteralPath $SelectionPath)) {
    throw "Selection file not found: $SelectionPath"
}

$symbols = New-Object 'System.Collections.Generic.HashSet[string]'
Get-Content -LiteralPath $SelectionPath | ForEach-Object {
    if ($_ -match '^(super|ultra|virtualize|mutation)\t') {
        $parts = $_ -split "`t"
        if ($parts.Count -ge 3) {
            [void]$symbols.Add($parts[2])
        }
    }
}

if ($symbols.Count -eq 0) {
    throw "No symbols were loaded from $SelectionPath"
}

$lines = Get-Content -LiteralPath $MapPath
$out = New-Object System.Collections.Generic.List[string]
$inPublics = $false
$kept = 0

foreach ($line in $lines) {
    if (-not $inPublics) {
        $out.Add($line)
        if ($line -match '^\s*Address\s+Publics by Value') {
            $inPublics = $true
        }
        continue
    }

    if ($line -match '^\s*([0-9A-Fa-f]{4}:[0-9A-Fa-f]{8})\s+(\S+)\s+([0-9A-Fa-f]{16})\s+(?:f\s+)?(.+?\.obj)\s*$') {
        $seg = $Matches[1]
        $sym = $Matches[2]
        $rva = $Matches[3]
        $obj = $Matches[4]

        if ($seg.StartsWith('0001:') -and $symbols.Contains($sym)) {
            $out.Add($line)
            $kept++
        }
    }
}

$dir = Split-Path -Parent $OutputPath
if ($dir -and -not (Test-Path -LiteralPath $dir)) {
    New-Item -ItemType Directory -Path $dir | Out-Null
}

$out | Set-Content -LiteralPath $OutputPath -Encoding ASCII
Write-Host "Wrote $OutputPath"
Write-Host "Kept $kept symbols from $($symbols.Count) selections"
