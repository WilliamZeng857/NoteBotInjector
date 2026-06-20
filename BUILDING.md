# NoteBotInjector 构建入口

根目录只保留三个官方构建入口：

```bat
build_clean.bat
```

干净构建。会重新生成 `build__auth_dll_cache/` 和 `build__injector_exe_cache/`，并把 EXE 复制到 `dist__release_artifacts/`。

```bat
build_dll.bat
```

增量 DLL 构建。适合只改 `src__auth_dll/` 时使用。

```bat
build_all.bat
```

增量 DLL + EXE 构建。适合改了 `src__injector_exe/` 或 EXE 启动器逻辑时使用。

EXE 发布产物看这里：

```text
dist__release_artifacts/NoteBotInjector.exe
```

DLL 只允许存在这里：

```text
build__auth_dll_cache/NoteBotAuth.dll
```

内部缓存目录：

```text
build__injector_exe_cache/  EXE 的 CMake/Ninja 构建缓存
build__auth_dll_cache/      DLL 的 CMake/Ninja 构建缓存
```

辅助脚本统一放在：

```text
tools__project_helpers/build/
```

VMP 保护计划和相关脚本统一放在：

```text
tools__project_helpers/vmp_plan/
```

源码目录：

```text
src__injector_exe/  NoteBotInjector.exe 的 C++ 源码
qml__injector_exe/  NoteBotInjector.exe 内置兜底 QML
src__auth_dll/      NoteBotAuth.dll 的 C++ / QML 源码
```
