"""
Filter NoteBotInjector.map to only critical project functions.
Run automatically after every build.
"""

import os
import re
import shutil

MAP_DIR = r'C:\NB\build__injector_exe_cache'
MAP_FILE = os.path.join(MAP_DIR, 'NoteBotInjector.map')
BACKUP_FILE = os.path.join(MAP_DIR, 'NoteBotInjector_full.map')

CRITICAL_PATTERNS = [
    r'\?doActivate@Backend@@',
    r'\?doDiagnose@Backend@@',
    r'\?doInject@Backend@@',
    r'\?doLocalVerify@Backend@@',
    r'\?resetActivation@Backend@@',
    r'\?callDll@Backend@@',
    r'\?injectDll@Win32Injector@@',
    r'\?AesGcmDecrypt@NBAuth@@',
    r'\?AesGcmEncrypt@NBAuth@@',
    r'\?HmacSha256@NBAuth@@',
    r'\?SecureRandomBytes@NBAuth@@',
]

EXCLUDE_PATTERNS = [
    r'<lambda_',
    r'\?\$_',
    r'\?dtor\$',
    r'\?impl@\?\$QCallableObject',
]

def filter_map():
    if not os.path.exists(MAP_FILE):
        print(f'[MAP] {MAP_FILE} not found, skipping filter')
        return

    # Backup full map (only if backup doesn't exist yet)
    if not os.path.exists(BACKUP_FILE):
        shutil.copy2(MAP_FILE, BACKUP_FILE)
        print(f'[MAP] Full map backed up to {BACKUP_FILE}')

    with open(MAP_FILE, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Find header end (the "Publics by Value" section start)
    header_end = 0
    for i, line in enumerate(lines):
        if 'Publics by Value' in line:
            header_end = i + 2  # Include the blank line after the header
            break

    if header_end == 0:
        header_end = 79  # Fallback to standard MSVC map header length

    header = lines[:header_end]

    # Filter function lines
    func_lines = []
    for line in lines[header_end:]:
        if not line.startswith(' 0001:'):
            continue
        # Must match at least one critical pattern
        if not any(re.search(p, line) for p in CRITICAL_PATTERNS):
            continue
        # Must not match any exclude pattern
        if any(re.search(p, line) for p in EXCLUDE_PATTERNS):
            continue
        func_lines.append(line)

    with open(MAP_FILE, 'w', encoding='utf-8') as f:
        f.writelines(header)
        f.writelines(func_lines)

    print(f'[MAP] Filtered to {len(func_lines)} critical functions ({len(lines)} -> {len(header) + len(func_lines)} lines)')

if __name__ == '__main__':
    filter_map()
