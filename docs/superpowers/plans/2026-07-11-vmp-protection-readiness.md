# VMProtect Protection Readiness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers-subagent-driven-development (recommended) or superpowers-executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make all four non-overlay release artifacts selectable for stable function-level VMProtect protection without changing user-visible behavior.

**Architecture:** Mark only small deterministic protection leaves as `NBVmp_*` and keep I/O, callback, process, and workflow coordinators unprotected. Generate every VMP project from a checked-in TSV and the current linker MAP.

**Tech Stack:** C++17, Qt 6, MSVC linker MAP files, CMake, PowerShell, Python 3, VMProtect SDK.

---

### Task 1: Add a failing protection-readiness verifier

**Files:**
- Create: `tools__project_helpers/vmp_plan/verify-vmp-readiness.py`

- [ ] **Step 1: Require all four tables and MAP-enabled targets**

Implement a Python verifier that fails when any target table is absent or when
the root CMake file lacks `/MAP:NoteBotInjector.map`, `/MAP:NoteBotUpdater.map`,
and `/MAP:NoteBotModel.map`.

- [ ] **Step 2: Run static verifier before implementation**

Run: `python tools__project_helpers/vmp_plan/verify-vmp-readiness.py --skip-maps`

Expected: failure listing missing Injector, Updater, and Model tables.

- [ ] **Step 3: Extend verifier to inspect source and built MAP files**

Require every TSV `symbol_key` to occur in the target source and require a
full verification pass to resolve every table symbol in exactly one target MAP.

### Task 2: Establish common marker and linker contracts

**Files:**
- Create: `src__injector_exe/vmp_defs.h`
- Modify: `CMakeLists.txt`
- Modify: `build_clean.bat`

- [ ] **Step 1: Add shared noinline/SDK marker macros**

Use the Auth DLL macro pattern for Injector, Updater, and Model while keeping
macros no-op unless `NB_VMP_BUILD` is set.

- [ ] **Step 2: Add opt-in root CMake SDK discovery and MAP flags**

Emit all three root-target MAP files with `/DEBUG /OPT:NOREF /OPT:NOICF` and
only enable SDK markers when the explicit release-build option is on.

### Task 3: Add protection leaves and tables

**Files:**
- Modify: `src__injector_exe/backend.cpp`
- Modify: `src__injector_exe/updater.cpp`
- Modify: `src__updater_exe/updater_main.cpp`
- Modify: `src__model_dll/src/model_api.cpp`
- Create: `tools__project_helpers/vmp_plan/NoteBotInjector.protect.tsv`
- Create: `tools__project_helpers/vmp_plan/NoteBotUpdater.protect.tsv`
- Create: `tools__project_helpers/vmp_plan/NoteBotModel.protect.tsv`

- [ ] **Step 1: Mark only bounded Injector and updater leaves**

Use protected leaves for digest comparisons, identity parsing, artifact
metadata validation, updater argument validation, and replacement digest
comparison. Keep I/O and flow coordinators unchanged.

- [ ] **Step 2: Mark bounded Model computation leaves**

Use protected leaves for hook address/compatibility, rel32 calculation,
normalization, and byte construction. Keep process APIs and patch loops
unchanged.

- [ ] **Step 3: Align Auth table categories with marker source**

Correct existing map-filter coverage and category drift without expanding the
Auth DLL's behavior surface.

### Task 4: Make VMP generation target-agnostic

**Files:**
- Modify: `tools__project_helpers/vmp_plan/apply-vmp-plan.ps1`
- Modify: `tools__project_helpers/build/filter_map.py`
- Modify: `tools__project_helpers/build/filter_map_dll.py`

- [ ] **Step 1: Support all four target names**

Generate each `.vmp.generated.tsv` from `NBVmp_*` MAP symbols and reject
missing, duplicate, or unlisted selections.

- [ ] **Step 2: Preserve all protected Auth symbols in filtered maps**

Keep all five protected Auth object files in the post-build filtered MAP.

### Task 5: Build and verify

**Files:**
- Verify: `build_all.bat`
- Verify: all four MAP files and output artifacts

- [ ] **Step 1: Run a clean configure/build because CMake changed**

Run: `build_clean.bat`

Expected: successful Auth, Injector, Updater, and Model builds.

- [ ] **Step 2: Run full readiness verification**

Run: `python tools__project_helpers/vmp_plan/verify-vmp-readiness.py`

Expected: every TSV entry maps exactly once and no unlisted `NBVmp_*` symbol
remains.

- [ ] **Step 3: Check encoding, diff, artifact timestamps, then commit/push**

Run strict UTF-8/BOM checks, `git diff --check`, and stage only the changed
files before using the project commit script and pushing `origin/main`.
