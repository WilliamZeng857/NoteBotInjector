import subprocess
import sys
import os
import shlex

os.chdir(r'C:\NB\build__injector_exe_cache')

# Step 1: ninja link-only (skip object compilation, they are already fresh)
print("[1/3] Running ninja link...")
result = subprocess.run(['ninja', '-j20', 'NoteBotInjector.exe'], capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print(result.stderr)

# Step 2: fix the response file - one token per line
# Use shlex.split to correctly handle quoted paths with spaces
rsp_path = r'C:\NB\build__injector_exe_cache\CMakeFiles\NoteBotInjector.rsp'
if os.path.exists(rsp_path):
    with open(rsp_path, 'r', encoding='utf-8') as f:
        content = f.read()
    tokens = shlex.split(content, posix=False)
    with open(rsp_path, 'w', encoding='utf-8') as f:
        for t in tokens:
            f.write(t + '\n')
    print(f"[2/3] Fixed RSP: {len(tokens)} tokens, max line = {max(len(t) for t in tokens)}")

# Step 3: manual link step bypassing ninja's rsp regeneration
print("[3/3] Running manual link step...")
link_args = [
    r'C:\Users\William\AppData\Local\Programs\Python\Python313\Lib\site-packages\cmake\data\bin\cmake.exe',
    '-E', 'vs_link_exe',
    '--msvc-ver=1950',
    r'--intdir=CMakeFiles\NoteBotInjector.dir',
    r'--rc=C:\PROGRA~2\WI3CF2~1\10\bin\100261~1.0\x64\rc.exe',
    r'--mt=C:\PROGRA~2\WI3CF2~1\10\bin\100261~1.0\x64\mt.exe',
    '--manifests',
    '--',
    r'C:\PROGRA~1\MICROS~2\18\Insiders\VC\Tools\MSVC\1450~1.357\bin\Hostx64\x64\link.exe',
    '/nologo', r'@CMakeFiles\NoteBotInjector.rsp',
    '/out:NoteBotInjector.exe',
    '/implib:NoteBotInjector.lib',
    '/pdb:NoteBotInjector.pdb',
    '/version:0.0',
    '/machine:x64',
    '/INCREMENTAL:NO',
    '/subsystem:windows',
    '/MAP:NoteBotInjector.map'
]

result = subprocess.run(link_args, capture_output=True, text=True)
print(result.stdout)
if result.stderr:
    print(result.stderr)

if result.returncode == 0:
    print("Build succeeded!")
else:
    print(f"Link failed with code {result.returncode}")

sys.exit(result.returncode)
