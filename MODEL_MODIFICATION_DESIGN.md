# 启动阶段模型替换设计文档

本文档记录 `NoteBotInjector` 新版模型替换功能的产品语义、架构边界和实现计划。后续实现必须以本文档为目标锚点；如果上下文被压缩，代理需要先读本文档再继续工作。

## 背景

旧版皮肤/模型功能已经从 `C:\NB` 中删除，原因是交互难用、边界混乱，并且容易和主 DLL 注入、3D 模型预览混在一起。旧入口包括 `skinPreviewBtn`、`skinPanelRestored`、`NoteBotClassicSkinPatcher.exe` helper 链路和相关愤怒特效。

当前要重新实现的不是旧按钮复活，而是一个新的“启动阶段模型替换”功能：

- EXE 负责 UI、模型列表、资源缓存、3D 预览、按授权决定是否加载模型功能 DLL。
- 新的模型功能 DLL 负责核心模型替换逻辑。
- 主 DLL 注入链继续保留，不被模型替换功能影响。
- 3D 模型预览继续保留，不受模型替换总闸影响。

## 名称和文案

不要把功能叫做“游戏内模型修改”。这个说法不准确，因为该功能不能在游戏已经完全进入后随时修改模型；它必须在 Minecraft 启动阶段、登录包序列化链路触发时及时替换字段才有效。

推荐文案：

- 功能名称：`启动阶段模型替换`
- 总闸标签：`启动阶段模型替换`
- 左侧总闸：唯一运行开关
- 右侧状态：只显示当前状态，不作为按钮
- 状态文案：
  - `已关闭`
  - `等待密钥验证`
  - `等待模型资源`
  - `等待选择模型`
  - `等待 Minecraft 启动`
  - `正在准备替换`
  - `修改点已安装`
  - `登录包已命中`
  - `本次替换完成`
  - `停止中`
  - `已停止`
  - `失败`

## UI 位置

控制区放在主 `INJECT` 按钮上方，紧贴主注入按钮。

原因：

- 位置足够可见，用户不会漏掉总闸状态。
- 仍然和主 DLL 注入按钮保持视觉关系，用户能理解它影响“启动/注入前后的游戏效果”。
- 文案和按钮必须清楚表达它不等于主 DLL 注入，也不影响主 `INJECT`。

推荐 UI 形态：

- 一个紧凑控制条，不做大型卡片。
- 左侧显示总闸，用户只通过这个总闸开启/关闭模型替换功能。
- 右侧显示当前状态 chip，不可点击，不承担开始/停止按钮职责。
- 不挤压主注入按钮，不破坏现有主注入进度条。

## 总闸语义

后端属性建议命名为：

```text
modelModificationEnabled
```

用户可见文案使用 `启动阶段模型替换`。

总闸规则：

- 默认关闭。
- 持久化到 `QSettings("NoteBot", "Injector")`。
- 关闭时不清空当前已选模型。
- 关闭时不删除模型缓存。
- 关闭时不隐藏模型页。
- 关闭时不影响 3D 预览。
- 关闭时不影响模型列表刷新、模型授权获取和资源下载。
- 关闭时禁止开始新的启动阶段替换流程。
- 关闭时如果正在等待新 Minecraft 进程，必须立刻停止等待。
- 关闭时如果已经进入本次模型替换流程，不要硬切正在写入的本轮；本轮完成后停止，不再进入下一轮。
- 开启时不需要用户再点第二个按钮；密钥验证和模型资源准备完成后自动进入等待新 Minecraft 进程。
- 如果开启时条件未齐备，状态显示在等待密钥验证、等待模型资源或等待选择模型；条件齐备后自动开始等待。

## 启动和停止语义

总闸开启就代表运行意图开启。

如果上次退出前 `modelModificationEnabled=true`，下次启动后恢复为：

```text
总闸开启，等待密钥验证或等待 Minecraft 启动
```

不再需要用户手动点击 `等待下次启动`。程序完成密钥验证、模型授权和资源准备后，会自动等待新的 `Minecraft.Windows.exe`。

停止语义：

- 如果当前处于等待新进程阶段，关闭总闸必须立刻结束等待。
- 如果当前处于 resolver 探测但尚未开始写入阶段，应尽快结束。
- 如果当前已经开始本次替换写入/安装 hook，不要强行中断本轮，避免半写状态；应设置停止请求，让本轮完成后退出循环。
- 本轮结束后不再继续等待下一次 Minecraft 启动。
- 停止后保留日志，并追加类似 `[MODEL] 已停止等待` 的日志。
- 停止后不清空模型选择。

## 与主 DLL 注入的边界

主 DLL 注入链必须保持独立：

- UI 主注入按钮 `injectBtnRestored` 继续调用 `backend.doInject()`。
- `injectProgress`、`injectStageText`、`backend.doInject()` 和 Auth DLL 导出链不受影响。
- 即使启动阶段模型替换总闸关闭，主 `INJECT` 也必须照常可用。
- 点击主 `INJECT` 不应自动开始模型替换等待。
- 开启模型替换总闸不应自动执行主 DLL 注入。

## 与模型页和 3D 预览的边界

模型页继续作为授权模型列表和 3D 预览入口：

- 总闸关闭时模型页不变化。
- 总闸关闭时仍允许选模型。
- 总闸关闭时仍允许查看 3D 预览。
- 总闸关闭时仍允许刷新模型授权和下载模型资源。
- 模型卡片显示 active/owned 状态的现有逻辑不需要因为总闸关闭而改变。

3D 渲染在 EXE 内继续保留：

- `ModelPreview` 和 `ModelPreviewItem` 只负责预览。
- 不要把启动阶段替换的业务状态塞进 `ModelPreviewItem`。
- 渲染逻辑不进入新模型功能 DLL。

## 进程处理策略

只支持新启动的 `Minecraft.Windows.exe`。

第一版不支持“附加现有进程”：

- 不提供高级开关。
- 不对已经运行的 Minecraft 生效。
- 等待流程只认模型替换总闸开启并进入等待状态之后出现的新进程。

成功一次后：

- 如果总闸仍开启，并且用户没有停止，则继续等待下一次 Minecraft 启动。
- 如果用户关闭总闸，则本轮完成后退出，不再继续等待。

## 成功状态分层

不要把“安装 hook 成功”和“最终生效”混为一谈。

状态至少区分：

- 已找到新进程。
- 目标模块已就绪。
- resolver 已定位替换点。
- 修改点已安装。
- 登录包已命中。
- 本次替换完成。

旧 Python 的 `self-check login payload` 含义：

- hook 命中后，从目标进程内存只读扫描发送端登录 payload。
- 用来确认最终 payload 中是否真的出现了 `SkinData`、`SkinGeometryData`、`SkinResourcePatch`、`SkinAnimationData` 等字段。
- 这是内部验证能力，不是用户显式功能。
- UI 第一版只显示简短验证结果，不展示大段 payload。

## 授权和新 DLL

核心模型替换逻辑需要放入新的 DLL，而不是长期放在 EXE 或 Python helper 中。

推荐模块名：

```text
NoteBotModel.dll
```

职责划分：

- EXE：
  - UI 控制条。
  - 3D 预览。
  - 模型列表和模型资源缓存。
  - 判断用户是否拥有模型功能授权。
  - 按授权决定是否下载/加载 `NoteBotModel.dll`。
  - 调用 DLL 导出函数开始/停止等待。
- `NoteBotModel.dll`：
  - 等待新 Minecraft 进程。
  - 定位 `Minecraft.Windows.exe` 模块。
  - name-anchored hook resolver。
  - 远程内存分配和写入。
  - std::string 远程对象构造。
  - 跳板 shellcode 生成。
  - hook 安装。
  - 命中计数读取。
  - 简短 self-check。

授权策略：

- 没有模型功能授权时，EXE 不主动拉取 `NoteBotModel.dll`。
- 服务端也不允许未授权用户拉取 `NoteBotModel.dll`。
- 已加载 DLL 后，功能应能独立运行，不依赖 Python。
- 这部分授权下载链路可以后续接入；第一阶段可以先实现本地 DLL 架构和 EXE 调用边界。

## Python 逻辑迁移原则

旧参考实现：

```text
D:\Downloads\cheatengine-mcp-bridge-main\cheatengine-mcp-bridge-main\tools\skin_capture\classic_login_skin_patcher.py
```

迁移要求：

- 逻辑必须保持一致。
- 可以改语言、文件结构和接口，但不要自作聪明改变语义。
- resolver、等待流程、hook 安装、字段覆盖、时机判断必须对齐旧 Python。
- 特别是 `HOOKS`、`HOOK_LOCATORS`、`GROUP_REQUIREMENTS`、callsite 兼容检查、字段锚点查找和 `call_rel32_drift` 接受逻辑要保持等价。
- 不要把 name-anchored resolver 退化成纯固定 RVA。
- 不要删掉未来支持 `SkinAnimationData`、`PersonaPieces`、普通方块人只换贴图的接口空间。

## 第一版核心功能范围

第一版目标是让架构成型，并保留旧 Python 的核心模型替换能力。

必须保留的核心链路：

- `SkinId`
- `SkinData`
- `SkinImageWidth`
- `SkinImageHeight`
- `SkinResourcePatch`
- `SkinGeometryData`
- `SkinGeometryDataEngineVersion`
- `ArmSize`
- `TrustedSkin`
- `PremiumSkin`
- `PersonaSkin`

暂不作为第一版 UI 重点，但接口不能堵死：

- `SkinAnimationData`
- `PersonaPieces`
- 普通方块人只修改贴图、不使用自定义 geometry 的路线

## 实现建议

建议分层：

```text
src__model_dll/
  include/notebot_model_api.h
  src/model_api.cpp
  src/model_worker.cpp
  src/model_worker.h
  src/win_process.cpp
  src/win_process.h
  src/hook_resolver.cpp
  src/hook_resolver.h
  src/remote_memory.cpp
  src/remote_memory.h
  src/skin_payload.cpp
  src/skin_payload.h
```

EXE 侧建议新增：

```text
src__injector_exe/modelreplacementcontroller.h
src__injector_exe/modelreplacementcontroller.cpp
```

或者先集成在 `Backend` 中，等功能稳定后再拆 controller。考虑到 `backend.cpp` 已经很大，优先新增 controller 更清晰。

QML/Backend 属性建议：

```text
Q_PROPERTY(bool modelModificationEnabled READ modelModificationEnabled WRITE setModelModificationEnabled NOTIFY modelModificationEnabledChanged)
Q_PROPERTY(bool modelReplacementRunning READ modelReplacementRunning NOTIFY modelReplacementRunningChanged)
Q_PROPERTY(QString modelReplacementStatus READ modelReplacementStatus NOTIFY modelReplacementStatusChanged)

Q_INVOKABLE void startModelReplacementWait()
Q_INVOKABLE void stopModelReplacementWait()
```

持久化 key 建议：

```text
modelModificationEnabled
activeModel:<licenseKey>
```

## 验证要求

每轮源码/QML/资源/构建脚本改动后必须：

- 严格 UTF-8 解码检查。
- 保留原 BOM 状态。
- 搜索 `U+FFFD`。
- 搜索典型 mojibake 片段。
- 对 QML 改动做结构检查。
- 运行 `cmd /c C:\NB\build_all.bat`。
- 构建成功后自动提交本地 git。

如果 `CMakeLists.txt` 新增 target 后增量构建缓存不识别，改用：

```bat
cmd /c C:\NB\build_clean.bat
```

## 不做的事

第一版不做：

- 恢复旧 `NoteBotClassicSkinPatcher.exe` helper 链路。
- 恢复旧 `skinPreviewBtn` 和 `skinPanelRestored`。
- 附加已有 Minecraft 进程。
- 把 3D 预览移入模型 DLL。
- 让主 `INJECT` 自动触发模型替换等待。
- 大段展示 self-check payload。
