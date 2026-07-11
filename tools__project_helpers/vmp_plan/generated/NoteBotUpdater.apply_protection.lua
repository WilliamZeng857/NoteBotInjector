local manifest = {
  { id = "NBVmp_Updater_ArgValue", raw_symbol = "?NBVmp_Updater_ArgValue@?A0xcf0e99d6@@YA?AVQString@@AEBV?$QList@VQString@@@@AEBV2@@Z", address = 0x00000001400021C0, category = "mutation" },
  { id = "NBVmp_Updater_ArgUInt", raw_symbol = "?NBVmp_Updater_ArgUInt@?A0xcf0e99d6@@YAIAEBV?$QList@VQString@@@@AEBVQString@@@Z", address = 0x0000000140002130, category = "mutation" },
  { id = "NBVmp_Updater_HashesMatch", raw_symbol = "?NBVmp_Updater_HashesMatch@?A0xcf0e99d6@@YA_NAEBVQString@@0@Z", address = 0x00000001400022B0, category = "ultra" },
  { id = "NBVmp_Updater_IsReplaceActionSupported", raw_symbol = "?NBVmp_Updater_IsReplaceActionSupported@?A0xcf0e99d6@@YA_NAEBVQString@@@Z", address = 0x0000000140002310, category = "mutation" },
  { id = "NBVmp_Updater_RestartAllowedForAction", raw_symbol = "?NBVmp_Updater_RestartAllowedForAction@?A0xcf0e99d6@@YA_NAEBVQString@@0@Z", address = 0x0000000140002500, category = "mutation" },
}

local selection_applied = false
local run_log_path = [[C:\NB\tools__project_helpers\vmp_plan\generated\logs\NoteBotUpdater.vmprotect_last_run.log]]
local run_log_first_line = true

local function log(message)
  local line = "[NoteBotUpdater VMP] " .. tostring(message)
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
