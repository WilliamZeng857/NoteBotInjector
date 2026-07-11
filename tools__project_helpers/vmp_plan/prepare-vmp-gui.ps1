param()

$ErrorActionPreference = 'Stop'

$apply = Join-Path $PSScriptRoot 'apply-vmp-plan.ps1'
$verify = Join-Path $PSScriptRoot 'verify-vmp-readiness.py'

foreach ($target in 'Auth', 'Injector', 'Updater', 'Model') {
    & powershell -NoProfile -ExecutionPolicy Bypass -File $apply -Target $target
    if ($LASTEXITCODE -ne 0) {
        throw "VMP selector generation failed: $target"
    }
}

& python $verify
if ($LASTEXITCODE -ne 0) {
    throw 'VMP readiness verification failed'
}

Write-Host ''
Write-Host 'Open the matching current plain binary and empty .vmp project in VMProtect.'
Write-Host 'Paste only one matching line into the project Script box, then click Compile manually:'
foreach ($name in 'NoteBotAuth', 'NoteBotInjector', 'NoteBotUpdater', 'NoteBotModel') {
    $selector = Join-Path $PSScriptRoot "generated\$name.apply_protection.lua"
    Write-Host "dofile([[$selector]])"
}
