# Inject Report Lifecycle Design

## Goal

Prevent a successfully issued inject ticket from leaving the server-side
session locked when local injection fails or result reporting is interrupted.

## Design

The injector finalizes and reports every failure after ticket issuance. It
only reports injection success after `report_inject_result_v3` returns both
`rc == 0` and `data.accepted == true`.

The Auth DLL retains a pending report on disk until the server accepts it and
retries it after initialization and before issuing another ticket. The RPC
response model preserves the server `msg` so that a `1007` can be diagnosed.

## Scope

- Modify the injector and Auth DLL only.
- Do not alter the server lock semantics or VMP selection tables.
- Use the existing smoke harness for integration verification; the current
  concrete RPC client has no isolated test seam without a disproportionate
  refactor.
