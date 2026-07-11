#!/usr/bin/env python3
"""Generate Auth's selector by preserving the verified NoteBot Lua runtime."""

from __future__ import annotations

import re
import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REFERENCE = Path(
    r"D:\Downloads\cheatengine-mcp-bridge-main\cheatengine-mcp-bridge-main\NoteBot"
    r"\tools\vmp\apply_protection.lua"
)
TABLE = ROOT / "tools__project_helpers" / "vmp_plan" / "NoteBotAuth.protect.tsv"
DEFAULT_MAP = ROOT / "build__auth_dll_cache" / "NoteBotAuth.map"
DEFAULT_OUTPUT = ROOT / "tools__project_helpers" / "vmp_plan" / "generated" / "NoteBotAuth.apply_protection.lua"
DEFAULT_LOG = ROOT / "tools__project_helpers" / "vmp_plan" / "generated" / "logs" / "NoteBotAuth.vmprotect_last_run.log"

BEGIN = "-- BEGIN GENERATED SELECTION"
END = "-- END GENERATED SELECTION"
CATEGORY_NAMES = {
    "mutation": "Mutation",
    "virtualize": "Virtualization",
    "ultra": "Ultra",
    "super": "Ultra",
}
MAP_RE = re.compile(
    r"^\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\?NBVmp_\S*)\s+"
    r"([0-9A-Fa-f]{16})\s+(?:f\s+)?\S+\.obj\s*$"
)


def read_table() -> list[tuple[str, str]]:
    entries: list[tuple[str, str]] = []
    for raw in TABLE.read_text(encoding="utf-8").splitlines():
        if not raw or raw.startswith("#") or raw.startswith("category\t"):
            continue
        category, symbol, *_ = raw.split("\t")
        if category not in CATEGORY_NAMES or not symbol.startswith("NBVmp_"):
            raise ValueError(f"invalid protection entry: {raw}")
        entries.append((category, symbol))
    if len(entries) != len({symbol for _, symbol in entries}):
        raise ValueError("duplicate Auth protection symbol")
    if not entries:
        raise ValueError("empty Auth protection table")
    return entries


def read_map(path: Path) -> list[tuple[str, str]]:
    symbols: list[tuple[str, str]] = []
    for raw in path.read_text(encoding="utf-8", errors="strict").splitlines():
        match = MAP_RE.match(raw)
        if match:
            symbols.append((match.group(1), match.group(2)))
    return symbols


def render_selection(entries: list[tuple[str, str]], symbols: list[tuple[str, str]]) -> str:
    rows = [BEGIN, "local manifest = {"]
    for category, identifier in entries:
        matches = [(raw, address) for raw, address in symbols if identifier in raw]
        if len(matches) != 1:
            raise ValueError(f"{identifier}: expected one map symbol, found {len(matches)}")
        raw, address = matches[0]
        rows.append(
            f'  {{ id = "{identifier}", raw_symbol = "{raw}", '
            f'address = 0x{int(address, 16):X}, protection = "{CATEGORY_NAMES[category]}" }},'
        )
    rows.extend(["}", END])
    return "\n".join(rows)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, default=DEFAULT_MAP)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--log", type=Path, default=DEFAULT_LOG)
    args = parser.parse_args()

    baseline = REFERENCE.read_text(encoding="utf-8")
    begin = baseline.find(BEGIN)
    end = baseline.find(END)
    if begin < 0 or end < begin:
        raise ValueError("verified reference selector markers are missing")
    end += len(END)

    entries = read_table()
    selection = render_selection(entries, read_map(args.map))
    rendered = baseline[:begin] + selection + baseline[end:]
    old_log = re.search(r"local run_log_path = \[\[(.*?)\]\]", rendered)
    if not old_log:
        raise ValueError("verified reference log path declaration is missing")
    rendered = rendered[:old_log.start(1)] + str(args.log) + rendered[old_log.end(1):]
    old_add = '  local protected = try_call(functions, "addByAddress", addr, ct)\n  if not protected then\n    log("FAIL addByAddress: " .. entry.id)\n    return false\n  end\n'
    new_add = '  local add_ok, protected = pcall(functions.addByAddress, functions, addr, ct)\n  if not add_ok then\n    log("FAIL addByAddress: " .. entry.id .. " error=" .. tostring(protected))\n    return false\n  end\n  if not protected then\n    log("FAIL addByAddress: " .. entry.id .. " error=returned-nil")\n    return false\n  end\n'
    if old_add not in rendered:
        raise ValueError("verified addByAddress call site is missing")
    rendered = rendered.replace(old_add, new_add, 1)
    old_set = '    local ok = pcall(protected.setCompilationType, protected, ct)\n    if not ok then\n      log("FAIL setCompilationType: " .. entry.id)\n      return false\n    end\n'
    new_set = '    local ok, err = pcall(protected.setCompilationType, protected, ct)\n    if not ok then\n      log("FAIL setCompilationType: " .. entry.id .. " error=" .. tostring(err))\n      return false\n    end\n'
    if old_set not in rendered:
        raise ValueError("verified setCompilationType call site is missing")
    rendered = rendered.replace(old_set, new_set, 1)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.log.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered, encoding="utf-8", newline="\n")
    print(f"generated {args.output}: entries={len(entries)} map={args.map} baseline={REFERENCE}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
