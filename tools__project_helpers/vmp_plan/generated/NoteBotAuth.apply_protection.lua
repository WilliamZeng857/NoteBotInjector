-- VMProtect helper script for NoteBot.
-- Selection is generated from protection_manifest.json plus the current overlay.map.
-- Do not hand-edit the generated selection block or overlay.dll.vmp.

-- BEGIN GENERATED SELECTION
local manifest = {
  { id = "NBVmp_Dll_SafeFileNameAllowed", raw_symbol = "?NBVmp_Dll_SafeFileNameAllowed@Protected@NBAuth@@YA_N_N@Z", address = 0x180015080, protection = "Mutation" },
  { id = "NBVmp_Dll_MetadataIssue", raw_symbol = "?NBVmp_Dll_MetadataIssue@Protected@NBAuth@@YAH_N000@Z", address = 0x180014500, protection = "Virtualization" },
  { id = "NBVmp_Dll_DownloadedSizeMatches", raw_symbol = "?NBVmp_Dll_DownloadedSizeMatches@Protected@NBAuth@@YA_N_J0@Z", address = 0x1800144F0, protection = "Ultra" },
  { id = "NBVmp_Name_IsOriginalHook", raw_symbol = "?NBVmp_Name_IsOriginalHook@Protected@NBName@@YA_NPEBE0_K@Z", address = 0x180015A90, protection = "Mutation" },
  { id = "NBVmp_Name_IsAlreadyPatched", raw_symbol = "?NBVmp_Name_IsAlreadyPatched@Protected@NBName@@YA_NPEBE_K@Z", address = 0x180015A70, protection = "Mutation" },
  { id = "NBVmp_Name_BuildShellcode", raw_symbol = "?NBVmp_Name_BuildShellcode@Protected@NBName@@YA?AV?$vector@EV?$allocator@E@std@@@std@@_K00@Z", address = 0x1800158A0, protection = "Virtualization" },
  { id = "NBVmp_Name_MakeJmp5", raw_symbol = "?NBVmp_Name_MakeJmp5@Protected@NBName@@YA_N_K0QEAE@Z", address = 0x180015AC0, protection = "Mutation" },
  { id = "NBVmp_V3_NormalizeKey", raw_symbol = "?NBVmp_V3_NormalizeKey@Protected@NBAuth@@YA?AVQString@@AEBV3@@Z", address = 0x18001D7F0, protection = "Mutation" },
  { id = "NBVmp_V3_HashKeySha256", raw_symbol = "?NBVmp_V3_HashKeySha256@Protected@NBAuth@@YA?AVQString@@AEBV3@@Z", address = 0x18001D260, protection = "Virtualization" },
  { id = "NBVmp_V3_KeyHashLongEnough", raw_symbol = "?NBVmp_V3_KeyHashLongEnough@Protected@NBAuth@@YA_N_J@Z", address = 0x18001D3C0, protection = "Mutation" },
  { id = "NBVmp_V3_NormalizeFeatureFlags", raw_symbol = "?NBVmp_V3_NormalizeFeatureFlags@Protected@NBAuth@@YA?AVQJsonArray@@AEBV3@@Z", address = 0x18001D590, protection = "Virtualization" },
  { id = "NBVmp_Ticket_ComputeTicketSha256", raw_symbol = "?NBVmp_Ticket_ComputeTicketSha256@Protected@NBAuth@@YA?AVQString@@AEBV3@@Z", address = 0x1800179F0, protection = "Virtualization" },
  { id = "NBVmp_Ticket_DeriveWrapperKey", raw_symbol = "?NBVmp_Ticket_DeriveWrapperKey@Protected@NBAuth@@YA?AVQByteArray@@AEBVQString@@0@Z", address = 0x180017DA0, protection = "Ultra" },
  { id = "NBVmp_Ticket_DecryptWrapper", raw_symbol = "?NBVmp_Ticket_DecryptWrapper@Protected@NBAuth@@YA_NAEBVQByteArray@@000AEAV3@PEAVQString@@@Z", address = 0x180017B50, protection = "Ultra" },
  { id = "NBVmp_Ticket_WrapperFieldsAllowed", raw_symbol = "?NBVmp_Ticket_WrapperFieldsAllowed@Protected@NBAuth@@YA_NHI_N000@Z", address = 0x18001A030, protection = "Virtualization" },
  { id = "NBVmp_Ticket_ServerFieldsAllowed", raw_symbol = "?NBVmp_Ticket_ServerFieldsAllowed@Protected@NBAuth@@YA_NH_N000000_J@Z", address = 0x180019FF0, protection = "Virtualization" },
  { id = "NBVmp_Ticket_CanonicalResultFeatureFlags", raw_symbol = "?NBVmp_Ticket_CanonicalResultFeatureFlags@Protected@NBAuth@@YA_J_J@Z", address = 0x180017080, protection = "Virtualization" },
  { id = "NBVmp_Ticket_ComputeResultHmacHex", raw_symbol = "?NBVmp_Ticket_ComputeResultHmacHex@Protected@NBAuth@@YA?AVQString@@AEBVQByteArray@@0PEAV3@@Z", address = 0x1800177D0, protection = "Ultra" },
  { id = "NBVmp_Verify_SignatureHexToFixed", raw_symbol = "?NBVmp_Verify_SignatureHexToFixed@Protected@NBAuth@@YA_NAEBVQString@@AEAV?$array@E$0BAA@@std@@PEAV3@@Z", address = 0x18001BAB0, protection = "Ultra" },
  { id = "NBVmp_Verify_FormalServerKeyReady", raw_symbol = "?NBVmp_Verify_FormalServerKeyReady@Protected@NBAuth@@YA_NHAEBVQString@@AEBV?$vector@EV?$allocator@E@std@@@std@@PEAV3@@Z", address = 0x18001B360, protection = "Ultra" },
  { id = "NBVmp_Verify_ServerTicketSignature", raw_symbol = "?NBVmp_Verify_ServerTicketSignature@Protected@NBAuth@@YA_NAEBV?$vector@EV?$allocator@E@std@@@std@@AEBVQString@@AEBV?$array@E$0BAA@@4@PEAV5@@Z", address = 0x18001B840, protection = "Ultra" },
  { id = "NBVmp_Verify_TicketShaMatches", raw_symbol = "?NBVmp_Verify_TicketShaMatches@Protected@NBAuth@@YA_NAEBVQString@@0PEAV3@@Z", address = 0x18001BCC0, protection = "Mutation" },
  { id = "NBVmp_Verify_ServerKeyMetadataMatches", raw_symbol = "?NBVmp_Verify_ServerKeyMetadataMatches@Protected@NBAuth@@YA_NHAEBVQString@@H0PEAV3@@Z", address = 0x18001B640, protection = "Mutation" },
  { id = "NBVmp_Verify_TargetPid", raw_symbol = "?NBVmp_Verify_TargetPid@Protected@NBAuth@@YA_NIIPEAVQString@@@Z", address = 0x18001BC40, protection = "Mutation" },
  { id = "NBVmp_Verify_DllShaMatches", raw_symbol = "?NBVmp_Verify_DllShaMatches@Protected@NBAuth@@YA_NAEBVQString@@0PEAV3@@Z", address = 0x18001B240, protection = "Mutation" },
  { id = "NBVmp_Verify_TtlElapsed", raw_symbol = "?NBVmp_Verify_TtlElapsed@Protected@NBAuth@@YA_N_K00PEAVQString@@@Z", address = 0x18001BD60, protection = "Ultra" },
  { id = "NBVmp_Verify_ReplayEntryMatches", raw_symbol = "?NBVmp_Verify_ReplayEntryMatches@Protected@NBAuth@@YA_NAEBVQString@@000@Z", address = 0x18001B540, protection = "Ultra" },
  { id = "NBVmp_Verify_ResultHmacEquals", raw_symbol = "?NBVmp_Verify_ResultHmacEquals@Protected@NBAuth@@YA_NAEBVQString@@0@Z", address = 0x18001B5E0, protection = "Ultra" },
  { id = "NBVmp_Verify_SignDevicePayload", raw_symbol = "?NBVmp_Verify_SignDevicePayload@Protected@NBAuth@@YA_NAEBVQByteArray@@0AEAV?$array@E$0BAA@@std@@@Z", address = 0x18001B920, protection = "Ultra" },
  { id = "NBVmp_Verify_TicketResponseReady", raw_symbol = "?NBVmp_Verify_TicketResponseReady@Protected@NBAuth@@YA_NAEBVQString@@0@Z", address = 0x18001BCA0, protection = "Ultra" },
  { id = "NBVmp_Verify_WrapperKeyReady", raw_symbol = "?NBVmp_Verify_WrapperKeyReady@Protected@NBAuth@@YA_NAEBVQByteArray@@@Z", address = 0x18001BDD0, protection = "Ultra" },
  { id = "NBVmp_Verify_InjectResultEnvelopeMatches", raw_symbol = "?NBVmp_Verify_InjectResultEnvelopeMatches@Protected@NBAuth@@YA_NAEBVQString@@H0000000@Z", address = 0x18001B440, protection = "Ultra" },
  { id = "NBVmp_Verify_FinalAllow", raw_symbol = "?NBVmp_Verify_FinalAllow@Protected@NBAuth@@YA_N_N0000PEAVQString@@@Z", address = 0x18001B2E0, protection = "Ultra" },
}
-- END GENERATED SELECTION

local selection_applied = false

-- Keep the complete VMProtect console trace beside this script. The first
-- message from a new dofile() run replaces the prior trace; compilation
-- callbacks in that same run append to it.
local run_log_path = [[C:\NB\tools__project_helpers\vmp_plan\generated\logs\NoteBotAuth.vmprotect_last_run.log]]
local run_log_first_line = true
local run_log_write_failed = false

local function append_run_log(line)
  if type(io) ~= "table" or type(io.open) ~= "function" then
    return
  end

  local mode = run_log_first_line and "w" or "a"
  local opened, file = pcall(io.open, run_log_path, mode)
  if not opened or not file then
    if not run_log_write_failed and print then
      print("[NoteBot VMP] WARN: cannot write log: " .. run_log_path)
    end
    run_log_write_failed = true
    return
  end

  local written = pcall(function()
    file:write(line .. "\r\n")
    file:close()
  end)
  if written then
    run_log_first_line = false
  elseif not run_log_write_failed and print then
    print("[NoteBot VMP] WARN: cannot append log: " .. run_log_path)
    run_log_write_failed = true
  end
end

local function log(msg)
  local line = "[NoteBot VMP] " .. tostring(msg)
  if print then
    print(line)
  end
  append_run_log(line)
end

log("LOADED selector=v6.6.01 entries=" .. #manifest .. " matcher=hex")

local function first_defined(...)
  for i = 1, select("#", ...) do
    local value = select(i, ...)
    if value ~= nil then
      return value
    end
  end
  return nil
end

local function try_call(obj, method, ...)
  if not obj then return nil end
  local fn = obj[method]
  if not fn then return nil end
  local ok, value = pcall(fn, obj, ...)
  if ok then return value end
  return nil
end

local function vmprotect_core()
  -- Official VMProtect Lua API: vmprotect.core(). It is a namespace function,
  -- so passing vmprotect as a self argument makes the factory call fail.
  if type(vmprotect) ~= "table" or type(vmprotect.core) ~= "function" then
    return nil
  end
  local ok, value = pcall(vmprotect.core)
  if ok then return value end
  log("ERROR: vmprotect.core() failed: " .. tostring(value))
  return nil
end

local function compilation_type(name)
  if name == "Virtualization" then
    return first_defined(rawget(_G, "ctVirtualization"), rawget(_G, "Virtualization"), 0)
  elseif name == "Mutation" then
    return first_defined(rawget(_G, "ctMutation"), rawget(_G, "Mutation"), 1)
  elseif name == "Ultra" then
    return first_defined(rawget(_G, "ctUltra"), rawget(_G, "Ultra"), 2)
  end
  return nil
end

local function function_name(fn)
  return try_call(fn, "name") or try_call(fn, "displayName") or try_call(fn, "fullName")
end

local function function_address(fn)
  return try_call(fn, "address")
end

local function address_key(value)
  if type(value) == "number" then
    return string.format("%X", value)
  end

  -- VMProtect 3.9 prints its uint64 userdata as bare hexadecimal text, for
  -- example `180074AB0`, while other API builds include a `0x` prefix.
  local rendered = tostring(value)
  local hex = string.match(rendered, "0[xX]([%x]+)")
  if not hex then
    hex = string.match(rendered, "^%s*([%x]+)%s*$")
  end
  if hex then
    hex = string.gsub(string.upper(hex), "^0+", "")
    return hex == "" and "0" or hex
  end
  return nil
end

local function address_matches(left, right)
  local left_key = address_key(left)
  local right_key = address_key(right)
  return left_key ~= nil and left_key == right_key
end

local function format_address(value)
  local key = address_key(value)
  if key then return "0x" .. key end
  return tostring(value)
end

local function function_count(map)
  return try_call(map, "count") or try_call(map, "size") or 0
end

local function function_item(map, index)
  return try_call(map, "item", index) or try_call(map, "get", index)
end

local function lower(value)
  return string.lower(value or "")
end

local function is_meta_name(name)
  local value = lower(name)
  return string.find(value, "$unwind$", 1, true)
      or string.find(value, "$chain$", 1, true)
      or string.find(value, "$cppxdata$", 1, true)
      or string.find(value, "$stateunwindmap$", 1, true)
      or string.find(value, "$ip2state$", 1, true)
      or string.find(value, "?dtor$", 1, true)
      or string.find(value, "?catch$", 1, true)
end

local function raw_anchor(entry)
  local scope, function_name_part = string.match(entry.id, "^(.+)::([^:]+)$")
  if scope and function_name_part then
    return "?" .. function_name_part .. "@" .. scope .. "@@"
  end
  return "?" .. entry.id .. "@"
end

local function candidate_score(entry, item_name, item_address)
  if not item_name or is_meta_name(item_name) then
    return -100000
  end

  local item = lower(item_name)
  local id = lower(entry.id)
  local raw = lower(entry.raw_symbol)
  local anchor = lower(raw_anchor(entry))
  local score = 0

  if item == id then score = score + 10000 end
  if item == raw then score = score + 9000 end
  if string.find(item, raw, 1, true) then score = score + 8000 end
  if string.find(item, anchor, 1, true) then score = score + 7000 end
  if string.find(item, id, 1, true) then score = score + 4000 end
  if address_matches(item_address, entry.address) then score = score + 100000 end
  return score
end

local function resolve_context()
  local candidates = {}
  local current_core = vmprotect_core()
  if current_core then table.insert(candidates, current_core) end
  local legacy_core = rawget(_G, "core")
  if legacy_core then table.insert(candidates, legacy_core) end

  for _, candidate in ipairs(candidates) do
    if candidate then
      -- VMProtect 3.9 keeps the input function map on the architecture object.
      local arch = try_call(candidate, "inputArchitecture")
      if arch then
        local map = try_call(arch, "mapFunctions")
        local functions = first_defined(try_call(arch, "functions"), try_call(arch, "intelFunctions"))
        if map and functions then
          local source = candidate == current_core
              and "vmprotect.core.inputArchitecture"
              or "legacy-core.inputArchitecture"
          return map, functions, source
        end
      end

      -- Newer VMProtect builds can expose these lists directly from Core.
      local map = try_call(candidate, "mapFunctions")
      local functions = first_defined(try_call(candidate, "functions"), try_call(candidate, "intelFunctions"))
      if map and functions then
        return map, functions, candidate == current_core and "vmprotect.core" or "legacy-core"
      end
    end
  end

  log("ERROR: VMProtect input architecture/map is unavailable. Reopen the current overlay.dll project, then compile again.")
  return nil, nil, nil
end

local function clear_protection_list(functions)
  local clear = functions and functions.clear
  if type(clear) ~= "function" then
    log("FAIL functions.clear is unavailable")
    return false
  end
  local ok, err = pcall(clear, functions)
  if not ok then
    log("FAIL functions.clear: " .. tostring(err))
    return false
  end
  return true
end

local function find_function(map, entry)
  -- Prefer generated values, but accept VMProtect's own display-name and address
  -- representation when it rebases the input image or omits MSVC raw symbols.
  for _, candidate in ipairs({ entry.raw_symbol, entry.id }) do
    local fn = try_call(map, "itemByName", candidate)
    if fn then
      local addr = function_address(fn)
      if addr then
        return fn, address_matches(addr, entry.address) and "exact-name" or "exact-name-rebased"
      end
    end
  end

  local count = function_count(map)
  local best = nil
  local best_name = nil
  local best_address = nil
  local best_score = -100000
  local tied = 0
  for i = 0, count - 1 do
    local fn = function_item(map, i)
    local item_name = function_name(fn)
    local item_address = function_address(fn)
    local score = candidate_score(entry, item_name, item_address)
    if score > best_score then
      best = fn
      best_name = item_name
      best_address = item_address
      best_score = score
      tied = 1
    elseif score == best_score and score > 0 then
      tied = tied + 1
    end
  end

  if best and best_score > 0 and tied == 1 and best_address then
    if not address_matches(best_address, entry.address) then
      log("REBASING " .. entry.id .. " expected=" .. format_address(entry.address) .. " actual=" .. format_address(best_address))
    end
    return best, "stable-anchor"
  end
  if best and best_score > 0 and tied == 1 then
    log("FAIL no address on stable anchor: " .. entry.id)
    return nil, "no-address"
  end
  if best and best_score > 0 then
    log("AMBIGUOUS " .. entry.id .. " candidates=" .. tied .. " score=" .. best_score .. " name=" .. tostring(best_name))
  end
  return nil, "miss"
end

local function protect_one(map, functions, entry)
  local fn, match_kind = find_function(map, entry)
  if not fn then
    log("MISS: " .. entry.id .. " raw=" .. entry.raw_symbol)
    return false
  end

  local ct = compilation_type(entry.protection)
  if ct == nil then
    log("SKIP unsupported protection: " .. entry.protection .. " for " .. entry.id)
    return false
  end

  local addr = function_address(fn)
  if not addr then
    log("FAIL no function address: " .. entry.id)
    return false
  end

  local add_ok, protected = pcall(functions.addByAddress, functions, addr, ct)
  if not add_ok then
    log("FAIL addByAddress: " .. entry.id .. " error=" .. tostring(protected))
    return false
  end
  if not protected then
    log("FAIL addByAddress: " .. entry.id .. " error=returned-nil")
    return false
  end
  if protected.setCompilationType then
    local ok, err = pcall(protected.setCompilationType, protected, ct)
    if not ok then
      log("FAIL setCompilationType: " .. entry.id .. " error=" .. tostring(err))
      return false
    end
  end
  log("OK " .. entry.protection .. " (" .. match_kind .. "): " .. entry.id)
  return true
end

function ApplyNoteBotProtection()
  if selection_applied then
    log("selection already applied; skipping duplicate pass")
    return true
  end

  local map, functions, api = resolve_context()
  if not map or not functions then
    return false
  end
  if not clear_protection_list(functions) then
    return false
  end

  local ok = 0
  local miss = 0
  log("function map: api=" .. api .. " count=" .. function_count(map) .. " selection=" .. #manifest)
  for _, entry in ipairs(manifest) do
    if protect_one(map, functions, entry) then
      ok = ok + 1
    else
      miss = miss + 1
    end
  end
  selection_applied = miss == 0
  log("summary: api=" .. api .. " ok=" .. ok .. " miss=" .. miss)
  return selection_applied
end

function OnBeforeCompilation()
  log("OnBeforeCompilation: applying generated selection")
  return ApplyNoteBotProtection()
end
