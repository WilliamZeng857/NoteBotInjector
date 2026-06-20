# NoteBot V3 授权、更新与注入票据系统重写规格书

## 0. 文档状态

- 状态：实施规格书
- 作用：本文件是 V3 重写的唯一设计基线
- 当前阶段：先写文档，后清空旧验证源码，再按文档重建
- 当前轮次不做的事：不删除源码、不修改线上服务端行为、不切线上旧用户

本文件不是思路摘要，而是后续删除、重建、联调、灰度、验收的唯一准绳。  
后续实现时，所有“删什么、留什么、接口叫什么、文件放哪、字段长什么样、状态机怎么跑、上线怎么灰度”，都以本文件为准。

---

## 1. Summary

本次 V3 重写按三层分工进行：

1. `EXE`
   - 启动器
   - 渲染 / UI
   - 下载更新总控
   - 自更新总控
   - 进程扫描
   - 拉起注入

2. `NoteBotAuth.dll`
   - 授权核心
   - 设备身份
   - 本地状态
   - 服务端协议
   - 心跳
   - 票据请求
   - wrapper 生成
   - 结果回报

3. 游戏内业务 `DLL`
   - 离线验票
   - 校验自身
   - 放行功能
   - 回写结果

本次允许更新 `EXE`。更新后：

- `EXE` 负责自己联网检查更新
- `EXE` 负责下载新 `EXE`
- `EXE` 负责释放和维护极小的 `updater.exe`
- `EXE` 负责拉起 `updater.exe` 完成替换
- `EXE` 负责下载与替换 `NoteBotAuth.dll`
- 核心授权与票据逻辑仍然优先放在 `NoteBotAuth.dll`

V3 与 V2 完全旁路：

- 新命令
- 新目录
- 新数据文件
- 新票据格式
- 新设备身份模型

旧文件不读、不迁移、不解释。

---

## 2. 当前上下文与已确认事实

### 2.1 本地客户端工程

- 根目录：`C:\NB`
- EXE 工程：`C:\NB\src__injector_exe`
- 授权 DLL 工程：`C:\NB\src__auth_dll`

### 2.2 服务端工程

- 根目录：`/home/william/NoteBot-API`
- 当前服务端：Node.js
- 当前主通讯：TCP + AES-256-GCM

### 2.3 当前 EXE 的授权 DLL 加载位置

当前 EXE 从以下路径加载授权 DLL：

- `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`

V3 继续沿用这个文件名和这个加载路径。

### 2.4 当前本地已知残留

当前本机残留至少包括：

- `%LOCALAPPDATA%\NoteBotInjector\license.dat`
- `%LOCALAPPDATA%\NoteBotInjector\license_v2.dat`（逻辑存在）
- `%LOCALAPPDATA%\NoteBotInjector\dlls\auth.dat`
- `%LOCALAPPDATA%\NoteBotInjector\dlls\...`
- `%LOCALAPPDATA%\NoteBotInjector\cache\qmlcache\...`
- `%LOCALAPPDATA%\NoteBotInjector\NoteBotAuth.dll`

### 2.5 当前旧服务端命令

已确认旧命令包括：

- `auth_v2`
- `license_migrate`
- `diagnose_v2`
- `inject_report`
- `auth_dll_bootstrap`
- `dll_download_secure`
- `dll_update_check_v2`

### 2.6 当前旧客户端污染点

旧客户端污染点主要集中在：

- `C:\NB\src__auth_dll\src\api.cpp`
- `C:\NB\src__auth_dll\src\auth\cloud_client_tcp.cpp`
- `C:\NB\src__auth_dll\src\auth\license_file_manager.cpp`
- `C:\NB\src__auth_dll\src\auth\mutual_verify.cpp`
- `C:\NB\src__injector_exe\backend.cpp`

### 2.7 当前已确认的本地代码边界

目前已经确认：

- `EXE` 以后允许继续更新
- `updater.exe` 采用方案 A，只做本地替换器，不联网
- UI 全归 `EXE`
- 授权核心全归 `NoteBotAuth.dll`
- `EXE <-> DLL` 采用“最小导出 + nb_call(action, json)”
- 授权状态回传采用单个 JSON 快照
- 游戏内业务 `DLL` 不联网、不续票、不做心跳

---

## 3. V3 要解决的问题

### 3.1 V2 的核心结构问题

当前 V2 已经出现以下结构性问题：

1. 本地 `license_v2.dat` 和旧 `auth.dat` 共同参与验证，状态来源混乱。
2. 服务端修改 tier 或 feature 后，客户端可能继续信任旧本地状态，导致等级刷新滞后。
3. EXE、授权 DLL、游戏内业务 DLL 的职责边界不清，验证链纠缠。
4. 旧协议、旧文件、旧目录残留容易污染新状态。
5. 旧 `diagnose_v2` 同时承担“校验、修复、下发 DLL 信息”等多重职责，语义过重。
6. EXE 与游戏 DLL 之间基于旧固定文件桥的实现不够稳，也不利于灰度升级。
7. 服务端当前 V2 设备模型和本地 token 模型耦合过深，不适合继续修补。

### 3.2 V3 的根目标

V3 必须解决的是结构问题，而不是补几个分支：

1. V3 与 V2 完全旁路。
2. 新验证逻辑不解释旧文件，不迁移旧状态，不复用旧 token。
3. 新系统所有验证围绕“设备身份 + 在线状态 + 单次注入票据”展开。
4. EXE 和游戏 DLL 的文件桥重做，旧 `auth.dat` 彻底作废。
5. 服务端负责设备身份、在线状态、业务 DLL 策略、单次注入票据。
6. 游戏 DLL 完全离线运行，不做长期联网验证。
7. 下载链路保留，但重新接入 V3。
8. 先灰度测试，再全量切换。

### 3.3 用户体验目标

V3 的用户体验目标固定如下：

1. 正常用户第一次输入 key 就能激活。
2. 同一个 key 默认允许 2 台设备。
3. 同一个 key 同时只允许 1 台设备在线。
4. 第 3 台设备激活时自动替换最久未使用设备，但不能无限频繁切换。
5. 断网时可以打开注入器看状态，但不能检查、不能注入、不能拿新票据。
6. 游戏 DLL 注入后不需要长期联网。
7. 旧目录和旧文件残留不会影响 V3。

---

## 4. 设计总原则

### 4.1 V3 与 V2 完全旁路

V3 不得解释或兼容以下任何旧状态：

- `license.dat`
- `license_v2.dat`
- `auth.dat`
- 旧 `token_wire`
- 旧 `device_id`
- 旧 `diagnose_v2` 语义

V3 只能读写自己的：

- 新目录
- 新文件
- 新命令
- 新服务端数据

### 4.2 第一动作是写文档，不是删源码

后续真正进入施工阶段时，固定顺序为：

1. 审核本文件
2. 对照本文件清空旧验证源码
3. 重建客户端 V3 骨架
4. 重建服务端 V3 通道
5. 做特殊 key 灰度
6. 稳定后全量切换

### 4.3 安全根边界

V3 有两层保护：

1. 服务端签名票据：真正决定是否放行
2. `Auth DLL` 外层包装与加密：只负责增加逆向成本

必须明确：

- 外层 AES-GCM 包装不是安全根
- wrapper JSON 不是安全根
- `EXE` 本地附加字段不是安全根

真正的安全根只有：

- 服务端私钥签出的内层票据
- 游戏 DLL 内置服务端公钥验签

### 4.4 游戏 DLL 不长期联网

游戏 DLL 在注入后完全离线：

- 不做心跳
- 不做续票
- 不做在线状态同步
- 只读本地票据文件
- 只做本地验证

### 4.5 职责分工

服务端负责：

- key 管理
- 设备管理
- 在线状态管理
- DLL 策略下发
- 单次注入票据签发
- 注入结果审计
- 更新清单下发

客户端 EXE 负责：

- UI / 渲染
- 文件更新
- 自更新
- 下载管理
- 进程扫描
- 注入触发
- 调 Auth DLL

Auth DLL 负责：

- V3 状态机
- 本地状态文件
- 服务端协议
- 设备身份
- 心跳
- 票据申请
- wrapper 生成
- 结果回报

游戏 DLL 负责：

- 读取票据文件
- 解外层包装
- 验服务端签名
- 校验自身 DLL hash
- 放行或拒绝功能
- 写结果文件

---

## 5. Update Architecture

### 5.1 更新总控归 EXE

V3 中，所有文件更新动作统一归 `EXE`。

`EXE` 负责：

- 更新自己
- 更新 `updater.exe`
- 更新 `NoteBotAuth.dll`
- 下载目标业务 DLL

`Auth DLL` 只负责返回：

- 当前授权协议版本
- 当前授权状态快照
- 当前需要的服务端动作
- 当前业务 DLL 策略
- 当前票据与结果状态

### 5.2 自更新方案

`EXE` 自更新采用方案 A。

固定流程：

1. `EXE` 自己联网
2. `EXE` 自己拉更新清单
3. `EXE` 自己下载新主程序
4. `EXE` 拉起 `updater.exe`
5. `updater.exe` 等待主程序退出
6. `updater.exe` 覆盖旧主程序
7. `updater.exe` 重启新主程序
8. `updater.exe` 退出

### 5.3 updater.exe 的职责边界

`updater.exe` 是极小的本地替换器。

它只负责：

- 等主 EXE 退出
- 备份旧文件
- 覆盖文件
- 校验替换结果
- 重启主 EXE
- 退出

它明确不负责：

- 联网
- 授权
- 心跳
- 设备身份
- 票据
- 游戏 DLL

### 5.4 updater.exe 路径

固定路径：

- `%LOCALAPPDATA%\NoteBotInjector\updater\NoteBotUpdater.exe`

### 5.5 启动更新顺序

V3 中，EXE 启动后的更新顺序固定为：

1. 检查 / 释放 `updater.exe`
2. 检查主 `EXE`
3. 若主 `EXE` 需要更新，则下载并交给 `updater.exe`
4. 主 `EXE` 更新完成后，再检查 `NoteBotAuth.dll`
5. `NoteBotAuth.dll` 更新完成后，才进入 UI 与授权状态机

### 5.6 更新清单

服务端更新清单统一维护三类产物：

- `EXE`
- `Auth DLL`
- 游戏内业务 `DLL`

更新清单必须支持三层筛选：

- 按 key
- 按渠道
- 按协议版本

### 5.7 版本规则

`EXE` 和 `Auth DLL` 的版本规则固定如下：

- 都有独立版本号
- 版本格式统一为同一套三段式语义版本
- 两者在同一发布批次里可以一起发
- 但编号各自独立递增
- 版本必须同时写入：
  - 文件资源
  - 服务端更新清单
  - 日志

更新器读取版本时明确禁止：

- 为了取版本先 `LoadLibrary`

更新检查只允许读取：

- 文件版本资源
- 文件 hash
- 文件大小

---

## 6. EXE 与 Auth DLL 的接口

### 6.1 UI 归属

V3 中，所有授权相关界面和渲染全部归 `EXE`。

明确约束：

- `Auth DLL` 不再提供 QML
- `Auth DLL` 不再承载渲染逻辑
- `EXE` 独占 UI 状态展示和交互

### 6.2 接口形状

V3 中，`EXE <-> Auth DLL` 采用：

- 少量稳定导出
- 统一 JSON 业务入口

稳定导出固定为：

- `nb_init`
- `nb_shutdown`
- `nb_get_protocol_version`
- `nb_get_abi_version`
- `nb_self_check`

业务统一走：

- `nb_call(action, json)`

后续所有授权业务动作，包括：

- 激活
- 检查
- 心跳
- 策略查询
- 票据申请
- 结果回报
- 状态快照

都通过 `nb_call` 实现。

### 6.3 状态回传

授权状态不再通过大量零散 getter 组合。

统一使用：

- `nb_call("get_status_snapshot", {})`

返回单个 JSON 状态快照，由 `EXE` 负责渲染。

快照字段至少包含：

- `active`
- `online`
- `kicked`
- `tier_name`
- `tier_value`
- `feature_flags`
- `license_status`
- `key_id`
- `device_id`
- `bound_dll_name`
- `bound_dll_sha256`
- `protocol_version`
- `abi_version`
- `last_verified_at_utc`
- `last_heartbeat_at_utc`
- `replace_cooldown_remaining_sec`
- `network_available`
- `update_state`
- `auth_dll_version`

### 6.4 NoteBotAuth.dll 的职责

`NoteBotAuth.dll` 在 V3 中是授权核心。

它负责：

- V3 本地状态机
- 设备身份生成与维护
- 本地授权文件读写
- 服务端 V3 协议
- 心跳
- 业务 DLL 策略请求
- 注入票据请求
- wrapper 生成
- 结果回报

它不负责：

- UI 渲染
- QML 提供
- 主程序自更新
- `updater.exe` 更新

### 6.5 游戏内业务 DLL 的职责

游戏内业务 DLL 只负责：

- 读取本地单次票据文件
- 解外层包装
- 验服务端签名
- 校验自身 `sha256`
- 放行功能
- 写结果文件

它明确不负责：

- 长期联网
- 心跳
- 续票
- 在线状态管理

---

## 7. Local Files and Paths

### 7.1 V3 本地状态目录

固定目录：

- `%LOCALAPPDATA%\NoteBotInjector\state_v3\`

### 7.2 V3 下载目录

固定目录：

- `%LOCALAPPDATA%\NoteBotInjector\dlls_v3\`

### 7.3 固定文件

固定文件如下：

- `state_v3\license_v3.dat`
- `state_v3\inject_ticket_v3.dat`
- `state_v3\inject_result_v3.dat`
- `state_v3\consumed_tickets_v3.json`

### 7.4 旧残留处理

旧残留全部旁路：

- `license.dat`
- `license_v2.dat`
- `dlls\auth.dat`
- 旧 `dlls\`
- 旧 `qmlcache`

规则固定为：

- 不删除
- 不迁移
- 不读取
- 不解释

### 7.5 license_v3.dat 内容

`license_v3.dat` 固定包含：

- `schema_version`
- `channel`
- `key_plaintext`
- `key_id`
- `key_hash`
- `device_id`
- `device_public_key_pem`
- `device_private_key_dpapi_blob`
- `server_pubkey_version`
- `server_pubkey_fingerprint`
- `last_known_tier`
- `last_known_feature_flags`
- `last_known_dll_name`
- `last_known_dll_sha256`
- `last_verified_at_utc`
- `online_state_cache`

### 7.6 consumed_tickets_v3.json 规则

本地反重放缓存规则：

- 文件：`consumed_tickets_v3.json`
- 最多保留最近 256 条
- 时间窗口 24 小时

每条至少包含：

- `ticket_id`
- `ticket_sha256`
- `consumed_at_utc`

---

## 8. Device Identity

### 8.1 设备身份模型

首次激活时客户端生成：

- 设备 RSA-2048 密钥对
- 新 `device_id`

### 8.2 私钥存储方式

设备私钥只保存在本地，并使用机器级 DPAPI 加密。

明确禁止：

- 明文落盘
- 纯静态 AES
- 纯 EXE 常量隐藏

### 8.3 本地 key 策略

客户端本地保留原始 key。

但必须满足：

- 原始 key 与设备私钥一起使用机器级 DPAPI 保护

### 8.4 服务端记录

服务端记录：

- 原始 key
- `key_id`
- `key_hash`
- `device_id`
- 设备公钥

### 8.5 设备数量限制

默认规则：

- 每个 key 最多 2 台设备

第 3 台设备激活时：

1. 自动替换最久未使用设备
2. 24 小时内最多自动替换一次
3. 超过频率限制则拒绝

### 8.6 在线规则

默认规则：

- 同一 key 同时只允许 1 台设备在线

这里的“在线”定义为：

- `EXE` 在线
- 不是游戏内 `DLL` 在线

### 8.7 心跳规则

固定规则：

- `EXE` 每 30 秒心跳一次
- 服务端 90 秒未收到即判离线

### 8.8 顶下线规则

当新设备上线时：

1. 服务端检查同 key 是否已有在线设备
2. 若有，则踢掉旧在线设备
3. 新设备获得在线资格

旧设备 UI 固定表现：

- 显示“已被其他设备顶下线”
- 停止心跳
- 禁止注入

---

## 9. Service-Side V3 Model

### 9.1 主通讯

V3 服务端继续沿用现有 TCP AES-GCM 通道，不新开 HTTP 作为主协议。

### 9.2 新命令

V3 固定命令：

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- `dll_policy_v3`
- `issue_inject_ticket_v3`
- `report_inject_result_v3`
- `update_manifest_v3`

### 9.3 请求签名

除首次激活外，所有请求都由设备私钥签名。

### 9.4 V3 数据目录

V3 服务端数据独立存放：

- `data/v3/licenses.json`
- `data/v3/devices.json`
- `data/v3/online_sessions.json`
- `data/v3/inject_sessions.json`

### 9.4.1 服务端数据落盘与并发规则

上述四个 JSON 数据文件固定遵守以下规则：

- 单次逻辑修改先在内存中完成，再整体写回
- 写回固定使用“临时文件 + flush/fsync + 原子 rename 覆盖”模式
- 不允许直接原地覆盖正式文件
- 同一时刻只允许一个写入临界区持有对应数据文件的逻辑锁
- `licenses.json` / `devices.json` / `online_sessions.json` / `inject_sessions.json` 的锁粒度固定为“文件级”
- 先改多个文件时，固定加锁顺序为：
  1. `licenses.json`
  2. `devices.json`
  3. `online_sessions.json`
  4. `inject_sessions.json`
- 不允许逆序拿锁，避免死锁
- 读路径固定只读取“已完成 rename 的正式文件”，不读取临时文件
- 若写入中途失败，则保留旧正式文件不变，并把错误写审计日志

### 9.5 licenses.json 字段

固定字段：

- `key_plaintext`
- `key_id`
- `key_hash`
- `tier`
- `feature_flags`
- `features`
- `bound_dll_channel`
- `bound_overlay_dll`
- `max_devices`
- `max_online_sessions`
- `replace_cooldown_sec`
- `status`
- `expires_at`
- `created_at`
- `updated_at`

默认值：

- `max_devices = 2`
- `max_online_sessions = 1`
- `replace_cooldown_sec = 86400`

### 9.6 devices.json 字段

固定字段：

- `key_id`
- `device_id`
- `device_public_key_pem`
- `first_seen_at`
- `last_seen_at`
- `last_online_at`
- `replaced_at`
- `status`

状态允许值：

- `active`
- `replaced`
- `revoked`

### 9.7 online_sessions.json 字段

固定字段：

- `key_id`
- `device_id`
- `heartbeat_nonce`
- `last_heartbeat_at_utc`
- `expires_at`

字段类型固定：

- `key_id` / `device_id` / `heartbeat_nonce` / `last_heartbeat_at_utc` / `expires_at`：字符串

### 9.8 inject_sessions.json 字段

固定字段：

- `key_id`
- `device_id`
- `session_id`
- `ticket_id`
- `ticket_sha256`
- `dll_name`
- `dll_sha256`
- `issued_at`
- `expires_at`
- `status`
- `result_reason`

状态允许值：

- `issued`
- `success`
- `fail`
- `expired`

字段类型固定：

- `key_id` / `device_id` / `session_id` / `ticket_id` / `ticket_sha256` / `dll_name` / `dll_sha256` / `issued_at` / `expires_at` / `status` / `result_reason`：字符串

### 9.9 灰度发布方式

灰度规则固定：

- 新增 V3 更新通道
- 特殊 key 才会收到 V3 `EXE/Auth DLL/业务 DLL` 策略
- V2 用户继续走旧通道
- V2/V3 并存期间，数据、命令、目录完全隔离

---

## 10. Inject Flow and Ticket Design

### 10.1 点击注入固定顺序

固定顺序如下：

1. `device_check_v3`
2. `dll_policy_v3`
3. 下载目标业务 `DLL` 到 `dlls_v3`
4. 本地校验 `sha256`
5. `issue_inject_ticket_v3`
6. `Auth DLL` 生成 wrapper 并写 `inject_ticket_v3.dat`
7. `EXE` 拉起注入
8. 游戏内 `DLL` 读取票据、校验、删除票据
9. 游戏内 `DLL` 写 `inject_result_v3.dat`
10. `Auth DLL/EXE` 读取结果并删除结果文件
11. `Auth DLL` 回报 `report_inject_result_v3`

### 10.2 禁止顺序

明确禁止：

- 先拿 30 秒票据再下载业务 `DLL`
- 下载前就提前生成 wrapper
- 游戏 DLL 自己去服务端拉票

### 10.3 注入会话锁

服务端在 `issue_inject_ticket_v3` 时创建：

- `120s` 注入会话锁

结果回报后立即释放；未回报则超时释放。

### 10.4 内层服务端票据

内层票据为 canonical JSON，字段固定顺序如下：

1. `magic`
2. `version`
3. `alg`
4. `key_id`
5. `device_id`
6. `session_id`
7. `ticket_id`
8. `tier`
9. `feature_flags`
10. `dll_name`
11. `dll_sha256`
12. `issued_at_server`
13. `expires_at_server`
14. `ttl_ms`
15. `nonce`

固定值：

- `magic = "NB_INJECT_TICKET"`
- `version = 3`
- `alg = "RS256"`
- `ttl_ms = 30000`

服务端对整段票据做 `RSA-SHA256` 签名。

`Auth DLL` 不允许修改内层票据，只允许完整保留。

### 10.5 外层 wrapper

外层 wrapper 由 `Auth DLL` 生成，字段固定顺序如下：

1. `magic`
2. `wrapper_version`
3. `created_at_utc`
4. `received_tick_ms`
5. `target_pid`
6. `exe_version`
7. `auth_dll_protocol`
8. `ticket_sha256`
9. `server_ticket_payload`
10. `server_ticket_signature`

固定值：

- `magic = "NB_TICKET_WRAP_V3"`
- `wrapper_version = 3`

外层 wrapper 再做一层 AES-GCM 包装。

### 10.6 wrapper key 派生

固定规则：

- `SHA256("NB_WRAP_V3" || session_id || ticket_sha256 || build_salt || fixed_fragment_a || fixed_fragment_b)`

### 10.7 包装二进制格式

`inject_ticket_v3.dat` 固定二进制布局：

1. `magic[16]`
2. `wrapper_version_u32_be`
3. `session_id_len_u32_be`
4. `session_id_utf8_bytes`
5. `ticket_sha256_len_u32_be`
6. `ticket_sha256_utf8_bytes`
7. `nonce[12]`
8. `tag[16]`
9. `cipher_len_u32_be`
10. `cipher_bytes`

补充规则：

- `session_id` 与 `ticket_sha256` 明文头只用于让游戏内业务 DLL 派生 wrapper key 和做外层一致性检查
- 明文头不是信任根
- 除明文头外，wrapper JSON 其余字段仍然全部在 AES-GCM 密文内

### 10.8 信任边界

必须明确：

- 内层服务端签名票据是真正信任根
- 外层 wrapper 只增加逆向难度，不是信任根

### 10.9 游戏内 DLL 校验

必须同时做：

- wrapper magic/version 正确
- 当前 PID 与 `target_pid` 一致
- `ticket_sha256` 一致
- 服务端公钥验签通过
- 当前业务 `DLL` 自身 `sha256` 与票据中 `dll_sha256` 一致
- `GetTickCount64() - received_tick_ms <= 30000`
- 本地 UTC 与服务端绝对时间差只做风险日志，不作唯一放行条件
- `ticket_id + ticket_sha256` 未被消费过

### 10.10 成功后的动作

若校验通过：

1. 解析 tier
2. 解析 feature_flags
3. 启用功能
4. 删除 `inject_ticket_v3.dat`
5. 写 `inject_result_v3.dat`

### 10.11 失败后的动作

若校验失败：

1. 不启用功能
2. 尽量删除 `inject_ticket_v3.dat`
3. 写 `inject_result_v3.dat`

### 10.12 注入失败处置补充规则

- 命中 `signature_invalid` / `wrapper_invalid` / `dll_hash_mismatch` /
  `replay_blocked` / `ticket_expired` / `tamper_suspected` / `inject_failed`
  时，本次注入直接失败
- 可疑失败不立即清空本地授权，不自动删除 `license_v3.dat`
- 不额外锁死当前 `EXE` 进程的注入功能；用户仍可按正常流程再次点击注入
- 任意失败场景均不自动重试；若需再次尝试，必须重新申请新票并重走完整注入链
- very early 失败只要仍具备写文件能力，仍必须尽量写出 `inject_result_v3.dat`
- 若业务 `DLL` 完全无法写出结果文件，则由 `Auth DLL/EXE` 本地归类为
  `inject_failed:no_result_file`
- 本轮 V3 只处理一次性注入门禁，不引入“运行中二次吊销/撤销放行”机制

---

## 11. Result File

### 11.1 固定字段

结果文件固定包含：

- `magic`
- `version`
- `session_id`
- `ticket_id`
- `ticket_sha256`
- `status`
- `reason`
- `dll_version`
- `processed_tick_ms`
- `result_hmac`

### 11.2 HMAC 规则

`result_hmac` 使用 wrapper key 做：

- `HMAC-SHA256`

### 11.3 EXE/Auth DLL 结果读取规则

只允许信任：

- HMAC 正确的结果文件

HMAC 不正确时，一律视为：

- 结果伪造
- 结果损坏
- 结果无效

### 11.4 文件删除规则

- DLL 读完票据后删除 `inject_ticket_v3.dat`
- `Auth DLL/EXE` 读完结果后删除 `inject_result_v3.dat`

### 11.5 success 扩展字段

当 `status=success` 时，结果文件允许追加以下已验证后的薄结果字段：

- `granted_tier`
- `granted_feature_flags`
- `verified_dll_sha256`

这些字段只代表“本次已验证后的放行结果”，外层后续日志、上报、功能分流只允许信任这份结果，
不再回头重新解释原始票据。

### 11.6 reason 分层规则

- `reason` 固定为机器可读短字符串
- 允许使用 `main_reason:detail_code` 子码形式，例如：
  - `signature_invalid:rsa_verify_failed`
  - `wrapper_invalid:aes_gcm_decrypt_failed`
  - `dll_hash_mismatch:self_sha256_mismatch`
  - `inject_failed:no_result_file`
- UI 只展示主类原因
- 本地日志与服务端上报保留完整子码

### 11.7 本地日志规则

业务 `DLL` 验证完成后，本地日志应记录详细摘要，但不得原样落盘完整票据、完整签名或完整 wrapper。

推荐记录：

- `ticket_id`
- `ticket_sha256`
- `target_pid`
- `verified_dll_sha256`
- `granted_tier`
- `granted_feature_flags`
- `issued_at_server`
- `expires_at_server`
- `verified_at_utc`
- `server_pubkey_version`
- `server_pubkey_fingerprint`

### 11.8 结果缺失规则

- 若等待窗口内始终未出现 `inject_result_v3.dat`，`Auth DLL/EXE` 固定本地归类为：
  - `status=inject_failed`
  - `reason=no_result_file`
- 该场景不与 `signature_invalid`、`wrapper_invalid` 或 `ticket_expired` 混用

---

## 12. Purge Plan

第二阶段开始时，先清空旧验证逻辑。

### 12.1 客户端重点清空

- `src__auth_dll/src/api.cpp` 中旧 `nb_activate/nb_verify/nb_diagnose/nb_reset` 语义
- `src__auth_dll/src/auth/license_file_manager.*`
- `src__auth_dll/src/auth/mutual_verify.*`
- `src__auth_dll/src/auth/cloud_client_tcp.*` 中旧 `auth_v2/license_migrate/diagnose_v2/inject_report/token_wire/device_id`
- `src__auth_dll/src/auth/distributed_keys.*`
- `src__auth_dll/src/auth/key_frag_0..7.cpp`
- `src__auth_dll/src/auth/dll_pubkey_embedded.*`
- `src__auth_dll/src/protected/protected_auth_ops.*`
- `src__auth_dll/src/protected/protected_inject_ops.*`

### 12.2 客户端壳保留

明确保留：

- `EXE` 的 UI、日志、扫描、进程选择、注入触发
- 下载校验基础能力
- `Win32Injector`

### 12.3 服务端重点清空

- `src/auth_v2.js`
- `src/diagnose.js`
- `src/auth.js`
- `src/handler.js` 中旧 `auth_v2/license_migrate/diagnose_v2/inject_report` 分发
- 旧 `data/licenses.json` 的 V2 设备/token 解释逻辑

### 12.4 purge 顺序

固定顺序：

1. 写规格书
2. 删客户端旧验证逻辑
3. 删服务端旧验证逻辑
4. 保留更新/下载/壳层
5. 重建 V3

---

## 13. Protection

### 13.1 最强保护对象

最强壳优先给：

- `NoteBotAuth.dll`
- 游戏内业务 `DLL`

`EXE` 可以保留主体、渲染、依赖库，不必承载最强保护。

### 13.2 VMP 重点保护

VMP 重点保护：

- 服务端公钥重组
- wrapper 解密与 key 派生
- RSA 验签
- `GetTickCount64` 过期判断
- `ticket_id` 反重放
- `feature_flags` 解析
- sealed gate context 安装与 cookie 校验
- `tier / feature_flags` 解码读取
- 最终放行分支

并且新增硬约束：

- V3 必须从第一天就按“可裸跑、可加壳、加壳后不改接口”设计，不允许最后再集中补 VMP 适配。
- `Auth DLL` 内部必须显式分成“外层稳定壳”和“内层受保护核”。
- 外层稳定壳负责：
  - 导出函数
  - Qt / JSON 编排
  - 文件路径
  - 日志回调
  - 线程调度
- 内层受保护核负责：
  - 公钥重组
  - wrapper key 派生
  - 验签
  - 反重放
  - feature 解析
  - 最终放行判断
- 不允许把真正敏感逻辑散写在 `api.cpp`、UI 桥接层、QML 或大型状态编排代码里，后续再往 VMP 区搬。
- 导出名、调用约定、协议版本号在加壳前后必须保持稳定，避免因为壳而改 ABI。

### 13.3 安全边界声明

本文件明确声明：

- 外层 AES-GCM 不是安全根
- wrapper JSON 不是安全根
- `EXE` 本地附加字段不是安全根
- 真正的安全根只有服务端签名票据

---

## 14. UI and Status

### 14.1 检查按钮

检查按钮语义固定为：

- 有本地设备状态 -> 刷新设备状态
- 无本地设备状态 -> 首次激活

### 14.2 离线状态

离线时 UI 表现固定为：

- 允许打开
- 显示缓存等级和功能
- 提示当前离线
- 禁止注入

### 14.3 被顶下线状态

被顶下线时 UI 固定显示：

- 已被其他设备顶下线

### 14.4 替换冷却状态

设备替换冷却命中时固定显示：

- 设备切换过于频繁

### 14.5 票据过期状态

票据过期时固定显示：

- 注入票据已过期，请重试

### 14.6 票据失效状态

票据被判定为已消费或重放拦截时固定显示：

- 本次注入凭据已失效，请重新发起注入

### 14.7 签名失败状态

命中签名校验失败或疑似篡改时固定显示：

- 签名校验失败
- 程序可能被篡改

### 14.8 组件校验失败状态

命中业务 `DLL` 哈希不匹配时固定显示：

- 组件校验失败，请更新后重试

### 14.9 旧术语废弃

V2 术语在 V3 UI 中废弃：

- `diagnose_v2`
- `license_migrate`
- `token_wire`

V3 UI 统一只出现：

- 检查
- 注入
- 在线
- 离线
- 被顶下线

---

## 15. Rollout

### 15.1 V3 更新通道

V3 必须使用全新更新通道：

- 不碰旧 `auth_dll_bootstrap`
- 不碰旧 `dll_update_check_v2`

### 15.2 特殊 key 灰度

服务端必须支持：

- 只对特殊 key 下发 V3 `EXE`
- 只对特殊 key 下发 V3 `Auth DLL`
- 只对特殊 key 下发 V3 业务 DLL

### 15.3 V2 / V3 并存

并存原则固定：

- V2 继续走旧通道
- V3 只走新通道
- 两者命令、目录、数据不共享

### 15.4 本阶段不做的事

本阶段明确不做：

- 旧系统自动迁移
- 旧数据自动升级
- 旧本地文件自动修复

即：

- 无迁移
- 纯旁路

---

## 16. Acceptance

验收必须覆盖以下项目：

1. 首次激活成功
2. 重复检查成功
3. 被顶下线后本机禁止注入
4. 第三台设备替换成功
5. 24 小时冷却命中时替换失败
6. 断网可开 UI 不可注入
7. 下载慢于 30 秒仍能成功注入
8. 票据重放失败
9. wrapper 篡改失败
10. 服务端签名篡改失败
11. DLL `sha256` 不符失败
12. 结果文件伪造失败
13. 协议版本不符拒绝加载
14. 旧目录残留对 V3 无影响
15. V2 与 V3 并存不串线
16. 普通权限运行正常
17. 管理员权限运行正常
18. Windows 10 正常
19. Windows 11 正常
20. Windows 7 兼容性单独验证，不作为当前阶段既成验收结论

---

## 17. 实施顺序

后续真实施工顺序固定如下：

1. 创建并审核本规格书
2. 清空旧验证源码
3. 重建客户端 V3 骨架
4. 重建服务端 V3 通道
5. 对特殊 key 灰度
6. 稳定后全量切换

---

## 18. 当前轮次默认约束

- 文档路径固定为：`C:\NB\AUTH_V3_REWRITE_SPEC.md`
- 第一实际动作固定为：写文档，不删源码
- 第二实际动作固定为：按 purge 清单清空旧验证逻辑
- 当前轮次只允许落实文档，不允许执行源码删除

---

## 19. 逐文件施工边界

本章节把本地和服务端的代码边界写死，避免之后“删过头”或者“留毒瘤”。

### 19.1 EXE 侧文件边界

#### 保留为主，按需改造

- `C:\NB\src__injector_exe\main.cpp`
  - 保留为 EXE 启动入口
  - 需要接入新的启动顺序：`updater -> EXE -> Auth DLL`

- `C:\NB\src__injector_exe\backend.h`
  - 保留
  - 调整对外属性，改为围绕状态快照渲染

- `C:\NB\src__injector_exe\backend.cpp`
  - 保留外壳
  - 删除旧 V2 业务语义
  - 改造为：
    - 统一通过 `nb_call` 调业务
    - 读取状态快照
    - 管控自更新、Auth DLL 更新、业务 DLL 下载、注入触发

- `C:\NB\src__injector_exe\updater.h`
  - 保留，但扩展支持：
    - 主 EXE 更新清单解析
    - `updater.exe` 更新清单解析
    - Auth DLL 更新清单解析

- `C:\NB\src__injector_exe\updater.cpp`
  - 保留大部分下载与传输基础
  - 扩展出统一更新清单获取逻辑
  - 明确禁止把授权核心塞回这里

- `C:\NB\src__injector_exe\logmodel.*`
  - 保留

- `C:\NB\src__injector_exe\processmodel.*`
  - 保留

- `C:\NB\src__injector_exe\win32process.*`
  - 保留

- `C:\NB\src__injector_exe\win32injector.*`
  - 保留

#### 新增文件

- `C:\NB\src__injector_exe\self_updater_apply.cpp`
  - 如果最终不单独做 `updater.exe` 项目，则这里实现 `--apply-self-update` 模式

- `C:\NB\src__injector_exe\version_info_exe.rc`
  - 若当前资源文件不够完整，则补 EXE 文件版本资源

### 19.2 Auth DLL 侧文件边界

#### 删除旧语义并重写

- `C:\NB\src__auth_dll\src\api.cpp`
  - 保留 DLL 导出入口文件这个“位置”
  - 但内部状态机、旧激活/验证/诊断语义全部重写

- `C:\NB\src__auth_dll\src\auth\cloud_client_tcp.*`
  - 保留 TCP / AES-GCM 传输基础
  - 删除旧 V2 协议
  - 重写成 V3 命令客户端

- `C:\NB\src__auth_dll\src\auth\license_file_manager.*`
  - 删除旧 `license.dat/license_v2.dat` 读写解释
  - 重写为 `license_v3.dat` 管理器

- `C:\NB\src__auth_dll\src\auth\mutual_verify.*`
  - 整体删除旧 `auth.dat` 模型
  - 重写为 V3 票据文件和结果文件桥

- `C:\NB\src__auth_dll\src\auth\distributed_keys.*`
  - 删除旧公钥分片语义
  - 重写为 V3 服务端公钥装载/版本管理

- `C:\NB\src__auth_dll\src\auth\dll_pubkey_embedded.*`
  - 删除旧 DLL 公钥嵌入语义
  - 替换为 V3 服务端公钥资源

- `C:\NB\src__auth_dll\src\auth\key_frag_0.cpp` 至 `key_frag_7.cpp`
  - 删除整个旧公钥分片链
  - 若 V3 仍需分片嵌入，则重新生成，不能沿用旧内容

- `C:\NB\src__auth_dll\src\auth\license_token.*`
  - 删除旧 token 结构体语义
  - 若保留文件，则重写为 V3 canonical ticket helpers

- `C:\NB\src__auth_dll\src\auth\offline_cache.*`
  - 删除旧“离线 token 缓存验证”语义
  - V3 不允许离线注入

- `C:\NB\src__auth_dll\src\protected\protected_auth_ops.*`
  - 删除旧 token->tier/feature 路径
  - 改为 V3 状态和错误码辅助

- `C:\NB\src__auth_dll\src\protected\protected_inject_ops.*`
  - 删除旧 shared auth input 逻辑
  - 改为 V3 wrapper/ticket 相关辅助

#### 保留并适配

- `C:\NB\src__auth_dll\src\protected\protected_download_ops.*`
  - 保留下载元数据和下载后 hash 校验思路
  - 适配 V3 元数据字段

- `C:\NB\src__auth_dll\src\crypto\crypto_utils.*`
  - 保留密码学工具底座
  - 扩展 DPAPI、RSA、HMAC、AES-GCM 所需 helper

- `C:\NB\src__auth_dll\src\crypto\vmp_defs.h`
  - 保留

#### 从 Auth DLL 中挪出

- `C:\NB\src__auth_dll\qml\main.qml`
- `C:\NB\src__auth_dll\src\qml_embedded.h`
- `C:\NB\src__auth_dll\embed_qml.py`

这些不再是 V3 Auth DLL 的职责。若短期不删，可先停止使用，再在后续清理。

### 19.3 服务端文件边界

#### 删除旧语义并重写

- `/home/william/NoteBot-API/src/auth_v2.js`
- `/home/william/NoteBot-API/src/diagnose.js`
- `/home/william/NoteBot-API/src/auth.js`
- `/home/william/NoteBot-API/src/handler.js` 中旧 V2 分发

#### 保留并扩展

- `/home/william/NoteBot-API/src/crypto.js`
  - 保留 RSA/AES 基础
  - 新增 V3 canonical sign/verify helpers

- `/home/william/NoteBot-API/src/db.js`
  - 保留 JSON 数据文件读写基础
  - 增加 `data/v3/*` 管理器

- `/home/william/NoteBot-API/src/dll_delivery.js`
  - 保留下载 URL 下发骨架

- `/home/william/NoteBot-API/src/dll_policy.js`
  - 保留策略骨架
  - 扩展按 key / 渠道 / 协议版本筛选

#### 新增文件

- `src/v3_protocol.js`
- `src/v3_devices.js`
- `src/v3_online.js`
- `src/v3_inject.js`
- `src/v3_updates.js`

文件名可调整，但职责必须分离成这几块。

---

## 20. 稳定导出与 nb_call 动作清单

### 20.1 稳定导出函数

V3 最终稳定导出固定为：

1. `nb_init()`
2. `nb_shutdown()`
3. `nb_set_log_callback(cb)`
4. `nb_set_state_callback(cb)`
5. `nb_self_check()`
6. `nb_get_protocol_version()`
7. `nb_get_abi_version()`
8. `nb_call(const char* action, const char* json, char* out_json, int out_size)`

### 20.2 兼容期导出

为了让当前 `EXE` 逐步过渡，短期内允许保留以下旧导出，但内部必须转调 `nb_call`：

- `nb_activate`
- `nb_verify`
- `nb_diagnose`
- `nb_reset`
- `nb_download`
- `nb_download_secure`
- `nb_inject_async`
- `nb_get_status`
- `nb_get_tier`
- `nb_is_active`

兼容期结束后，这些旧导出可以变成壳，甚至空壳，但不能再承载真正业务。

### 20.3 nb_call 动作清单

V3 `nb_call` 固定支持以下动作：

- `get_status_snapshot`
- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- `dll_policy_v3`
- `issue_inject_ticket_v3`
- `consume_inject_result_v3`
- `report_inject_result_v3`
- `download_overlay_dll_v3`
- `reset_v3_local_state`
- `get_update_snapshot`

### 20.4 旧动作兼容映射

若 `EXE` 短期仍然调用旧动作，则 Auth DLL 内部必须映射为：

- `verify` -> `device_check_v3`
- `activate` -> `activate_device_v3`
- `diagnose` -> `dll_policy_v3`

但外部 UI 与日志必须逐步改成 V3 语义。

---

## 21. 状态快照 JSON 规范

`nb_call("get_status_snapshot", {})` 返回的 JSON 固定字段如下：

- `ok`
- `protocol_version`
- `abi_version`
- `active`
- `online`
- `kicked`
- `license_status`
- `tier_name`
- `tier_value`
- `feature_flags`
- `key_id`
- `device_id`
- `bound_dll_name`
- `bound_dll_sha256`
- `last_verified_at_utc`
- `last_heartbeat_at_utc`
- `replace_cooldown_remaining_sec`
- `network_available`
- `update_state`
- `auth_dll_version`

字段说明：

- `active`：仅当当前状态为 `verified_active` 时才为 `true`，不是“有文件就 true”
- `online`：当前设备是否在服务端在线
- `kicked`：是否被其他设备顶下线
- `license_status`：用户可读文本，仅供展示，不参与签名、验签和协议判定
- `tier_name`：如 `None/Trial/Premium/Dev`
- `feature_flags`：当前生效能力位
- `network_available`：仅表示最近一次联网动作是否成功连到服务端，不表示票据链、下载链或注入链一定可继续
- `update_state`：`idle/checking/downloading/ready/error`
- 正式返回位置固定为 `data.status_snapshot`
- 顶层 `status_snapshot` 只允许在兼容期镜像存在，不能作为正式契约

`license_v3.dat` 生命周期状态固定为：

- `missing`
- `pending_local_identity`
- `verified_active`
- `corrupt`

### 21.1 状态映射表

Auth DLL 内部状态与 `status_snapshot` 固定映射如下：

| 内部状态 | active | online | kicked | network_available | license_status | update_state | 是否允许注入 |
|---|---:|---:|---:|---:|---|---|---:|
| `booting` | false | false | false | false | 初始化中 | `idle` | 否 |
| `ready_inactive` | false | false | false | false | 未激活 | `idle` | 否 |
| `pending_local_identity` | false | false | false | false | 已生成本地设备身份，待联网激活 | `idle` | 否 |
| `ready_active` | true | 视最近服务端结果而定 | false | 视最近联网动作而定 | 已激活 | `idle` | 是 |
| `offline_cached` | true | false | false | false | 当前离线，显示缓存状态 | `idle` | 否 |
| `kicked` | true | false | true | true | 已被其他设备顶下线 | `idle` | 否 |
| `fatal_error` | false | false | false | false | 本地状态错误 | `error` | 否 |

补充规则：

- `corrupt` 是 `license_v3.dat` 生命周期判定，不是独立内部运行状态；当本地文件进入 `corrupt` 时，Auth DLL 运行态进入 `fatal_error`
- `device_check_v3` 在 `pending_local_identity` 下固定返回：
  - `rc = 1007`
  - `message = 当前设备尚未完成联网激活`
  - `status_snapshot` 继续保持 `pending_local_identity` 语义

---

## 22. 本地状态文件格式

### 22.1 license_v3.dat 明文逻辑结构

加密前逻辑结构固定为 JSON：

- `schema_version`
- `channel`
- `key_plaintext`
- `key_id`
- `key_hash`
- `device_id`
- `device_public_key_pem`
- `device_private_key_dpapi_blob_b64`
- `server_pubkey_version`
- `server_pubkey_fingerprint`
- `last_known_tier`
- `last_known_tier_value`
- `last_known_feature_flags`
- `last_known_dll_name`
- `last_known_dll_sha256`
- `last_verified_at_utc`
- `online_state_cache`

### 22.2 online_state_cache 字段

`online_state_cache` 至少包含：

- `online`
- `kicked`
- `last_heartbeat_at_utc`
- `last_server_message`
- `replace_cooldown_remaining_sec`

字段类型固定：

- `schema_version` / `server_pubkey_version` / `last_known_tier_value` / `last_known_feature_flags`：整数
- `online_state_cache`：JSON 对象
- `online_state_cache.online` / `online_state_cache.kicked`：布尔
- `online_state_cache.replace_cooldown_remaining_sec`：整数
- 其余字段：字符串

### 22.3 文件保护规则

`license_v3.dat` 整体使用机器级 DPAPI 保护。  
如果需要版本头，则格式固定为：

1. 文件头 magic
2. schema version
3. DPAPI blob

状态约束：

- 首次仅生成本地设备身份时，状态视为 `pending_local_identity`
- 联网激活成功并拿到服务端确认后，状态视为 `verified_active`
- 文件存在但无法解密、magic 不符、schema 不符或 JSON 损坏时，状态视为 `corrupt`

### 22.4 canonical JSON 与编码规则

凡是本规格书中写明为 canonical JSON 的内容，固定遵守以下实施规则：

- 编码固定为 UTF-8
- 固定无 BOM
- 固定使用紧凑 JSON，不允许 pretty-print、额外空格、尾逗号
- 字段顺序固定以本规格书列出的顺序为唯一顺序
- 字符串转义固定使用标准 JSON 转义
- 控制字符的 `\\u00xx` 转义固定使用小写十六进制
- 布尔值固定使用 `true/false`
- 整数字段固定为 JSON number，不允许写成字符串或 `1.0`

摘要、签名、时间字段编码固定如下：

- `nonce`：小写 hex
- `device_signature`：小写 hex
- `server_ticket_signature`：小写 hex
- `sha256` / `md5` / `ticket_sha256` / `result_hmac`：小写 hex
- `timestamp_utc` / `issued_at_server` / `expires_at_server` / `published_at_utc`：ISO-8601 UTC，必须带 `Z`
- 不允许本地时区偏移格式，如 `+08:00`

以下字段固定为“JSON 字符串，内容本身仍然是 canonical JSON 原文 UTF-8”：

- `payload`
- `server_ticket_payload`

---

## 23. 服务端命令请求与响应规范

### 23.1 activate_device_v3

请求字段：

- `cmd`
- `key_plaintext`
- `device_id`
- `device_public_key_pem`
- `timestamp_utc`
- `nonce`
- `client_version`
- `protocol_version`

响应字段：

- `status`
- `key_id`
- `key_hash`
- `tier`
- `feature_flags`
- `device_id`
- `server_pubkey_version`
- `server_pubkey_fingerprint`
- `bound_dll_name`
- `bound_dll_sha256`

字段类型规则：

- `cmd` / `key_plaintext` / `device_id` / `device_public_key_pem` / `timestamp_utc` / `nonce` / `client_version`：字符串
- `protocol_version`：整数
- `status` / `key_id` / `key_hash` / `tier` / `device_id` / `server_pubkey_fingerprint` / `bound_dll_name` / `bound_dll_sha256`：字符串
- `feature_flags` / `server_pubkey_version`：整数

`tier` 枚举允许值仅有：

- `None`
- `Trial`
- `Premium`
- `Dev`

`status` 枚举白名单至少包含：

- `ok`
- `rejected`
- `kicked`
- `replace_cooldown`
- `expired`
- `protocol_mismatch`
- `invalid_signature`
- `not_found`

结果规则：

| 场景 | rc | message | 是否刷新快照 | 是否启动心跳 |
|---|---:|---|---:|---:|
| 激活成功 | `0` | 激活成功 | 是 | 是 |
| 本地参数错误 | `1001` | 未输入密钥 | 是 | 否 |
| 本地 DPAPI / 私钥失败 | `1004/1005` | 本地设备身份生成失败 | 是 | 否 |
| 网络失败 | `1006` | 无法连接服务器 | 是 | 否 |
| 服务端拒绝 | `1007` | 服务端拒绝激活 | 是 | 否 |

补充规则：

- 服务端拒绝激活时，本地 `pending_local_identity` 状态固定保留
- 激活失败时不得清空本地已生成的设备身份

### 23.2 device_check_v3

请求字段：

- `cmd`
- `key_id`
- `device_id`
- `timestamp_utc`
- `nonce`
- `client_version`
- `protocol_version`
- `payload`
- `device_signature`

响应字段：

- `status`
- `online`
- `kicked`
- `tier`
- `feature_flags`
- `bound_dll_name`
- `bound_dll_sha256`
- `replace_cooldown_remaining_sec`

字段类型规则：

- `cmd` / `key_id` / `device_id` / `timestamp_utc` / `nonce` / `client_version` / `payload` / `device_signature`：字符串
- `protocol_version`：整数
- `status` / `tier` / `bound_dll_name` / `bound_dll_sha256`：字符串
- `online` / `kicked`：布尔
- `feature_flags` / `replace_cooldown_remaining_sec`：整数

其中：

- `payload` 固定为 canonical JSON 字符串，不是嵌套对象
- `device_signature` 固定为对 `payload` 原始 UTF-8 字节做 RSA-SHA256 后得到的小写 hex

状态规则：

- 成功时：`rc=0`，`message` 为成功文本，`data.status_snapshot` 刷新为最新在线态
- 本地状态缺失：`rc=1002`
- 本地状态损坏或解密失败：`rc=1003/1004`
- 被其他设备顶下线：`rc=1008`
- 命中替换冷却：`rc=1009`
- 网络失败：`rc=1006`

结果规则：

| 场景 | rc | message | 是否刷新快照 | 是否启动/维持心跳 |
|---|---:|---|---:|---:|
| 检查成功 | `0` | 设备检查成功 | 是 | 是 |
| 本地缺失 | `1002` | 未发现 license_v3.dat | 是 | 否 |
| 本地损坏 | `1003/1004` | license_v3.dat 损坏 | 是 | 否 |
| 未完成联网激活 | `1007` | 当前设备尚未完成联网激活 | 是 | 否 |
| 被顶下线 | `1008` | 已被其他设备顶下线 | 是 | 停止 |
| 替换冷却 | `1009` | 设备切换过于频繁 | 是 | 否 |
| 网络失败 | `1006` | 无法连接服务器 | 是 | 仅 `verified_active` 允许转入 `offline_cached` |

### 23.3 device_heartbeat_v3

请求字段：

- `cmd`
- `key_id`
- `device_id`
- `timestamp_utc`
- `nonce`
- `device_signature`

响应字段：

- `status`
- `online`
- `kicked`
- `server_time_utc`

字段类型规则：

- `cmd` / `key_id` / `device_id` / `timestamp_utc` / `nonce` / `device_signature` / `status` / `server_time_utc`：字符串
- `online` / `kicked`：布尔

状态规则：

- 成功时：`rc=0`
- 连续 3 次失败后本地进入 `offline_cached`
- 服务端返回 kicked 时本地进入 `kicked` 并停止后续心跳

结果规则：

| 场景 | rc | message | 是否刷新快照 | 是否继续心跳 |
|---|---:|---|---:|---:|
| 心跳成功 | `0` | 心跳成功 | 是 | 是 |
| 本地签名失败 | `1004/1005` | 心跳签名失败 | 是 | 是 |
| 网络失败 | `1010` | 心跳失败 | 是 | 是，直到连续 3 次失败 |
| 服务端 kicked | `1008` | 已被其他设备顶下线 | 是 | 否 |

补充规则：

- 连续失败计数固定不持久化，仅保留在内存态
- 收到 `kicked` 后不得清空 `license_v3.dat`，只更新 `online_state_cache` 与运行态快照

### 23.4 dll_policy_v3

请求字段：

- `cmd`
- `key_id`
- `device_id`
- `timestamp_utc`
- `nonce`
- `device_signature`

响应字段：

- `status`
- `dll_name`
- `dll_sha256`
- `dll_md5`
- `dll_size`
- `download_url`
- `channel`
- `expires_in`

### 23.5 issue_inject_ticket_v3

请求字段：

- `cmd`
- `key_id`
- `device_id`
- `dll_name`
- `dll_sha256`
- `target_pid`
- `timestamp_utc`
- `nonce`
- `device_signature`

响应字段：

- `status`
- `session_id`
- `ticket_id`
- `server_ticket_payload`
- `server_ticket_signature`
- `expires_in`

字段类型规则：

- `cmd` / `key_id` / `device_id` / `dll_name` / `dll_sha256` / `timestamp_utc` / `nonce` / `device_signature`：字符串
- `target_pid`：整数
- `status` / `session_id` / `ticket_id` / `server_ticket_payload` / `server_ticket_signature`：字符串
- `expires_in`：整数

补充规则：

- `server_ticket_payload` 固定为 canonical JSON 字符串
- `server_ticket_signature` 固定为对 `server_ticket_payload` 原始 UTF-8 字节做 RSA-SHA256 后得到的小写 hex
- 服务端发放票据前必须确认对应 `dll_sha256` 与当前策略仍一致

结果规则：

| 场景 | rc | message | 是否刷新快照 | 是否写 ticket 文件 |
|---|---:|---|---:|---:|
| 票据发放成功 | `0` | 票据发放成功 | 是 | 是 |
| 本地参数错误 | `1001` | 缺少 target_pid 或 DLL 信息 | 否 | 否 |
| 设备未激活 / 被踢 / 冷却 | `1007/1008/1009` | 对应状态文本 | 是 | 否 |
| 策略无效 | `1011` | 当前 DLL 策略无效 | 是 | 否 |
| 下载或 hash 前置失败 | `1012/1013` | 下载失败或 hash 不匹配 | 是 | 否 |
| 服务端拒绝票据 | `1014` | 票据请求失败 | 是 | 否 |

### 23.6 report_inject_result_v3

请求字段：

- `cmd`
- `key_id`
- `device_id`
- `session_id`
- `ticket_id`
- `status`
- `reason`
- `timestamp_utc`
- `nonce`
- `device_signature`

响应字段：

- `status`
- `accepted`

字段类型规则：

- `cmd` / `key_id` / `device_id` / `session_id` / `ticket_id` / `status` / `reason` / `verified_tier` / `verified_dll_sha256` / `verified_at_utc` / `issued_at_server` / `expires_at_server` / `server_pubkey_fingerprint` / `timestamp_utc` / `nonce` / `device_signature`：字符串
- `verified_tier_value` / `verified_feature_flags` / `server_pubkey_version`：整数
- `accepted`：布尔

补充规则：

- `status` 请求值推荐枚举：
  - `success`
  - `ticket_expired`
  - `wrapper_invalid`
  - `signature_invalid`
  - `dll_hash_mismatch`
  - `replay_blocked`
  - `tamper_suspected`
  - `result_hmac_invalid`
  - `inject_failed`
- `reason` 固定为简短机器可读字符串，不写长文本段落
- `reason` 允许带机器层子码，例如 `main_reason:detail_code`
- `dll_hash_mismatch` 与 `tamper_suspected` 固定视为高风险上报事件
- 第一版服务端只做高优先级记录和告警，不自动冻结设备，不自动冻结密钥
- 当 `status=success` 时，客户端应同步上报：
  - `verified_tier`
  - `verified_tier_value`
  - `verified_feature_flags`
  - `verified_dll_sha256`
  - `verified_at_utc`
  - `issued_at_server`
  - `expires_at_server`
  - `server_pubkey_version`
  - `server_pubkey_fingerprint`

结果规则：

| 场景 | rc | message | 是否释放会话锁 | 是否删除本地结果文件 |
|---|---:|---|---:|---:|
| 上报成功 | `0` | 结果上报成功 | 是 | 是 |
| 本地结果无效 | `1018/1019` | 结果文件无效 | 否 | 否 |
| 服务端拒绝上报 | `1020` | 结果上报失败 | 否 | 否 |
| 网络失败 | `1020` | 结果上报失败 | 否 | 否 |

### 23.7 update_manifest_v3

`update_manifest_v3` 是 V3 唯一正式更新协议命令。

请求字段：

- `cmd`
- `client_kind`
- `channel`
- `current_main_version`
- `current_main_version_code`
- `current_auth_dll_version`
- `current_auth_dll_version_code`
- `current_updater_version`
- `current_updater_version_code`
- `protocol_version`
- `key_id`
- `device_id`
- `timestamp_utc`
- `nonce`
- `device_signature`

响应字段：

- `status`
- `artifacts`

字段类型规则：

- `cmd` / `client_kind` / `channel` / `current_main_version` / `current_auth_dll_version` / `current_updater_version` / `key_id` / `device_id` / `timestamp_utc` / `nonce` / `device_signature` / `status`：字符串
- `current_main_version_code` / `current_auth_dll_version_code` / `current_updater_version_code` / `protocol_version`：整数
- `artifacts`：JSON 数组

补充规则：

- `client_kind` 当前固定为 `NoteBotInjector`
- `artifacts` 的每一项固定遵守 34 节更新清单字段定义
- 若当前 key 不在 V3 灰度范围，服务端固定返回空 `artifacts`

---

## 24. 启动状态机

### 24.1 EXE 启动状态机

固定顺序：

1. 初始化日志
2. 准备 `updater.exe`
3. 检查 EXE 更新
4. 若需要则进入自更新流程
5. 检查 Auth DLL 更新
6. 加载 Auth DLL
7. 验协议版本
8. 请求状态快照
9. 加载 UI
10. 启动进程扫描

### 24.2 Auth DLL 启动状态机

固定顺序：

1. 初始化本地状态
2. 尝试读取 `license_v3.dat`
3. 构建内存态状态快照
4. 等待 `EXE` 通过 `nb_call` 发业务请求

---

## 25. 自更新状态机

### 25.0 更新协议统一口径

本规格书中涉及更新的名词固定解释如下：

- `get_update_snapshot`：本地 `nb_call` 动作，仅用于 Auth DLL 向 EXE 回传当前宿主侧更新阶段与缓存状态，不是服务端命令
- `update_manifest_v3`：服务端更新清单命令名，固定作为 V3 正式更新协议入口
- `auth_dll_bootstrap_v3`：本阶段明确废弃，不进入 V3 正式实现，不再作为 Auth DLL 更新元数据入口

补充规则：

- V3 更新统一走 `update_manifest_v3`
- 不再保留“bootstrap 一套、清单一套”双正式协议
- 旧 `auth_dll_bootstrap` / `dll_update_check_v2` 全部视为 V2 遗留，不得复用到 V3

### 25.1 EXE 更新自己

固定流程：

1. 读取当前 EXE 文件版本资源
2. 从服务端拉更新清单
3. 比对版本 / hash / size
4. 下载新 EXE 到临时文件
5. 校验下载文件
6. 拉起 `updater.exe`
7. 退出当前 EXE
8. `updater.exe` 覆盖正式文件
9. `updater.exe` 重启主 EXE

### 25.2 EXE 更新 updater.exe

固定流程：

1. 读取当前 updater 文件版本资源
2. 拉清单
3. 若需要则下载新 updater 到临时文件
4. 校验下载结果
5. 当 updater 未运行时直接替换

### 25.3 EXE 更新 Auth DLL

固定流程：

1. 读取 `NoteBotAuth.dll` 文件版本资源和 hash
2. 拉清单
3. 若需要则下载新 DLL 到临时文件
4. 校验下载结果
5. 若尚未 `LoadLibrary`，直接替换
6. 若已加载，则本轮不替换，要求下次冷启动前完成替换

### 25.4 明确禁止

禁止：

- 为读取 DLL 版本先 `LoadLibrary`
- 让已加载 DLL 自己覆盖自己
- 让 updater 去联网拿更新清单

补充规则：

- `version` 固定为三段式字符串，如 `3.4.63`
- `version_code` 固定为单调递增整数
- 服务端更新比较逻辑固定先比 `version_code`，`version` 仅用于展示与日志
- `required=true` 时：
  - 若目标是 `main_exe`，固定阻止进入主 UI，必须先完成 EXE 更新
  - 若目标是 `auth_dll`，固定允许进入最小启动态，但禁止授权检查与注入，直到 DLL 更新完成
  - 若目标是 `overlay_dll`，固定不阻止进入 UI，但阻止注入
  - 不允许用户在当前版本上手动跳过 `required=true` 更新

---

## 26. 票据与结果文件的实施级约束

### 26.1 inject_ticket_v3.dat

生成方：

- `Auth DLL`

消费方：

- 游戏内业务 DLL

删除方：

- 游戏内业务 DLL

实施级二进制规则：

- 文件头 magic 固定为 ASCII
- `wrapper_version/session_id_len/ticket_sha256_len/cipher_len` 固定全部为 `uint32 big-endian`
- 外层 AES-GCM IV/nonce 长度固定为 `12` 字节
- `ticket_sha256` 固定计算对象为“内层 `server_ticket_payload` canonical JSON 原文 UTF-8 字节”
- 不允许对 wrapper、密文或解包后的对象重新序列化后再计算 `ticket_sha256`

### 26.2 inject_result_v3.dat

生成方：

- 游戏内业务 DLL

消费方：

- `Auth DLL/EXE`

删除方：

- `Auth DLL/EXE`

实施级规则：

- `result_hmac` 固定输入拼接顺序为：
  - `magic`
  - `version`
  - `session_id`
  - `ticket_id`
  - `ticket_sha256`
  - `status`
  - `reason`
  - `dll_version`
  - `processed_tick_ms`
  - `granted_tier`
  - `granted_feature_flags`
  - `verified_dll_sha256`
  - `issued_at_server`
  - `expires_at_server`
  - `server_pubkey_version`
  - `server_pubkey_fingerprint`
- 拼接时固定使用 canonical JSON 原文 UTF-8 字节再做 HMAC-SHA256
- 非 success 时，上述扩展字段固定写空字符串或 `0`，但仍进入 canonical JSON 与 HMAC 计算

### 26.3 反重放

游戏内业务 DLL 验票成功后必须把：

- `ticket_id`
- `ticket_sha256`

写入本地 `consumed_tickets_v3.json`。  
之后同票据再次出现直接拒绝。

补充规则：

- 票据一旦进入游戏内 `DLL` 校验流程，无论最终结果是 `success`、
  `wrapper_invalid`、`signature_invalid`、`dll_hash_mismatch`、
  `replay_blocked`、`ticket_expired`、`tamper_suspected`、`inject_failed`
  或等待结果超时，均不得再次复用
- 若业务 `DLL` 已完成有效校验并具备写本地缓存能力，则由业务 `DLL`
  写入 `consumed_tickets_v3.json`
- 若 very early 失败、结果文件缺失或等待结果超时导致业务 `DLL`
  未能完成本地写入，则 `Auth DLL/EXE` 仍必须把该票据视为已消费，并要求下次重新申请新票
- `consumed_tickets_v3.json` 是独立 replay cache，不因换密钥、重置授权、重新激活或客户端重启而主动清空

### 26.4 业务 DLL 本地 sealed gate context

游戏内业务 `DLL` 验票成功后，不得把 `verified / tier / feature_flags`
作为普通裸全局变量直接长期信任。

正式规则固定为：

- 验票成功后安装一份 `sealed gate context`
- `sealed gate context` 至少包含：
  - 混淆后的 `tier`
  - 混淆后的 `feature_flags`
  - 本次票据派生的 `mask`
  - 本次票据派生的 `seal`
  - `gate_cookie`
- `gate_cookie` 必须绑定本次注入上下文，至少覆盖：
  - `ticket_id`
  - `ticket_sha256` 或 wrapper key 派生材料
  - `dll_sha256`
  - `target_pid`
  - `tier`
  - `feature_flags`
  - `dll_version`
- `GetTier()` / `GetCapabilities()` / `HasCapability()` 不得直接返回裸变量
- 每次读取 `tier / feature_flags` 前必须重新校验 `gate_cookie`
- `gate_cookie` 校验失败时固定返回未授权状态：
  - `tier = 0`
  - `feature_flags = 0`
- 关键功能入口不得只信初始化阶段的一次性放行，必须在执行前再次读取 sealed gate context

该规则的目标不是替代 VMP，而是降低低成本绕过：

- 单改 `verified = true`
- 单改 `tier = 3`
- 单改 `feature_flags = 0xFFFFFFFF`
- hook 单一 getter 返回固定值

`consumed_tickets_v3.json` 的 `entries` 项结构固定为：

- `ticket_id`
- `ticket_sha256`
- `consumed_at_utc`

裁剪策略固定为：

- 先删除超过 24h 窗口的旧项
- 如仍超过 `256` 条，则按 `consumed_at_utc` 从旧到新裁剪

---

## 27. 保护与壳边界

### 27.1 最强保护对象

最强壳优先给：

- `NoteBotAuth.dll`
- 游戏内业务 `DLL`

### 27.2 EXE 保护策略

`EXE` 可以保留：

- 主体逻辑
- 渲染逻辑
- 依赖库
- 更新流程

不要求承载最强保护。

### 27.3 VMP 重点保护点

VMP 重点保护：

- 服务端公钥重组
- wrapper 解密与 key 派生
- RSA 验签
- `GetTickCount64` 过期判断
- `ticket_id` 反重放
- `feature_flags` 解析
- sealed gate context 安装
- sealed gate context cookie 校验
- `tier / feature_flags` 解码读取
- 最终放行分支

新增工程边界：

- V3 不是“先实现，再考虑 VMP”，而是骨架阶段就必须兼容 VMP。
- `api.cpp`、Qt 编排层、日志层、路径层属于外层稳定壳，尽量保持薄，不承载真正安全根。
- 票据验签、wrapper 解包、key 派生、反重放、放行判断必须拆到独立 protected 模块，便于单独上 VMP。
- 任何新增敏感算法默认先进 protected 模块，再由外层壳调用。
- sealed gate context 必须拆成独立小函数，至少区分：
  - 安装：建议 `Ultra`
  - cookie 校验：建议 `Ultra`
  - `tier / feature_flags` 读取：建议 `Mutation`
  - 热路径 UI / 渲染不得进入重保护

### 27.3.1 VMP 保护表实施规则

VMP 保护表固定为唯一真相源，规则如下：

- 任意新增 `NBVmp_*` 符号必须登记到保护表，否则壳流程直接失败
- 非 `NBVmp_*` 符号默认不得进入正式保护表
- 每条保护表项必须包含：
  - `symbol`
  - `category`
  - `module`
  - `reason`

分类允许值仅有：

- `mutation`
- `virtualize`
- `ultra`

新增 protected 模块边界固定为：

- `protected_v3_ops.*`：key/hash/id/feature 归一化
- `protected_ticket_ops.*`：wrapper key 派生、ticket 校验、result HMAC
- `protected_verify_ops.*`：签名验签、反重放主判定

以下对象默认不得上 `Ultra`：

- Qt 容器编排
- 文件遍历与普通路径 I/O
- 线程调度与定时器外壳
- 导出壳函数

壳后验收新增失败条件：

- 导出缺失
- `nb_call` JSON 被破坏
- `get_status_snapshot` 缺字段
- 保护表与 map 不一致

### 27.4 安全边界声明

本文件必须明确声明：

- 外层 AES-GCM 不是安全根
- wrapper JSON 不是安全根
- `EXE` 本地附加字段不是安全根
- 真正的安全根只有服务端签名票据

---

## 28. UI 与状态文案

### 28.1 检查按钮

语义固定：

- 有本地设备状态 -> 刷新设备状态
- 无本地设备状态 -> 首次激活

### 28.2 离线状态

UI 表现固定：

- 允许打开
- 显示缓存等级和功能
- 提示当前离线
- 禁止注入

### 28.3 被顶下线状态

固定显示：

- 已被其他设备顶下线

### 28.4 替换冷却状态

固定显示：

- 设备切换过于频繁

### 28.5 票据过期状态

固定显示：

- 注入票据已过期，请重试

### 28.6 票据失效状态

固定显示：

- 本次注入凭据已失效，请重新发起注入

### 28.7 签名失败状态

固定显示：

- 签名校验失败
- 程序可能被篡改

### 28.8 组件校验失败状态

固定显示：

- 组件校验失败，请更新后重试

### 28.9 旧术语废弃

V2 术语在 V3 UI 中废弃：

- `diagnose_v2`
- `license_migrate`
- `token_wire`

V3 UI 统一只出现：

- 检查
- 注入
- 在线
- 离线
- 被顶下线

---

## 29. Rollout

### 29.1 V3 更新通道

V3 必须使用全新更新通道：

- 不碰旧 `auth_dll_bootstrap`
- 不碰旧 `dll_update_check_v2`
- 正式协议命令固定为 `update_manifest_v3`

### 29.2 特殊 key 灰度

服务端必须支持：

- 只对特殊 key 下发 V3 `EXE`
- 只对特殊 key 下发 V3 `Auth DLL`
- 只对特殊 key 下发 V3 业务 DLL

### 29.3 V2 / V3 并存

并存原则固定：

- V2 继续走旧通道
- V3 只走新通道
- 两者命令、目录、数据不共享

### 29.4 本阶段不做的事

本阶段明确不做：

- 旧系统自动迁移
- 旧数据自动升级
- 旧本地文件自动修复

即：

- 无迁移
- 纯旁路

---

## 30. Acceptance

验收必须覆盖以下项目：

1. 首次激活成功
2. 重复检查成功
3. 被顶下线后本机禁止注入
4. 第三台设备替换成功
5. 24 小时冷却命中时替换失败
6. 断网可开 UI 不可注入
7. 下载慢于 30 秒仍能成功注入
8. 票据重放失败
9. wrapper 篡改失败
10. 服务端签名篡改失败
11. DLL `sha256` 不符失败
12. 结果文件伪造失败
13. 协议版本不符拒绝加载
14. 旧目录残留对 V3 无影响
15. V2 与 V3 并存不串线
16. 普通权限运行正常
17. 管理员权限运行正常
18. Windows 10 正常
19. Windows 11 正常
20. Windows 7 兼容性单独验证，不作为当前阶段既成验收结论

---

## 31. 实施顺序

后续真实施工顺序固定如下：

1. 创建并审核本规格书
2. 清空旧验证源码
3. 重建客户端 V3 骨架
4. 重建服务端 V3 通道
5. 对特殊 key 灰度
6. 稳定后全量切换

---

## 32. 当前轮次默认约束

- 文档路径固定为：`C:\NB\AUTH_V3_REWRITE_SPEC.md`
- 第一实际动作固定为：写文档，不删源码
- 第二实际动作固定为：按 purge 清单清空旧验证逻辑
- 当前轮次只允许落实文档，不允许执行源码删除

---

## 33. updater.exe 施工细则

### 33.1 updater.exe 交付形态

- `updater.exe` 是单独产物
- 首次安装时由主 `EXE` 释放到：
  - `%LOCALAPPDATA%\NoteBotInjector\updater\NoteBotUpdater.exe`
- 后续每次启动都检查其版本与 hash
- 本阶段固定采用“单独 `updater.exe` 正式产物”路线
- `self_updater_apply.cpp` 仅作为废弃备选记录，不进入当前实现
- 主 `EXE` 固定从内置资源释放初始 updater
- 若检测到 updater 缺失、hash 不符或版本不符，则固定重新从内置资源释放
- updater 自身版本固定从文件版本资源读取，不通过 `LoadLibrary`

### 33.2 updater.exe 命令行协议

`updater.exe` 只支持以下命令行模式：

1. `--replace-main`
2. `--replace-auth-dll`
3. `--replace-file`

#### 33.2.1 --replace-main

参数固定：

- `--pid <old_exe_pid>`
- `--src <downloaded_new_exe_path>`
- `--dst <installed_exe_path>`
- `--restart <installed_exe_path>`
- `--backup <backup_exe_path>`

#### 33.2.2 --replace-auth-dll

参数固定：

- `--pid <hosting_exe_pid>`
- `--src <downloaded_new_dll_path>`
- `--dst <installed_auth_dll_path>`
- `--backup <backup_auth_dll_path>`

说明：

- 该模式只在 Auth DLL 尚未被当前进程加载时允许执行
- 若已加载，则不得触发该命令，只能等待下次冷启动
- 因此当前主 EXE 一旦已经 `LoadLibrary(NoteBotAuth.dll)`，本轮绝对禁止再触发 `--replace-auth-dll`

#### 33.2.3 --replace-file

参数固定：

- `--wait-pid <optional_pid>`
- `--src <source_file>`
- `--dst <target_file>`
- `--backup <backup_file>`
- `--restart <optional_restart_target>`

此模式为通用兜底模式，仅供未来扩展，不作为首选路径。

### 33.3 updater.exe 执行状态机

固定状态如下：

1. 解析参数
2. 校验源文件存在
3. 若要求等待 PID，则轮询目标 PID 退出
4. 创建备份
5. 删除旧文件
6. 复制新文件
7. 校验复制结果
8. 若需要，则重启目标 EXE
9. 写日志
10. 退出

### 33.4 updater.exe 失败回滚

回滚规则固定：

1. 若删除旧文件前失败，则直接退出，不做回滚
2. 若删除旧文件后、复制新文件前失败，则尝试用 backup 还原
3. 若复制后校验失败，则删除新文件并用 backup 还原
4. 若重启主 EXE 失败，保持新文件不回滚，只写错误日志

补充规则：

- `backup` 文件默认仅保留最近一份成功替换前镜像
- 新版本启动并完成最小自检后，允许清理上一次 backup
- 若新版本未成功启动，则保留 backup 供下次人工恢复

### 33.5 updater.exe 日志文件

固定路径：

- `%LOCALAPPDATA%\NoteBotInjector\updater\logs\updater.log`

固定字段：

- `ts_utc`
- `action`
- `src`
- `dst`
- `backup`
- `old_pid`
- `status`
- `error_code`
- `error_message`

---

## 34. EXE 更新清单与替换规则

### 34.1 更新清单统一字段

服务端给 `EXE` 的更新清单项，三类产物统一字段如下：

- `artifact_type`
- `channel`
- `version`
- `version_code`
- `file_name`
- `sha256`
- `md5`
- `size`
- `download_url`
- `required`
- `protocol_min`
- `protocol_max`
- `key_scope`
- `published_at_utc`

字段类型固定：

- `artifact_type` / `channel` / `version` / `file_name` / `sha256` / `md5` / `download_url` / `key_scope` / `published_at_utc`：字符串
- `version_code` / `size` / `protocol_min` / `protocol_max`：整数
- `required`：布尔

### 34.2 artifact_type 允许值

- `main_exe`
- `auth_dll`
- `overlay_dll`
- `updater_exe`

### 34.3 EXE 对各产物的更新策略

#### main_exe

- 允许在线下载
- 不允许当前进程覆盖自己
- 必须交给 `updater.exe`

#### auth_dll

- 若未 `LoadLibrary`，允许立即替换
- 若已 `LoadLibrary`，只允许下载到缓存并标记“下次启动替换”

#### overlay_dll

- 总是下载到 `dlls_v3`
- 不做“覆盖运行中文件”策略

#### updater_exe

- 若未运行，允许直接替换
- 若正在运行，则本轮跳过并记录

### 34.4 更新缓存目录

固定目录：

- `%LOCALAPPDATA%\NoteBotInjector\updates\main\`
- `%LOCALAPPDATA%\NoteBotInjector\updates\auth\`
- `%LOCALAPPDATA%\NoteBotInjector\updates\updater\`

缓存文件命名固定为：

- `<artifact>-<version>-<sha256prefix>.bin`

---

## 35. Auth DLL 状态机细则

### 35.1 初始化状态机

固定状态：

1. `booting`
2. `load_local_state`
3. `ready_inactive`
4. `ready_active`
5. `offline_cached`
6. `kicked`
7. `fatal_error`

### 35.2 激活动作状态机

`activate_device_v3` 固定状态：

1. 参数检查
2. 生成设备密钥对
3. 生成 `device_id`
4. 发送激活请求
5. 解析服务端响应
6. 写入 `license_v3.dat`
7. 刷新内存态状态快照
8. 启动心跳定时器

### 35.3 检查动作状态机

`device_check_v3` 固定状态：

1. 读取本地状态
2. 用设备私钥签名请求
3. 发给服务端
4. 解析响应
5. 刷新 tier/feature/dll 策略缓存
6. 刷新 UI 快照

补充规则：

- 若当前仍处于 `pending_local_identity`，固定不发送真正的联网检查请求，直接返回 `1007`
- 若当前为 `verified_active` 且联网失败，允许转入 `offline_cached`

### 35.4 心跳状态机

固定行为：

- 每 30 秒触发一次
- 若连续 3 次失败，则进入 `offline_cached`
- 若收到 `kicked`，立即进入 `kicked`
- `kicked` 状态下不再继续心跳

### 35.5 注入动作状态机

`issue_inject_ticket_v3 + wrapper + result` 固定状态：

1. `precheck`
2. `fetch_policy`
3. `download_overlay`
4. `verify_overlay_hash`
5. `issue_ticket`
6. `write_ticket_file`
7. `inject_process`
8. `wait_result_file`
9. `verify_result_hmac`
10. `report_result`
11. `cleanup`

补充规则：

- 任意失败场景均不自动重试
- 用户如需再次尝试，必须重新点击注入，由宿主重新申请新票并重走完整链
- `wait_result_file` 超时后，固定本地归类为 `inject_failed:no_result_file`
- `wait_result_file` 超时不允许继续复用上一张票据

---

## 36. nb_call 动作契约

### 36.1 通用返回格式

所有 `nb_call` 返回 JSON 固定包含：

- `ok`
- `action`
- `rc`
- `message`

如有数据，再附加：

- `data`

### 36.2 rc 错误码空间

V3 固定错误码：

- `0` = OK
- `1001` = INVALID_ARGUMENT
- `1002` = LOCAL_STATE_MISSING
- `1003` = LOCAL_STATE_CORRUPT
- `1004` = DPAPI_DECRYPT_FAILED
- `1005` = DEVICE_KEY_MISSING
- `1006` = NETWORK_UNAVAILABLE
- `1007` = SERVER_REJECTED
- `1008` = DEVICE_KICKED
- `1009` = DEVICE_REPLACE_COOLDOWN
- `1010` = HEARTBEAT_FAILED
- `1011` = DLL_POLICY_INVALID
- `1012` = DLL_DOWNLOAD_FAILED
- `1013` = DLL_HASH_MISMATCH
- `1014` = TICKET_REQUEST_FAILED
- `1015` = TICKET_WRITE_FAILED
- `1016` = INJECT_FAILED
- `1017` = RESULT_TIMEOUT
- `1018` = RESULT_INVALID
- `1019` = RESULT_HMAC_FAILED
- `1020` = REPORT_FAILED
- `1021` = PROTOCOL_MISMATCH
- `1022` = SELF_CHECK_FAILED

### 36.3 动作列表与 data 形状

#### get_status_snapshot

返回：

- `data.status_snapshot`
- 顶层 `status_snapshot` 若存在，仅视为兼容镜像

#### activate_device_v3

输入：

- `key_plaintext`

返回：

- `data.key_id`
- `data.device_id`
- `data.tier`
- `data.tier_name`
- `data.tier_value`
- `data.feature_flags`
- `data.status_snapshot`

#### device_check_v3

输入：

- 空或可选 `force=true`

返回：

- `data.status_snapshot`

#### device_heartbeat_v3

一般不由 EXE 直接调，内部定时器使用

#### dll_policy_v3

返回：

- `data.dll_name`
- `data.dll_sha256`
- `data.dll_size`
- `data.download_url`

#### issue_inject_ticket_v3

输入：

- `target_pid`

返回：

- `data.session_id`
- `data.ticket_id`
- `data.ticket_sha256`

#### consume_inject_result_v3

返回：

- `data.session_id`
- `data.ticket_id`
- `data.status`
- `data.reason`
- `data.granted_tier`
- `data.granted_feature_flags`
- `data.verified_dll_sha256`
- `data.issued_at_server`
- `data.expires_at_server`
- `data.server_pubkey_version`
- `data.server_pubkey_fingerprint`
- `data.verified_at_utc`

说明：

- `consume_inject_result_v3` 是 Auth DLL 本地动作
- 它只负责读取、校验并消费 `inject_result_v3.dat`
- 真实服务端回报固定使用 `report_inject_result_v3`

#### report_inject_result_v3

输入：

- `session_id`
- `ticket_id`
- `status`
- `reason`
- `verified_tier`
- `verified_tier_value`
- `verified_feature_flags`
- `verified_dll_sha256`
- `verified_at_utc`
- `issued_at_server`
- `expires_at_server`
- `server_pubkey_version`
- `server_pubkey_fingerprint`

返回：

- `data.accepted`

#### reset_v3_local_state

行为：

- 清空 `state_v3` 下 V3 状态
- 不触碰旧 V2 文件
- 一并清空当前 V3 key 的 QSettings 缓存

#### get_update_snapshot

返回：

- `data.main_exe`
- `data.auth_dll`
- `data.updater_exe`
- `data.overlay_dll`

---

## 37. 样本 JSON

### 37.1 license_v3.dat 逻辑样本

```json
{
  "schema_version": 3,
  "channel": "stable",
  "key_plaintext": "NB-XXXX-XXXX-XXXX",
  "key_id": "kid_7f0b1234abcd5678",
  "key_hash": "7f0b...",
  "device_id": "did_a1b2c3d4e5f60718293a4b5c6d7e8f90",
  "device_public_key_pem": "-----BEGIN PUBLIC KEY----- ...",
  "device_private_key_dpapi_blob_b64": "AQAAANCM...",
  "server_pubkey_version": 4,
  "server_pubkey_fingerprint": "sha256:abcd...",
  "last_known_tier": "Dev",
  "last_known_tier_value": 3,
  "last_known_feature_flags": 31,
  "last_known_dll_name": "NoteBotOverlay_v3.dll",
  "last_known_dll_sha256": "abcd1234...",
  "last_verified_at_utc": "2026-06-04T14:00:00Z",
  "online_state_cache": {
    "online": true,
    "kicked": false,
    "last_heartbeat_at_utc": "2026-06-04T14:00:00Z",
    "last_server_message": "ok"
  }
}
```

### 37.2 状态快照样本

```json
{
  "ok": true,
  "action": "get_status_snapshot",
  "rc": 0,
  "message": "ok",
  "data": {
    "status_snapshot": {
      "active": true,
      "online": true,
      "kicked": false,
      "license_status": "已激活",
      "tier_name": "Dev",
      "tier_value": 3,
      "feature_flags": 31,
      "key_id": "kid_7f0b1234abcd5678",
      "device_id": "did_a1b2c3d4e5f60718293a4b5c6d7e8f90",
      "bound_dll_name": "NoteBotOverlay_v3.dll",
      "bound_dll_sha256": "abcd1234...",
      "last_verified_at_utc": "2026-06-04T14:00:00Z",
      "last_heartbeat_at_utc": "2026-06-04T14:00:00Z",
      "replace_cooldown_remaining_sec": 0,
      "network_available": true,
      "update_state": "idle",
      "auth_dll_version": "3.4.43",
      "protocol_version": 3,
      "abi_version": 1
    }
  }
}
```

### 37.3 inject_result_v3.dat 逻辑样本

```json
{
  "magic": "NB_TICKET_RESULT_V3",
  "version": 3,
  "session_id": "sess_123",
  "ticket_id": "ticket_123",
  "ticket_sha256": "abcd1234",
  "status": "success",
  "reason": "",
  "dll_version": "overlay_3.0.0",
  "processed_tick_ms": 123456789,
  "result_hmac": "5e9a..."
}
```

---

## 38. 日志与审计规范

### 38.1 客户端本地日志类别

客户端日志固定分类：

- `BOOT`
- `UPDATE`
- `AUTH`
- `DEVICE`
- `HEARTBEAT`
- `POLICY`
- `TICKET`
- `INJECT`
- `RESULT`
- `ERROR`

### 38.2 服务端审计字段

服务端事件必须至少记录：

- `ts_utc`
- `ip`
- `key_id`
- `device_id`
- `action`
- `status`
- `session_id`
- `ticket_id`
- `dll_name`
- `details`

### 38.3 敏感信息日志规则

日志中不得明文输出：

- 设备私钥
- DPAPI blob
- 服务端私钥
- wrapper key

原始 key 仅在服务端管理操作日志中允许可见，在客户端日志中默认打码。

---

## 39. purge 后最小骨架要求

### 39.1 api.cpp purge 后最小骨架

`api.cpp` 删完旧逻辑后，至少保留：

- DLL 生命周期导出
- 回调注册
- `nb_get_protocol_version`
- `nb_get_abi_version`
- `nb_self_check`
- `nb_call`

不允许在 purge 之后还残留：

- `license_v2`
- `token_wire`
- `diagnose_v2`
- `auth_v2`
- `MutualVerifier`

### 39.2 cloud_client_tcp.cpp purge 后最小骨架

至少保留：

- TCP 建连
- AES-GCM 加解密
- V3 命令发送
- JSON 收发

不允许残留：

- `auth_v2`
- `license_migrate`
- `diagnose_v2`
- `inject_report`

### 39.3 backend.cpp purge 后最小骨架

至少保留：

- DLL 加载
- 更新流程
- 状态快照读取
- 注入触发
- 日志桥

不允许继续把：

- 旧本地验证
- 旧激活语义
- 旧诊断语义

写成 EXE 自己的业务判断。
