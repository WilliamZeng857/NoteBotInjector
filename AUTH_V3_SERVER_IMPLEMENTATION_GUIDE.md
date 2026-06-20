# NoteBot V3 服务端实施说明书（以本地 `C:\NB` 代码为准）

## 1. 目的与边界

这份文档不是 `AUTH_V3_REWRITE_SPEC.md` 的缩写版。

这份文档的目标只有一个：

- **把当前本地 `C:\NB` 已冻结的客户端实现，翻译成一份服务端可直接照着开工的实施说明书**

从现在开始，服务端实现应以这份文档和本地代码为准，而不是反过来要求 EXE / Auth DLL 改口。

本文覆盖：

- Auth DLL 联网授权三件套
- DLL 策略、票据签发、结果回报
- 更新清单协议
- 本地文件与票据链约束
- 服务端模块划分、数据模型、扩展规则

本文不覆盖：

- VMP 壳后发布流程
- 业务 DLL 内部具体功能逻辑
- UI 设计细节

---

## 2. 本地冻结结论

当前本地已经可以视为**服务端唯一适配目标**。

服务端以后要跟的，不再是“大概的 V3 思路”，而是下面这些已经落进代码的正式契约：

1. Auth DLL 稳定导出与加载门禁
2. `nb_call` 动作名
3. `ok / action / rc / message / data` 返回壳
4. `data.status_snapshot` 正式状态快照
5. 授权 RPC 请求字段顺序与签名输入
6. `update_manifest_v3` 请求/响应形状
7. `inject_ticket_v3.dat` / `inject_result_v3.dat` / `consumed_tickets_v3.json` 本地格式
8. Overlay 对服务端票据、公钥、签名、hash、TTL、replay 的校验规则

---

## 3. 本地代码真相源

后续服务端开发时，优先看这些文件。

### 3.1 Auth DLL / 协议

- `src__auth_dll/src/v3/v3_rpc_client.h:8`
- `src__auth_dll/src/v3/v3_rpc_client.cpp:87`
- `src__auth_dll/src/v3/v3_actions.cpp:72`
- `src__auth_dll/src/v3/v3_state.h:14`
- `src__auth_dll/src/v3/v3_state.cpp:362`
- `src__auth_dll/src/api.cpp:107`

### 3.2 更新链

- `src__injector_exe/updater.cpp:189`
- `src__injector_exe/updater.cpp:485`
- `src__injector_exe/updater.cpp:648`
- `src__injector_exe/backend.cpp:332`
- `src__injector_exe/backend.cpp:570`
- `src__updater_exe/updater_main.cpp:210`

### 3.3 票据链 / Overlay

- `src__auth_dll/src/v3/v3_state.cpp:1228`
- `src__auth_dll/src/v3/v3_state.cpp:1531`
- `src__auth_dll/src/protected/protected_ticket_ops.cpp:9`
- `src__auth_dll/src/protected/protected_verify_ops.cpp:5`
- `src__auth_dll/src/v3/v3_server_pubkey.h:34`
- `src__overlay_dll/overlay_main.cpp:153`
- `src__overlay_dll/overlay_main.cpp:270`
- `src__overlay_dll/overlay_main.cpp:465`

---

## 4. 本地组件边界

### 4.1 EXE

职责：

- 启动和加载 Auth DLL
- 按门禁检查 `nb_self_check / protocol / abi`
- 拉更新清单、下载产物、释放 `updater.exe`
- 执行注入顺序壳
- 把宿主更新状态通过 `set_host_update_snapshot` 回传给 Auth DLL

代码锚点：

- DLL 加载门禁：`src__injector_exe/backend.cpp:332`
- 更新状态 sideband：`src__injector_exe/backend.cpp:512`
- 注入顺序壳：`src__injector_exe/backend.cpp:1147`

### 4.2 NoteBotAuth.dll

职责：

- 本地设备身份与 `license_v3.dat`
- 联网授权三件套
- DLL 策略请求
- 服务端票据请求
- wrapper 生成与 `inject_ticket_v3.dat` 写入
- 结果文件 HMAC 校验
- 结果上报

代码锚点：

- 动作分发：`src__auth_dll/src/v3/v3_actions.cpp:72`
- 状态机主体：`src__auth_dll/src/v3/v3_state.cpp:724`

### 4.3 NoteBotOverlayV3.dll

职责：

- 读取本地 wrapper 票据文件
- 派生 wrapper key
- 解包 wrapper
- 用正式服务端公钥验签
- 校验自身 `sha256`
- 校验 30 秒 Tick 窗口
- replay 拦截
- 写 `inject_result_v3.dat`

代码锚点：

- 票据解析：`src__overlay_dll/overlay_main.cpp:270`
- 正式验签根：`src__overlay_dll/overlay_main.cpp:541`
- 结果文件写入：`src__overlay_dll/overlay_main.cpp:450`

### 4.4 updater.exe

职责：

- 本地替换
- 可替换主 EXE
- 可替换 Auth DLL
- 可替换任意单文件
- 不联网
- 不接授权
- 不接票据

代码锚点：

- 命令行协议：`src__updater_exe/updater_main.cpp:231`

---

## 5. 稳定导出与宿主加载门禁

EXE 只允许加载满足以下条件的 Auth DLL：

1. `nb_self_check() == 0`
2. `nb_get_protocol_version() == 3`
3. `nb_get_abi_version() == 1`

代码锚点：

- `src__injector_exe/backend.cpp:390`
- `src__injector_exe/backend.cpp:407`

正式稳定导出：

- `nb_init`
- `nb_shutdown`
- `nb_get_protocol_version`
- `nb_get_abi_version`
- `nb_self_check`
- `nb_call`

兼容导出仍存在，但服务端不需要围绕它们设计：

- `nb_activate`
- `nb_verify`
- `nb_diagnose`
- `nb_reset`
- `nb_inject`
- `nb_inject_async`

---

## 6. `nb_call` 正式动作面

本地动作分发见 `src__auth_dll/src/v3/v3_actions.cpp:72`。

当前服务端必须配合的正式动作：

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- `dll_policy_v3`
- `issue_inject_ticket_v3`
- `report_inject_result_v3`
- `get_status_snapshot`
- `get_update_snapshot`
- `set_host_update_snapshot`
- `download_overlay_dll_v3`

本地内部消费但不属于服务端接口：

- `consume_inject_result_v3`
- `get_local_layout`
- `name_*`

---

## 7. `nb_call` 正式返回壳

所有正式动作都收口成：

```json
{
  "ok": true,
  "action": "device_check_v3",
  "rc": 0,
  "message": "ok",
  "data": {}
}
```

代码锚点：

- `src__auth_dll/src/v3/v3_actions.cpp:39`

正式状态快照位置固定为：

- `data.status_snapshot`

兼容期顶层 `status_snapshot` 仍可能存在，但**服务端设计不要依赖它**。

代码锚点：

- `src__auth_dll/src/v3/v3_actions.cpp:59`
- `src__auth_dll/src/v3/v3_actions.cpp:102`

---

## 8. 状态快照字段

正式字段定义见：

- `src__auth_dll/src/v3/v3_state.h:68`
- `src__auth_dll/src/v3/v3_state.cpp:2311`

固定字段：

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
- `protocol_version`
- `abi_version`

服务端必须理解的本地显示语义：

- `active=true` 的前提不是“有文件”，而是 `hasVerifiedLicenseLocked() == true`
- `kicked=true` 后本地停心跳
- `offline_cached` 在本地显示为 `license_status = 当前离线，显示缓存状态`

代码锚点：

- verified 判定：`src__auth_dll/src/v3/v3_state.cpp:2289`
- kicked / offline 处理：`src__auth_dll/src/v3/v3_state.cpp:887`
- 状态文本：`src__auth_dll/src/v3/v3_state.cpp:683`

---

## 9. 错误码空间

错误码定义：

- `src__auth_dll/src/v3/v3_state.h:14`

固定范围：

- `0`
- `1001` - `1022`

关键错误码：

| rc | 含义 |
|---|---|
| `1001` | 本地参数错误 |
| `1002` | 本地状态文件缺失 |
| `1003` | 本地状态文件损坏 |
| `1004` | DPAPI 解密失败 |
| `1005` | 设备私钥缺失 / 签名失败 |
| `1006` | 网络不可达 |
| `1007` | 服务端拒绝 |
| `1008` | 被顶下线 |
| `1009` | 替换冷却 |
| `1010` | 心跳失败 |
| `1011` | DLL 策略无效 |
| `1014` | 票据请求失败 |
| `1015` | 票据写入失败 |
| `1017` | 结果文件超时未到 |
| `1018` | 结果文件无效 / 无待消费上下文 |
| `1019` | 结果 HMAC 校验失败 |
| `1020` | 结果上报失败 |
| `1021` | `protocol_mismatch` |
| `1022` | 自检失败保留位 |

服务端状态到本地 rc 的映射：

- `ok -> 0`
- `kicked -> 1008`
- `replace_cooldown -> 1009`
- `protocol_mismatch -> 1021`
- 其他非 ok -> 1007

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:2258`

---

## 10. 传输层协议

### 10.1 授权 / 票据 / 结果 / 更新 都走 TCP + AES-GCM

Auth DLL RPC 客户端：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:14`
- `src__auth_dll/src/v3/v3_rpc_client.cpp:355`

EXE 更新客户端：

- `src__injector_exe/updater.cpp:536`
- `src__injector_exe/updater.cpp:581`

### 10.2 固定事实

- Host：`notebot-api.fucku.top`
- Port：本地当前代码默认常量仍是 `30065`，但服务端 V3 目标正式端口已定为 `30165`
- 请求先 AES-GCM 加密
- 再发 `uint32 big-endian` 长度前缀
- 后跟密文
- 响应也同样是 `uint32 big-endian` 长度前缀 + AES-GCM 密文

说明：

- 这代表服务端当前必须按本地现有 PSK 与 framing 兼容
- 真正切流到 `30165` 前，需要同步本地客户端端口常量

### 10.3 服务端必须做的事情

1. 读 4 字节 big-endian 长度
2. 读完整密文
3. 用与本地相同的 32 字节 PSK 做 AES-GCM 解密
4. 解析 UTF-8 JSON
5. 返回 UTF-8 JSON 明文
6. 再用同一 PSK 做 AES-GCM 加密
7. 回写 4 字节 big-endian 长度 + 密文

### 10.4 关于密钥材料

这份文档不重复抄出原始 PSK 字节。

服务端实现必须直接对齐本地嵌入常量：

- Auth RPC：`src__auth_dll/src/v3/v3_rpc_client.cpp:14`
- Update RPC：`src__injector_exe/updater.cpp:14`

如果后续要换 PSK，必须本地和服务端同步升级，不能只改一边。

---

## 11. canonical JSON 规则

本地 canonical JSON 生成器：

- Auth RPC：`src__auth_dll/src/v3/v3_rpc_client.cpp:424`
- Update RPC：`src__injector_exe/updater.cpp:117`
- 本地签名 payload：`src__auth_dll/src/v3/v3_state.cpp:1991`

固定规则：

1. UTF-8
2. 无 BOM
3. 无 pretty-print
4. 无额外空格
5. 字段顺序固定为代码构造顺序
6. 字符串按标准 JSON 转义
7. 控制字符使用小写 `\u00xx`
8. 布尔值为 `true/false`
9. 整数为 JSON number

**服务端签名验签时必须按同样字节序列处理，不能自行重排字段。**

---

## 12. 设备身份与 `license_v3.dat`

### 12.1 本地路径

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:362`

固定目录：

- `%LOCALAPPDATA%\NoteBotInjector\state_v3\`
- `%LOCALAPPDATA%\NoteBotInjector\dlls_v3\`

固定文件：

- `state_v3/license_v3.dat`
- `state_v3/inject_ticket_v3.dat`
- `state_v3/inject_result_v3.dat`
- `state_v3/consumed_tickets_v3.json`

### 12.2 `license_v3.dat` 外层格式

代码锚点：

- 读：`src__auth_dll/src/v3/v3_state.cpp:387`
- 写：`src__auth_dll/src/v3/v3_state.cpp:438`

固定格式：

1. 前 4 字节：`NBV3`
2. 后 4 字节：`schema_version`，当前为 `3`
3. 剩余内容：整体 DPAPI machine scope 保护后的 JSON

说明：

- 这是本地文件格式，不是服务端网络格式
- 服务端不需要直接读写它
- 但服务端回包字段必须能填充它

### 12.3 `license_v3.dat` 内层 JSON 字段

代码锚点：

- 反序列化：`src__auth_dll/src/v3/v3_state.cpp:479`
- 序列化：`src__auth_dll/src/v3/v3_state.cpp:508`

固定字段：

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

### 12.4 派生规则

代码锚点：

- key 规范化：`src__auth_dll/src/protected/protected_v3_ops.cpp:26`
- key hash：`src__auth_dll/src/protected/protected_v3_ops.cpp:34`
- key id：`src__auth_dll/src/protected/protected_v3_ops.cpp:42`
- pending 激活初始化：`src__auth_dll/src/v3/v3_state.cpp:2031`

固定规则：

- `key_plaintext`：去空格、转大写
- `key_hash`：`SHA256(normalized_key)`，小写 hex
- `key_id`：`kid_` + `key_hash` 前 16 位
- `device_id`：由本地设备公钥派生，具体逻辑在 `deriveDeviceId(...)` 和设备身份生成链

服务端要点：

- 服务端不能重新定义 `key_id`
- 服务端必须接受客户端上传的 `device_id`
- 服务端要把最终确认过的 `key_id / key_hash / server_pubkey_version / server_pubkey_fingerprint` 回给客户端

---

## 13. 正式服务端公钥

本地正式公钥锚点：

- `src__auth_dll/src/v3/v3_server_pubkey.h:34`

固定事实：

- `server_pubkey_version = 4`
- `server_pubkey_fingerprint = sha256:caba170778c0380bbcc3535ce540a35d75bd25f4fb62bd29e56ac072ff6b59f8`

服务端要求：

1. 服务端持有与该公钥对应的私钥
2. `issue_inject_ticket_v3` 的签名必须由该私钥完成
3. 若将来更换服务端签票公钥：
   - 必须本地先升级嵌入公钥
   - 必须同步变更 `server_pubkey_version`
   - 必须同步变更 `server_pubkey_fingerprint`

Overlay 还会检查：

- 本地 `license_v3.dat` 中记录的 `server_pubkey_version`
- 本地 `license_v3.dat` 中记录的 `server_pubkey_fingerprint`

代码锚点：

- `src__overlay_dll/overlay_main.cpp:541`

---

## 14. Auth 服务端命令总览

当前本地真实请求的正式服务端命令有 7 个：

1. `activate_device_v3`
2. `device_check_v3`
3. `device_heartbeat_v3`
4. `dll_policy_v3`
5. `issue_inject_ticket_v3`
6. `report_inject_result_v3`
7. `update_manifest_v3`

其中：

- 1~6 由 Auth DLL 使用
- 7 由 EXE 使用

---

## 15. `activate_device_v3`

### 15.1 本地请求模型

代码锚点：

- 结构体：`src__auth_dll/src/v3/v3_rpc_client.h:8`
- 组包：`src__auth_dll/src/v3/v3_rpc_client.cpp:87`

字段顺序固定：

1. `cmd`
2. `key_plaintext`
3. `device_id`
4. `device_public_key_pem`
5. `timestamp_utc`
6. `nonce`
7. `client_version`
8. `protocol_version`

请求体示意：

```json
{
  "cmd": "activate_device_v3",
  "key_plaintext": "XXXX-XXXX",
  "device_id": "did_xxx",
  "device_public_key_pem": "-----BEGIN PUBLIC KEY-----\n...\n-----END PUBLIC KEY-----\n",
  "timestamp_utc": "2026-06-04T13:00:00Z",
  "nonce": "001122...",
  "client_version": "3.4.58",
  "protocol_version": 3
}
```

### 15.2 本地期望响应字段

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:115`

固定字段：

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

**注意：本地这里不是从 `tier_value` 取值，而是从 `tier` 推导 `tierValue`。**

也就是说服务端正式应返回：

- `tier = "None" | "Trial" | "Premium" | "Dev"`

本地会自己把它转成数值。

补充固定规则：

- 服务端只负责把 `tier / feature_flags` 写入已签名票据
- 业务 `DLL` 验签成功后必须安装 sealed gate context
- 业务 `DLL` 不得把 `tier / feature_flags` 裸存为单一可信全局变量
- 业务功能入口读取等级或能力时，必须通过 sealed gate context cookie 校验后的 getter

### 15.3 本地处理规则

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:724`

成功后本地会：

1. 写回 `license_v3.dat`
2. 记录 `key_id / key_hash`
3. 记录 `server_pubkey_version / fingerprint`
4. 刷新 `tier / feature_flags / bound_dll`
5. 标记 `online=true`
6. 启动心跳定时器

---

## 16. `device_check_v3`

### 16.1 本地签名 payload

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:1975`

内层 payload 顺序固定：

1. `cmd`
2. `key_id`
3. `device_id`
4. `timestamp_utc`
5. `nonce`
6. `protocol_version`

这段 canonical JSON 再由设备私钥做 `RSA-SHA256` 签名。

### 16.2 外层请求

代码锚点：

- 结构体：`src__auth_dll/src/v3/v3_rpc_client.h:32`
- 组包：`src__auth_dll/src/v3/v3_rpc_client.cpp:130`

字段顺序固定：

1. `cmd`
2. `key_id`
3. `device_id`
4. `timestamp_utc`
5. `nonce`
6. `client_version`
7. `payload`
8. `device_signature`
9. `protocol_version`

关键点：

- `payload` 是**canonical JSON 字符串**
- 不是对象

### 16.3 本地期望响应

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:159`

固定字段：

- `status`
- `online`
- `kicked`
- `tier`
- `feature_flags`
- `bound_dll_name`
- `bound_dll_sha256`
- `replace_cooldown_remaining_sec`

### 16.4 本地行为

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:830`

服务端要点：

1. 若设备尚未激活，本地直接返回 `1007`，不会发真正联网检查
2. 若服务端回 `kicked`，本地：
   - 标记 `kicked=true`
   - `online=false`
   - 停心跳
3. 若服务端回 `replace_cooldown`，本地返回 `1009`
4. 若网络失败，本地进入离线缓存显示

---

## 17. `device_heartbeat_v3`

### 17.1 请求字段

代码锚点：

- 结构体：`src__auth_dll/src/v3/v3_rpc_client.h:55`
- 组包：`src__auth_dll/src/v3/v3_rpc_client.cpp:172`

字段顺序固定：

1. `cmd`
2. `key_id`
3. `device_id`
4. `timestamp_utc`
5. `nonce`
6. `device_signature`

这里没有单独的 `payload` 字段。

### 17.2 响应字段

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:197`

固定字段：

- `status`
- `online`
- `kicked`
- `server_time_utc`

### 17.3 本地行为

代码锚点：

- 定时器：`src__auth_dll/src/api.cpp:114`
- 心跳状态机：`src__auth_dll/src/v3/v3_state.cpp:948`
- 心跳启动：`src__auth_dll/src/v3/v3_state.cpp:2272`

固定规则：

- 间隔 30 秒
- 连续 3 次失败后进入 `offline_cached`
- 收到 `kicked` 立即停心跳

服务端应按 90 秒无心跳判离线，与本地 30 秒发送节奏配合。

---

## 18. `dll_policy_v3`

### 18.1 签名输入

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:2099`

canonical payload 顺序：

1. `cmd`
2. `key_id`
3. `device_id`
4. `timestamp_utc`
5. `nonce`

再用设备私钥签名。

### 18.2 网络请求

代码锚点：

- 结构体：`src__auth_dll/src/v3/v3_rpc_client.h:70`
- 组包：`src__auth_dll/src/v3/v3_rpc_client.cpp:204`

外层字段：

1. `cmd`
2. `key_id`
3. `device_id`
4. `timestamp_utc`
5. `nonce`
6. `device_signature`

### 18.3 响应字段

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:229`

固定字段：

- `status`
- `dll_name`
- `dll_sha256`
- `dll_md5`
- `dll_size`
- `download_url`
- `channel`
- `expires_in`

### 18.4 服务端要求

本地会拒绝以下情况：

- `dll_name` 为空
- `dll_sha256` 为空
- `dll_size <= 0`

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:2148`

---

## 19. `issue_inject_ticket_v3`

### 19.1 本地正式调用路径

代码锚点：

- EXE 触发：`src__injector_exe/backend.cpp:1239`
- Auth DLL 请求：`src__auth_dll/src/v3/v3_state.cpp:1228`
- RPC 组包：`src__auth_dll/src/v3/v3_rpc_client.cpp:240`

EXE 传给 Auth DLL 的正式字段：

- `target_pid`
- `dll_name`
- `dll_sha256`
- `exe_version`

Auth DLL 若没有直接传入服务端票据，则会正式向服务端发请求。

### 19.2 服务端请求字段

字段顺序固定：

1. `cmd`
2. `key_id`
3. `device_id`
4. `dll_name`
5. `dll_sha256`
6. `timestamp_utc`
7. `nonce`
8. `device_signature`
9. `target_pid`

### 19.3 服务端响应字段

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:269`

固定字段：

- `status`
- `session_id`
- `ticket_id`
- `server_ticket_payload`
- `server_ticket_signature`
- `expires_in`

### 19.4 服务端必须签发的 inner ticket

本地只认：

- `server_ticket_payload`：canonical JSON 字符串
- `server_ticket_signature`：对应 payload 的小写 hex 签名

`ticket_sha256` 计算方式：

- 对 `server_ticket_payload` 原始 UTF-8 字节做 SHA256

代码锚点：

- `src__auth_dll/src/protected/protected_ticket_ops.cpp:30`

inner ticket 需要至少覆盖这些字段，才能满足 Overlay 消费端：

- `magic`
- `version`
- `alg`
- `key_id`
- `device_id`
- `session_id`
- `ticket_id`
- `tier`
- `feature_flags`
- `dll_name`
- `dll_sha256`
- `issued_at_server`
- `expires_at_server`
- `ttl_ms`
- `nonce`

Overlay 解析锚点：

- `src__overlay_dll/overlay_main.cpp:319`

### 19.5 当前服务端实现的硬门槛

在真正返回 `server_ticket_payload / server_ticket_signature` 之前，服务端必须先满足：

1. 服务端持有可加载的签票私钥
2. 由该私钥导出的公钥 SPKI 指纹，必须严格等于：
   - `sha256:caba170778c0380bbcc3535ce540a35d75bd25f4fb62bd29e56ac072ff6b59f8`

否则服务端应明确返回：

- `status = "rejected"`
- `msg = "ticket_signing_key_mismatch"` 或同义错误

说明：

- 这是为了避免服务端签出一张 Overlay 一定不会认可的废票
- 当前远端 `keys/server_priv.pem` 若导出的公钥指纹不匹配，就必须停在这个显式拒绝态
- 后台 `GET /api/overview` 与启动日志都应直接暴露这个状态，避免把问题误判成协议或 wrapper 实现错误

当前服务端还支持通过环境变量切换签票私钥路径：

- `NB_V3_TICKET_SIGNING_KEY_PATH`

默认值：

- `keys/server_priv.pem`

这意味着后续只要把正确私钥落盘，并把该环境变量指向它，就不需要再改服务端代码。

当前服务端还应提供两条固定运维命令：

- `npm run verify:ticket-signer`
  - 直接输出 `ready / fingerprint / keyPath / reason`
- `npm run scan:key-fingerprints`
  - 扫描仓库与主机上的 PEM / KEY / CERT 指纹
  - 用于确认正确正式私钥是否已经落到机器上

当前后台还应提供与这两条命令对应的只读接口：

- `GET /api/ticket-signer`
- `GET /api/key-scan?compact=true|false`

以及用于查看更新链上报的：

- `GET /api/runtime-reports?limit=N`

管理 HTTPS 当前还固定提供：

- `POST /api/login`
- `POST /api/logout`
- `GET /api/session`
- `GET /api/overview`

其中 `GET /api/session` 在已登录状态下返回：

- `logged_in=true`
- `role=admin`
- `expires_at_utc`
- `csrf_token`

服务端当前实现采用：

- `HttpOnly` session cookie
- 单独 `csrf` cookie
- 写操作必须同时满足同源来源检查与 `x-nb-csrf` 校验

### 19.6 Auth DLL 本地 wrapper 生成规则

代码锚点：

- wrapper JSON：`src__auth_dll/src/v3/v3_state.cpp:106`
- 票据写入：`src__auth_dll/src/v3/v3_state.cpp:1356`

wrapper key 派生：

- `SHA256("NB_WRAP_V3" || session_id || ticket_sha256 || build_salt || fixed_fragment_a || fixed_fragment_b)`

代码锚点：

- `src__auth_dll/src/protected/protected_ticket_ops.cpp:16`

说明：

- 这是本地 Auth DLL / Overlay 内部共享逻辑
- 服务端不参与 wrapper key 派生

---

## 20. `inject_ticket_v3.dat` 本地文件格式

写入代码：

- `src__auth_dll/src/v3/v3_state.cpp:1412`

读取代码：

- `src__overlay_dll/overlay_main.cpp:270`

固定布局：

1. 16 字节 magic：`NB_TICKET_WRAP_V3`
2. `uint32 big-endian`：`wrapper_version`
3. `uint32 big-endian`：`session_id` 长度
4. `session_id` UTF-8
5. `uint32 big-endian`：`ticket_sha256` 长度
6. `ticket_sha256` UTF-8
7. 12 字节 AES-GCM IV
8. 16 字节 AES-GCM Tag
9. `uint32 big-endian`：cipher 长度
10. cipher bytes

服务端不直接生成这个文件，但服务端发的 `server_ticket_payload / signature` 必须能让本地正确写出并让 Overlay 正确解出。

---

## 21. Overlay 对服务端票据的正式校验

代码锚点：

- `src__overlay_dll/overlay_main.cpp:465`

固定校验顺序：

1. `inject_ticket_v3.dat` 格式正确
2. wrapper AES-GCM 解密成功
3. wrapper `magic == NB_TICKET_WRAP_V3`
4. wrapper `wrapper_version == 3`
5. wrapper `ticket_sha256` 与外层一致
6. inner ticket JSON 可解析
7. `payload.session_id == envelope.session_id`
8. 当前 PID 等于 `target_pid`
9. 本地重新计算 `ticket_sha256` 与 outer 一致
10. 本地 `license_v3.dat` 记录的公钥版本/指纹与内置正式公钥一致
11. `server_ticket_signature` 能被内置正式公钥验签
12. 当前 Overlay 自身 `sha256 == payload.dll_sha256`
13. `GetTickCount64() <= received_tick_ms + 30000`
14. replay cache 未命中

失败状态枚举：

- `wrapper_invalid`
- `signature_invalid`
- `dll_hash_mismatch`
- `replay_blocked`
- `ticket_expired`
- `inject_failed`

代码锚点：

- `src__overlay_dll/overlay_main.cpp:527`

---

## 22. `inject_result_v3.dat` 本地文件格式

写入代码：

- `src__overlay_dll/overlay_main.cpp:450`

读取代码：

- `src__auth_dll/src/v3/v3_state.cpp:1531`

固定 JSON 字段：

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

`result_hmac` 计算对象：

- 对 canonical result JSON 的 UTF-8 原文做 `HMAC-SHA256`
- key = wrapper key

canonical result JSON 代码锚点：

- Overlay 侧：`src__overlay_dll/overlay_main.cpp:153`
- Auth DLL 侧：`src__auth_dll/src/v3/v3_state.cpp:131`

---

## 23. replay cache 规则

代码锚点：

- 查询：`src__auth_dll/src/v3/v3_state.cpp:1784`
- 裁剪：`src__auth_dll/src/v3/v3_state.cpp:1809`
- 追加：`src__auth_dll/src/v3/v3_state.cpp:1867`

固定文件：

- `consumed_tickets_v3.json`

固定 entries 结构：

- `ticket_id`
- `ticket_sha256`
- `consumed_at_utc`

固定策略：

1. 先删 24 小时外旧项
2. 再按时间升序裁到最多 256 条
3. replay 判定键为 `ticket_id + ticket_sha256`

Overlay 自己也有一份本地 replay 逻辑：

- `src__overlay_dll/overlay_main.cpp:344`

---

## 24. `report_inject_result_v3`

### 24.1 Auth DLL 上报前提

只有当：

1. `consume_inject_result_v3` 已成功
2. `m_pendingReport.valid == true`

本地才会真正发上报请求。

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:1632`

### 24.2 请求字段

字段顺序固定：

1. `cmd`
2. `key_id`
3. `device_id`
4. `session_id`
5. `ticket_id`
6. `status`
7. `reason`
8. `timestamp_utc`
9. `nonce`
10. `device_signature`

### 24.3 响应字段

代码锚点：

- `src__auth_dll/src/v3/v3_rpc_client.cpp:309`

固定字段：

- `status`
- `accepted`

本地只有在以下条件下才算成功：

- `status == "ok"`
- `accepted == true`

代码锚点：

- `src__auth_dll/src/v3/v3_state.cpp:1711`

服务端建议：

- 无论结果成功失败，只要业务上已接收，都应释放该 `session_id / ticket_id` 会话锁

---

## 25. 更新协议：`update_manifest_v3`

### 25.1 本地正式地位

这是 V3 唯一正式更新命令。

代码锚点：

- `src__injector_exe/updater.cpp:648`

### 25.2 请求有两种形态

#### A. 已激活设备签名请求

代码锚点：

- `src__injector_exe/updater.cpp:189`
- `src__injector_exe/updater.cpp:230`
- `src__injector_exe/updater.cpp:665`

字段顺序：

1. `cmd`
2. `client_kind`
3. `channel`
4. `current_main_version`
5. `current_auth_dll_version`
6. `current_updater_version`
7. `key_id`
8. `device_id`
9. `timestamp_utc`
10. `nonce`
11. `current_main_version_code`
12. `current_auth_dll_version_code`
13. `current_updater_version_code`
14. `protocol_version`

再用设备私钥对这段 canonical JSON 进行 `RSA-SHA256` 签名，放进外层请求的：

- `device_signature`

#### B. 匿名最小请求

代码锚点：

- `src__injector_exe/updater.cpp:673`

本地当前行为不是换一套命令，而是：

- 仍发 `update_manifest_v3`
- 仍走同一 JSON 形状
- 但没有 verified license 时，`key_id / device_id / device_signature` 为空字符串

服务端必须支持这种匿名最小请求，至少返回公共更新清单。

### 25.3 响应顶层

代码锚点：

- `src__injector_exe/updater.cpp:703`

固定字段：

- `status`
- `msg`
- `expires_in`
- `artifacts`

成功要求：

- `status == "ok"`

### 25.4 `artifacts` 数组项结构

代码锚点：

- `src__injector_exe/updater.cpp:470`

固定字段：

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

当前本地识别 4 类产物：

- `main_exe`
- `auth_dll`
- `updater_exe`
- `overlay_dll`

代码锚点：

- `src__injector_exe/updater.cpp:720`

### 25.5 本地对 `required` 的解释

代码锚点：

- overlay required：`src__injector_exe/backend.cpp:722`
- main required：`src__injector_exe/backend.cpp:773`
- auth required：`src__injector_exe/backend.cpp:816`

固定行为：

- `main_exe.required=true`：
  - 下载新主程序
  - 拉起 `updater.exe --replace-main`
  - 阻断继续进入正式主流程
- `auth_dll.required=true`：
  - 允许进入最小壳
  - 但阻止授权和注入
- `overlay_dll.required=true`：
  - 不阻止进 UI
  - 但阻止注入

---

## 26. `updater.exe` 正式命令行协议

代码锚点：

- `src__updater_exe/updater_main.cpp:231`

只允许三种命令：

- `--replace-main`
- `--replace-auth-dll`
- `--replace-file`

固定参数：

- `--src`
- `--dst`
- `--backup`
- `--pid` 或 `--wait-pid`
- `--restart` 仅主程序替换可用

额外限制：

- `--replace-auth-dll` **不允许**带 `--restart`

代码锚点：

- `src__updater_exe/updater_main.cpp:237`

---

## 27. 服务端建议模块划分

建议直接按本地协议面拆成以下模块。

### 27.1 `transport/`

职责：

- TCP server
- 长度前缀 framing
- AES-GCM encrypt/decrypt
- 统一 request / response 读写

### 27.2 `protocol/`

职责：

- canonical JSON 生成
- 字段顺序定义
- 枚举和值域校验
- 协议版本检查

### 27.3 `auth/`

职责：

- `activate_device_v3`
- `device_check_v3`
- `device_heartbeat_v3`
- 设备替换逻辑
- 单在线踢线逻辑
- 24h 冷却

### 27.4 `policy/`

职责：

- `dll_policy_v3`
- 业务 DLL 灰度
- 按 key / channel / protocol 选择产物

### 27.5 `ticket/`

职责：

- `issue_inject_ticket_v3`
- inner ticket canonical JSON 生成
- RSA-SHA256 签名
- 120 秒会话锁
- `report_inject_result_v3`

### 27.6 `updates/`

职责：

- `update_manifest_v3`
- `main_exe / auth_dll / updater_exe / overlay_dll` 四类产物选择
- 匿名最小清单
- 已激活签名清单

### 27.7 `storage/`

职责：

- `licenses`
- `devices`
- `online_sessions`
- `inject_sessions`
- `artifacts / manifests`

---

## 28. 服务端数据模型建议

建议至少拆以下存储面。

### 28.1 `licenses`

主键建议：

- `key_id`

字段建议：

- `key_id`
- `key_hash`
- `key_plaintext_norm`
- `tier`
- `tier_value`
- `feature_flags`
- `bound_dll_name`
- `bound_dll_sha256`
- `channel`
- `enabled`

### 28.2 `devices`

联合键建议：

- `key_id + device_id`

字段建议：

- `key_id`
- `device_id`
- `device_public_key_pem`
- `first_activated_at_utc`
- `last_seen_at_utc`
- `last_online_at_utc`
- `replaced_at_utc`
- `replace_count_24h`

### 28.3 `online_sessions`

字段建议：

- `key_id`
- `device_id`
- `online`
- `last_heartbeat_at_utc`
- `kicked`

### 28.4 `inject_sessions`

字段建议：

- `session_id`
- `ticket_id`
- `key_id`
- `device_id`
- `dll_name`
- `dll_sha256`
- `target_pid`
- `issued_at_utc`
- `expires_at_utc`
- `locked`
- `reported`

### 28.5 `artifacts`

字段建议：

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

---

## 29. 明确的服务端实现注意点

### 29.1 不要自己改字段名

尤其不要把这些改掉：

- `bound_dll_name`
- `bound_dll_sha256`
- `replace_cooldown_remaining_sec`
- `server_ticket_payload`
- `server_ticket_signature`
- `device_signature`

### 29.2 不要把 `payload` 变对象

当前本地明确要求：

- `payload` 是字符串
- 其内容本身是 canonical JSON

### 29.3 不要依赖 `tier_value` 作为 Auth RPC 正式输入

当前本地 Auth RPC 真正解析的是：

- `tier`

而不是 `tier_value`。

服务端最稳妥做法：

- 返回 `tier`
- 也可以额外返回 `tier_value`
- 但不能只返回 `tier_value` 而没有 `tier`

### 29.4 feature flags 最稳妥用整数

当前本地支持：

- number
- 或字符串十进制 / `0x...`

正式建议服务端直接返回整数。

### 29.5 更新匿名请求必须支持

当前 EXE 没有 verified license 时仍会请求：

- `update_manifest_v3`

只是 `key_id / device_id / device_signature` 为空。

服务端不要把这种情况一律当恶意请求。

### 29.6 服务端状态枚举建议

最少必须支持：

- `ok`
- `rejected`
- `kicked`
- `replace_cooldown`
- `protocol_mismatch`

更新清单建议：

- `status = ok`
- 失败时 `status != ok`，并带 `msg`

---

## 30. 推荐的服务端实施顺序

### 第一阶段：先打通授权联网三件套

1. `activate_device_v3`
2. `device_check_v3`
3. `device_heartbeat_v3`

### 第二阶段：打通票据链

4. `dll_policy_v3`
5. `issue_inject_ticket_v3`
6. `report_inject_result_v3`

### 第三阶段：打通更新链

7. `update_manifest_v3`

### 第四阶段：做联调验收

8. 首次激活
9. 重复检查
10. 顶号
11. 替换冷却
12. 心跳离线
13. DLL 策略下载
14. 真实票据签发
15. Overlay 真实验签
16. 结果上报
17. 匿名更新清单
18. 已激活签名更新清单

---

## 31. 这份文档之后的工作纪律

从现在开始：

1. **服务端按这份文档和本地代码适配**
2. **不要默认反向修改本地 EXE / Auth DLL**
3. 如果联调时发现冲突，先判断：
   - 是服务端没跟上
   - 还是本地代码和本地代码之间已经自相矛盾
4. 只有在本地实现确实互相打架时，才允许回头修本地

默认原则：

- **服务端跟本地**
- **不是本地跟服务端**

---

## 32. 一句话结论

后面服务端要实现的，不是“抽象上的 V3”。

后面服务端要实现的是：

- **当前 `C:\NB` 这套已经冻结的 EXE / Auth DLL / Overlay / updater 本地协议现实。**
