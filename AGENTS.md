# C:\NB Agent Instructions

本文件是 `C:\NB` 项目的本地代理提示词。进入本项目后，优先遵守这里的编辑规则，尤其是中文、QML、C++、Markdown 文件的编码安全规则。

## 项目定位

- 当前目录 `C:\NB` 是 NoteBotInjector 的真实工作目录，不是普通临时副本。
- `NoteBotInjector.exe` 是 EXE 壳、更新和 UI 入口；`NoteBotAuth.dll` 是授权、诊断、安全下载、注入和 V3 状态逻辑。
- 主要源码目录：
  - `src__injector_exe/`：EXE C++ 后端、Qt/QML 桥、注入器外壳。
  - `qml__injector_exe/`：EXE 内置 QML，尤其 `main.qml`。
  - `src__auth_dll/`：Auth DLL C++、V3 RPC、授权/票据/注入逻辑。
  - `resources__injector_exe/`：Qt resource、图标、shader、内嵌 updater/helper。
- 官方构建入口：
  - `build_clean.bat`：清理并重新配置/构建 DLL + EXE。
  - `build_all.bat`：日常增量构建 DLL + EXE。
  - `build_dll.bat`：只构建 Auth DLL。

## 中文和编码硬规则

- 本项目关键文本文件目前按 UTF-8 处理，很多文件是 UTF-8 with BOM。编辑文件时必须保留原文件的编码和 BOM 状态。
- 不要用 Windows PowerShell 5.1 的默认 `Get-Content | Set-Content`、`Out-File`、重定向 `>`、`>>` 来重写源码或文档文件。
- 不要把包含中文的文件读成字符串后整文件写回，除非已经明确用字节级方式保留 UTF-8 和 BOM。
- 特别危险文件：
  - `qml__injector_exe/main.qml`
  - `src__injector_exe/backend.cpp`
  - `CHANGELOG.md`
  - 任何含大量中文 UI 文案、日志文案、QML 字符串的文件
- 小范围代码/文本编辑优先使用 `apply_patch`，让 diff 保持可审查、可回滚。
- 如果必须脚本化编辑，必须使用明确编码的 .NET API，且先备份原文件：
  - 读取：`[System.IO.File]::ReadAllText($path, [System.Text.UTF8Encoding]::new($true, $true))`
  - 写入时按原 BOM 决定使用 UTF-8 BOM 或 UTF-8 no BOM。
  - 写入后重新用严格 UTF-8 解码验证。
- 任何改动后都要检查是否出现编码为 `U+FFFD` 的替换字符或典型 mojibake 片段，例如 `锛`、`涓`。如果出现，立刻停止，不要继续扩大修改。

## 安全编辑流程

编辑前：

- 先确认目标文件真实路径，避免改到旧副本或旁边项目。
- 对 `main.qml`、`backend.cpp`、`CHANGELOG.md` 这类高风险文件，先检查编码、BOM、文件大小和最近修改时间。
- 用 `rg`、`Select-String -LiteralPath`、只读方式定位小范围上下文，不要为了搜索而重写文件。

编辑时：

- 一次只改一个明确问题，尽量限制在少量相邻行。
- 不要用全文件格式化、全文件替换、自动整理 import、自动换行重排来顺手清理。
- 不要根据注释大改逻辑。本项目存在历史乱码注释，行为和调用链优先于注释。
- QML 中不要大段移动括号结构。先定位组件边界，再做最小 patch。
- 修改中文文案时，尽量只替换目标字符串，不碰周围转义符、引号、括号和绑定表达式。

编辑后：

- 重新检查目标文件是严格有效 UTF-8。
- 检查 BOM 状态是否和编辑前一致；如果不一致，除非有明确理由，否则恢复。
- 搜索目标文件内是否出现 `U+FFFD` 或 mojibake。
- 完成任何源码、QML、资源或构建脚本更改后，默认主动运行 `build_all.bat` 完成构建验证，不要等用户手动提醒；如果确实无法构建，必须明确说明原因。
- 对 QML 改动，至少做括号/关键锚点检查，并运行 `build_all.bat`。
- 对 EXE/DLL 版本发布改动，更新对应版本号、构建、确认产物时间戳，并记录 `CHANGELOG.md`。

## PowerShell 使用边界

允许：

- `Get-ChildItem -LiteralPath ...`
- `Select-String -LiteralPath ...`
- `rg ...`
- `[System.IO.File]::ReadAllBytes(...)` 做只读编码检查
- 构建脚本、只读查询、哈希/时间戳检查

禁止用于源码/文档写入：

- `Set-Content` 默认编码
- `Out-File` 默认编码
- `>` / `>>`
- `(Get-Content file) -replace ... | Set-Content file`
- 对中文文件做整文件字符串替换

如果确实需要 PowerShell 写文件，必须满足：

- 先备份；
- 显式 UTF-8；
- 保留原 BOM；
- 写后严格 UTF-8 解码；
- 写后比较 diff；
- 写后搜索 `U+FFFD` 替换字符、`锛`、`涓` 等损坏迹象。

## 当前项目高风险经验

- `qml__injector_exe/main.qml` 过去发生过严重语法和编码损坏；修复时应小补丁、快验证，不要连续大范围替换。
- 如果 QML 解析错误开始“移动位置”，不要继续盲修。回到最近一次确定有效的结构边界，删除或隔离坏块，再逐步恢复。
- `src__auth_dll` 和 `src__injector_exe` 的职责不要混淆：EXE 负责 UI、更新、加载 DLL；Auth DLL 负责 V3 授权/票据/注入 API。
- DLL 相关改动通常需要确认 `build__auth_dll_cache/NoteBotAuth.dll` 是否真的更新；EXE 发布产物看 `dist__release_artifacts/NoteBotInjector.exe`。
- 旧备份 EXE 不一定可信，当前源码和当前构建产物优先。

## 默认响应方式

- 用户称呼是“帕帕”。正常对话中可以自然使用这个称呼，但不要为了显得亲近而过度重复。
- 面向用户默认用简体中文说明。
- 先给结论，再给关键证据和下一步。
- 如果涉及文件修改，明确说明改了哪些文件、是否构建、是否验证编码。
- 遇到中文/编码风险时，把编码验证当成正确性的一部分，而不是额外步骤。
