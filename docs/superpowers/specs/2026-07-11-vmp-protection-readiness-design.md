# VMProtect Protection Readiness Design

## Scope

Prepare these independently shipped artifacts for function-level VMProtect
selection:

- `NoteBotInjector.exe`
- `NoteBotUpdater.exe`
- `NoteBotAuth.dll`
- `NoteBotModel.dll`

`overlay.dll` and every DLL that is injected into the target process remain
outside this design. `NoteBotModel.dll` is in scope because the injector loads
it locally and it performs its own process-patch workflow.

## Protection Boundary

The protected unit is a small `NBVmp_*` function with a stable emitted symbol.
Each unit is `NB_NOINLINE` and is assigned one of these categories in a TSV
table:

- `mutation`: short normalization, parsing, comparison, or argument checks.
- `virtualize`: deterministic transforms with bounded data and no operating
  system side effects.
- `ultra`: small trust decisions, digest equality, relative-jump computation,
  and shellcode byte construction.

Large coordinator functions are explicitly excluded from direct protection:
network transport, file reads and writes, DLL loading, process handles, remote
memory allocation/writes, waits, Qt callbacks, update replacement flow, and
the top-level injection/patch loops. They call protected leaves but keep their
existing ordering, logging, error handling, and observable behavior.

## Per-artifact Selection

`NoteBotAuth.dll` remains the highest trust boundary. Its ticket wrapper,
signature, HMAC, replay, and final-allow leaves remain protected. The map
filter must retain every `protected_*_ops.cpp.obj` object so the table and MAP
cannot drift.

`NoteBotInjector.exe` protects local key identity derivation, artifact digest
comparison, and remote model metadata validation. Its UI, loading sequence,
and injection coordinator remain ordinary code.

`NoteBotUpdater.exe` protects command argument parsing, replacement argument
validation, and copied-file digest equality. Waiting, file copying, rollback,
and restart remain ordinary code.

`NoteBotModel.dll` protects hook address/rel32 derivation, compatibility
decisions, arm-size normalization, and jump/shellcode byte construction.
`patchProcess`, `installHook`, remote allocation, remote writes, and the wait
loop remain ordinary coordinators.

## Build and Automation Contract

Every target emits an unoptimized-for-selection linker MAP with stable
`NBVmp_*` symbols. One common VMP-plan generator accepts all four targets and
generates a target-specific Lua selector from its TSV table and current MAP.
The VMProtect `.vmp` projects remain GUI-owned empty projects: the generator
must never add Procedure entries, names, or addresses. A readiness verifier
requires that every table row resolves to exactly one marker symbol in the
corresponding MAP, that no marker symbol is unlisted, and that every project
remains free of Procedure entries.

VMProtect SDK markers are opt-in. Normal developer builds leave marker macros
as no-ops and preserve runtime behavior. A release-protection build enables
the common CMake option and points `NB_VMP_SDK_DIR` at the SDK include root.

## Verification

The readiness verifier has a static mode for source/table/CMake integrity and
a full mode after build for MAP/table symbol resolution. The release build
also verifies UTF-8 validity, unchanged BOM state for modified existing files,
`git diff --check`, and all four output artifacts. The user opens the current
plain binary and matching empty project in VMProtect, pastes the generated
`dofile(...)` loader into the project Script field, and clicks Compile manually.
