@echo off
REM ============================================================
REM OFFICIAL FULL BUILD - clean cache and rebuild everything
REM Use this when:
REM   - First time build
REM   - CMakeLists.txt changed (added/removed sources, link options)
REM   - Qt path changed
REM   - Weird errors that smell like stale cache
REM For daily small changes use build_dll.bat or build_all.bat.
REM ============================================================

call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /d C:\NB

echo [DEBUG] Current dir: %CD%
echo [DEBUG] cl.exe path:
where cl.exe

if exist build__auth_dll_cache rmdir /s /q build__auth_dll_cache
mkdir build__auth_dll_cache
set "NB_AUTH_VMP_CMAKE_FLAGS="
if /I "%NB_AUTH_ENABLE_VMP_MARKERS%"=="1" set "NB_AUTH_VMP_CMAKE_FLAGS=%NB_AUTH_VMP_CMAKE_FLAGS% -DNB_AUTH_ENABLE_VMP_MARKERS=ON"
if not "%NB_VMP_SDK_DIR%"=="" set "NB_AUTH_VMP_CMAKE_FLAGS=%NB_AUTH_VMP_CMAKE_FLAGS% -DNB_VMP_SDK_DIR=""%NB_VMP_SDK_DIR%"""
cmake -B build__auth_dll_cache -S src__auth_dll -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/QtBuild/qt6-static -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded %NB_AUTH_VMP_CMAKE_FLAGS%
ninja -j20 -C build__auth_dll_cache
if %errorlevel% neq 0 (
    echo [BUILD] DLL compile FAILED
    exit /b 1
)
echo [BUILD] NoteBotAuth.dll compiled OK

if exist build__injector_exe_cache rmdir /s /q build__injector_exe_cache
mkdir build__injector_exe_cache
cmake -B build__injector_exe_cache -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/QtBuild/qt6-static -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
if %errorlevel% neq 0 (
    echo [BUILD] EXE/Updater/Overlay CMake configure FAILED
    exit /b 1
)

ninja -j20 -C build__injector_exe_cache
if %errorlevel% neq 0 (
    echo [BUILD FIX] Ninja link failed, fixing response file and retrying...
    python C:\NB\tools__project_helpers\build\build_fix.py
    if %errorlevel% equ 0 (
        echo Build succeeded!
        python C:\NB\tools__project_helpers\build\filter_map.py
    )
) else (
    echo Build succeeded!
    python C:\NB\tools__project_helpers\build\filter_map.py
)

python C:\NB\tools__project_helpers\build\filter_map_dll.py

if not exist dist__release_artifacts mkdir dist__release_artifacts
copy /y build__injector_exe_cache\NoteBotInjector.exe dist__release_artifacts\NoteBotInjector.exe >nul
if exist build__injector_exe_cache\NoteBotUpdater.exe copy /y build__injector_exe_cache\NoteBotUpdater.exe dist__release_artifacts\NoteBotUpdater.exe >nul
if exist build__injector_exe_cache\NoteBotModel.dll copy /y build__injector_exe_cache\NoteBotModel.dll dist__release_artifacts\NoteBotModel.dll >nul

echo.
echo ============================================================
echo [DONE] Build complete.
echo DIST EXE : dist__release_artifacts\NoteBotInjector.exe
echo DIST UPD : dist__release_artifacts\NoteBotUpdater.exe
echo DIST MDL : dist__release_artifacts\NoteBotModel.dll
echo BUILD EXE: build__injector_exe_cache\NoteBotInjector.exe
echo BUILD DLL: build__auth_dll_cache\NoteBotAuth.dll
echo ============================================================
