# Model Runtime Authorization Rollout

本文件是 NoteBot 模型运行时授权功能的一口气落地计划。后续如果上下文压缩，继续工作时优先读取本文件。

## 最终目标

把旧的“自定义名称”功能彻底退役，改造成“模型授权”。

模型授权控制 `NoteBotModel.dll` 是否允许被某个密钥下载、加载和使用。它不是 EXE 启动资格，也不是本地 DLL 存在即启用。最终判断永远是：

```text
当前密钥 + 云端授权状态 > 本地 NoteBotModel.dll 文件存在状态
```

## 核心产品规则

- EXE 正常启动并进入主界面，不在进程启动早期判定模型权限。
- 进入主界面后，客户端才检查当前是否有密钥。
- 没有密钥时，不请求模型权限，不下载模型 DLL，不显示模型注入全局开关。
- 有密钥时，客户端拿密钥向服务端请求模型运行时策略。
- 服务端以密钥记录上的 `modelRuntimeEnabled` 为准。
- 已授权且服务端已有可用 `NoteBotModel.dll` 时，才返回下载地址、文件名、大小、SHA256、MD5、过期时间等信息。
- 未授权时，服务端拒绝，客户端不显示模型注入全局开关。
- 即使本地已经存在 `NoteBotModel.dll`，只要云端未授权，客户端也必须禁用模型功能。
- 授权取消后，客户端下次刷新权限时立即隐藏模型注入全局开关、停止模型监听/等待进程逻辑、尝试卸载 DLL，并删除本地 DLL 文件。
- 如果卸载 DLL 失败，客户端仍必须隐藏和禁用功能，并提示重启后彻底失效。

## 服务端目标

远端项目路径：

```text
/home/william/NoteBot-API
```

管理前端与 V3 服务端在这个项目内推进。

### 服务端数据模型

- `LicenseRecord` 保留并使用：
  - `modelRuntimeEnabled: boolean`
- “自定义名称”相关字段不再作为前端功能暴露：
  - `features.customName`
  - `custom_name`
  - `featureFlags` 第 16 位
- 为兼容旧数据，可以继续容忍旧字段存在，但前端不再使用它们管理功能。

### 服务端接口

保留/完善：

```http
POST /api/licenses/:keyId/set-model-runtime
```

请求：

```json
{ "enabled": true }
```

行为：

- 只有管理员可调用。
- 修改指定密钥的 `modelRuntimeEnabled`。
- 返回更新后的 license。
- 失败时返回明确错误，前端用于回滚按钮状态。

保留/完善：

```http
GET /api/model-runtime
POST /api/model-runtime/upload
```

用途：

- 管理台上传/查看当前 `NoteBotModel.dll`。
- 模型运行时 DLL 作为独立 artifact 管理。

保留/完善 TCP 命令：

```text
model_runtime_policy_v1
```

行为：

- 校验 key、device、签名、在线状态。
- 检查 `modelRuntimeEnabled`。
- 检查当前是否有 enabled 的 `model_runtime` artifact。
- 未授权返回 `model_runtime_not_granted`。
- 没有可用 DLL 返回 `model_runtime_artifact_missing`。
- 本地 DLL SHA256 未变化时返回 `not_modified`。
- 需要下载时返回下载 URL、SHA256、MD5、大小、ABI/协议字段。

### 服务端安全边界

- 前端隐藏按钮不是安全边界。
- 服务端必须拒绝未授权密钥获取模型 DLL 下载策略。
- 即使用户手动构造请求，也必须通过 key/device/signature 和 `modelRuntimeEnabled` 校验。
- 对象存储 URL 只由服务端策略下发，不给未授权用户。

## 管理前端目标

主要文件预计在：

```text
/home/william/NoteBot-API/frontend/src/views/KeysView.vue
```

### 替换旧自定义名称入口

原“自定义名称”按钮位置保留，功能改为“模型授权”。

按钮规则：

- 文案：`模型授权`
- 状态：`ON` / `OFF`
- 已授权：绿色/激活态
- 未授权：灰色/普通态
- hover/click 动效沿用现有按钮风格
- 点击后调用 `POST /api/licenses/:keyId/set-model-runtime`
- 请求中禁用按钮或显示处理中状态
- 成功后更新该密钥的 `modelRuntimeEnabled`
- 失败后回滚 UI，并显示错误

### 创建/编辑密钥

原“自定义名称”勾选项退役。

替换为：

```text
允许使用模型替换
```

或：

```text
允许下载模型运行时 DLL
```

推荐 UI 文案：

```text
模型授权
允许该密钥下载并启用 NoteBotModel.dll。
```

创建密钥时将该值写入 `modelRuntimeEnabled`。
编辑密钥时修改 `modelRuntimeEnabled`。

### 列表显示

密钥列表中显示：

- `模型授权 ON`
- `模型授权 OFF`

不要再显示“自定义名称”。

## 客户端目标

本地项目路径：

```text
C:\NB
```

关键方向：

- 客户端以现有模型渲染/模型选择 UI 为基准。
- 模型预览和模型选择不因为未授权而消失。
- 消失/出现的是“模型注入全局开关”和实际模型运行时注入能力。

### 启动时机

客户端启动流程必须保持：

1. EXE 正常启动。
2. 初始化 UI。
3. 进入主界面。
4. 读取当前密钥状态。
5. 如果有密钥，再请求模型运行时策略。
6. 根据云端策略决定是否下载/加载/显示模型注入开关。

不得在 EXE 进程启动早期强行加载 `NoteBotModel.dll`。
不得因为本地存在 `NoteBotModel.dll` 就直接显示模型注入开关。

### 授权通过

服务端返回可下载策略后：

1. 如果本地没有 DLL，下载。
2. 如果本地 DLL SHA256 不匹配，重新下载。
3. 校验 SHA256。
4. 加载 DLL。
5. 加载成功后显示模型注入全局开关。
6. 用户打开全局开关后，才开始等待新的游戏进程。

### 未授权或授权取消

如果服务端返回未授权：

1. 隐藏模型注入全局开关。
2. 停止模型监听/等待进程逻辑。
3. 尝试卸载已加载的 `NoteBotModel.dll`。
4. 删除本地 `NoteBotModel.dll`。
5. 如果卸载失败，仍保持功能隐藏和禁用，并提示用户重启后彻底失效。

### 进行中注入边界

如果已经处于一次模型注入流程中：

- 用户关闭总闸时，本次如果已经进入不可安全中断阶段，可以完成本次流程，再停止下一轮。
- 如果只是等待新游戏进程，关闭总闸必须立刻停止等待。
- 授权被取消时优先停止下一轮，若能安全中止则中止，不能安全中止则完成当前流程后进入禁用状态。

### 本地 DLL 路径

`NoteBotModel.dll` 不应作为 EXE 同目录固定依赖。

推荐存放在受控缓存目录，例如：

```text
%LOCALAPPDATA%\NoteBotInjector\model_runtime\NoteBotModel.dll
```

实际实现以当前客户端已有路径策略为准，但必须满足：

- 不在 EXE 启动早期自动加载。
- 未授权时可删除。
- 下载后校验 SHA256。

## 构建与验证

### 服务端

必须运行：

```bash
cd /home/william/NoteBot-API
npm --prefix server_v3 run build
npm --prefix frontend run build
```

需要验证：

- 管理前端不白屏。
- 密钥管理页面不再出现“自定义名称”。
- 原按钮位置显示“模型授权 ON/OFF”。
- 切换模型授权后服务端 license 字段变化。
- 未登录访问管理接口返回 401，而不是 500。
- server_v3 进程重启后正常启动。

### 客户端

必须运行：

```bat
C:\NB\build_all.bat
```

需要验证：

- 中文/UTF-8 没有乱码。
- QML 能构建。
- 主 DLL 注入不受影响。
- 模型预览页面不受影响。
- 未授权时模型注入全局开关不显示。
- 授权时模型注入全局开关在 DLL 下载/加载成功后显示。
- 授权取消后按钮隐藏、监听停止、DLL 尝试卸载并删除。

## Git 规则

本地 `C:\NB`：

- 按 `AGENTS.md`，构建成功后提交并推送。
- 只提交本轮相关文件。
- 不提交临时调试文件、备份文件、截图、无关产物。

远端 `/home/william/NoteBot-API`：

- 当前分支可能是 `safe-backup`。
- 提交前必须检查工作区已有脏文件，避免混入 DLL、备份、脚本、私密数据。
- 如果 GitHub push 因网络失败，保留本地 commit，并明确说明失败原因。

## 实施顺序

1. 审计远端 `KeysView.vue` 和 `admin.ts` 中自定义名称逻辑。
2. 将管理前端自定义名称按钮替换为模型授权按钮。
3. 将创建/编辑密钥表单里的自定义名称替换为模型授权字段。
4. 确认服务端 `set-model-runtime` 接口和 license 序列化完整。
5. 构建远端 frontend/server_v3。
6. 重启 server_v3 并验证日志。
7. 审计本地客户端模型运行时加载/开关逻辑。
8. 实现主界面后模型授权刷新、下载、校验、加载、隐藏、卸载、删除。
9. 构建本地客户端。
10. 编码检查、乱码检查、提交、推送。

## 不允许做的事

- 不要恢复旧“自定义名称”入口。
- 不要让本地 DLL 存在状态绕过云端授权。
- 不要在 EXE 启动早期加载模型 DLL。
- 不要删除主 DLL 注入链路。
- 不要影响 `backend.doInject()`、主 DLL 注入进度条、主注入按钮。
- 不要隐藏模型预览/模型选择页面。
- 不要使用 PowerShell 默认编码重写中文文件。

