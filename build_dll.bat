@echo off
REM ============================================================
REM OFFICIAL INCREMENTAL BUILD - DLL ONLY
REM Prerequisite: run build_clean.bat once first.
REM Use this when only src__auth_dll/ changed and EXE does not need relinking.
REM ============================================================

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\NB

if not exist build__auth_dll_cache\build.ninja (
    echo [ERR] build__auth_dll_cache/ missing or not configured. Run build_clean.bat first.
    exit /b 1
)

echo [BUILD-DLL] DLL incremental compile...
ninja -j20 -C build__auth_dll_cache
if %errorlevel% neq 0 (
    echo [BUILD-DLL] DLL compile FAILED
    exit /b 1
)

python C:\NB\tools__project_helpers\build\filter_map_dll.py

echo.
echo ============================================================
echo [DONE] DLL incremental build complete.
echo BUILD DLL: build__auth_dll_cache\NoteBotAuth.dll
echo ============================================================
