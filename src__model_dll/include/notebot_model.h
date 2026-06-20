#ifndef NOTEBOT_MODEL_H
#define NOTEBOT_MODEL_H

#if defined(NB_MODEL_EXPORTS) || defined(NoteBotModel_EXPORTS)
#define NBMAPI __declspec(dllexport)
#else
#define NBMAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*NBM_LogCallback)(const char *msg);
typedef void (*NBM_StateCallback)(const char *key, const char *value);

NBMAPI int nbm_get_protocol_version(void);
NBMAPI int nbm_get_abi_version(void);
NBMAPI int nbm_init(void);
NBMAPI void nbm_shutdown(void);
NBMAPI void nbm_set_log_callback(NBM_LogCallback cb);
NBMAPI void nbm_set_state_callback(NBM_StateCallback cb);
NBMAPI int nbm_start_wait(const char *json_config);
NBMAPI int nbm_stop_wait(void);
NBMAPI int nbm_is_running(void);
NBMAPI int nbm_is_waiting_for_process(void);

#ifdef __cplusplus
}
#endif

#endif
