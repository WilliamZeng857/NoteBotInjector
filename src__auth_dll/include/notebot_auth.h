#ifndef NOTEBOT_AUTH_H
#define NOTEBOT_AUTH_H

#ifdef NB_AUTH_EXPORTS
#define NBAPI __declspec(dllexport)
#else
#define NBAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 返回码 */
#define NB_OK                0
#define NB_ERR_EXPIRED       1
#define NB_ERR_INVALID_KEY   2
#define NB_ERR_NETWORK       3
#define NB_ERR_SIG_INVALID   4
#define NB_ERR_REVOKED       5
#define NB_ERR_MACHINE_LIMIT 6
#define NB_ERR_OTHER         7
#define NB_ERR_NOT_ACTIVE    8
#define NB_ERR_DLL_DOWNLOAD  9
#define NB_ERR_INJECT        10
#define NB_ERR_TAMPERED      11

/* 等级 */
#define NB_TIER_NONE     0
#define NB_TIER_TRIAL    1
#define NB_TIER_PREMIUM  2
#define NB_TIER_DEV      3

/* 日志回调: DLL 内部所有日志通过此回调发回 EXE */
typedef void (*NB_LogCallback)(const char* msg);

/* 进度回调: 0-100 */
typedef void (*NB_ProgressCallback)(int percent);

/* 状态回调: DLL 状态变化时通知 EXE */
/* key: 状态名称, value: 状态值 */
typedef void (*NB_StateCallback)(const char* key, const char* value);

/* ============================================================
 *  生命周期
 * ============================================================ */
NBAPI int  nb_init(void);
NBAPI void nb_shutdown(void);
NBAPI int  nb_get_protocol_version(void);
NBAPI int  nb_get_abi_version(void);
NBAPI void nb_set_log_callback(NB_LogCallback cb);
NBAPI void nb_set_state_callback(NB_StateCallback cb);

/* ============================================================
 *  密钥管理
 * ============================================================ */
NBAPI void nb_set_key(const char* key);
NBAPI int  nb_get_key(char* buf, int buf_size);

/* ============================================================
 *  授权流程
 * ============================================================ */

/* 激活接口保留给 EXE，具体语义将在 V3 重建阶段重新接入 */
NBAPI int nb_activate(const char* key);

/* 本地检查接口保留给 EXE，具体状态文件语义将在 V3 重建阶段接回 */
NBAPI int nb_verify(void);

/* 注入前检查接口保留给 EXE，V3 票据链重建后再恢复 */
NBAPI int nb_diagnose(char* out_dll_name, int name_size,
                      char* out_dll_md5,  int md5_size);

/* 重置当前 DLL 内部状态；不再触碰旧授权文件 */
NBAPI void nb_reset(void);

/* ============================================================
 *  DLL 下载
 * ============================================================ */

/* 从服务端下载指定 DLL */
/* out_path: 下载后的完整路径 */
/* progress_cb: 可为 NULL */
NBAPI int nb_download(const char* dll_name,
                      const char* expected_md5,
                      char* out_path, int path_size,
                      NB_ProgressCallback progress_cb);

/* ============================================================
 *  注入
 * ============================================================ */

/* 注入接口保留给 EXE，旧共享授权桥已删除 */
NBAPI int nb_inject(int pid, const char* dll_path);

/* 异步注入接口保留，待 V3 票据流程重接 */
NBAPI int nb_inject_async(int pid, const char* dll_path);

/* ============================================================
 *  状态查询
 * ============================================================ */
NBAPI int          nb_is_active(void);
NBAPI int          nb_get_tier(void);   /* NB_TIER_* */
NBAPI int          nb_get_status(char* buf, int buf_size);
NBAPI unsigned int nb_get_expires(void);
NBAPI unsigned int nb_get_feature_flags(void);
NBAPI int          nb_has_capability(unsigned int cap);

/* 返回 DLL 版本号字符串，如 "3.4.19" */
NBAPI int nb_get_version(char* buf, int buf_size);

/* 更新接口保留；具体更新协议将在 V3 重建阶段重新接入 */
NBAPI int nb_check_update(char* out_version, int ver_size,
                          char* out_md5, int md5_size);

/* 下载接口保留；具体下载协议将在 V3 重建阶段重新接入 */
NBAPI int nb_download_secure(const char* dll_name,
                              const char* expected_md5,
                              char* out_path, int path_size,
                              NB_ProgressCallback progress_cb);

NBAPI int nb_call(const char* action,
                  const char* json,
                  char* out_json,
                  int out_size);

/* ============================================================
 *  防篡改自校验
 * ============================================================ */
/* 返回 NB_OK 表示 DLL 完整, NB_ERR_TAMPERED 表示被篡改 */
NBAPI int nb_self_check(void);

#ifdef __cplusplus
}
#endif

#endif /* NOTEBOT_AUTH_H */
