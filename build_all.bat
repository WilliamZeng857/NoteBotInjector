@echo off
REM ============================================================
REM OFFICIAL INCREMENTAL BUILD - DLL + EXE
REM Prerequisite: run build_clean.bat once first.
REM Use build_clean.bat when:
REM   - CMakeLists.txt changed (added/removed sources, link options)
REM   - Qt path changed
REM   - build cache folders were deleted
REM   - Weird errors that smell like stale cache
REM Use build_dll.bat when you only need the DLL.
REM ============================================================

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\NB
taskkill /f /im NoteBotInjector.exe >nul 2>nul

REM Check prerequisite
if not exist build__auth_dll_cache\build.ninja (
    echo [ERR] build__auth_dll_cache/ missing or not configured. Run build_clean.bat first.
    exit /b 1
)

REM 1) Incremental DLL compile
echo [BUILD-ALL] DLL incremental compile...
ninja -j20 -C build__auth_dll_cache
if %errorlevel% neq 0 (
    echo [BUILD-ALL] DLL compile FAILED
    exit /b 1
)
echo [BUILD-ALL] DLL OK

if not exist build__injector_exe_cache\build.ninja (
    echo [ERR] build__injector_exe_cache/ missing or not configured. Run build_clean.bat first.
    exit /b 1
)

REM 2) Incremental EXE compile
echo [BUILD-ALL] EXE incremental compile...
ninja -j20 -C build__injector_exe_cache
if %errorlevel% neq 0 (
    echo [BUILD-ALL] EXE link failed, fixing response file and retrying...
    python C:\NB\tools__project_helpers\build\build_fix.py
)
if not exist build__injector_exe_cache\NoteBotInjector.exe (
    echo [BUILD-ALL] EXE compile FAILED
    exit /b 1
)
echo [BUILD-ALL] EXE OK

REM 3) Filter MAP
python C:\NB\tools__project_helpers\build\filter_map.py
python C:\NB\tools__project_helpers\build\filter_map_dll.py
copy /y build__injector_exe_cache\NoteBotInjector.exe dist__release_artifacts\NoteBotInjector.exe >nul
if exist build__injector_exe_cache\NoteBotUpdater.exe copy /y build__injector_exe_cache\NoteBotUpdater.exe dist__release_artifacts\NoteBotUpdater.exe >nul

echo.
echo ============================================================
echo [DONE] Full incremental build complete.
echo DIST EXE : dist__release_artifacts\NoteBotInjector.exe
echo DIST UPD : dist__release_artifacts\NoteBotUpdater.exe
echo BUILD EXE: build__injector_exe_cache\NoteBotInjector.exe
echo BUILD DLL: build__auth_dll_cache\NoteBotAuth.dll
echo ============================================================
