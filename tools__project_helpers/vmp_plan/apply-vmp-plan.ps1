param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Injector', 'Auth')]
    [string]$Target
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
$planDir = $PSScriptRoot

switch ($Target) {
    'Injector' {
        $baseName = 'NoteBotInjector'
        $binaryName = 'NoteBotInjector.exe'
        $buildDir = Join-Path $root 'build__injector_exe_cache'
    }
    'Auth' {
        $baseName = 'NoteBotAuth'
        $binaryName = 'NoteBotAuth.dll'
        $buildDir = Join-Path $root 'build__auth_dll_cache'
    }
}

$mapPath = Join-Path $buildDir "$baseName.map"
$selectionPath = Join-Path $planDir "$baseName.protect.tsv"
$effectiveSelectionPath = $selectionPath
$projectPath = Join-Path $buildDir "$binaryName.vmp"
$backupMap = Join-Path $buildDir "$baseName.full.map"

if (-not (Test-Path -LiteralPath $selectionPath)) {
    throw "Missing plan file: $selectionPath"
}

if (-not (Test-Path -LiteralPath $buildDir)) {
    throw "Build directory not found: $buildDir"
}

if (Test-Path -LiteralPath $mapPath) {
    if (-not (Test-Path -LiteralPath $backupMap)) {
        Copy-Item -LiteralPath $mapPath -Destination $backupMap
        Write-Host "Backed up $mapPath -> $backupMap"
    }
    Write-Host "Leaving original map in place: $mapPath"
}
else {
    Write-Host "Map not found: $mapPath"
}

function Resolve-Dumpbin {
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

    return $null
}

function Get-ProtectionEntries {
    param([string]$Path)

    $entries = New-Object System.Collections.Generic.List[object]
    $seen = New-Object 'System.Collections.Generic.HashSet[string]'

    Get-Content -LiteralPath $Path | ForEach-Object {
        $line = $_.Trim()
        if (-not $line -or $line.StartsWith('#') -or $line.StartsWith('category')) {
            return
        }

        $parts = $_ -split "`t"
        if ($parts.Count -lt 4) {
            throw "Invalid protection row: $_"
        }

        $category = $parts[0].Trim().ToLowerInvariant()
        $symbolKey = $parts[1].Trim()
        $module = $parts[2].Trim()
        $note = $parts[3].Trim()

        if ($category -notin @('mutation', 'virtualize', 'ultra', 'super')) {
            throw "Unsupported protection category '$category' in $Path"
        }
        if (-not $symbolKey) {
            throw "Missing symbol_key in $Path"
        }
        if (-not $seen.Add($symbolKey)) {
            throw "Duplicate symbol_key in ${Path}: $symbolKey"
        }

        $entries.Add([pscustomobject]@{
            Category  = $category
            SymbolKey = $symbolKey
            Module    = $module
            Note      = $note
        })
    }

    if ($entries.Count -eq 0) {
        throw "No protection entries found in $Path"
    }

    return $entries
}

function Get-AuthMapCandidates {
    param([string]$Path)

    $items = New-Object System.Collections.Generic.List[object]
    if (-not (Test-Path -LiteralPath $Path)) {
        return $items
    }

    Get-Content -LiteralPath $Path | ForEach-Object {
        if ($_ -match '^\s*([0-9A-Fa-f]{4}:[0-9A-Fa-f]{8})\s+(\S*NBVmp_\S*)\s+([0-9A-Fa-f]{16})\s+(?:f\s+)?(.+?\.obj)\s*$') {
            $items.Add([pscustomobject]@{
                Segment = $Matches[1]
                Symbol  = $Matches[2]
                Address = $Matches[3]
                Object  = $Matches[4]
                Source  = 'map'
            })
        }
    }

    return $items
}

function Get-AuthObjectCandidates {
    param([string]$BuildDir)

    $dumpbin = Resolve-Dumpbin
    if (-not $dumpbin) {
        return @()
    }

    $objectRoot = Join-Path $BuildDir 'CMakeFiles\NoteBotAuth.dir'
    if (-not (Test-Path -LiteralPath $objectRoot)) {
        return @()
    }

    $items = New-Object System.Collections.Generic.List[object]
    Get-ChildItem -LiteralPath $objectRoot -Filter '*.obj' -Recurse | ForEach-Object {
        $objPath = $_.FullName
        $objName = $_.Name
        & $dumpbin /symbols $objPath | ForEach-Object {
            if ($_ -match '\|\s+(\?NBVmp_[^\s]+)') {
                $items.Add([pscustomobject]@{
                    Segment = ''
                    Symbol  = $Matches[1]
                    Address = '0'
                    Object  = $objName
                    Source  = 'obj'
                })
            }
        }
    }

    return $items
}

function Find-MatchingCandidates {
    param(
        [object[]]$Candidates,
        [string]$SymbolKey
    )

    $matches = @($Candidates | Where-Object { $_.Symbol -like "*$SymbolKey*" })
    return $matches
}

function Assert-NoUnknownProtectedSymbols {
    param(
        [object[]]$Candidates,
        [object[]]$Entries
    )

    $unknown = New-Object System.Collections.Generic.List[string]
    foreach ($candidate in $Candidates) {
        $matched = $false
        foreach ($entry in $Entries) {
            if ($candidate.Symbol -like "*$($entry.SymbolKey)*") {
                $matched = $true
                break
            }
        }
        if (-not $matched) {
            $unknown.Add($candidate.Symbol)
        }
    }

    if ($unknown.Count -gt 0) {
        $distinct = $unknown | Sort-Object -Unique
        throw ("Found NBVmp_* symbols missing from protection table:{0}{1}" -f [Environment]::NewLine, ($distinct -join [Environment]::NewLine))
    }
}

$entries = Get-ProtectionEntries -Path $selectionPath

if ($Target -eq 'Auth') {
    $mapCandidates = @(Get-AuthMapCandidates -Path $mapPath)
    $objectCandidates = @()
    if ($mapCandidates.Count -eq 0) {
        $objectCandidates = @(Get-AuthObjectCandidates -BuildDir $buildDir)
    }

    $allCandidates = if ($mapCandidates.Count -gt 0) { $mapCandidates } else { $objectCandidates }
    if ($allCandidates.Count -eq 0) {
        throw "Could not resolve any NBVmp_* symbols from map or object files"
    }

    Assert-NoUnknownProtectedSymbols -Candidates $allCandidates -Entries $entries

    $generatedSelection = Join-Path $buildDir "$baseName.vmp.generated.tsv"
    $rows = New-Object System.Collections.Generic.List[string]
    $rows.Add("# $baseName generated VMP selection")
    $rows.Add("# source_table: $selectionPath")
    if (Test-Path -LiteralPath $mapPath) {
        $rows.Add("# source_map: $mapPath")
    }
    $rows.Add("category`taddress`tsymbol`tobject`tmodule`tnote")

    foreach ($entry in $entries) {
        $matches = @(Find-MatchingCandidates -Candidates $allCandidates -SymbolKey $entry.SymbolKey)
        if ($matches.Count -eq 0) {
            throw "Symbol not found for protection entry: $($entry.SymbolKey)"
        }
        if ($matches.Count -gt 1) {
            $rendered = $matches | ForEach-Object { $_.Symbol }
            throw ("Multiple candidate symbols matched protection entry '{0}': {1}" -f $entry.SymbolKey, ($rendered -join ', '))
        }

        $match = $matches[0]
        $rows.Add(("{0}`t{1}`t{2}`t{3}`t{4}`t{5}" -f `
            $entry.Category, `
            $match.Address, `
            $match.Symbol, `
            $match.Object, `
            $entry.Module, `
            $entry.Note))
    }

    $rows | Set-Content -LiteralPath $generatedSelection -Encoding ASCII
    $effectiveSelectionPath = $generatedSelection
    Write-Host "Generated Auth VMP selection: $generatedSelection"
}

$builder = Join-Path $planDir 'build-vmp-project.ps1'
& $builder `
    -TemplateProjectPath $(if (Test-Path -LiteralPath $projectPath) { $projectPath } else { $null }) `
    -SelectionPath $effectiveSelectionPath `
    -OutputProjectPath $projectPath `
    -InputFileName $binaryName

Write-Host "Updated VMP project: $projectPath"
