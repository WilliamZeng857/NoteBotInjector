param(
    [string]$Message = "",
    [string[]]$Path = @(),
    [switch]$All
)

$ErrorActionPreference = "Stop"
if ($args.Count -gt 0) {
    $Path += $args
}

$repoRoot = (& git rev-parse --show-toplevel 2>$null).Trim()
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location -LiteralPath $repoRoot

$status = @(& git status --porcelain)
if ($status.Count -eq 0) {
    Write-Host "No git changes to commit."
    exit 0
}

if ($Path.Count -gt 0) {
    & git add -- $Path
} elseif ($All) {
    & git add -A
} else {
    throw "Refusing to stage implicitly. Pass -All or explicit -Path values."
}

$stagedFiles = @(& git diff --cached --name-only)
if ($stagedFiles.Count -eq 0) {
    Write-Host "No staged changes to commit."
    exit 0
}

function Get-DefaultSubject {
    param([string[]]$Files)

    if ($Files.Count -eq 1) {
        $file = $Files[0]
        if ($file -eq "AGENTS.md") { return "Update agent workflow instructions" }
        if ($file -eq "README.md") { return "Update project README" }
        return "Update $file"
    }

    if ($Files -contains "AGENTS.md") { return "Update agent workflow and helper scripts" }
    if ($Files -contains "README.md") { return "Update project documentation" }
    if ($Files | Where-Object { $_ -like "qml__injector_exe/*" }) { return "Update injector UI" }
    if ($Files | Where-Object { $_ -like "src__auth_dll/*" -or $_ -like "src__injector_exe/*" }) { return "Update injector source" }
    if ($Files | Where-Object { $_ -like "*.bat" -or $_ -like "*.ps1" }) { return "Update project scripts" }

    return "Update project files"
}

$subject = $Message.Trim()
if (-not $subject) {
    $subject = Get-DefaultSubject -Files $stagedFiles
}

$stat = (& git diff --cached --stat -- $stagedFiles) -join [Environment]::NewLine
$body = "Auto-generated from staged diff." + [Environment]::NewLine + [Environment]::NewLine + $stat

& git commit -m $subject -m $body
& git status --short
