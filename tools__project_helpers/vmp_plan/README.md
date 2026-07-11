# NoteBot VMP Status

## Current State

VMP GUI automation is disabled. Do not put any generated Lua selector in a
`.vmp` Script field and do not use the generated selector files. The previous
callback path allowed VMProtect to persist `Procedure` entries into a project
and resulted in a GUI read failure. Only the source-level protection boundaries,
TSV policy tables, and current linker MAP evidence are valid at this point.

## Ownership Boundary

- `NoteBotAuth.dll.vmp`, `NoteBotInjector.exe.vmp`, `NoteBotUpdater.exe.vmp`,
  and `NoteBotModel.dll.vmp` are VMProtect GUI-owned empty projects.
- Never add a `Procedure`, `MapAddress`, function name, RVA, or address to a
  project XML file.
- `apply-vmp-plan.ps1` reads the current linker MAP and protection table, then
  generates the Lua selector. It does not modify a `.vmp` project.

No VMP compile workflow is currently approved. Any future automation must be
validated against a disposable GUI project before it can touch one of these
four projects.
