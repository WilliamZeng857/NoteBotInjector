#!/usr/bin/env python3
"""Generate a VMProtect GUI selector from a MAP-derived TSV."""

from __future__ import annotations

import argparse
import csv
from pathlib import Path


RUNTIME = r'''local selection_applied = false
local run_log_path = [[__LOG_PATH__]]
local run_log_first_line = true

local function log(message)
  local line = "[__TARGET__ VMP] " .. tostring(message)
  if print then print(line) end
  if type(io) ~= "table" or type(io.open) ~= "function" then return end
  local mode = run_log_first_line and "w" or "a"
  local ok, file = pcall(io.open, run_log_path, mode)
  if ok and file then
    pcall(function() file:write(line .. "\r\n"); file:close() end)
    run_log_first_line = false
  end
end

local function try_call(object, method, ...)
  if not object or type(object[method]) ~= "function" then return nil end
  local ok, value = pcall(object[method], object, ...)
  return ok and value or nil
end

local function first_defined(...)
  for i = 1, select("#", ...) do
    local value = select(i, ...)
    if value ~= nil then return value end
  end
  return nil
end

local function address_key(value)
  if type(value) == "number" then return string.format("%X", value) end
  local value_text = tostring(value)
  local hex = string.match(value_text, "0[xX]([%x]+)") or string.match(value_text, "^%s*([%x]+)%s*$")
  if not hex then return nil end
  hex = string.gsub(string.upper(hex), "^0+", "")
  return hex == "" and "0" or hex
end

local function addresses_match(left, right)
  local left_key, right_key = address_key(left), address_key(right)
  return left_key ~= nil and left_key == right_key
end

local function compilation_type(kind)
  if kind == "virtualize" then return first_defined(rawget(_G, "ctVirtualization"), rawget(_G, "Virtualization"), 0) end
  if kind == "mutation" then return first_defined(rawget(_G, "ctMutation"), rawget(_G, "Mutation"), 1) end
  if kind == "ultra" or kind == "super" then return first_defined(rawget(_G, "ctUltra"), rawget(_G, "Ultra"), 2) end
  return nil
end

local function resolve_context()
  local candidates = {}
  if type(vmprotect) == "table" and type(vmprotect.core) == "function" then
    local ok, core = pcall(vmprotect.core)
    if ok and core then table.insert(candidates, core) end
  end
  if rawget(_G, "core") then table.insert(candidates, rawget(_G, "core")) end
  for _, core_object in ipairs(candidates) do
    local arch = try_call(core_object, "inputArchitecture")
    if arch then
      local map = try_call(arch, "mapFunctions")
      local functions = first_defined(try_call(arch, "functions"), try_call(arch, "intelFunctions"))
      if map and functions then return map, functions, "inputArchitecture" end
    end
    local map = try_call(core_object, "mapFunctions")
    local functions = first_defined(try_call(core_object, "functions"), try_call(core_object, "intelFunctions"))
    if map and functions then return map, functions, "core" end
  end
  log("ERROR: input architecture/function map unavailable; reopen the current plain binary and compile again")
  return nil, nil, nil
end

local function count_items(map)
  return try_call(map, "count") or try_call(map, "size") or 0
end

local function item_at(map, index)
  return try_call(map, "item", index) or try_call(map, "get", index)
end

local function raw_anchor(raw_symbol)
  return string.match(raw_symbol, "^(%?[^@]+@[^@]+@@)") or raw_symbol
end

local function find_function(map, entry)
  for _, name in ipairs({entry.raw_symbol, entry.id}) do
    local direct = try_call(map, "itemByName", name)
    if direct and try_call(direct, "address") then return direct, "direct-name" end
  end

  local found, found_kind = nil, nil
  local anchor = raw_anchor(entry.raw_symbol)
  for index = 0, count_items(map) - 1 do
    local candidate = item_at(map, index)
    local candidate_name = try_call(candidate, "name") or try_call(candidate, "displayName") or try_call(candidate, "fullName") or ""
    local candidate_address = try_call(candidate, "address")
    local matches = candidate_name == entry.raw_symbol or candidate_name == entry.id
        or string.find(candidate_name, entry.raw_symbol, 1, true)
        or string.find(candidate_name, anchor, 1, true)
    if matches and candidate_address then
      if found then
        log("AMBIGUOUS " .. entry.id .. " while resolving generated MAP symbol")
        return nil, "ambiguous"
      end
      found = candidate
      found_kind = addresses_match(candidate_address, entry.address) and "map-address" or "rebased-name"
    end
  end
  return found, found_kind or "miss"
end

local function apply_selection()
  if selection_applied then
    log("selection already applied; skipping duplicate pass")
    return true
  end
  local map, functions, api = resolve_context()
  if not map or not functions then return false end
  if type(functions.clear) ~= "function" then
    log("FAIL functions.clear is unavailable")
    return false
  end
  local cleared = pcall(functions.clear, functions)
  if not cleared then
    log("FAIL functions.clear")
    return false
  end
  local ok, miss = 0, 0
  log("function map: api=" .. api .. " count=" .. count_items(map) .. " selection=" .. #manifest)
  for _, entry in ipairs(manifest) do
    local fn, match_kind = find_function(map, entry)
    local address = fn and try_call(fn, "address")
    local compilation = compilation_type(entry.category)
    local protected = address and compilation ~= nil and try_call(functions, "addByAddress", address, compilation)
    if protected and type(protected.setCompilationType) == "function" then
      if not pcall(protected.setCompilationType, protected, compilation) then protected = nil end
    end
    if protected then
      ok = ok + 1
      log("OK " .. entry.category .. " (" .. match_kind .. "): " .. entry.id)
    else
      miss = miss + 1
      log("MISS " .. entry.id .. " raw=" .. entry.raw_symbol)
    end
  end
  selection_applied = miss == 0
  log("summary: api=" .. api .. " ok=" .. ok .. " miss=" .. miss)
  return selection_applied
end

function OnBeforeCompilation()
  log("OnBeforeCompilation: applying generated MAP selection")
  return apply_selection()
end
'''


def lua_quote(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def read_selection(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="ascii", newline="") as file:
        rows = list(csv.DictReader((line for line in file if not line.startswith("#")), delimiter="\t"))
    if not rows:
        raise ValueError(f"{path}: no MAP-derived selection rows")
    expected = {"category", "address", "symbol", "object", "module", "note"}
    if set(rows[0]) != expected:
        raise ValueError(f"{path}: unexpected selection columns")
    seen: set[str] = set()
    for row in rows:
        if row["category"] not in {"mutation", "virtualize", "ultra", "super"}:
            raise ValueError(f"{path}: invalid category for {row['symbol']}")
        if row["address"] in {"", "0"} or not row["address"].startswith("0") or not row["symbol"].startswith("?"):
            raise ValueError(f"{path}: row is not MAP-derived: {row['symbol']}")
        if row["symbol"] in seen:
            raise ValueError(f"{path}: duplicate raw symbol: {row['symbol']}")
        seen.add(row["symbol"])
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True)
    parser.add_argument("--selection", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    rows = read_selection(args.selection)
    log_dir = args.output.parent / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    table = ["local manifest = {"]
    for row in rows:
        table.append(
            f'  {{ id = "{lua_quote(row["symbol"].split("@", 1)[0][1:])}", '
            f'raw_symbol = "{lua_quote(row["symbol"])}", '
            f'address = 0x{row["address"].upper()}, category = "{row["category"]}" }},'
        )
    table.append("}")
    rendered = "\n".join(table) + "\n\n" + RUNTIME
    rendered = rendered.replace("__TARGET__", args.target)
    rendered = rendered.replace("__LOG_PATH__", str(log_dir / f"{args.target}.vmprotect_last_run.log"))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered, encoding="utf-8", newline="\n")
    print(f"generated {args.output}: protected={len(rows)} selection={args.selection}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
