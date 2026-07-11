"""
Filter NoteBotAuth.map: keep only real business functions, drop everything else.
Run after DLL build to generate a clean MAP for VMP.
"""

import os
import re
import shutil

MAP_DIR = r'C:\NB\build__auth_dll_cache'
MAP_FILE = os.path.join(MAP_DIR, 'NoteBotAuth.map')
BACKUP_FILE = os.path.join(MAP_DIR, 'NoteBotAuth_full.map')
LATEST_FULL_FILE = os.path.join(MAP_DIR, 'NoteBotAuth.full.latest.map')

# Only KEEP these project source files
PROJECT_OBJ_FILES = {
    'api.cpp.obj',
    'crypto_utils.cpp.obj',
    'protected_download_ops.cpp.obj',
    'protected_name_ops.cpp.obj',
    'protected_ticket_ops.cpp.obj',
    'protected_verify_ops.cpp.obj',
    'protected_v3_ops.cpp.obj',
    'name_injector.cpp.obj',
}

# Compiler-generated junk — DROP
JUNK_PATTERNS = [
    r'^nb_',                       # All C exports
    r'<lambda_',
    r'\?dtor\$',
    r'\?ctor\$',
    r'\?_Copy@',
    r'\?_Move@',
    r'\?_Delete_this@',
    r'\?_Do_call@',
    r'\?_Get@',
    r'\?_Target_type@',
    r'\?impl@\?\$QCallableObject',
    r'\?\$_Invoke@',
    r'\?\$_Call@',
    r'\?\$_Apply@',
    r'\?\$_Func_impl',
]

def is_valid_function(line):
    if not line.startswith(' 0001:'):
        return False
    parts = line.strip().rsplit(' ', 1)
    if len(parts) < 2:
        return False
    obj_file = parts[-1]
    if obj_file not in PROJECT_OBJ_FILES:
        return False
    # Extract function name (second token)
    tokens = parts[0].strip().split()
    if len(tokens) < 2:
        return False
    name = tokens[1]
    for pat in JUNK_PATTERNS:
        if re.search(pat, name):
            return False
    if 'NBVmp_' in name:
        return True
    # Must be in our own namespaces; generated/runtime noise is dropped.
    if 'NBAuth' not in name and 'NBName' not in name:
        return False
    return True

def filter_map():
    if not os.path.exists(MAP_FILE):
        print(f'[MAP-DLL] {MAP_FILE} not found, skipping filter')
        return

    shutil.copy2(MAP_FILE, LATEST_FULL_FILE)
    print(f'[MAP-DLL] Latest full map saved to {LATEST_FULL_FILE}')

    if not os.path.exists(BACKUP_FILE):
        shutil.copy2(MAP_FILE, BACKUP_FILE)
        print(f'[MAP-DLL] First full map backed up to {BACKUP_FILE}')

    with open(MAP_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    header_end = 0
    for i, line in enumerate(lines):
        if 'Publics by Value' in line:
            header_end = i + 2
            break
    if header_end == 0:
        header_end = 79

    header = lines[:header_end]

    func_lines = []
    for line in lines[header_end:]:
        if is_valid_function(line):
            func_lines.append(line)

    with open(MAP_FILE, 'w', encoding='utf-8') as f:
        f.writelines(header)
        f.writelines(func_lines)

    print(f'[MAP-DLL] Filtered: {len(lines)} -> {len(header) + len(func_lines)} lines')
    print(f'[MAP-DLL] Kept {len(func_lines)} clean functions')

    by_file = {}
    for line in func_lines:
        parts = line.strip().rsplit(' ', 1)
        if len(parts) >= 2:
            obj = parts[-1]
            by_file[obj] = by_file.get(obj, 0) + 1

    print('\n[MAP-DLL] Functions by source file:')
    for obj, count in sorted(by_file.items(), key=lambda x: -x[1]):
        print(f'  {obj}: {count}')

if __name__ == '__main__':
    filter_map()
