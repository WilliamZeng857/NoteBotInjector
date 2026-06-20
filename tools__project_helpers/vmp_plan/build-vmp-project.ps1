param(
    [Parameter(Mandatory = $false)]
    [string]$TemplateProjectPath,

    [Parameter(Mandatory = $true)]
    [string]$SelectionPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputProjectPath,

    [Parameter(Mandatory = $false)]
    [string]$InputFileName
)

$ErrorActionPreference = 'Stop'

function Get-UndecoratedName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Symbol
    )

    if (-not ('DbgHelpUndecorator' -as [type])) {
        Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class DbgHelpUndecorator {
    [DllImport("dbghelp.dll", CharSet=CharSet.Ansi, SetLastError=true)]
    public static extern uint UnDecorateSymbolName(
        string name,
        StringBuilder output,
        uint maxStringLength,
        uint flags);
}
"@
    }

    $sb = New-Object System.Text.StringBuilder 4096
    # VMProtect project samples use undecorated C++ names without return
    # types/calling-convention, while keeping class/struct/const in args.
    $flags = [uint32](0x0002 -bor 0x0004 -bor 0x0080)
    [void][DbgHelpUndecorator]::UnDecorateSymbolName($Symbol, $sb, $sb.Capacity, $flags)
    $name = $sb.ToString()

    if (-not $name) {
        return $Symbol
    }

    return $name.Trim()
}

if (-not (Test-Path -LiteralPath $SelectionPath)) {
    throw "Selection file not found: $SelectionPath"
}

$selected = New-Object System.Collections.Generic.List[object]
Get-Content -LiteralPath $SelectionPath | ForEach-Object {
    if ($_ -match '^(super|ultra|virtualize|mutation)\t([^\t]+)\t([^\t]+)\t') {
        $selected.Add([pscustomobject]@{
            Category = $Matches[1]
            Rva      = $Matches[2]
            Symbol   = $Matches[3]
        })
    }
}

if ($selected.Count -eq 0) {
    throw "No procedures found in $SelectionPath"
}

if ($TemplateProjectPath -and (Test-Path -LiteralPath $TemplateProjectPath)) {
    $xml = [xml](Get-Content -LiteralPath $TemplateProjectPath -Raw)
}
else {
    $xml = [xml]@'
<?xml version="1.0" encoding="UTF-8"?>
<Document Version="2">
    <Protection InputFileName="" Options="332544" VMCodeSectionName=".???" VMComplexity="20" />
</Document>
'@
}

$nsMgr = New-Object System.Xml.XmlNamespaceManager($xml.NameTable)

$protection = $xml.DocumentElement.Protection
if (-not $protection) {
    throw "Invalid project file: missing <Protection>"
}

if ($InputFileName) {
    [void]$protection.SetAttribute('InputFileName', $InputFileName)
}

$existingProcedures = $protection.SelectSingleNode('Procedures')
if ($existingProcedures) {
    [void]$protection.RemoveChild($existingProcedures)
}

$procedures = $xml.CreateElement('Procedures')

foreach ($item in $selected) {
    $proc = $xml.CreateElement('Procedure')
    $displayName = Get-UndecoratedName -Symbol $item.Symbol
    [void]$proc.SetAttribute('MapAddress', $displayName)
    [void]$proc.SetAttribute('IncludedInCompilation', 'true')

    # Keep the XML explicit and simple: select the function now, tune the
    # compilation style later inside VMProtect if needed.
    [void]$proc.SetAttribute('Options', '0')
    switch ($item.Category) {
        'super' {
            [void]$proc.SetAttribute('CompilationType', '2')
        }
        'ultra' {
            [void]$proc.SetAttribute('CompilationType', '2')
        }
        'virtualize' {
            [void]$proc.SetAttribute('CompilationType', '1')
        }
        'mutation' {
            [void]$proc.SetAttribute('CompilationType', '0')
        }
        default {
            [void]$proc.SetAttribute('CompilationType', '0')
        }
    }
    [void]$procedures.AppendChild($proc)
}

[void]$protection.AppendChild($procedures)

$dir = Split-Path -Parent $OutputProjectPath
if ($dir -and -not (Test-Path -LiteralPath $dir)) {
    New-Item -ItemType Directory -Path $dir | Out-Null
}

$settings = New-Object System.Xml.XmlWriterSettings
$settings.Indent = $true
$settings.IndentChars = '    '
$settings.NewLineChars = "`r`n"
$settings.NewLineHandling = 'Replace'
$settings.Encoding = New-Object System.Text.UTF8Encoding($false)

$writer = [System.Xml.XmlWriter]::Create($OutputProjectPath, $settings)
try {
    $xml.Save($writer)
}
finally {
    $writer.Close()
}

Write-Host "Wrote $OutputProjectPath"
Write-Host "Added $($selected.Count) procedures"
