#include "v3_rpc_client.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>

#include "crypto/crypto_utils.h"

namespace NBAuth::V3 {

namespace {

static const uint8_t g_rpcPsk[32] = {
    0xbb, 0xfd, 0xb6, 0x21, 0x90, 0xaf, 0x1e, 0x5e,
    0x62, 0xc7, 0x2b, 0xf9, 0xa0, 0x28, 0x26, 0x57,
    0xfb, 0x0f, 0x95, 0xe9, 0xc7, 0xbd, 0xec, 0x7f,
    0x70, 0x70, 0x83, 0xa3, 0xd4, 0xff, 0x58, 0x23
};

constexpr const char *kDefaultHost = "notebot-api.fucku.top";
constexpr quint16 kDefaultPort = 30165;

QString escapeJson(const QString &value)
{
    QString out;
    out.reserve(value.size() + 8);
    for (const QChar ch : value) {
        switch (ch.unicode()) {
        case '\"': out += QStringLiteral("\\\""); break;
        case '\\': out += QStringLiteral("\\\\"); break;
        case '\b': out += QStringLiteral("\\b"); break;
        case '\f': out += QStringLiteral("\\f"); break;
        case '\n': out += QStringLiteral("\\n"); break;
        case '\r': out += QStringLiteral("\\r"); break;
        case '\t': out += QStringLiteral("\\t"); break;
        default:
            if (ch.unicode() < 0x20) {
                out += QStringLiteral("\\u%1").arg(static_cast<int>(ch.unicode()), 4, 16, QLatin1Char('0'));
            } else {
                out += ch;
            }
            break;
        }
    }
    return out;
}

void appendField(QStringList &parts, const QString &key, const QString &rawValue)
{
    parts.append(QStringLiteral("\"%1\":%2").arg(key, rawValue));
}

QJsonObject parseObject(const QByteArray &json, QString &error)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (!doc.isObject()) {
        error = parseError.error == QJsonParseError::NoError
            ? QStringLiteral("invalid_json_object")
            : parseError.errorString();
        return {};
    }
    return doc.object();
}

quint32 parseFeatureFlags(const QJsonValue &value)
{
    if (value.isDouble()) {
        return static_cast<quint32>(value.toInteger());
    }
    if (value.isString()) {
        bool ok = false;
        const QString text = value.toString().trimmed();
        quint32 parsed = text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
            ? text.mid(2).toUInt(&ok, 16)
            : text.toUInt(&ok, 10);
        return ok ? parsed : 0;
    }
    return 0;
}

} // namespace

RpcClientV3::RpcClientV3() = default;

bool RpcClientV3::activateDevice(const RpcActivateRequest &request,
                                 RpcActivateResponse &response,
                                 QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("activate_device_v3")},
            {QStringLiteral("key_plaintext"), request.keyPlaintext},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("device_public_key_pem"), request.devicePublicKeyPem},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("client_version"), request.clientVersion},
        },
        {
            {QStringLiteral("protocol_version"), request.protocolVersion},
        }).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.keyId = obj.value(QStringLiteral("key_id")).toString();
    response.keyHash = obj.value(QStringLiteral("key_hash")).toString();
    response.tier = obj.value(QStringLiteral("tier")).toString();
    response.tierValue = tierValueFromVariant(obj.value(QStringLiteral("tier")));
    response.featureFlags = parseFeatureFlags(obj.value(QStringLiteral("feature_flags")));
    response.deviceId = obj.value(QStringLiteral("device_id")).toString();
    response.serverPubkeyVersion = obj.value(QStringLiteral("server_pubkey_version")).toInt();
    response.serverPubkeyFingerprint =
        obj.value(QStringLiteral("server_pubkey_fingerprint")).toString();
    response.boundDllName = obj.value(QStringLiteral("bound_dll_name")).toString();
    response.boundDllSha256 = obj.value(QStringLiteral("bound_dll_sha256")).toString();
    return true;
}

bool RpcClientV3::deviceCheck(const RpcCheckRequest &request,
                              RpcCheckResponse &response,
                              QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("device_check_v3")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("client_version"), request.clientVersion},
            {QStringLiteral("payload"), request.payloadJson},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {
            {QStringLiteral("protocol_version"), request.protocolVersion},
        }).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.online = obj.value(QStringLiteral("online")).toBool(false);
    response.kicked = obj.value(QStringLiteral("kicked")).toBool(false);
    response.tier = obj.value(QStringLiteral("tier")).toString();
    response.tierValue = tierValueFromVariant(obj.value(QStringLiteral("tier")));
    response.featureFlags = parseFeatureFlags(obj.value(QStringLiteral("feature_flags")));
    response.boundDllName = obj.value(QStringLiteral("bound_dll_name")).toString();
    response.boundDllSha256 = obj.value(QStringLiteral("bound_dll_sha256")).toString();
    response.replaceCooldownRemainingSec =
        obj.value(QStringLiteral("replace_cooldown_remaining_sec")).toVariant().toLongLong();
    return true;
}

bool RpcClientV3::deviceHeartbeat(const RpcHeartbeatRequest &request,
                                  RpcHeartbeatResponse &response,
                                  QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("device_heartbeat_v3")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {}).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.online = obj.value(QStringLiteral("online")).toBool(false);
    response.kicked = obj.value(QStringLiteral("kicked")).toBool(false);
    response.serverTimeUtc = obj.value(QStringLiteral("server_time_utc")).toString();
    return true;
}

bool RpcClientV3::dllPolicy(const RpcDllPolicyRequest &request,
                            RpcDllPolicyResponse &response,
                            QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("dll_policy_v3")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {}).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.dllName = obj.value(QStringLiteral("dll_name")).toString();
    response.dllSha256 = obj.value(QStringLiteral("dll_sha256")).toString().trimmed().toLower();
    response.dllMd5 = obj.value(QStringLiteral("dll_md5")).toString().trimmed().toLower();
    response.dllSize = obj.value(QStringLiteral("dll_size")).toVariant().toLongLong();
    response.downloadUrl = obj.value(QStringLiteral("download_url")).toString().trimmed();
    response.channel = obj.value(QStringLiteral("channel")).toString().trimmed();
    response.expiresIn = obj.value(QStringLiteral("expires_in")).toInt(0);
    return true;
}

bool RpcClientV3::modelEntitlements(const RpcModelEntitlementsRequest &request,
                                    RpcModelEntitlementsResponse &response,
                                    QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("model_entitlements_v1")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {
            {QStringLiteral("protocol_version"), request.protocolVersion},
        }).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.models = obj.value(QStringLiteral("models")).toArray();
    return true;
}

bool RpcClientV3::issueInjectTicket(const RpcIssueTicketRequest &request,
                                    RpcIssueTicketResponse &response,
                                    QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("issue_inject_ticket_v3")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("dll_name"), request.dllName},
            {QStringLiteral("dll_sha256"), request.dllSha256},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {
            {QStringLiteral("target_pid"), static_cast<qint64>(request.targetPid)},
        }).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.sessionId = obj.value(QStringLiteral("session_id")).toString().trimmed();
    response.ticketId = obj.value(QStringLiteral("ticket_id")).toString().trimmed();
    response.serverTicketPayload =
        obj.value(QStringLiteral("server_ticket_payload")).toString().trimmed();
    response.serverTicketSignature =
        obj.value(QStringLiteral("server_ticket_signature")).toString().trimmed().toLower();
    response.expiresIn = obj.value(QStringLiteral("expires_in")).toInt(0);
    return true;
}

bool RpcClientV3::reportInjectResult(const RpcReportResultRequest &request,
                                     RpcReportResultResponse &response,
                                     QString &error) const
{
    QByteArray json = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("report_inject_result_v3")},
            {QStringLiteral("key_id"), request.keyId},
            {QStringLiteral("device_id"), request.deviceId},
            {QStringLiteral("session_id"), request.sessionId},
            {QStringLiteral("ticket_id"), request.ticketId},
            {QStringLiteral("status"), request.status},
            {QStringLiteral("reason"), request.reason},
            {QStringLiteral("verified_tier"), request.verifiedTier},
            {QStringLiteral("verified_dll_sha256"), request.verifiedDllSha256},
            {QStringLiteral("verified_at_utc"), request.verifiedAtUtc},
            {QStringLiteral("issued_at_server"), request.issuedAtServer},
            {QStringLiteral("expires_at_server"), request.expiresAtServer},
            {QStringLiteral("server_pubkey_fingerprint"), request.serverPubkeyFingerprint},
            {QStringLiteral("timestamp_utc"), request.timestampUtc},
            {QStringLiteral("nonce"), request.nonceHex},
            {QStringLiteral("device_signature"), request.deviceSignatureHex},
        },
        {
            {QStringLiteral("verified_tier_value"), static_cast<qint64>(request.verifiedTierValue)},
            {QStringLiteral("verified_feature_flags"), static_cast<qint64>(request.verifiedFeatureFlags)},
            {QStringLiteral("server_pubkey_version"), static_cast<qint64>(request.serverPubkeyVersion)},
        }).toUtf8();

    QByteArray plain = sendEncryptedJson(json, 15000, error);
    if (plain.isEmpty()) {
        return false;
    }

    const QJsonObject obj = parseObject(plain, error);
    if (obj.isEmpty()) {
        return false;
    }

    response.status = obj.value(QStringLiteral("status")).toString();
    response.accepted = obj.value(QStringLiteral("accepted")).toBool(false);
    return true;
}

QByteArray RpcClientV3::aesGcmEncrypt(const QByteArray &plaintext) const
{
    uint8_t iv[NBAuth::AES_GCM_IV_SIZE];
    uint8_t tag[NBAuth::AES_GCM_TAG_SIZE];
    NBAuth::ByteVector ct;
    if (!NBAuth::SecureRandomBytes(iv, NBAuth::AES_GCM_IV_SIZE)) {
        return {};
    }
    if (!NBAuth::AesGcmEncrypt(g_rpcPsk, iv,
                               reinterpret_cast<const uint8_t *>(plaintext.constData()),
                               static_cast<size_t>(plaintext.size()),
                               nullptr, 0, ct, tag)) {
        return {};
    }

    QByteArray result;
    result.append(reinterpret_cast<const char *>(iv), NBAuth::AES_GCM_IV_SIZE);
    result.append(reinterpret_cast<const char *>(ct.data()), static_cast<int>(ct.size()));
    result.append(reinterpret_cast<const char *>(tag), NBAuth::AES_GCM_TAG_SIZE);
    return result;
}

QByteArray RpcClientV3::aesGcmDecrypt(const QByteArray &ciphertext) const
{
    if (ciphertext.size() < static_cast<int>(NBAuth::AES_GCM_IV_SIZE + NBAuth::AES_GCM_TAG_SIZE)) {
        return {};
    }

    const uint8_t *iv = reinterpret_cast<const uint8_t *>(ciphertext.constData());
    const uint8_t *ct = iv + NBAuth::AES_GCM_IV_SIZE;
    const size_t ctLen =
        static_cast<size_t>(ciphertext.size()) - NBAuth::AES_GCM_IV_SIZE - NBAuth::AES_GCM_TAG_SIZE;
    const uint8_t *tag = ct + ctLen;

    NBAuth::ByteVector pt;
    if (!NBAuth::AesGcmDecrypt(g_rpcPsk, iv, ct, ctLen, nullptr, 0, tag, pt)) {
        return {};
    }
    return QByteArray(reinterpret_cast<const char *>(pt.data()), static_cast<int>(pt.size()));
}

QByteArray RpcClientV3::sendEncryptedJson(const QByteArray &requestJson,
                                          int timeoutMs,
                                          QString &error) const
{
    const QByteArray encrypted = aesGcmEncrypt(requestJson);
    if (encrypted.isEmpty()) {
        error = QStringLiteral("encrypt_failed");
        return {};
    }

    QTcpSocket socket;
    socket.connectToHost(QString::fromLatin1(kDefaultHost), kDefaultPort);
    if (!socket.waitForConnected(timeoutMs)) {
        error = socket.errorString();
        return {};
    }

    QByteArray packet;
    QDataStream ds(&packet, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint32>(encrypted.size());
    packet.append(encrypted);

    socket.write(packet);
    if (!socket.waitForBytesWritten(5000)) {
        error = QStringLiteral("write_timeout");
        socket.close();
        return {};
    }
    if (!socket.waitForReadyRead(timeoutMs)) {
        error = QStringLiteral("read_timeout");
        socket.close();
        return {};
    }

    QByteArray lenBytes = socket.read(4);
    if (lenBytes.size() != 4) {
        error = QStringLiteral("bad_length_prefix");
        socket.close();
        return {};
    }

    QDataStream lenStream(lenBytes);
    lenStream.setByteOrder(QDataStream::BigEndian);
    quint32 respLen = 0;
    lenStream >> respLen;

    QByteArray respEncrypted;
    while (respEncrypted.size() < static_cast<int>(respLen)) {
        if (socket.bytesAvailable() > 0) {
            respEncrypted.append(socket.read(respLen - respEncrypted.size()));
        } else if (!socket.waitForReadyRead(5000)) {
            break;
        }
    }
    socket.close();

    if (respEncrypted.size() != static_cast<int>(respLen)) {
        error = QStringLiteral("short_response");
        return {};
    }

    QByteArray plain = aesGcmDecrypt(respEncrypted);
    if (plain.isEmpty()) {
        error = QStringLiteral("decrypt_failed");
    }
    return plain;
}

QString canonicalJsonString(const QList<QPair<QString, QString>> &stringFields,
                            const QList<QPair<QString, qint64>> &integerFields,
                            const QList<QPair<QString, bool>> &boolFields)
{
    QStringList parts;
    parts.reserve(stringFields.size() + integerFields.size() + boolFields.size());

    for (const auto &item : stringFields) {
        appendField(parts,
                    item.first,
                    QStringLiteral("\"%1\"").arg(escapeJson(item.second)));
    }
    for (const auto &item : integerFields) {
        appendField(parts, item.first, QString::number(item.second));
    }
    for (const auto &item : boolFields) {
        appendField(parts, item.first, item.second ? QStringLiteral("true")
                                                   : QStringLiteral("false"));
    }

    return QStringLiteral("{%1}").arg(parts.join(QStringLiteral(",")));
}

QString randomNonceHex(int sizeBytes)
{
    QByteArray bytes(sizeBytes, Qt::Uninitialized);
    if (!NBAuth::SecureRandomBytes(reinterpret_cast<uint8_t *>(bytes.data()),
                                   static_cast<size_t>(bytes.size()))) {
        return QString();
    }
    return QString::fromLatin1(bytes.toHex());
}

QString tierNameFromValue(int tierValue)
{
    switch (tierValue) {
    case 3: return QStringLiteral("Dev");
    case 2: return QStringLiteral("Premium");
    case 1: return QStringLiteral("Trial");
    default: return QStringLiteral("None");
    }
}

int tierValueFromVariant(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toInt(0);
    }
    const QString text = value.toString().trimmed();
    if (text.compare(QStringLiteral("dev"), Qt::CaseInsensitive) == 0) return 3;
    if (text.compare(QStringLiteral("premium"), Qt::CaseInsensitive) == 0) return 2;
    if (text.compare(QStringLiteral("trial"), Qt::CaseInsensitive) == 0) return 1;
    return 0;
}

} // namespace NBAuth::V3
