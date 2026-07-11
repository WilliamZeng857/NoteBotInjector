param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Auth', 'Injector', 'Updater', 'Model')]
    [string]$Target,

    [switch]$Force
)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..\..')).Path
switch ($Target) {
    'Auth' { $projectPath = Join-Path $root 'build__auth_dll_cache\NoteBotAuth.dll.vmp'; $binaryName = 'NoteBotAuth.dll'; $selectorPath = Join-Path $root 'tools__project_helpers\vmp_plan\generated\NoteBotAuth.apply_protection.lua' }
    'Injector' { $projectPath = Join-Path $root 'build__injector_exe_cache\NoteBotInjector.exe.vmp'; $binaryName = 'NoteBotInjector.exe' }
    'Updater' { $projectPath = Join-Path $root 'build__injector_exe_cache\NoteBotUpdater.exe.vmp'; $binaryName = 'NoteBotUpdater.exe' }
    'Model' { $projectPath = Join-Path $root 'dist__model_runtime_artifacts\NoteBotModel.dll.vmp'; $binaryName = 'NoteBotModel.dll' }
}

if (-not $Force -and (Get-Process -Name 'VMProtect*' -ErrorAction SilentlyContinue)) {
    throw 'Close VMProtect before restoring a project file.'
}

$timestamp = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmss')
$backupPath = "$projectPath.broken-$timestamp.bak"
if (Test-Path -LiteralPath $projectPath) {
    Copy-Item -LiteralPath $projectPath -Destination $backupPath -ErrorAction Stop
}

$selectorLua = "dofile([[$selectorPath]])"
$content = "<?xml version=`"1.0`" encoding=`"UTF-8`" ?>`r`n<Document Version=`"2`">`r`n    <Protection InputFileName=`"$binaryName`" Options=`"332544`" VMCodeSectionName=`".???`" VMComplexity=`"20`">`r`n        <Procedures></Procedures>`r`n    </Protection>`r`n    <Script><![CDATA[$selectorLua]]></Script>`r`n</Document>`r`n"
[System.IO.File]::WriteAllText($projectPath, $content, [System.Text.UTF8Encoding]::new($false))

Write-Host "Restored empty VMP project: $projectPath"
Write-Host "Preserved prior project: $backupPath"
