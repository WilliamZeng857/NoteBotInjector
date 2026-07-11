# NoteBot VMP Status

## Current State

`NoteBotAuth.dll` uses a script-owned VMProtect project. Its `.vmp` file keeps
an empty `<Procedures>` list and loads the generated selector with `dofile()`.
The selector derives every function address from the current linker MAP; do not
add a `Procedure`, `MapAddress`, function name, RVA, or address by hand.

After a successful VMP compile, run:

```powershell
python .\tools__project_helpers\vmp_plan\verify-auth-vmp-protection.py
```

The checker requires all planned functions to resolve in the current MAP, be
present in the generated selector, appear as successful VMP selections in the
current compile log, and exist in a newer protected PE artifact that loads.

## Ownership Boundary

- `NoteBotAuth.dll.vmp`, `NoteBotInjector.exe.vmp`, `NoteBotUpdater.exe.vmp`,
  and `NoteBotModel.dll.vmp` are VMProtect GUI-owned empty projects.
- Never add a `Procedure`, `MapAddress`, function name, RVA, or address to a
  project XML file.
- `apply-vmp-plan.ps1` reads the current linker MAP and protection table, then
  generates the Lua selector. It does not modify a `.vmp` project.

The Auth compile workflow is approved only through its generated selector and
the self-check script above. The other projects remain GUI-owned empty projects.
