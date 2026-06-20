# NoteBot V3 服务端详细设计文档

## 1. 文档目的

这份文档用于定义 `server_v3` 的正式服务端设计。

它不是旧服务端的修补计划，也不是 `AUTH_V3_REWRITE_SPEC.md` 的摘要。

它的作用是把以下几件事定死：

1. V3 服务端的职责边界
2. 端口和协议拓扑
3. 客户端更新、下载、授权、票据的正式流程
4. 管理 HTTPS 的安全边界
5. 数据模型与存储组织
6. 新旧并存和迁移方式
7. 后续实现顺序

---

## 2. 当前前提

### 2.1 现状

当前线上旧服务端仍在运行，使用旧端口：

- `30065`：旧 TCP 主业务口
- `30066`：旧本地 HTTP
- `30067`：旧公网 HTTPS

新服务端 `server_v3` 已经独立建立基础骨架，使用新端口：

- `30165`
- `30166`
- `30167`

### 2.2 核心原则

本次不是在旧 `src/` 上继续修。

本次是：

- 在同一个工作区内新建 `server_v3/`
- 保留必要数据语义
- 底层框架、模块边界、运行逻辑全部重写

### 2.3 单一真相源

对于服务端设计，当前以这份文档为准。

对于客户端协议口型，当前以本地 `C:\NB` 代码为准。

服务端必须适配客户端，而不是反过来要求客户端改口。

---

## 3. 总体目标

`server_v3` 要解决的问题不是“旧代码太丑”，而是以下正式目标：

1. 提供完整 V3 客户端协议服务
2. 与旧业务并存，不影响现有运行
3. 把管理 HTTPS 与客户端业务彻底隔离
4. 把下载控制面和下载数据面理顺
5. 保留未来切对象存储/CDN 的扩展空间
6. 保证后续维护、加命令、加字段、升级协议时不需要再推倒重来

---

## 4. 端口与职责边界

### 4.1 `30165` - 客户端正式业务入口

`30165` 是客户端唯一正式业务入口。

它同时承载两类流量：

1. 私有 TCP 主协议
2. 标准 HTTPS 下载

也就是说：

- 客户端授权、心跳、票据、更新检查都先走私有 TCP
- 客户端真正下载文件时，走 `https://notebot-api.fucku.top:30165/...`

### 4.2 `30167` - 纯管理 HTTPS

`30167` 永远只做管理后台用途。

它只服务：

- 登录页
- 管理后台前端
- 管理后台 API
- 最小探活接口

它不承载：

- 客户端授权
- 客户端更新
- 客户端票据
- 客户端下载

### 4.3 `30166` - 本地管理 HTTP

`30166` 是本地管理和探活用 HTTP。

它主要用于：

- 本机健康检查
- 本机开发调试
- 反向代理之前的本地直连调试

它不对公网暴露正式业务。

---

## 5. `30165` 的协议复用设计

### 5.1 为什么要复用

因为需求已经定死：

- 客户端所有正式业务都走 `30165`
- 下载也必须走客户端口
- 不额外新增下载端口

所以 `30165` 必须支持：

1. 私有 TCP 协议
2. HTTPS 下载

### 5.2 入口识别规则

`30165` 的 listener 必须先识别首包类型。

建议规则：

1. 若首字节和握手形态符合 TLS ClientHello，则进入 HTTPS 下载分支
2. 否则按私有 TCP 协议处理

### 5.3 私有 TCP 分支

私有 TCP 分支继续沿用当前客户端已有模型：

- 自定义长度前缀
- 自定义 AES-GCM 加密
- JSON 请求/响应

本次不把客户端主协议整体升级成 TLS。

### 5.4 HTTPS 下载分支

下载分支只负责：

- 验证短时下载 URL 中的签名/过期时间/产物信息
- 输出文件流
- 支持下载重试

不参与：

- 管理台逻辑
- 授权业务判断

---

## 6. 客户端正式协议范围

通过 `30165` 私有 TCP 承载的正式业务包括：

1. `activate_device_v3`
2. `device_check_v3`
3. `device_heartbeat_v3`
4. `dll_policy_v3`
5. `issue_inject_ticket_v3`
6. `report_inject_result_v3`
7. `update_manifest_v3`

其中：

- `update_manifest_v3` 用于启动阶段更新
- `dll_policy_v3` 用于授权后获取当前 key 绑定的 `NoteBot` 注入 DLL

---

## 7. 更新与下载设计

### 7.1 为什么要拆成两层

客户端存在两种完全不同的状态：

1. 没有 key，或者还没激活
2. 已经完成有效授权，有 key 绑定状态

所以更新必须拆成两层。

### 7.2 第一层：启动更新层

命令名：

- `update_manifest_v3`

特点：

- 无密钥可用
- 单线路
- 只返回启动必要组件

返回产物范围只包括：

- `main_exe`
- `auth_dll`
- `updater_exe`

这一层用于保证：

- 新机器首次启动也能更新
- 本地没有有效状态也能修复启动链
- 客户端不需要先有 key 才能拿到基础运行组件

### 7.3 第二层：业务 DLL 策略层

命令名：

- `dll_policy_v3`

特点：

- 必须已经完成有效授权
- 与 key 强绑定
- 返回当前 key 绑定的 `NoteBot` 注入 DLL

这一层不允许无密钥获取。

### 7.4 `update_manifest_v3` 的客户端流程

客户端启动后：

1. 连接 `30165`
2. 发 `update_manifest_v3`
3. 服务端返回：
   - 当前最新版本
   - `md5`
   - `sha256`
   - `size`
   - `required`
   - 短时有效下载 URL
   - URL 过期时间
4. 客户端直接下载
5. 下载失败时：
   - 如果 URL 还没过期，继续用原 URL 重试
   - 如果 URL 已过期，重新走第 2 步申请

### 7.5 下载 URL 的设计原则

下载 URL 必须满足：

1. 不可猜
2. 短时有效
3. 带签名
4. 带产物身份
5. 带过期时间
6. 可扩展到对象存储/CDN

建议 URL 语义：

- 第一版由本机服务端自己签发并校验
- 以后切对象存储/CDN 时，仍保留“服务端返回短时 URL”这一模型

客户端上层流程不变。

### 7.6 下载控制面与数据面

必须明确：

- 控制面：`30165` 私有 TCP
- 数据面：`30165` HTTPS 下载 URL

控制面负责：

- 判断客户端能否下载
- 返回下载元数据

数据面负责：

- 吐文件流

---

## 8. `update_manifest_v3` 请求与返回建议

### 8.1 请求特点

第一版：

- 无密钥也可发
- 不做灰度分流
- 默认只有一条稳定主线

### 8.2 请求中可带的扩展数据

每次检查更新时，客户端可以顺手带：

1. `client_runtime_info`
2. 当前本地已安装产物清单

### 8.3 `client_runtime_info`

第一版建议收这些：

- `hostname`
- `os_name`
- `os_version`
- `cpu_model`
- `gpu_model`
- `ram_mb`
- `system_disk_total_mb`
- `system_disk_free_mb`
- `client_version`
- `auth_dll_version`
- `protocol_version`

服务端第一版规则：

- 有就收
- 没有不报错
- 不参与授权主判定

### 8.4 已安装产物清单

第一版建议带：

- `main_exe.version`
- `main_exe.version_code`
- `auth_dll.version`
- `auth_dll.version_code`
- `updater_exe.version`
- `overlay_dll.version`
- `overlay_dll.sha256`

作用：

- 让服务端判断更精准
- 方便故障排查
- 方便之后检查“客户端到底装了什么”

### 8.5 返回内容建议

建议返回统一 manifest 结构，但只限启动组件范围。

例如：

```json
{
  "status": "ok",
  "server_time_utc": "2026-06-04T14:30:00Z",
  "artifacts": {
    "main_exe": {
      "version": "3.4.80",
      "version_code": 30480,
      "size": 43082240,
      "md5": "....",
      "sha256": "....",
      "required": true,
      "download_url": "https://notebot-api.fucku.top:30165/download/....",
      "url_expires_at_utc": "2026-06-04T14:31:30Z"
    },
    "auth_dll": {
      "version": "3.4.60",
      "version_code": 30460,
      "size": 12646400,
      "md5": "....",
      "sha256": "....",
      "required": true,
      "download_url": "https://notebot-api.fucku.top:30165/download/....",
      "url_expires_at_utc": "2026-06-04T14:31:30Z"
    },
    "updater_exe": {
      "version": "3.4.80",
      "version_code": 30480,
      "size": 7403520,
      "md5": "....",
      "sha256": "....",
      "required": false,
      "download_url": "https://notebot-api.fucku.top:30165/download/....",
      "url_expires_at_utc": "2026-06-04T14:31:30Z"
    }
  }
}
```

---

## 9. 管理 HTTPS 设计

### 9.1 总体边界

`30167` 是纯管理 HTTPS。

它不服务任何客户端正式业务。

### 9.2 未登录状态

未登录时：

- 只返回登录页所需最小资源
- 不返回后台主界面资源
- 不返回任何后台数据

### 9.3 已登录状态

已登录后：

- 才允许返回后台主界面资源
- 才允许调用后台管理 API

当前第一版最小后台 API 固定包括：

- `POST /api/login`
- `POST /api/logout`
- `GET /api/session`
- `GET /api/overview`

其中：

- `GET /api/session` 在已登录时返回 `logged_in / role / expires_at_utc / csrf_token`
- 根路径 `/` 未登录只返回最小登录页，已登录返回内嵌只读后台面板

当前第一版只读管理 API 还包括：

- `GET /api/licenses`
- `GET /api/devices`
- `GET /api/online-sessions`
- `GET /api/inject-sessions`
- `GET /api/artifacts`
- `GET /api/events?limit=N`
- `GET /api/runtime-reports?limit=N`
- `GET /api/ticket-signer`
- `GET /api/key-scan?compact=true|false`

后台当前可直接观察到：

- license 主数据
- 设备主数据
- 在线会话
- 注入会话
- 产物清单
- 最近事件
- 最近更新请求上报
- 当前 ticket signer 状态
- 当前主机范围内是否已存在匹配正式公钥链的私钥材料

### 9.4 探活

允许一个最小匿名接口：

- `GET /healthz`

它只返回最小状态，不返回业务详情。

例如：

```json
{
  "status": "ok",
  "service": "server_v3",
  "time_utc": "2026-06-04T14:30:00Z"
}
```

---

## 10. 管理 HTTPS 安全策略

### 10.1 默认拒绝

`30167` 必须采用默认拒绝策略。

只允许：

1. 明确注册的 API 路由
2. 明确指定的前端构建产物
3. 最小 `healthz`

### 10.2 Host 白名单

`30167` 必须强制 Host 白名单。

当前允许：

- `notebot-api.fucku.top`

其他 Host：

- 直接拒绝

### 10.3 不允许暴露真实文件系统

绝不允许直接暴露：

- `data_v3`
- `keys`
- `.git`
- `node_modules`
- 日志
- 任意真实磁盘目录

### 10.4 登录态

管理端第一版采用：

- 单管理员密码
- `HttpOnly Cookie session`

不采用：

- Bearer token 作为管理端登录态

### 10.5 当前已落地的安全能力

当前已经落地：

1. 登录失败限流
2. 管理端操作审计事件
3. Session 过期策略
4. CSRF 防护
5. 内容安全头
6. 统一错误响应

当前安全面固定包括：

- Host 白名单
- `HttpOnly Cookie session`
- 辅助 `csrf` cookie + `x-nb-csrf`
- 同源 `Origin / Referer` 检查
- 登录失败限流第一版
- 基础安全响应头
- HTML 页 nonce CSP
- 统一 `status=error` JSON 错误壳

当前还补了两层可见性：

- `overview.ticket_signer` 直接返回签票私钥 readiness / fingerprint / reason
- 启动日志会明确打印 `ticket signer not ready`
- `overview.ticket_signer.keyPath` 直接返回当前签票私钥来源路径

当前运维还应固定提供两条脚本命令：

- `npm run verify:ticket-signer`
- `npm run scan:key-fingerprints`

---

## 11. 数据模型设计

### 11.1 原则

新系统不再让一个 `licenses.json` 承担所有职责。

要分清：

- 授权真相数据
- 设备真相数据
- 在线状态
- 注入会话
- 产物信息
- 审计事件

### 11.2 建议的主数据集合

#### `licenses`

字段建议：

- `key_id`
- `key_plaintext_normalized`
- `key_hash`
- `tier`
- `tier_value`
- `feature_flags`
- `revoked`
- `max_devices`
- `bound_notebot_name`
- `bound_notebot_sha256`
- `created_at_utc`
- `updated_at_utc`

#### `devices`

字段建议：

- `key_id`
- `device_id`
- `device_public_key_pem`
- `first_activated_at_utc`
- `last_seen_at_utc`
- `status`

说明：

- `devices` 只承载 V3 正式设备身份，不再把旧 `system_hash / machine_hash` 当成正式设备真相
- 旧协议里的 `system_hash / machine_hash` 仅进入 `legacy_devices_archive`，用于审计和对账

#### `online_sessions`

字段建议：

- `key_id`
- `device_id`
- `online`
- `kicked`
- `last_heartbeat_at_utc`

#### `inject_sessions`

字段建议：

- `session_id`
- `ticket_id`
- `key_id`
- `device_id`
- `notebot_name`
- `notebot_sha256`
- `target_pid`
- `issued_at_utc`
- `expires_at_utc`
- `reported`
- `status`

#### `bootstrap_artifacts`

字段建议：

- `artifact_type`
- `version`
- `version_code`
- `file_name`
- `size`
- `md5`
- `sha256`
- `required`
- `enabled`
- `published_at_utc`

#### `notebot_artifacts`

字段建议：

- `artifact_id`
- `file_name`
- `size`
- `md5`
- `sha256`
- `version`
- `published_at_utc`
- `enabled`

#### `runtime_reports`

字段建议：

- `key_id`
- `device_id`
- `reported_at_utc`
- `client_version`
- `auth_dll_version`
- `payload_json`

#### `events`

字段建议：

- `ts_utc`
- `category`
- `action`
- `status`
- `key_id`
- `device_id`
- `details`

---

## 12. 旧数据迁移思路

### 12.1 旧文件

当前旧数据文件包括：

- `data/licenses.json`
- `data/activations.json`
- `data/dlls.json`
- `data/settings.json`
- `data/events.json`

### 12.2 要继承的语义

应继承：

- 已存在的 key
- tier / featureFlags
- activatedAt / expiresAt
- devices 绑定关系
- 当前 DLL 元数据
- 下载模式设置

### 12.3 不继续直接复用的东西

不继续作为新系统真相源的：

- 旧 `activations.json`
- 旧 `events.json` 的堆数组写法
- 旧 `db.js` 那种“读写+归一化+业务规则揉一层”的实现方式

### 12.4 迁移策略

第一步做：

- 旧数据只读 importer

从旧文件导入到：

- `server_v3/data_v3/`

迁移完成后：

- 新系统只读写 `data_v3`
- 旧 `data/` 保留为历史快照

---

## 13. 传输层模块设计

### 13.1 `transport/tcp`

职责：

- 监听 `30165`
- 首包识别协议类型
- 分流：
  - 私有 TCP
  - HTTPS 下载

### 13.2 `transport/https-admin`

职责：

- 监听 `30167`
- 承载管理后台
- 登录页与后台 API
- Host 白名单与安全头

### 13.3 `transport/http-local`

职责：

- 监听 `30166`
- 本地调试和本地健康检查

---

## 14. 协议层模块设计

### 14.1 `protocol/commands`

维护正式命令枚举：

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- `dll_policy_v3`
- `issue_inject_ticket_v3`
- `report_inject_result_v3`
- `update_manifest_v3`

### 14.2 `protocol/errors`

维护与客户端一致的错误码体系。

### 14.3 `protocol/status`

维护服务端返回的状态枚举与内部语义映射。

### 14.4 `protocol/canonical-json`

统一生成：

- 更新检查
- 票据签名
- 各类签名载荷

避免不同模块各写一套拼接逻辑。

---

## 15. 业务模块设计

### 15.1 `domain/auth`

负责：

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- 在线策略
- kicked
- 替换冷却

### 15.2 `domain/updates`

负责：

- `update_manifest_v3`
- 启动组件清单
- 短时下载 URL 签发
- `client_runtime_info` 记录

### 15.3 `domain/notebot`

负责：

- `dll_policy_v3`
- 当前 key 绑定的 `NoteBot` 注入 DLL 策略

### 15.4 `domain/ticket`

负责：

- `issue_inject_ticket_v3`
- `report_inject_result_v3`
- 会话锁
- 结果接收状态

### 15.5 `domain/admin`

负责：

- 管理员登录
- Session
- 后台管理操作

---

## 16. 下载服务设计

### 16.1 抽象原则

下载必须拆成两层：

1. 下载授权签发
2. 文件流输出

### 16.2 第一版实现

第一版：

- 由本机服务端签发下载 URL
- 由本机 `30165` 的 HTTPS 分支输出文件流

### 16.3 后续扩展

以后可以替换为：

- 对象存储
- CDN
- 外部下载源

但客户端只认：

- 服务端先返回短时有效 URL

这个模型不变。

---

## 17. 实施顺序

### 阶段 1：基础设施

1. `server_v3` 目录骨架
2. 端口和配置
3. `30165` 协议复用 listener
4. `30167` 管理 HTTPS 壳
5. `30166` 本地 HTTP 壳

### 阶段 2：更新层

1. `update_manifest_v3`
2. 启动组件清单
3. 短时下载 URL
4. 下载数据面
5. `client_runtime_info`

### 阶段 3：授权层

1. `activate_device_v3`
2. `device_check_v3`
3. `device_heartbeat_v3`

### 阶段 4：业务 DLL 层

1. `dll_policy_v3`
2. key 绑定 `NoteBot` 注入 DLL

### 阶段 5：票据层

1. `issue_inject_ticket_v3`
2. `report_inject_result_v3`

### 阶段 6：管理台

1. 登录
2. Session
3. key 管理
4. device 管理
5. 产物管理
6. 审计查看

### 阶段 7：迁移和切流

1. 旧数据导入
2. 本地客户端联调
3. 新端口验证
4. 正式切流
5. 删除旧业务

---

## 18. 非目标

这份设计明确不做的事情：

1. 不继续修旧 `src/` 的内部结构
2. 不让 `30167` 承接客户端业务
3. 不把管理 HTTPS 和客户端下载混在一起
4. 不在第一版做多线路灰度分发
5. 不在第一版把客户端主协议改成 TLS

---

## 19. 结论

`server_v3` 的正式方向已经明确：

1. 新旧并存
2. 客户端统一走 `30165`
3. 管理后台统一走 `30167`
4. 启动组件更新与 key 绑定业务 DLL 策略彻底拆开
5. 下载用真实短时 URL，但仍受 `30165` 控制
6. 管理 HTTPS 按默认拒绝策略设计
7. 后续实现以这份文档为准
