#include "notebot_auth.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QtGlobal>
#include <chrono>
#include <cstring>
#include <thread>

#include "v3/v3_actions.h"
#include "v3/v3_state.h"
#include "win32injector.h"

namespace {

constexpr char kDllVersion[] = "3.5.50";

struct GlobalState {
    NB_LogCallback logCb = nullptr;
    NB_StateCallback stateCb = nullptr;
    NBAuth::V3::StateManager v3;
    QTimer *heartbeatTimer = nullptr;
};

GlobalState g;

void emitLog(const QString &msg)
{
    if (!g.logCb) {
        return;
    }
    const QByteArray bytes = msg.toUtf8();
    g.logCb(bytes.constData());
}

void emitState(const QString &key, const QString &value)
{
    if (!g.stateCb) {
        return;
    }
    const QByteArray keyBytes = key.toUtf8();
    const QByteArray valueBytes = value.toUtf8();
    g.stateCb(keyBytes.constData(), valueBytes.constData());
}

void publishSnapshotState()
{
    const NBAuth::V3::StatusSnapshot snapshot = g.v3.snapshot();
    emitState(QStringLiteral("license_status"), snapshot.licenseStatus);
    emitState(QStringLiteral("license_active"),
              snapshot.active ? QStringLiteral("true") : QStringLiteral("false"));
    emitState(QStringLiteral("license_tier"), snapshot.tierName);
    emitState(QStringLiteral("protocol_version"),
              QString::number(snapshot.protocolVersion));
    emitState(QStringLiteral("abi_version"),
              QString::number(snapshot.abiVersion));
    emitState(QStringLiteral("key_id"), snapshot.keyId);
    emitState(QStringLiteral("device_id"), snapshot.deviceId);
}

int writeTextOut(const QByteArray &data, char *buf, int bufSize)
{
    if (!buf || bufSize <= 0) {
        return -1;
    }
    int len = data.size();
    if (len >= bufSize) {
        len = bufSize - 1;
    }
    if (len > 0) {
        std::memcpy(buf, data.constData(), len);
    }
    buf[len] = '\0';
    return len;
}

int writeJsonOut(const QJsonObject &obj, char *outJson, int outSize)
{
    return writeTextOut(QJsonDocument(obj).toJson(QJsonDocument::Compact), outJson, outSize);
}

void clearOutput(char *buf, int bufSize)
{
    if (buf && bufSize > 0) {
        buf[0] = '\0';
    }
}

QJsonObject parseInputJson(const char *json)
{
    const QString inJson = json ? QString::fromUtf8(json) : QStringLiteral("{}");
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(inJson.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject{};
    }
    return doc.object();
}

} // namespace

NBAPI int nb_init(void)
{
    if (!g.v3.initialize()) {
        emitLog(QStringLiteral("[AUTH] V3 本地状态骨架初始化失败"));
        return NB_ERR_OTHER;
    }

    if (!g.heartbeatTimer) {
        g.heartbeatTimer = new QTimer();
        g.heartbeatTimer->setInterval(30000);
        QObject::connect(g.heartbeatTimer, &QTimer::timeout, []() {
            QString message;
            const int rc = g.v3.heartbeatOnce(&message);
            publishSnapshotState();
            if (rc == NBAuth::V3::kRcOk) {
                emitLog(QStringLiteral("[AUTH] %1").arg(message));
            } else if (!message.isEmpty()) {
                emitLog(QStringLiteral("[AUTH] %1").arg(message));
            }
        });
    }
    g.v3.bindHeartbeatTimer(g.heartbeatTimer);

    publishSnapshotState();
    emitLog(QStringLiteral("[AUTH] 授权核心已就绪"));
    return NB_OK;
}

NBAPI void nb_shutdown(void)
{
    if (g.heartbeatTimer) {
        g.heartbeatTimer->stop();
        g.heartbeatTimer->deleteLater();
        g.heartbeatTimer = nullptr;
    }
    g.v3.shutdown();
    g.logCb = nullptr;
    g.stateCb = nullptr;
}

NBAPI int nb_get_protocol_version(void)
{
    return g.v3.protocolVersion();
}

NBAPI int nb_get_abi_version(void)
{
    return g.v3.abiVersion();
}

NBAPI void nb_set_log_callback(NB_LogCallback cb)
{
    g.logCb = cb;
}

NBAPI void nb_set_state_callback(NB_StateCallback cb)
{
    g.stateCb = cb;
    publishSnapshotState();
}

NBAPI void nb_set_key(const char* key)
{
    const QString normalized = key ? QString::fromUtf8(key) : QString();
    g.v3.setLicenseKey(normalized);

    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(QStringLiteral("licenseKey"), g.v3.licenseKey());

    emitState(QStringLiteral("license_key_present"),
              g.v3.licenseKey().isEmpty() ? QStringLiteral("false")
                                          : QStringLiteral("true"));
}

NBAPI int nb_get_key(char* buf, int buf_size)
{
    return writeTextOut(g.v3.licenseKey().toUtf8(), buf, buf_size);
}

NBAPI int nb_activate(const char* key)
{
    QString message;
    const int rc = g.v3.activateWithServer(key ? QString::fromUtf8(key) : g.v3.licenseKey(),
                                           &message);
    publishSnapshotState();
    emitLog(QStringLiteral("[AUTH] %1").arg(message.isEmpty()
        ? QStringLiteral("激活动作已执行")
        : message));
    return rc;
}

NBAPI int nb_verify(void)
{
    QString message;
    const int rc = g.v3.checkWithServer(&message);
    publishSnapshotState();
    if (rc == NBAuth::V3::kRcOk) {
        emitLog(QStringLiteral("[AUTH] %1").arg(message));
    } else {
        emitLog(QStringLiteral("[AUTH] %1").arg(message.isEmpty()
            ? QStringLiteral("V3 设备检查失败")
            : message));
    }
    return rc;
}

NBAPI int nb_diagnose(char* out_dll_name, int name_size,
                      char* out_dll_md5,  int md5_size)
{
    clearOutput(out_dll_name, name_size);
    clearOutput(out_dll_md5, md5_size);
    const QJsonObject resp = NBAuth::V3::dispatchAction(
        g.v3, QStringLiteral("dll_policy_v3"), QJsonObject{});
    const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
    const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
    const QString dllName = data.value(QStringLiteral("dll_name")).toString();
    const QString dllSha256 = data.value(QStringLiteral("dll_sha256")).toString();
    if (!dllName.isEmpty()) {
        writeTextOut(dllName.toUtf8(), out_dll_name, name_size);
    }
    if (!dllSha256.isEmpty()) {
        writeTextOut(dllSha256.toUtf8(), out_dll_md5, md5_size);
    }
    emitLog(QStringLiteral("[AUTH] %1").arg(
        resp.value(QStringLiteral("message")).toString(
            rc == NBAuth::V3::kRcOk
                ? QStringLiteral("已获取当前 DLL 策略缓存")
                : QStringLiteral("当前 DLL 策略缓存不可用"))));
    return rc;
}

NBAPI void nb_reset(void)
{
    g.v3.resetRuntimeState();
    publishSnapshotState();
    emitLog(QStringLiteral("[AUTH] 已重置 DLL 运行态；旧授权链不会再被触碰"));
}

NBAPI int nb_download(const char* dll_name,
                      const char* expected_md5,
                      char* out_path, int path_size,
                      NB_ProgressCallback progress_cb)
{
    Q_UNUSED(dll_name);
    Q_UNUSED(expected_md5);
    Q_UNUSED(progress_cb);
    clearOutput(out_path, path_size);
    emitLog(QStringLiteral("[AUTH] V3 下载总控规划保留在 EXE，本接口待重接"));
    return NB_ERR_OTHER;
}

NBAPI int nb_inject(int pid, const char* dll_path)
{
    const QString dllPath = dll_path ? QString::fromUtf8(dll_path).trimmed() : QString();
    if (pid <= 0 || dllPath.isEmpty()) {
        emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
        emitLog(QStringLiteral("[AUTH] 缺少有效的 target_pid 或 dll_path"));
        return NB_ERR_INJECT;
    }

    const QJsonObject policyResp = NBAuth::V3::dispatchAction(
        g.v3, QStringLiteral("dll_policy_v3"), QJsonObject{});
    if (policyResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER) != NBAuth::V3::kRcOk) {
        emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
        emitLog(QStringLiteral("[AUTH] %1").arg(
            policyResp.value(QStringLiteral("message")).toString(QStringLiteral("DLL 策略不可用"))));
        return NB_ERR_INJECT;
    }

    const QJsonObject policyData = policyResp.value(QStringLiteral("data")).toObject();
    const QString boundDllName = policyData.value(QStringLiteral("dll_name")).toString();
    const QString boundDllSha256 = policyData.value(QStringLiteral("dll_sha256")).toString();

    QJsonObject ticketReq;
    ticketReq[QStringLiteral("target_pid")] = pid;
    ticketReq[QStringLiteral("dll_name")] = boundDllName;
    ticketReq[QStringLiteral("dll_sha256")] = boundDllSha256;
    ticketReq[QStringLiteral("exe_version")] = QStringLiteral("3.5.70");

    const QJsonObject issueResp = NBAuth::V3::dispatchAction(
        g.v3, QStringLiteral("issue_inject_ticket_v3"), ticketReq);
    if (issueResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER) != NBAuth::V3::kRcOk) {
        emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
        emitLog(QStringLiteral("[AUTH] %1").arg(
            issueResp.value(QStringLiteral("message")).toString(QStringLiteral("票据生成失败"))));
        return NB_ERR_INJECT;
    }

    const bool injected = Win32Injector::injectDll(
        static_cast<uint>(pid),
        dllPath,
        [](const QString &msg) { emitLog(msg); });
    if (!injected) {
        QJsonObject finalizeReq;
        finalizeReq[QStringLiteral("status")] = QStringLiteral("inject_failed");
        finalizeReq[QStringLiteral("reason")] = QStringLiteral("inject_failed:remote_inject_failed");
        const QJsonObject finalizeResp = NBAuth::V3::dispatchAction(
            g.v3, QStringLiteral("finalize_inject_failure_v3"), finalizeReq);
        const QJsonObject reportResp = NBAuth::V3::dispatchAction(
            g.v3, QStringLiteral("report_inject_result_v3"), QJsonObject{});
        emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
        emitLog(QStringLiteral("[AUTH] 兼容注入壳执行远程注入失败"));
        emitLog(QStringLiteral("[AUTH] %1").arg(
            finalizeResp.value(QStringLiteral("message")).toString(
                QStringLiteral("待处理票据已作废"))));
        emitLog(QStringLiteral("[AUTH] %1").arg(
            reportResp.value(QStringLiteral("message")).toString(
                QStringLiteral("结果上报失败"))));
        return NB_ERR_INJECT;
    }

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 5000) {
        QJsonObject consumeResp = NBAuth::V3::dispatchAction(
            g.v3, QStringLiteral("consume_inject_result_v3"), QJsonObject{});
        const int consumeRc = consumeResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        if (consumeRc == NBAuth::V3::kRcOk) {
            const QJsonObject reportResp = NBAuth::V3::dispatchAction(
                g.v3, QStringLiteral("report_inject_result_v3"), QJsonObject{});
            const QString resultMessage =
                consumeResp.value(QStringLiteral("message")).toString(QStringLiteral("已消费注入结果"));
            emitLog(QStringLiteral("[AUTH] %1").arg(resultMessage));
            emitLog(QStringLiteral("[AUTH] %1").arg(
                reportResp.value(QStringLiteral("message")).toString(
                    QStringLiteral("本地结果已确认"))));
            emitState(QStringLiteral("inject_state"), QStringLiteral("success"));
            return NB_OK;
        }
        if (consumeRc != NBAuth::V3::kRcResultTimeout) {
            QJsonObject finalizeReq;
            if (consumeRc == NBAuth::V3::kRcResultHmacFailed) {
                finalizeReq[QStringLiteral("status")] = QStringLiteral("tamper_suspected");
                finalizeReq[QStringLiteral("reason")] = QStringLiteral("tamper_suspected:result_hmac_invalid");
            } else {
                finalizeReq[QStringLiteral("status")] = QStringLiteral("inject_failed");
                finalizeReq[QStringLiteral("reason")] = QStringLiteral("inject_failed:result_file_invalid");
            }
            const QJsonObject finalizeResp = NBAuth::V3::dispatchAction(
                g.v3, QStringLiteral("finalize_inject_failure_v3"), finalizeReq);
            const QJsonObject reportResp = NBAuth::V3::dispatchAction(
                g.v3, QStringLiteral("report_inject_result_v3"), QJsonObject{});
            emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
            emitLog(QStringLiteral("[AUTH] %1").arg(
                consumeResp.value(QStringLiteral("message")).toString(QStringLiteral("注入结果校验失败"))));
            emitLog(QStringLiteral("[AUTH] %1").arg(
                finalizeResp.value(QStringLiteral("message")).toString(
                    QStringLiteral("待处理票据已作废"))));
            emitLog(QStringLiteral("[AUTH] %1").arg(
                reportResp.value(QStringLiteral("message")).toString(
                    QStringLiteral("结果上报失败"))));
            return NB_ERR_INJECT;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    QJsonObject finalizeReq;
    finalizeReq[QStringLiteral("status")] = QStringLiteral("inject_failed");
    finalizeReq[QStringLiteral("reason")] = QStringLiteral("inject_failed:no_result_file");
    const QJsonObject finalizeResp = NBAuth::V3::dispatchAction(
        g.v3, QStringLiteral("finalize_inject_failure_v3"), finalizeReq);
    const QJsonObject reportResp = NBAuth::V3::dispatchAction(
        g.v3, QStringLiteral("report_inject_result_v3"), QJsonObject{});
    emitState(QStringLiteral("inject_state"), QStringLiteral("error"));
    emitLog(QStringLiteral("[AUTH] 等待注入结果超时"));
    emitLog(QStringLiteral("[AUTH] %1").arg(
        finalizeResp.value(QStringLiteral("message")).toString(
            QStringLiteral("待处理票据已作废"))));
    emitLog(QStringLiteral("[AUTH] %1").arg(
        reportResp.value(QStringLiteral("message")).toString(
            QStringLiteral("结果上报失败"))));
    return NB_ERR_INJECT;
}

NBAPI int nb_inject_async(int pid, const char* dll_path)
{
    const int targetPid = pid;
    const QByteArray dllPath = dll_path ? QByteArray(dll_path) : QByteArray();
    emitState(QStringLiteral("inject_state"), QStringLiteral("injecting"));
    std::thread([targetPid, dllPath]() {
        const int rc = nb_inject(targetPid, dllPath.constData());
        emitState(QStringLiteral("inject_state"),
                  rc == NB_OK ? QStringLiteral("success") : QStringLiteral("error"));
    }).detach();
    return NB_OK;
}

NBAPI int nb_is_active(void)
{
    return g.v3.isActive() ? 1 : 0;
}

NBAPI int nb_get_tier(void)
{
    return g.v3.tierCode();
}

NBAPI int nb_get_status(char* buf, int buf_size)
{
    return writeTextOut(g.v3.statusText().toUtf8(), buf, buf_size);
}

NBAPI unsigned int nb_get_expires(void)
{
    return g.v3.expiresEpoch();
}

NBAPI unsigned int nb_get_feature_flags(void)
{
    return g.v3.featureFlagsMask();
}

NBAPI int nb_has_capability(unsigned int cap)
{
    Q_UNUSED(cap);
    return 0;
}

NBAPI int nb_get_version(char* buf, int buf_size)
{
    return writeTextOut(QByteArray(kDllVersion), buf, buf_size);
}

NBAPI int nb_check_update(char* out_version, int ver_size,
                          char* out_md5, int md5_size)
{
    clearOutput(out_version, ver_size);
    clearOutput(out_md5, md5_size);
    emitLog(QStringLiteral("[AUTH] 更新总控已规划回 EXE，本接口仅保留兼容壳"));
    return NB_ERR_OTHER;
}

NBAPI int nb_download_secure(const char* dll_name,
                             const char* expected_md5,
                             char* out_path, int path_size,
                             NB_ProgressCallback progress_cb)
{
    Q_UNUSED(dll_name);
    Q_UNUSED(expected_md5);
    Q_UNUSED(progress_cb);
    clearOutput(out_path, path_size);
    emitLog(QStringLiteral("[AUTH] secure download 旧链已移除，等待 V3 EXE 更新总控接入"));
    return NB_ERR_OTHER;
}

NBAPI int nb_call(const char* action,
                  const char* json,
                  char* out_json,
                  int out_size)
{
    const QString act = action ? QString::fromUtf8(action).trimmed() : QString();
    const QJsonObject req = parseInputJson(json);
    const QJsonObject resp = NBAuth::V3::dispatchAction(g.v3, act, req);
    publishSnapshotState();
    return writeJsonOut(resp, out_json, out_size);
}

NBAPI int nb_self_check(void)
{
    return NB_OK;
}
