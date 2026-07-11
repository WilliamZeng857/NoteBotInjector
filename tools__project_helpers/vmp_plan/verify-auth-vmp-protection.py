#!/usr/bin/env python3
"""Verify that the current NoteBotAuth VMP compile selected every planned function."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PLAN_DIR = ROOT / "tools__project_helpers" / "vmp_plan"
DEFAULT_TABLE = PLAN_DIR / "NoteBotAuth.protect.tsv"
DEFAULT_MAP = ROOT / "build__auth_dll_cache" / "NoteBotAuth.map"
DEFAULT_PROJECT = ROOT / "build__auth_dll_cache" / "NoteBotAuth.dll.vmp"
DEFAULT_SELECTOR = PLAN_DIR / "generated" / "NoteBotAuth.apply_protection.lua"
DEFAULT_LOG = PLAN_DIR / "generated" / "logs" / "NoteBotAuth.vmprotect_last_run.log"
DEFAULT_INPUT_DLL = ROOT / "build__auth_dll_cache" / "NoteBotAuth.dll"
DEFAULT_PROTECTED_DLL = ROOT / "build__auth_dll_cache" / "NoteBotAuth.vmp.dll"

CATEGORY_NAMES = {
    "mutation": "Mutation",
    "virtualize": "Virtualization",
    "ultra": "Ultra",
    "super": "Ultra",
}
TABLE_RE = re.compile(r"^(mutation|virtualize|ultra|super)\t(NBVmp_[^\t]+)\t[^\t]+\t.+$")
MAP_RE = re.compile(
    r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\?NBVmp_\S*)\s+"
    r"([0-9A-Fa-f]{16})\s+(?:f\s+)?\S+\.obj\s*$"
)
SELECTOR_RE = re.compile(
    r'^\s*\{ id = "(NBVmp_[^"]+)", raw_symbol = "([^"]+)", '
    r'address = 0x([0-9A-Fa-f]+), protection = "([^"]+)" \},\s*$',
    re.MULTILINE,
)
LOG_OK_RE = re.compile(r"^\[NoteBot VMP\] OK (\w+) \([^)]*\): (NBVmp_\S+)$", re.MULTILINE)
LOG_SUMMARY_RE = re.compile(r"^\[NoteBot VMP\] summary: .* ok=(\d+) miss=(\d+)$", re.MULTILINE)


@dataclass(frozen=True)
class PlannedFunction:
    category: str
    identifier: str


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="strict")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def read_table(path: Path) -> list[PlannedFunction]:
    entries: list[PlannedFunction] = []
    for line_no, raw in enumerate(read_text(path).splitlines(), 1):
        if not raw or raw.startswith("#") or raw.startswith("category\t"):
            continue
        match = TABLE_RE.fullmatch(raw)
        if not match:
            raise ValueError(f"{path}:{line_no}: invalid protection row")
        entries.append(PlannedFunction(match.group(1), match.group(2)))
    if not entries:
        raise ValueError(f"{path}: no protection entries")
    if len({entry.identifier for entry in entries}) != len(entries):
        raise ValueError(f"{path}: duplicate protection identifier")
    return entries


def read_map(path: Path) -> dict[str, tuple[str, int]]:
    symbols: dict[str, tuple[str, int]] = {}
    for raw in read_text(path).splitlines():
        match = MAP_RE.match(raw)
        if match:
            raw_symbol, address = match.groups()
            symbols[raw_symbol] = (raw_symbol, int(address, 16))
    return symbols


def check_pe(path: Path, label: str, errors: list[str]) -> None:
    data = path.read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        errors.append(f"{label}: not a DOS/PE file")
        return
    pe_offset = int.from_bytes(data[0x3C:0x40], "little")
    if pe_offset + 4 > len(data) or data[pe_offset:pe_offset + 4] != b"PE\0\0":
        errors.append(f"{label}: invalid PE signature")


def check_runtime(path: Path, errors: list[str]) -> tuple[int, int] | None:
    if sys.platform != "win32":
        return None
    try:
        library = ctypes.WinDLL(str(path))
        abi = library.nb_get_abi_version
        protocol = library.nb_get_protocol_version
        abi.restype = ctypes.c_int
        protocol.restype = ctypes.c_int
        return abi(), protocol()
    except OSError as exc:
        errors.append(f"protected DLL LoadLibrary failed: {exc}")
    except AttributeError as exc:
        errors.append(f"protected DLL ABI export missing: {exc}")
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--table", type=Path, default=DEFAULT_TABLE)
    parser.add_argument("--map", type=Path, default=DEFAULT_MAP)
    parser.add_argument("--project", type=Path, default=DEFAULT_PROJECT)
    parser.add_argument("--selector", type=Path, default=DEFAULT_SELECTOR)
    parser.add_argument("--log", type=Path, default=DEFAULT_LOG)
    parser.add_argument("--input-dll", type=Path, default=DEFAULT_INPUT_DLL)
    parser.add_argument("--protected-dll", type=Path, default=DEFAULT_PROTECTED_DLL)
    parser.add_argument("--skip-load", action="store_true")
    args = parser.parse_args()

    errors: list[str] = []
    required_paths = {
        "protection table": args.table,
        "linker map": args.map,
        "VMP project": args.project,
        "generated selector": args.selector,
        "VMP run log": args.log,
        "input DLL": args.input_dll,
        "protected DLL": args.protected_dll,
    }
    for label, path in required_paths.items():
        if not path.is_file():
            errors.append(f"missing {label}: {path}")
    if errors:
        return report(errors)

    try:
        planned = read_table(args.table)
        map_symbols = read_map(args.map)
        selector_rows = {
            identifier: (raw_symbol, int(address, 16), protection)
            for identifier, raw_symbol, address, protection in SELECTOR_RE.findall(read_text(args.selector))
        }
    except (OSError, UnicodeError, ValueError) as exc:
        return report([str(exc)])

    project = read_text(args.project)
    expected_dofile = f"dofile([[{args.selector}]])"
    if '<Procedures></Procedures>' not in project:
        errors.append("VMP project is not an empty script-owned project")
    if expected_dofile not in project:
        errors.append("VMP project does not load the current generated selector")

    expected_ids = {entry.identifier for entry in planned}
    if set(selector_rows) != expected_ids:
        missing = sorted(expected_ids - set(selector_rows))
        unexpected = sorted(set(selector_rows) - expected_ids)
        if missing:
            errors.append("selector missing: " + ", ".join(missing))
        if unexpected:
            errors.append("selector unexpected: " + ", ".join(unexpected))

    for entry in planned:
        matches = [value for raw, value in map_symbols.items() if entry.identifier in raw]
        if len(matches) != 1:
            errors.append(f"MAP resolution for {entry.identifier}: expected 1, got {len(matches)}")
            continue
        raw_symbol, address = matches[0]
        selected = selector_rows.get(entry.identifier)
        if selected is None:
            continue
        if selected[0] != raw_symbol or selected[1] != address:
            errors.append(f"selector drift for {entry.identifier}: current MAP differs")
        if selected[2] != CATEGORY_NAMES[entry.category]:
            errors.append(f"selector category mismatch for {entry.identifier}")

    log = read_text(args.log)
    summary_matches = LOG_SUMMARY_RE.findall(log)
    if len(summary_matches) != 1:
        errors.append("VMP log has no unambiguous summary")
    else:
        ok_count, miss_count = map(int, summary_matches[0])
        if ok_count != len(planned) or miss_count != 0:
            errors.append(f"VMP summary expected ok={len(planned)} miss=0, got ok={ok_count} miss={miss_count}")
    if re.search(r"^\[NoteBot VMP\] (?:FAIL|MISS|AMBIGUOUS|SKIP)", log, re.MULTILINE):
        errors.append("VMP log contains a failed, missed, ambiguous, or skipped function")

    applied = {identifier: protection for protection, identifier in LOG_OK_RE.findall(log)}
    for entry in planned:
        actual = applied.get(entry.identifier)
        expected = CATEGORY_NAMES[entry.category]
        if actual != expected:
            errors.append(f"VMP application mismatch for {entry.identifier}: expected {expected}, got {actual or 'none'}")

    check_pe(args.input_dll, "input DLL", errors)
    check_pe(args.protected_dll, "protected DLL", errors)
    if sha256(args.input_dll) == sha256(args.protected_dll):
        errors.append("protected DLL hash equals the input DLL")
    if args.protected_dll.stat().st_mtime < args.log.stat().st_mtime:
        errors.append("protected DLL is older than the VMP compile log")
    if args.protected_dll.stat().st_mtime < args.input_dll.stat().st_mtime:
        errors.append("protected DLL is older than the input DLL")

    runtime = None if args.skip_load else check_runtime(args.protected_dll, errors)
    if errors:
        return report(errors)

    categories = {name: sum(entry.category == name for entry in planned) for name in CATEGORY_NAMES}
    print("AUTH_VMP_PROTECTION_OK")
    print(f"functions={len(planned)} mutation={categories['mutation']} virtualize={categories['virtualize']} ultra={categories['ultra']}")
    print(f"input_sha256={sha256(args.input_dll)}")
    print(f"protected_sha256={sha256(args.protected_dll)}")
    if runtime is not None:
        print(f"runtime_abi={runtime[0]} runtime_protocol={runtime[1]}")
    print("evidence=MAP+selector+VMProtect compile log+protected PE+runtime load")
    return 0


def report(errors: list[str]) -> int:
    print("AUTH_VMP_PROTECTION_FAILED")
    for error in errors:
        print(f"- {error}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
