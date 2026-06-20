#include "protected_ticket_ops.h"

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonValue>

#include "crypto/crypto_utils.h"

namespace NBAuth::Protected {

namespace {

constexpr char kWrapPrefix[] = "NB_WRAP_V3";
constexpr char kBuildSalt[] = "NB_AUTH_V3_BUILD_3_4_50";
constexpr char kFixedFragmentA[] = "NB_WRAP_FRAGMENT_A";
constexpr char kFixedFragmentB[] = "NB_WRAP_FRAGMENT_B";

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
                out += QStringLiteral("\\u%1")
                           .arg(static_cast<int>(ch.unicode()), 4, 16, QLatin1Char('0'));
            } else {
                out += ch;
            }
            break;
        }
    }
    return out;
}

QJsonObject parseJsonObject(const QByteArray &bytes, QString *error)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (!doc.isObject()) {
        if (error) {
            *error = parseError.error == QJsonParseError::NoError
                ? QStringLiteral("invalid_json_object")
                : QStringLiteral("json_parse_failed");
        }
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
        const quint32 parsed = text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
            ? text.mid(2).toUInt(&ok, 16)
            : text.toUInt(&ok, 10);
        return ok ? parsed : 0;
    }
    return 0;
}

int parseTierValue(const QJsonValue &value)
{
    if (value.isDouble()) {
        return value.toInt();
    }
    const QString normalized = value.toString().trimmed().toLower();
    if (normalized == QStringLiteral("trial")) {
        return 1;
    }
    if (normalized == QStringLiteral("premium")) {
        return 2;
    }
    if (normalized == QStringLiteral("dev")) {
        return 3;
    }
    return 0;
}

} // namespace

NB_NOINLINE QByteArray NBVmp_Ticket_DeriveWrapperKey(const QString &sessionId,
                                                     const QString &ticketSha256)
{
    QByteArray material;
    material.reserve(128);
    material.append(kWrapPrefix);
    material.append(sessionId.toUtf8());
    material.append(ticketSha256.toUtf8());
    material.append(kBuildSalt);
    material.append(kFixedFragmentA);
    material.append(kFixedFragmentB);
    return QCryptographicHash::hash(material, QCryptographicHash::Sha256);
}

NB_NOINLINE QString NBVmp_Ticket_ComputeTicketSha256(const QString &serverTicketPayload)
{
    return QString::fromLatin1(
               QCryptographicHash::hash(serverTicketPayload.toUtf8(),
                                        QCryptographicHash::Sha256).toHex())
        .toLower();
}

NB_NOINLINE bool NBVmp_Ticket_DecryptWrapper(const QByteArray &wrapperKey,
                                             const QByteArray &iv,
                                             const QByteArray &tag,
                                             const QByteArray &cipher,
                                             QByteArray &wrapperPlaintext,
                                             QString *error)
{
    NB_VMP_ULTRA("NB.Ticket.DecryptWrapper");
    wrapperPlaintext.clear();
    if (wrapperKey.size() != static_cast<int>(::NBAuth::AES_GCM_KEY_SIZE) ||
        iv.size() != static_cast<int>(::NBAuth::AES_GCM_IV_SIZE) ||
        tag.size() != static_cast<int>(::NBAuth::AES_GCM_TAG_SIZE) ||
        cipher.isEmpty()) {
        if (error) {
            *error = QStringLiteral("wrapper_invalid:wrapper_shape_invalid");
        }
        NB_VMP_END();
        return false;
    }

    ::NBAuth::ByteVector plaintext;
    const bool ok = ::NBAuth::AesGcmDecrypt(
        reinterpret_cast<const uint8_t *>(wrapperKey.constData()),
        reinterpret_cast<const uint8_t *>(iv.constData()),
        reinterpret_cast<const uint8_t *>(cipher.constData()),
        static_cast<size_t>(cipher.size()),
        nullptr,
        0,
        reinterpret_cast<const uint8_t *>(tag.constData()),
        plaintext);
    if (!ok) {
        if (error) {
            *error = QStringLiteral("wrapper_invalid:aes_gcm_decrypt_failed");
        }
        NB_VMP_END();
        return false;
    }

    wrapperPlaintext = QByteArray(reinterpret_cast<const char *>(plaintext.data()),
                                  static_cast<int>(plaintext.size()));
    NB_VMP_END();
    return true;
}

NB_NOINLINE bool NBVmp_Ticket_ParseWrapperJson(const QByteArray &wrapperPlaintext,
                                               ParsedWrapperTicket &out,
                                               QString *error)
{
    NB_VMP_VIRTUALIZE("NB.Ticket.ParseWrapperJson");
    out = ParsedWrapperTicket{};
    const QJsonObject obj = parseJsonObject(wrapperPlaintext, error);
    if (obj.isEmpty()) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("wrapper_invalid:wrapper_json_invalid");
        }
        NB_VMP_END();
        return false;
    }

    out.magic = obj.value(QStringLiteral("magic")).toString().trimmed();
    out.wrapperVersion = obj.value(QStringLiteral("wrapper_version")).toInt(0);
    out.createdAtUtc = obj.value(QStringLiteral("created_at_utc")).toString().trimmed();
    out.receivedTickMs = obj.value(QStringLiteral("received_tick_ms")).toVariant().toULongLong();
    out.targetPid = obj.value(QStringLiteral("target_pid")).toVariant().toUInt();
    out.exeVersion = obj.value(QStringLiteral("exe_version")).toString().trimmed();
    out.authDllProtocol = obj.value(QStringLiteral("auth_dll_protocol")).toInt(0);
    out.ticketSha256 = obj.value(QStringLiteral("ticket_sha256")).toString().trimmed().toLower();
    out.serverTicketPayload =
        obj.value(QStringLiteral("server_ticket_payload")).toString().trimmed();
    out.serverTicketSignature =
        obj.value(QStringLiteral("server_ticket_signature")).toString().trimmed().toLower();

    if (out.magic != QStringLiteral("NB_TICKET_WRAP_V3") ||
        out.wrapperVersion != 3 ||
        out.targetPid == 0 ||
        out.ticketSha256.isEmpty() ||
        out.serverTicketPayload.isEmpty() ||
        out.serverTicketSignature.isEmpty()) {
        if (error) {
            *error = QStringLiteral("wrapper_invalid:wrapper_fields_invalid");
        }
        NB_VMP_END();
        return false;
    }

    NB_VMP_END();
    return true;
}

NB_NOINLINE bool NBVmp_Ticket_ParseServerTicket(const QString &payload,
                                                ParsedServerTicket &out,
                                                QString *error)
{
    NB_VMP_VIRTUALIZE("NB.Ticket.ParseServerTicket");
    out = ParsedServerTicket{};
    const QJsonObject obj = parseJsonObject(payload.toUtf8(), error);
    if (obj.isEmpty()) {
        if (error && error->isEmpty()) {
            *error = QStringLiteral("wrapper_invalid:server_ticket_json_invalid");
        }
        NB_VMP_END();
        return false;
    }

    out.magic = obj.value(QStringLiteral("magic")).toString().trimmed();
    out.version = obj.value(QStringLiteral("version")).toInt(0);
    out.alg = obj.value(QStringLiteral("alg")).toString().trimmed();
    out.keyId = obj.value(QStringLiteral("key_id")).toString().trimmed();
    out.deviceId = obj.value(QStringLiteral("device_id")).toString().trimmed();
    out.sessionId = obj.value(QStringLiteral("session_id")).toString().trimmed();
    out.ticketId = obj.value(QStringLiteral("ticket_id")).toString().trimmed();
    out.tier = obj.value(QStringLiteral("tier")).toString().trimmed();
    out.tierValue = parseTierValue(obj.value(QStringLiteral("tier")));
    out.featureFlags = parseFeatureFlags(obj.value(QStringLiteral("feature_flags")));
    out.dllName = obj.value(QStringLiteral("dll_name")).toString().trimmed();
    out.dllSha256 = obj.value(QStringLiteral("dll_sha256")).toString().trimmed().toLower();
    out.issuedAtServer = obj.value(QStringLiteral("issued_at_server")).toString().trimmed();
    out.expiresAtServer = obj.value(QStringLiteral("expires_at_server")).toString().trimmed();
    out.ttlMs = obj.value(QStringLiteral("ttl_ms")).toVariant().toLongLong();
    out.nonce = obj.value(QStringLiteral("nonce")).toString().trimmed().toLower();

    if (out.magic != QStringLiteral("NB_INJECT_TICKET") ||
        out.version != 3 ||
        out.alg.compare(QStringLiteral("RS256"), Qt::CaseInsensitive) != 0 ||
        out.sessionId.isEmpty() ||
        out.ticketId.isEmpty() ||
        out.dllSha256.isEmpty() ||
        out.issuedAtServer.isEmpty() ||
        out.expiresAtServer.isEmpty() ||
        out.ttlMs <= 0) {
        if (error) {
            *error = QStringLiteral("wrapper_invalid:server_ticket_fields_invalid");
        }
        NB_VMP_END();
        return false;
    }

    NB_VMP_END();
    return true;
}

NB_NOINLINE QString NBVmp_Ticket_CanonicalResultJson(const QString &sessionId,
                                                     const QString &ticketId,
                                                     const QString &ticketSha256,
                                                     const QString &status,
                                                     const QString &reason,
                                                     const QString &dllVersion,
                                                     qint64 processedTickMs,
                                                     const QString &grantedTier,
                                                     qint64 grantedFeatureFlags,
                                                     const QString &verifiedDllSha256,
                                                     const QString &issuedAtServer,
                                                     const QString &expiresAtServer,
                                                     int serverPubkeyVersion,
                                                     const QString &serverPubkeyFingerprint)
{
    NB_VMP_VIRTUALIZE("NB.Ticket.CanonicalResultJson");
    const QString json = QStringLiteral(
        "{\"magic\":\"NB_TICKET_RESULT_V3\",\"version\":3,\"session_id\":\"%1\","
        "\"ticket_id\":\"%2\",\"ticket_sha256\":\"%3\",\"status\":\"%4\","
        "\"reason\":\"%5\",\"dll_version\":\"%6\",\"processed_tick_ms\":%7,"
        "\"granted_tier\":\"%8\",\"granted_feature_flags\":%9,"
        "\"verified_dll_sha256\":\"%10\",\"issued_at_server\":\"%11\","
        "\"expires_at_server\":\"%12\",\"server_pubkey_version\":%13,"
        "\"server_pubkey_fingerprint\":\"%14\"}")
        .arg(escapeJson(sessionId),
             escapeJson(ticketId),
             escapeJson(ticketSha256),
             escapeJson(status),
             escapeJson(reason),
             escapeJson(dllVersion),
             QString::number(processedTickMs),
             escapeJson(grantedTier),
             QString::number(grantedFeatureFlags),
             escapeJson(verifiedDllSha256),
             escapeJson(issuedAtServer),
             escapeJson(expiresAtServer),
             QString::number(serverPubkeyVersion),
             escapeJson(serverPubkeyFingerprint));
    NB_VMP_END();
    return json;
}

NB_NOINLINE QString NBVmp_Ticket_ComputeResultHmacHex(const QByteArray &wrapperKey,
                                                      const QByteArray &canonicalResultUtf8,
                                                      QString *error)
{
    NB_VMP_ULTRA("NB.Ticket.ComputeResultHmacHex");
    if (wrapperKey.size() != static_cast<int>(::NBAuth::HMAC_KEY_SIZE) ||
        canonicalResultUtf8.isEmpty()) {
        if (error) {
            *error = QStringLiteral("result_hmac_invalid:input_shape_invalid");
        }
        NB_VMP_END();
        return {};
    }

    ::NBAuth::FixedHash32 out{};
    const bool ok = ::NBAuth::HmacSha256(
        reinterpret_cast<const uint8_t *>(wrapperKey.constData()),
        static_cast<size_t>(wrapperKey.size()),
        reinterpret_cast<const uint8_t *>(canonicalResultUtf8.constData()),
        static_cast<size_t>(canonicalResultUtf8.size()),
        out);
    if (!ok) {
        if (error) {
            *error = QStringLiteral("result_hmac_invalid:hmac_compute_failed");
        }
        NB_VMP_END();
        return {};
    }

    const QString hmacHex = QString::fromLatin1(
                                QByteArray(reinterpret_cast<const char *>(out.data()),
                                           static_cast<int>(out.size()))
                                    .toHex())
                                .toLower();
    NB_VMP_END();
    return hmacHex;
}

} // namespace NBAuth::Protected
