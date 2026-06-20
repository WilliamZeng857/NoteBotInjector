# NoteBotInjector

> 本仓库由 Codex/GPT 负责全部维护与日常修改。

NoteBotInjector 是 NoteBot 的 Windows Qt/QML 启动器项目，负责本地启动界面、更新器、V3 授权状态、模型预览，以及主 DLL 注入流程的桌面端集成。

当前仓库保存的是启动器、授权 DLL、更新器和相关构建脚本的源码快照。构建产物、缓存目录、验证截图和临时工作区不会进入 Git。

## 功能概览

- Windows 桌面启动器：基于 Qt 6 / QML 的主界面。
- V3 授权链路：由 `NoteBotAuth.dll` 负责授权状态、票据、本地状态和受保护操作。
- 主 DLL 注入流程：保留独立的注入按钮、注入阶段文本和进度状态。
- 模型库与 3D 预览：使用 `ModelPreview` 渲染模型资源，支持模型列表展示和实时预览。
- 自更新支持：构建 `NoteBotUpdater.exe`，并在 EXE 资源中内嵌 updater。
- 构建辅助工具：包含 MAP 过滤、VMP 保护计划和构建修复脚本。

## 项目结构

```text
C:\NB
├─ qml__injector_exe/          EXE 内置 QML，主界面入口
├─ resources__injector_exe/    Qt 资源、图标、shader、内嵌 updater
├─ src__injector_exe/          NoteBotInjector.exe 后端和 Qt/QML 桥
├─ src__auth_dll/              NoteBotAuth.dll 授权、状态、注入 API
├─ src__updater_exe/           NoteBotUpdater.exe 更新器
├─ tools__project_helpers/     构建、MAP、VMP 辅助脚本
├─ _admin_console_lab/         管理端实验界面
├─ build_all.bat               增量构建 DLL + EXE
├─ build_clean.bat             清理并重新配置构建
└─ build_dll.bat               只构建 Auth DLL
```

## 构建环境

当前工程按本机 Windows 构建环境组织，主要依赖：

- Visual Studio 2026 Insiders C++ 工具链
- Qt 6 静态构建：`C:\QtBuild\qt6-static`
- CMake
- Ninja
- Python

构建脚本会调用：

```bat
"C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
```

如果本机 VS 或 Qt 路径不同，需要先调整构建脚本和 `CMakeLists.txt`。

## 构建命令

日常增量构建：

```bat
build_all.bat
```

干净重建：

```bat
build_clean.bat
```

只构建授权 DLL：

```bat
build_dll.bat
```

发布产物默认输出到：

```text
dist__release_artifacts/NoteBotInjector.exe
```

授权 DLL 构建产物位于：

```text
build__auth_dll_cache/NoteBotAuth.dll
```

`build_all.bat` 会在构建前自动结束正在运行的 `NoteBotInjector.exe`，避免 EXE 被占用导致链接失败。

## 开发注意事项

- `qml__injector_exe/main.qml`、`src__injector_exe/backend.cpp` 和 `CHANGELOG.md` 是高风险文件，修改时必须小范围补丁、避免整文件重写。
- 本项目大量文件使用 UTF-8，部分文件带 BOM。编辑中文、QML、C++、Markdown 时要保留原编码和 BOM 状态。
- 不要使用 PowerShell 5.1 默认 `Get-Content | Set-Content`、`Out-File`、`>` 或 `>>` 重写源码和文档。
- 改动源码、QML、资源或构建脚本后，默认运行 `build_all.bat` 做构建验证。
- 构建缓存、发布产物、截图、`node_modules` 和临时远程工作区由 `.gitignore` 排除。

## 关键文档

- `BUILDING.md`：构建入口和产物位置说明。
- `AGENTS.md`：本项目的代理协作与中文编码安全规则。
- `CHANGELOG.md`：历史改动记录。
- `AUTH_V3_REWRITE_SPEC.md`：V3 授权重写设计说明。
- `AUTH_V3_SERVER_IMPLEMENTATION_GUIDE.md`：V3 服务端实现参考。

## 当前版本

当前 CMake 项目版本：

```text
3.6.76
```

版本发布时需要同步更新 `CMakeLists.txt`、相关资源版本信息和 `CHANGELOG.md`。
