# NoteBot V3 服务端重建方案

## 1. 这份文档是干什么的

这份文档不再讨论“旧服务端每个文件怎么修”。

这份文档只做一件事：

- **定义如何在现有服务端工作区里，新建一套 `server_v3/`，完整重写服务端底层实现，同时保留必要的业务语义和历史数据。**

目标不是补丁式重构。

目标是：

- 旧服务端继续跑
- 新服务端在同仓库并行开发
- 最终切流到新服务端
- 再把旧业务整块删除

---

## 2. 已确认的核心决策

### 2.1 要保留的东西

只保留这些“业务真相”：

- 已存在的密钥数据
- 已激活状态
- 设备绑定关系
- 在线状态语义
- tier / feature_flags
- 当前 DLL 元数据语义
- HTTPS / 域名 / 端口这些部署外壳
- 与本地 `C:\NB` EXE / Auth DLL 已冻结的协议口型

### 2.2 不保留的东西

这些默认全部推倒重写：

- 旧 TCP 分发结构
- 旧 `handler.js` 命令大杂烩
- 旧 `auth.js / auth_v2.js / diagnose.js` 的内部组织
- 旧数据库读写层
- 旧 DLL 下载逻辑
- 旧 Web 管理接口的内部实现方式
- 旧模块边界
- 旧 V1 / V2 / 过渡时代逻辑的代码结构

### 2.3 新旧并存策略

在原仓库 `/home/william/NoteBot-API` 内新建：

- `server_v3/`

旧代码继续留在：

- `src/`

新代码全部进入：

- `server_v3/`

这样做的结果是：

- 旧业务还能继续运行
- 新业务能干净开发
- 后期删旧代码时，只需要删旧目录，不会伤到新结构

---

## 3. 为什么必须整套重写

远端现状已经说明问题不是“某个模块写差了”，而是**整体结构已经不可维护**。

### 3.1 旧服务端当前真实问题

根据远端代码检查，当前服务端存在这些结构性问题：

1. **TCP 协议分发过于集中**
   - `src/handler.js` 同时处理：
     - V1 auth
     - V2 auth
     - diagnose
     - migrate
     - inject_report
     - DLL 更新
     - DLL 下载
   - 这是典型的“所有业务都塞在一个入口判断里”

2. **Web、TCP、下载、管理台混在一起**
   - `src/index.js` 同时负责：
     - TCP server
     - HTTP server
     - HTTPS / mux
     - 登录
     - 静态资源
     - API 挂载

3. **旧业务版本长期叠床架屋**
   - `auth.js`
   - `auth_v2.js`
   - `diagnose.js`
   - `dll_update_check`
   - `auth_dll_bootstrap`
   - 这些不是“演进清楚的兼容层”，而是多代逻辑并存互相污染

4. **数据模型和业务模型耦合过紧**
   - `src/db.js` 直接对 JSON 文件做结构归一化、读写、设备语义、事件裁剪
   - 导致“存储格式”、“业务规则”、“兼容清洗”揉在同一层

5. **配置项过度膨胀但没有形成清晰边界**
   - `src/config.js` 把 TCP、TLS、Web、JWT、DLL、Qiniu、登录限制、路径策略都混在一个对象里

6. **旧客户端语义、旧下载方式、旧产物管理方式彼此缠绕**
   - `src/dll_policy.js`
   - `src/dll_delivery.js`
   - `src/dll_metadata.js`
   - `src/api.js`
   - 都在围绕旧世界做叠加，不适合作为 V3 正式基础

### 3.2 结论

所以我们这次不应该“重构旧结构”。

我们应该：

- **保留数据和业务语义**
- **彻底重写实现和模块边界**

---

## 4. 当前旧服务端里哪些东西必须继承

以下是本次重建时应继承的“业务层语义”，不是代码实现。

### 4.1 旧数据文件中的有效信息

远端现有数据文件：

- `data/licenses.json`
- `data/activations.json`
- `data/dlls.json`
- `data/settings.json`
- `data/events.json`

其中真正需要继承的语义有：

#### `licenses.json`

要保留：

- `key`
- `tier`
- `featureFlags`
- `durationDays`
- `activatedAt`
- `expiresAt`
- `revoked`
- `dllVersion`
- `maxDevices`
- `devices[]`
- `features`

设备数组里的有用字段：

- `deviceId`
- `machineHash`
- `systemHash`
- `firstSeenAt`
- `lastSeenAt`
- `status`

这些字段在 V3 中只保留为历史导入参考或归档数据：

- 不直接写进 V3 `devices` 正式主模型
- 不继续作为 V3 设备身份判定真相

#### `activations.json`

这是历史流水，不是新系统主真相源。

它可以保留为：

- 导入参考
- 审计参考

但不应继续作为新服务端的正式主数据模型。

#### `dlls.json`

要保留的语义：

- 当前 Auth DLL 元数据
- Overlay DLL 元数据
- 文件名 / hash / size / version / enabled 状态
- 当前选中的 Auth DLL

#### `settings.json`

要保留的语义：

- `dllDownloadMode`

#### `events.json`

只作为历史审计数据源。

不应继续沿用当前“事件数组直接堆 JSON 文件”的实现方式。

---

## 5. 与本地客户端对齐的硬边界

新服务端最终必须服从本地 `C:\NB` 已冻结方案。

也就是说，服务端设计可以重写，但以下内容不应擅自改口：

### 5.1 正式命令

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- `dll_policy_v3`
- `issue_inject_ticket_v3`
- `report_inject_result_v3`
- `update_manifest_v3`

### 5.2 正式返回和状态语义

Auth DLL 本地只认：

- 明确的 `status`
- 约定字段名
- 约定字段类型

### 5.3 设备 / 在线 / kicked / cooldown 语义

必须服从本地已定规则：

- 同 key 最多 2 台设备
- 同时只允许 1 台在线
- 第 3 台设备触发替换逻辑
- `24h` 内最多替换一次
- 新设备上线踢掉旧在线设备
- `90s` 无心跳判离线

### 5.4 票据链语义

必须服从本地 Overlay 和 Auth DLL 的真实消费规则：

- inner ticket canonical JSON
- `server_ticket_signature`
- `ticket_sha256`
- wrapper 外层由本地生成
- Overlay 只认正式服务端公钥

### 5.5 更新协议

正式只认：

- `update_manifest_v3`

不再复活：

- `auth_dll_bootstrap`
- `dll_update_check_v2`

---

## 6. 新服务端的技术方向

### 6.1 语言和运行时

建议继续使用 Node.js 生态，但**改为 TypeScript**。

理由：

1. 当前旧服务端已经是 Node.js 仓库
2. 迁移成本比跨语言重写低
3. TypeScript 能显著改善协议类型、模块边界、长期维护性

建议基线：

- Node.js 20+
- TypeScript

### 6.2 Web 框架

建议：

- **Fastify**

原因：

- 结构比当前手写 Express 入口更清晰
- 插件化强
- schema / validation / hooks 更适合协议型后端
- 性能和可维护性都够用

### 6.3 校验与 schema

建议：

- `zod`

用于：

- 配置校验
- TCP request schema
- HTTP request schema
- 响应 schema
- 数据导入 schema

### 6.4 日志

建议：

- `pino`

替代当前自写日志拼接器。

### 6.5 加密

继续使用 Node 自带 `crypto`，但改成独立模块：

- TCP AES-GCM transport crypto
- device signature verify
- server ticket sign
- fingerprint util

不要再混在业务 handler 里。

---

## 7. 新目录结构

建议在 `/home/william/NoteBot-API` 下新建：

```text
server_v3/
  package.json
  tsconfig.json
  src/
    app/
      bootstrap.ts
      runtime.ts
    config/
      env.ts
      schema.ts
    transport/
      tcp/
        server.ts
        session.ts
        framing.ts
        crypto.ts
      http/
        server.ts
        plugins/
        routes/
      https/
        tls.ts
    protocol/
      commands.ts
      status.ts
      errors.ts
      canonical-json.ts
      versions.ts
    domain/
      auth/
        auth.service.ts
        device-policy.ts
        online-policy.ts
      ticket/
        ticket.service.ts
        ticket-signing.ts
        inject-session.service.ts
      updates/
        update-manifest.service.ts
      dll/
        dll-policy.service.ts
        artifact.service.ts
      admin/
        admin-auth.service.ts
    repositories/
      licenses.repo.ts
      devices.repo.ts
      online-sessions.repo.ts
      inject-sessions.repo.ts
      artifacts.repo.ts
      events.repo.ts
      settings.repo.ts
    storage/
      json/
        json-file.ts
      importers/
        legacy-import.ts
      snapshots/
    adapters/
      legacy/
        licenses.ts
        dlls.ts
        settings.ts
    web/
      routes/
        auth.routes.ts
        keys.routes.ts
        dll.routes.ts
        events.routes.ts
        stats.routes.ts
      middleware/
    utils/
      time.ts
      ids.ts
      hash.ts
      logger.ts
    types/
      index.ts
  data_v3/
  scripts/
    import-legacy.ts
    export-snapshot.ts
```

---

## 8. 新系统的模块边界

### 8.1 `transport/`

职责：

- 只负责收发
- 不直接承载业务规则

分成：

- `transport/tcp/`
- `transport/http/`
- `transport/https/`

TCP 只负责：

- 长度前缀
- AES-GCM 解包
- JSON 解析
- 命令路由

HTTP 只负责：

- Admin API
- 下载 URL
- 健康检查

### 8.2 `protocol/`

职责：

- 定义命令名
- 定义字段顺序
- 定义错误码
- 定义状态枚举
- 定义 canonical JSON 规则

这层是客户端协议契约，不应掺业务存储。

### 8.3 `domain/`

职责：

- 真正的业务语义

拆分：

- `auth`
- `ticket`
- `updates`
- `dll`
- `admin`

### 8.4 `repositories/`

职责：

- 读写数据
- 不做业务决策

比如：

- `licenses.repo.ts` 只负责 key 记录
- `devices.repo.ts` 只负责设备记录
- `online-sessions.repo.ts` 只负责在线态

不要再像旧 `db.js` 那样什么都揉一起。

### 8.5 `adapters/legacy/`

职责：

- 旧数据导入
- 老字段兼容解释

只允许出现在迁移期。

不允许新业务长期依赖它。

---

## 9. 新数据模型设计

### 9.1 不再让一个 `licenses.json` 承担所有东西

旧服务端把：

- 授权状态
- 设备状态
- feature
- DLL 绑定
- 激活时间

都混进同一条 license 记录里。

新系统应拆开。

### 9.2 建议的主数据集合

#### `licenses`

字段建议：

- `keyId`
- `keyPlaintextNormalized`
- `keyHash`
- `tier`
- `tierValue`
- `featureFlags`
- `boundDllName`
- `boundDllSha256`
- `channel`
- `revoked`
- `maxDevices`
- `createdAtUtc`
- `updatedAtUtc`

#### `devices`

字段建议：

- `keyId`
- `deviceId`
- `devicePublicKeyPem`
- `firstActivatedAtUtc`
- `lastSeenAtUtc`
- `status`

说明：

- V3 `devices` 只存正式设备身份与活跃信息
- 旧 `systemHash / machineHash` 只进 `legacy_devices_archive.json`

#### `onlineSessions`

字段建议：

- `keyId`
- `deviceId`
- `online`
- `kicked`
- `lastHeartbeatAtUtc`
- `sessionVersion`

#### `injectSessions`

字段建议：

- `sessionId`
- `ticketId`
- `keyId`
- `deviceId`
- `dllName`
- `dllSha256`
- `targetPid`
- `issuedAtUtc`
- `expiresAtUtc`
- `reported`
- `status`

#### `artifacts`

字段建议：

- `artifactType`
- `channel`
- `version`
- `versionCode`
- `fileName`
- `sha256`
- `md5`
- `size`
- `downloadUrl`
- `required`
- `protocolMin`
- `protocolMax`
- `keyScope`
- `publishedAtUtc`
- `enabled`

#### `settings`

字段建议：

- `dllDownloadMode`
- `updatedAtUtc`

#### `events`

字段建议：

- `tsUtc`
- `category`
- `action`
- `status`
- `keyId`
- `deviceId`
- `details`

---

## 10. 旧数据迁移策略

### 10.1 原则

旧数据要保留，但旧格式不应继续成为新系统内部真相源。

### 10.2 第一步

新服务端先做：

- **只读导入器**

从旧文件读取：

- `data/licenses.json`
- `data/dlls.json`
- `data/settings.json`
- 必要时 `data/events.json`

导入到新结构：

- `server_v3/data_v3/`

### 10.3 第二步

当新系统验证通过后：

- 新系统只读写 `data_v3/`
- 旧 `data/` 变成历史快照

### 10.4 第三步

切流完成后：

- 旧 `src/`
- 旧 `data/` 的运行依赖

都可以清理掉，只保留归档快照。

---

## 11. 新系统必须先做的服务端能力

第一阶段不要先碰管理台花活，先做**协议主链**。

### 第一阶段：授权三件套

1. `activate_device_v3`
2. `device_check_v3`
3. `device_heartbeat_v3`

必须一次做对的逻辑：

- 设备上限
- 单在线
- kicked
- 24h cooldown
- 协议版本拒绝

### 第二阶段：票据三件套

4. `dll_policy_v3`
5. `issue_inject_ticket_v3`
6. `report_inject_result_v3`

### 第三阶段：更新清单

7. `update_manifest_v3`

### 第四阶段：管理台

管理台后做，而且只围绕新数据模型做，不再把旧 admin API 原样搬过去。

当前最小正式接口面固定为：

- `POST /api/login`
- `POST /api/logout`
- `GET /api/session`
- `GET /api/overview`

其中：

- `GET /api/session` 已固定返回 `logged_in / role / expires_at_utc / csrf_token`
- `/` 未登录返回最小登录页，已登录返回内嵌只读后台面板

当前最小只读管理面固定为：

- `GET /api/licenses`
- `GET /api/devices`
- `GET /api/online-sessions`
- `GET /api/inject-sessions`
- `GET /api/artifacts`
- `GET /api/events?limit=N`
- `GET /api/runtime-reports?limit=N`
- `GET /api/ticket-signer`
- `GET /api/key-scan?compact=true|false`

其中：

- `overview` 必须直接暴露当前签票私钥是否与本地正式公钥链一致
- 这不是附属信息，而是票据链联调前的必要自检面
- 当前签票私钥路径也必须可见，方便部署时快速确认是否指到了正确文件

当前部署侧还应固定支持：

- `NB_V3_TICKET_SIGNING_KEY_PATH`

这样后续拿到正确正式私钥后，只需改环境变量或替换文件，不需要再改代码。

当前运维侧还应固定保留：

- `npm run verify:ticket-signer`
- `npm run scan:key-fingerprints`

前者用于直接判断当前签票私钥是否就绪，后者用于整机排查是否已经存在匹配正式公钥链的私钥材料。

后台与脚本必须保持同一口径：

- 后台 `ticket-signer` / `key-scan` 返回的结论，应与运维脚本完全一致
- 后台审计事件应记录 `login / logout / host_reject / csrf_reject / unauthorized`

---

## 12. 新系统必须明确避免的坑

### 12.1 不要再有“超级 handler”

像旧 `src/handler.js` 这种：

- 一个文件承接所有 TCP 命令

在新系统里禁止出现。

### 12.2 不要让 repo 层带业务语义

像旧 `src/db.js` 那种：

- 一边读 JSON
- 一边自动修 license
- 一边改 device
- 一边裁事件

新系统禁止。

### 12.3 不要把 V1/V2/V3 混在正式目录

如果需要兼容旧命令：

- 放在 `compat/legacy/`

默认新正式目录只写 V3。

### 12.4 不要再把下载、Qiniu、本地缓存、API 和元数据揉成一坨

至少分开：

- artifact metadata
- delivery policy
- public download url
- local cache warmer

### 12.5 不要让配置再无限长大

配置必须分层：

- runtime
- transport
- security
- storage
- updates
- web-admin

---

## 13. 新系统的管理与扩展规则

### 13.1 新增命令规则

新增任何 TCP 命令都必须同时补：

1. request schema
2. response schema
3. handler
4. domain service
5. repository contract
6. audit event

### 13.2 新增字段规则

新增客户端协议字段必须：

1. 明确类型
2. 明确是否参与签名
3. 明确是否参与 canonical JSON
4. 明确是否为兼容字段

### 13.3 新增数据表规则

新增存储结构必须先判断：

- 是业务真相
- 还是缓存
- 还是审计

三者不能混写成一份。

### 13.4 管理台规则

管理台 API 只允许调用 domain service。

不能直接手搓 JSON 文件。

---

## 14. 实施顺序

### Step 1

在旧仓库里新建：

- `server_v3/`

只做脚手架，不碰旧 `src/`

### Step 2

落配置、日志、transport、protocol 四个底层骨架

### Step 3

实现旧数据只读 importer

### Step 4

实现授权三件套

### Step 5

实现票据三件套

### Step 6

实现 `update_manifest_v3`

### Step 7

实现新的 Admin API

### Step 8

联调 `C:\NB` 本地客户端

### Step 9

切流

### Step 10

删除旧业务

---

## 15. 最终结论

这次的正确做法不是：

- “把旧后端慢慢修顺眼”

而是：

- **保留数据语义**
- **冻结本地客户端协议**
- **在同一工作区新建 `server_v3/`**
- **把底层框架、模块边界、存储结构、协议实现全部重写**

一句话说完：

- **旧服务端保留历史价值。**
- **新服务端才是以后真正要维护的系统。**
