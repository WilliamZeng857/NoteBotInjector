# Inject Report Lifecycle Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers-executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close every issued inject session and expose server rejection reasons.

**Architecture:** The EXE owns injection orchestration and must finalize a ticket after a local injection failure. The Auth DLL owns RPC response parsing and durable pending-report retry state.

**Tech Stack:** C++17, Qt6 Core/Network, CMake, PowerShell smoke harness.

---

### Task 1: Preserve RPC rejection reasons

**Files:**
- Modify: `src__auth_dll/src/v3/v3_rpc_client.h`
- Modify: `src__auth_dll/src/v3/v3_rpc_client.cpp`

- [ ] Add a `message` field to issue and report RPC responses and populate it from `msg`.
- [ ] Verify through the Auth DLL action response that a server rejection message is retained.

### Task 2: Persist and retry pending reports

**Files:**
- Modify: `src__auth_dll/src/v3/v3_state.h`
- Modify: `src__auth_dll/src/v3/v3_state.cpp`

- [ ] Persist a valid pending report before attempting its RPC.
- [ ] Delete its durable record only after `status=ok` and `accepted=true`.
- [ ] Retry it during state initialization and before a new ticket is issued.

### Task 3: Close EXE-issued failure sessions

**Files:**
- Modify: `src__injector_exe/backend.cpp`

- [ ] On local DLL injection failure, finalize the ticket and report the failure before returning.
- [ ] On result consumption, require an accepted report before showing injection success.

### Task 4: Verify release behavior

**Files:**
- Verify: `build_all.bat`
- Verify: `build__auth_dll_cache/NoteBotAuth.dll`
- Verify: `dist__release_artifacts/NoteBotInjector.exe`

- [ ] Build the DLL and EXE.
- [ ] Run the existing V3 smoke harness and inspect VMP selection validation.
- [ ] Confirm touched UTF-8 files retain their BOM state and have no replacement characters.
