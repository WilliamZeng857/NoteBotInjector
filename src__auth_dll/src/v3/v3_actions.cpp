#include "v3_actions.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "name_injector.h"
#include "v3_state.h"

namespace NBAuth::V3 {

namespace {

void appendCompatibilitySnapshotFields(QJsonObject &resp, const StatusSnapshot &snapshot)
{
    resp[QStringLiteral("protocol_version")] = snapshot.protocolVersion;
    resp[QStringLiteral("abi_version")] = snapshot.abiVersion;
    resp[QStringLiteral("active")] = snapshot.active;
    resp[QStringLiteral("online")] = snapshot.online;
    resp[QStringLiteral("kicked")] = snapshot.kicked;
    resp[QStringLiteral("license_status")] = snapshot.licenseStatus;
    resp[QStringLiteral("tier_name")] = snapshot.tierName;
    resp[QStringLiteral("tier_value")] = snapshot.tierValue;
    resp[QStringLiteral("feature_flags")] = static_cast<qint64>(snapshot.featureFlags);
    resp[QStringLiteral("key_id")] = snapshot.keyId;
    resp[QStringLiteral("device_id")] = snapshot.deviceId;
    resp[QStringLiteral("bound_dll_name")] = snapshot.boundDllName;
    resp[QStringLiteral("bound_dll_sha256")] = snapshot.boundDllSha256;
    resp[QStringLiteral("last_verified_at_utc")] = snapshot.lastVerifiedAtUtc;
    resp[QStringLiteral("last_heartbeat_at_utc")] = snapshot.lastHeartbeatAtUtc;
    resp[QStringLiteral("replace_cooldown_remaining_sec")] = snapshot.replaceCooldownRemainingSec;
    resp[QStringLiteral("network_available")] = snapshot.networkAvailable;
    resp[QStringLiteral("update_state")] = snapshot.updateState;
    resp[QStringLiteral("auth_dll_version")] = snapshot.authDllVersion;
    resp[QStringLiteral("status_snapshot")] = statusSnapshotToJson(snapshot);
}

QJsonObject makeResp(bool ok,
                     const QString &action,
                     int rc,
                     const QString &message,
                     const QJsonObject &data,
                     const StatusSnapshot &snapshot,
                     bool includeCompatibilitySnapshot = false)
{
    QJsonObject resp;
    resp[QStringLiteral("ok")] = ok;
    resp[QStringLiteral("action")] = action;
    resp[QStringLiteral("rc")] = rc;
    resp[QStringLiteral("message")] = message;
    resp[QStringLiteral("data")] = data;
    if (includeCompatibilitySnapshot) {
        appendCompatibilitySnapshotFields(resp, snapshot);
    }
    return resp;
}

QJsonObject makeSnapshotResp(const QString &action,
                             int rc,
                             const QString &message,
                             const StatusSnapshot &snapshot)
{
    const QJsonObject snap = statusSnapshotToJson(snapshot);
    QJsonObject data;
    data[QStringLiteral("status_snapshot")] = snap;
    return makeResp(rc == kRcOk, action, rc, message, data, snapshot, true);
}

} // namespace

QJsonObject dispatchAction(StateManager &state, const QString &action, const QJsonObject &request)
{
    const QString act = action.trimmed();

    if (act == QStringLiteral("ping")) {
        QJsonObject data;
        data[QStringLiteral("pong")] = true;
        data[QStringLiteral("version")] = QStringLiteral("3.5.51");
        data[QStringLiteral("status_snapshot")] = statusSnapshotToJson(state.snapshot());
        QJsonObject resp = makeResp(true, act, kRcOk, QStringLiteral("pong"), data, state.snapshot(), true);
        resp[QStringLiteral("version")] = QStringLiteral("3.5.51");
        return resp;
    }

    if (act == QStringLiteral("get_protocol_info")) {
        QJsonObject data;
        data[QStringLiteral("protocol_version")] = state.protocolVersion();
        data[QStringLiteral("abi_version")] = state.abiVersion();
        data[QStringLiteral("status_snapshot")] = statusSnapshotToJson(state.snapshot());
        QJsonObject resp = makeResp(true, act, kRcOk, QStringLiteral("ok"), data, state.snapshot(), true);
        resp[QStringLiteral("protocol_version")] = state.protocolVersion();
        resp[QStringLiteral("abi_version")] = state.abiVersion();
        return resp;
    }

    if (act == QStringLiteral("get_status") ||
        act == QStringLiteral("get_status_snapshot")) {
        return makeSnapshotResp(act, kRcOk, QStringLiteral("ok"), state.snapshot());
    }

    if (act == QStringLiteral("get_update_snapshot")) {
        QJsonObject data = state.hostUpdateSnapshot();
        data[QStringLiteral("status_snapshot")] = statusSnapshotToJson(state.snapshot());
        return makeResp(true, act, kRcOk, QStringLiteral("ok"), data, state.snapshot(), true);
    }

    if (act == QStringLiteral("set_host_update_snapshot")) {
        state.setHostUpdateSnapshot(request.value(QStringLiteral("data")).toObject());
        return makeSnapshotResp(act, kRcOk, QStringLiteral("ok"), state.snapshot());
    }

    if (act == QStringLiteral("get_local_layout")) {
        QJsonObject data;
        data[QStringLiteral("paths")] = localPathsToJson(state.paths());
        data[QStringLiteral("exists")] = QJsonObject{
            {QStringLiteral("license_v3"), QFile::exists(state.paths().licensePath)},
            {QStringLiteral("inject_ticket_v3"), QFile::exists(state.paths().ticketPath)},
            {QStringLiteral("inject_result_v3"), QFile::exists(state.paths().resultPath)},
            {QStringLiteral("consumed_tickets_v3"),
             QFile::exists(state.paths().consumedTicketsPath)},
        };
        return makeResp(true, act, kRcOk, QStringLiteral("ok"), data, state.snapshot());
    }

    if (act == QStringLiteral("set_key")) {
        state.setLicenseKey(request.value(QStringLiteral("key")).toString());
        QJsonObject resp = makeSnapshotResp(act, kRcOk, QStringLiteral("ok"), state.snapshot());
        resp[QStringLiteral("key")] = state.licenseKey();
        return resp;
    }

    if (act == QStringLiteral("get_key")) {
        QJsonObject data;
        data[QStringLiteral("key")] = state.licenseKey();
        QJsonObject resp = makeResp(true, act, kRcOk, QStringLiteral("ok"), data, state.snapshot());
        resp[QStringLiteral("key")] = state.licenseKey();
        return resp;
    }

    if (act == QStringLiteral("reset") ||
        act == QStringLiteral("reset_runtime_state") ||
        act == QStringLiteral("reset_v3_local_state")) {
        state.resetRuntimeState();
        return makeSnapshotResp(act, kRcOk, QStringLiteral("已清空 V3 本地状态"), state.snapshot());
    }

    if (act == QStringLiteral("name_get_config")) {
        QJsonObject data = nameConfigToJson(state.nameConfig());
        return makeResp(true, act, kRcOk, QStringLiteral("ok"), data, state.snapshot());
    }

    if (act == QStringLiteral("name_set_enabled")) {
        QString message;
        state.setNameEnabled(request.value(QStringLiteral("enabled")).toBool(false), &message);
        QJsonObject data = nameConfigToJson(state.nameConfig());
        return makeResp(true, act, kRcOk, message, data, state.snapshot());
    }

    if (act == QStringLiteral("name_save_config")) {
        QString message;
        state.saveNameConfig(request.value(QStringLiteral("name")).toString(),
                             state.nameConfig().enabled,
                             &message);
        QJsonObject data = nameConfigToJson(state.nameConfig());
        return makeResp(true, act, kRcOk, message, data, state.snapshot());
    }

    if (act == QStringLiteral("name_apply_now")) {
        const uint targetPid = request.value(QStringLiteral("target_pid")).toVariant().toUInt();
        const QString name = request.value(QStringLiteral("name"))
                                 .toString(state.nameConfig().value)
                                 .trimmed();
        if (targetPid == 0 || name.isEmpty()) {
            return makeResp(false, act, kRcInvalidArgument,
                            QStringLiteral("缺少 target_pid 或 name"),
                            QJsonObject{},
                            state.snapshot());
        }

        const NBName::InjectResult result = NBName::injectName(targetPid, name, nullptr);
        QJsonObject data;
        data[QStringLiteral("target_pid")] = static_cast<qint64>(targetPid);
        data[QStringLiteral("name")] = name;
        data[QStringLiteral("applied")] = result.ok;
        return makeResp(result.ok,
                        act,
                        result.ok ? kRcOk : kRcInjectFailed,
                        result.message,
                        data,
                        state.snapshot());
    }

    if (act == QStringLiteral("verify") ||
        act == QStringLiteral("device_check_v3")) {
        QString message;
        const int rc = state.checkWithServer(&message);
        return makeSnapshotResp(act, rc, message, state.snapshot());
    }

    if (act == QStringLiteral("activate") ||
        act == QStringLiteral("activate_device_v3")) {
        QString message;
        const int rc = state.activateWithServer(
            request.value(QStringLiteral("key_plaintext"))
                .toString(request.value(QStringLiteral("key")).toString(state.licenseKey())),
            &message);
        QJsonObject data;
        data[QStringLiteral("key_id")] = state.snapshot().keyId;
        data[QStringLiteral("device_id")] = state.snapshot().deviceId;
        data[QStringLiteral("tier")] = state.snapshot().tierName;
        data[QStringLiteral("tier_name")] = state.snapshot().tierName;
        data[QStringLiteral("tier_value")] = state.snapshot().tierValue;
        data[QStringLiteral("feature_flags")] = static_cast<qint64>(state.snapshot().featureFlags);
        data[QStringLiteral("status_snapshot")] = statusSnapshotToJson(state.snapshot());
        return makeResp(rc == kRcOk, act, rc,
                        message,
                        data,
                        state.snapshot(),
                        true);
    }

    if (act == QStringLiteral("device_heartbeat_v3")) {
        QString message;
        const int rc = state.heartbeatOnce(&message);
        return makeSnapshotResp(act, rc, message, state.snapshot());
    }

    if (act == QStringLiteral("diagnose") ||
        act == QStringLiteral("dll_policy_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.getDllPolicy(data, &message);
        if (rc == kRcOk) {
            data[QStringLiteral("download_dir")] = state.paths().dllDir;
        }
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("model_entitlements_v1")) {
        QString message;
        QJsonObject data;
        const int rc = state.getModelEntitlements(data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("model_runtime_policy_v1")) {
        QString message;
        QJsonObject data;
        const int rc = state.getModelRuntimePolicy(request, data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("download_overlay_dll_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.downloadOverlayDll(request, data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("issue_inject_ticket_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.issueInjectTicket(request, data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("consume_inject_result_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.consumeInjectResult(data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("finalize_inject_failure_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.finalizePendingInjectFailure(
            request.value(QStringLiteral("status")).toString(QStringLiteral("inject_failed")),
            request.value(QStringLiteral("reason")).toString(QStringLiteral("inject_failed:no_result_file")),
            data,
            &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    if (act == QStringLiteral("report_inject_result_v3")) {
        QString message;
        QJsonObject data;
        const int rc = state.reportInjectResult(data, &message);
        return makeResp(rc == kRcOk, act, rc, message, data, state.snapshot(), true);
    }

    QJsonObject data;
    data[QStringLiteral("received_at_utc")] =
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    return makeResp(false, act, kRcInvalidArgument, QStringLiteral("unknown_action"), data, state.snapshot());
}

} // namespace NBAuth::V3
