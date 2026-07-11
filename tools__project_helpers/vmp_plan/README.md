# NoteBot VMP GUI Workflow

## Ownership Boundary

- `NoteBotAuth.dll.vmp`, `NoteBotInjector.exe.vmp`, `NoteBotUpdater.exe.vmp`,
  and `NoteBotModel.dll.vmp` are VMProtect GUI-owned empty projects.
- Never add a `Procedure`, `MapAddress`, function name, RVA, or address to a
  project XML file.
- `apply-vmp-plan.ps1` reads the current linker MAP and protection table, then
  generates the Lua selector. It does not modify a `.vmp` project.

## Before Every GUI Compile

1. Run the ordinary build so the plain binary and MAP come from the same build.
2. Generate all selectors without touching the VMP projects:

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File .\tools__project_helpers\vmp_plan\prepare-vmp-gui.ps1
   ```

3. Close and reopen VMProtect. Open the matching current plain binary and its
   empty `.vmp` project. In the project Script box, use only the `dofile(...)`
   line printed by the generator. Do not select procedures manually.
4. Click Compile. The Lua `OnBeforeCompilation()` callback selects all
   functions only after VMProtect has built its function map.
5. Accept a run only when the target log ends in `ok=<count> miss=0`.
   Any `MISS`, `AMBIGUOUS`, API error, or partial result is a failed pass.

## Logs

Each generated selector writes its GUI result to:

`tools__project_helpers\vmp_plan\generated\logs\<target>.vmprotect_last_run.log`

The VMP protection policy remains in the four `*.protect.tsv` files. Change
that policy and rebuild; never edit generated Lua tables by hand.
