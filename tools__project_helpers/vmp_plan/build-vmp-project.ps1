param()

$ErrorActionPreference = 'Stop'

throw @'
This legacy writer is intentionally disabled.

Do not write VMProtect Procedure entries, MapAddress values, function names,
or addresses into a .vmp project. VMP projects are GUI-owned empty projects.
Run apply-vmp-plan.ps1 instead. It derives the current MAP selection and
generates a Lua selector for the VMProtect GUI Script field.
'@
