#include "v3_state.h"

#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QByteArray>
#include <QHostAddress>
#include <QUrl>
#include <QUuid>
#include <QtGlobal>

#include <windows.h>
#include <wincrypt.h>
#include <winhttp.h>
#include <algorithm>
#include <cstring>

#include "crypto/crypto_utils.h"
#include "protected/protected_ticket_ops.h"
#include "protected/protected_verify_ops.h"
#include "protected/protected_v3_ops.h"
#include "v3_server_pubkey.h"

namespace NBAuth::V3 {

namespace {

constexpr int kProtocolVersion = 3;
constexpr int kAbiVersion = 1;
constexpr char kAuthDllVersion[] = "3.5.50";
constexpr int kAuthDllVersionCode = 30550;
constexpr char kMainExeVersion[] = "3.5.98";
constexpr int kMainExeVersionCode = 30598;
constexpr char kUpdaterExeVersion[] = "3.5.71";
constexpr int kUpdaterExeVersionCode = 30571;
constexpr char kLicenseMagic[] = "NBV3";
constexpr char kTicketFileMagic[16] = {
    'N','B','_','T','I','C','K','E','T','_','W','R','A','P','V','3'
};
constexpr char kWrapperMagic[] = "NB_TICKET_WRAP_V3";
constexpr char kResultMagic[] = "NB_TICKET_RESULT_V3";
constexpr char kPinnedDownloadCertSha256[] =
    "755a7e09f472a45ef9b6a50c87c845f7059fd1be825cfcdf61c7c503ba109126";

struct HttpDownloadResult {
    int statusCode = 0;
    QByteArray bytes;
    QString error;
    bool timedOut = false;
};

QString nowUtcIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QByteArray toQByteArray(const ::NBAuth::ByteVector &bytes)
{
    return QByteArray(reinterpret_cast<const char *>(bytes.data()),
                      static_cast<int>(bytes.size()));
}

QString toPemPublicKey(const QByteArray &der)
{
    QByteArray b64 = der.toBase64();
    QByteArray lines;
    for (int i = 0; i < b64.size(); i += 64) {
        lines += b64.mid(i, 64);
        lines += '\n';
    }

    return QString::fromLatin1("-----BEGIN PUBLIC KEY-----\n") +
           QString::fromLatin1(lines) +
           QString::fromLatin1("-----END PUBLIC KEY-----\n");
}

QString sha256Hex(const QByteArray &bytes)
{
    return QString::fromLatin1(
               QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex())
        .toLower();
}

QString hashBytesHex(const QByteArray &bytes, QCryptographicHash::Algorithm alg)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, alg).toHex()).toLower();
}

bool isIpLiteralHost(const QString &host)
{
    QHostAddress address;
    return address.setAddress(host);
}

QString safeArtifactFileName(const QString &name)
{
    return QFileInfo(name.trimmed()).fileName();
}

QString winHttpError(const QString &where)
{
    return QStringLiteral("%1 GetLastError=%2")
        .arg(where)
        .arg(static_cast<unsigned long>(GetLastError()));
}

bool verifyPinnedDownloadCertificate(HINTERNET request, QString *error)
{
    PCCERT_CONTEXT cert = nullptr;
    DWORD certSize = sizeof(cert);
    if (!WinHttpQueryOption(request, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &cert, &certSize) || !cert) {
        if (error) {
            *error = winHttpError(QStringLiteral("WinHttpQueryOption(SERVER_CERT_CONTEXT)"));
        }
        return false;
    }

    const QByteArray certDer(reinterpret_cast<const char *>(cert->pbCertEncoded),
                             static_cast<int>(cert->cbCertEncoded));
    CertFreeCertificateContext(cert);

    const QString actualSha256 = hashBytesHex(certDer, QCryptographicHash::Sha256);
    if (actualSha256 != QString::fromLatin1(kPinnedDownloadCertSha256)) {
        if (error) {
            *error = QStringLiteral("服务器下载证书指纹不匹配 expected=%1 actual=%2")
                         .arg(QString::fromLatin1(kPinnedDownloadCertSha256), actualSha256);
        }
        return false;
    }
    return true;
}

HttpDownloadResult httpGetNoProxy(const QUrl &url, qint64 expectedSize, int timeoutMs)
{
    HttpDownloadResult result;
    const QString encoded = url.toString(QUrl::FullyEncoded);
    const std::wstring urlW = encoded.toStdWString();

    URL_COMPONENTS parts;
    ZeroMemory(&parts, sizeof(parts));
    parts.dwStructSize = sizeof(parts);

    wchar_t host[512] = {0};
    wchar_t path[4096] = {0};
    parts.lpszHostName = host;
    parts.dwHostNameLength = _countof(host);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = _countof(path);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(urlW.c_str(), 0, 0, &parts)) {
        result.error = winHttpError(QStringLiteral("WinHttpCrackUrl"));
        return result;
    }

    QString pathAndQuery =
        QString::fromWCharArray(parts.lpszUrlPath, static_cast<int>(parts.dwUrlPathLength));
    if (parts.dwExtraInfoLength > 0 && parts.lpszExtraInfo) {
        pathAndQuery += QString::fromWCharArray(parts.lpszExtraInfo,
                                                static_cast<int>(parts.dwExtraInfoLength));
    }
    const std::wstring pathW = pathAndQuery.toStdWString();

    HINTERNET session = WinHttpOpen(L"NoteBotAuth",
                                    WINHTTP_ACCESS_TYPE_NO_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS,
                                    0);
    if (!session) {
        result.error = winHttpError(QStringLiteral("WinHttpOpen"));
        return result;
    }

    WinHttpSetTimeouts(session, 10000, 10000, timeoutMs, timeoutMs);

    HINTERNET connect = WinHttpConnect(session, host, parts.nPort, 0);
    if (!connect) {
        result.error = winHttpError(QStringLiteral("WinHttpConnect"));
        WinHttpCloseHandle(session);
        return result;
    }

    const DWORD flags =
        (parts.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    const bool isHttps = (parts.nScheme == INTERNET_SCHEME_HTTPS);
    const QString requestHost =
        QString::fromWCharArray(parts.lpszHostName, static_cast<int>(parts.dwHostNameLength));
    const bool pinnedIpDownload = isHttps && isIpLiteralHost(requestHost);

    HINTERNET request = WinHttpOpenRequest(connect,
                                           L"GET",
                                           pathW.c_str(),
                                           nullptr,
                                           WINHTTP_NO_REFERER,
                                           WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           flags);
    if (!request) {
        result.error = winHttpError(QStringLiteral("WinHttpOpenRequest"));
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return result;
    }

    if (pinnedIpDownload) {
        DWORD securityFlags = SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(request,
                         WINHTTP_OPTION_SECURITY_FLAGS,
                         &securityFlags,
                         sizeof(securityFlags));
    }

    const BOOL sent = WinHttpSendRequest(request,
                                         WINHTTP_NO_ADDITIONAL_HEADERS,
                                         0,
                                         WINHTTP_NO_REQUEST_DATA,
                                         0,
                                         0,
                                         0);
    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        const DWORD err = GetLastError();
        result.timedOut = (err == ERROR_WINHTTP_TIMEOUT);
        result.error = QStringLiteral("WinHTTP request failed GetLastError=%1")
                           .arg(static_cast<unsigned long>(err));
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return result;
    }

    if (pinnedIpDownload) {
        QString pinError;
        if (!verifyPinnedDownloadCertificate(request, &pinError)) {
            result.error = pinError;
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return result;
        }
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &status,
                        &statusSize,
                        WINHTTP_NO_HEADER_INDEX);
    result.statusCode = static_cast<int>(status);

    qint64 totalSize = expectedSize;
    wchar_t contentLengthText[64] = {0};
    DWORD contentLengthSize = sizeof(contentLengthText);
    if (WinHttpQueryHeaders(request,
                            WINHTTP_QUERY_CONTENT_LENGTH,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            contentLengthText,
                            &contentLengthSize,
                            WINHTTP_NO_HEADER_INDEX)) {
        bool okLen = false;
        const qint64 headerLen =
            QString::fromWCharArray(contentLengthText).toLongLong(&okLen);
        if (okLen && headerLen > 0) {
            totalSize = headerLen;
        }
    }

    QByteArray out;
    if (expectedSize > 0 && expectedSize <= 256 * 1024 * 1024) {
        out.reserve(static_cast<int>(expectedSize));
    }

    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) {
            result.error = winHttpError(QStringLiteral("WinHttpQueryDataAvailable"));
            break;
        }
        if (available == 0) {
            break;
        }

        QByteArray chunk;
        chunk.resize(static_cast<int>(available));
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), available, &read)) {
            const DWORD err = GetLastError();
            result.timedOut = (err == ERROR_WINHTTP_TIMEOUT);
            result.error = QStringLiteral("WinHttpReadData GetLastError=%1")
                               .arg(static_cast<unsigned long>(err));
            break;
        }
        chunk.resize(static_cast<int>(read));
        out.append(chunk);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    if (result.error.isEmpty() && totalSize > 0 && out.size() != totalSize) {
        result.error = QStringLiteral("download_size_mismatch");
    }
    result.bytes = out;
    return result;
}

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

QString canonicalWrapperJson(const QString &createdAtUtc,
                             quint64 receivedTickMs,
                             quint32 targetPid,
                             const QString &exeVersion,
                             int authDllProtocol,
                             const QString &ticketSha256,
                             const QString &serverTicketPayload,
                             const QString &serverTicketSignature)
{
    return QStringLiteral(
        "{\"magic\":\"%1\",\"wrapper_version\":3,\"created_at_utc\":\"%2\","
        "\"received_tick_ms\":%3,\"target_pid\":%4,\"exe_version\":\"%5\","
        "\"auth_dll_protocol\":%6,\"ticket_sha256\":\"%7\","
        "\"server_ticket_payload\":\"%8\",\"server_ticket_signature\":\"%9\"}")
        .arg(QString::fromLatin1(kWrapperMagic),
             escapeJson(createdAtUtc),
             QString::number(receivedTickMs),
             QString::number(targetPid),
             escapeJson(exeVersion),
             QString::number(authDllProtocol),
             escapeJson(ticketSha256),
             escapeJson(serverTicketPayload),
             escapeJson(serverTicketSignature));
}

QString canonicalResultJsonForHmac(const QString &sessionId,
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
    return Protected::NBVmp_Ticket_CanonicalResultJson(sessionId,
                                                       ticketId,
                                                       ticketSha256,
                                                       status,
                                                       reason,
                                                       dllVersion,
                                                       processedTickMs,
                                                       grantedTier,
                                                       grantedFeatureFlags,
                                                       verifiedDllSha256,
                                                       issuedAtServer,
                                                       expiresAtServer,
                                                       serverPubkeyVersion,
                                                       serverPubkeyFingerprint);
}

QString hmacHex(const QByteArray &key, const QByteArray &data)
{
    QString error;
    return Protected::NBVmp_Ticket_ComputeResultHmacHex(key, data, &error);
}

void appendLe32(QByteArray &bytes, quint32 value)
{
    bytes.append(static_cast<char>(value & 0xFF));
    bytes.append(static_cast<char>((value >> 8) & 0xFF));
    bytes.append(static_cast<char>((value >> 16) & 0xFF));
    bytes.append(static_cast<char>((value >> 24) & 0xFF));
}

void appendBe32(QByteArray &bytes, quint32 value)
{
    bytes.append(static_cast<char>((value >> 24) & 0xFF));
    bytes.append(static_cast<char>((value >> 16) & 0xFF));
    bytes.append(static_cast<char>((value >> 8) & 0xFF));
    bytes.append(static_cast<char>(value & 0xFF));
}

bool readLe32(const QByteArray &bytes, int offset, quint32 &value)
{
    if (offset < 0 || offset + 4 > bytes.size()) {
        return false;
    }
    const unsigned char *p =
        reinterpret_cast<const unsigned char *>(bytes.constData() + offset);
    value = static_cast<quint32>(p[0]) |
            (static_cast<quint32>(p[1]) << 8) |
            (static_cast<quint32>(p[2]) << 16) |
            (static_cast<quint32>(p[3]) << 24);
    return true;
}

bool saveBinaryFile(const QString &path, const QByteArray &data, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }
    if (file.write(data) != data.size()) {
        if (error) {
            *error = QStringLiteral("file_write_failed");
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = QStringLiteral("file_commit_failed");
        }
        return false;
    }
    return true;
}

QString computeFileSha256Path(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(64 * 1024);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            return QString();
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex()).toLower();
}

qint64 fileSizePath(const QString &path)
{
    QFileInfo info(path);
    return info.exists() ? info.size() : -1;
}

bool parseObjectFile(const QString &path, QJsonObject &obj, QString *error)
{
    QFile file(path);
    if (!file.exists()) {
        if (error) {
            *error = QStringLiteral("file_missing");
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (!doc.isObject()) {
        if (error) {
            *error = parseError.error == QJsonParseError::NoError
                ? QStringLiteral("invalid_json_object")
                : parseError.errorString();
        }
        return false;
    }
    obj = doc.object();
    return true;
}

QString deriveDeviceId(const QString &devicePublicKeyPem)
{
    const QString digest = sha256Hex(devicePublicKeyPem.toUtf8());
    if (digest.size() < 32) {
        return QString();
    }
    return QStringLiteral("did_%1").arg(digest.left(32));
}

bool dpapiProtectMachine(const QByteArray &plaintext, QByteArray &ciphertext, QString *error)
{
    DATA_BLOB inBlob{};
    DATA_BLOB outBlob{};
    inBlob.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plaintext.constData()));
    inBlob.cbData = static_cast<DWORD>(plaintext.size());

    if (!CryptProtectData(&inBlob,
                          L"NoteBot V3",
                          nullptr,
                          nullptr,
                          nullptr,
                          CRYPTPROTECT_LOCAL_MACHINE,
                          &outBlob)) {
        if (error) {
            *error = QStringLiteral("dpapi_protect_failed_%1").arg(GetLastError());
        }
        return false;
    }

    ciphertext = QByteArray(reinterpret_cast<const char *>(outBlob.pbData),
                            static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return true;
}

bool dpapiUnprotectMachine(const QByteArray &ciphertext, QByteArray &plaintext, QString *error)
{
    DATA_BLOB inBlob{};
    DATA_BLOB outBlob{};
    inBlob.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(ciphertext.constData()));
    inBlob.cbData = static_cast<DWORD>(ciphertext.size());

    if (!CryptUnprotectData(&inBlob,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            CRYPTPROTECT_LOCAL_MACHINE,
                            &outBlob)) {
        if (error) {
            *error = QStringLiteral("dpapi_unprotect_failed_%1").arg(GetLastError());
        }
        return false;
    }

    plaintext = QByteArray(reinterpret_cast<const char *>(outBlob.pbData),
                           static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return true;
}

QString fixedClientVersion()
{
    return QStringLiteral("3.5.50");
}

QString signatureToHex(const ::NBAuth::FixedSig256 &signature)
{
    return QByteArray(reinterpret_cast<const char *>(signature.data()),
                      static_cast<int>(signature.size())).toHex();
}

LocalPaths buildDefaultPaths()
{
    QString appRoot =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (!appRoot.endsWith(QStringLiteral("/NoteBotInjector"), Qt::CaseInsensitive) &&
        !appRoot.endsWith(QStringLiteral("\\NoteBotInjector"), Qt::CaseInsensitive)) {
        const QString localAppData =
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        appRoot = QDir(localAppData).absoluteFilePath(QStringLiteral("NoteBotInjector"));
    }

    LocalPaths paths;
    paths.appRootDir = appRoot;
    paths.stateDir = QDir(appRoot).absoluteFilePath(QStringLiteral("state_v3"));
    paths.dllDir = QDir(appRoot).absoluteFilePath(QStringLiteral("dlls_v3"));
    paths.licensePath = QDir(paths.stateDir).absoluteFilePath(QStringLiteral("license_v3.dat"));
    paths.ticketPath =
        QDir(paths.stateDir).absoluteFilePath(QStringLiteral("inject_ticket_v3.dat"));
    paths.resultPath =
        QDir(paths.stateDir).absoluteFilePath(QStringLiteral("inject_result_v3.dat"));
    paths.consumedTicketsPath =
        QDir(paths.stateDir).absoluteFilePath(QStringLiteral("consumed_tickets_v3.json"));
    return paths;
}

bool unwrapProtectedLicenseJson(const QString &path, QJsonObject &out, QString *error)
{
    QFile file(path);
    if (!file.exists()) {
        if (error) {
            *error = QStringLiteral("file_missing");
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }

    const QByteArray raw = file.readAll();
    if (raw.size() < 8 || std::memcmp(raw.constData(), kLicenseMagic, 4) != 0) {
        if (error) {
            *error = QStringLiteral("bad_magic");
        }
        return false;
    }

    quint32 schemaVersion = 0;
    std::memcpy(&schemaVersion, raw.constData() + 4, sizeof(schemaVersion));
    if (schemaVersion != 3) {
        if (error) {
            *error = QStringLiteral("unsupported_schema");
        }
        return false;
    }

    QByteArray decrypted;
    if (!dpapiUnprotectMachine(raw.mid(8), decrypted, error)) {
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(decrypted, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QStringLiteral("json_parse_failed");
        }
        return false;
    }

    out = doc.object();
    return true;
}

bool saveProtectedLicenseJson(const QString &path, const QJsonObject &obj, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }

    const QByteArray logicalJson = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray protectedJson;
    if (!dpapiProtectMachine(logicalJson, protectedJson, error)) {
        file.cancelWriting();
        return false;
    }

    QByteArray payload;
    payload.append(kLicenseMagic, 4);
    quint32 schemaVersion = 3;
    payload.append(reinterpret_cast<const char *>(&schemaVersion), sizeof(schemaVersion));
    payload.append(protectedJson);

    if (file.write(payload) != payload.size()) {
        if (error) {
            *error = QStringLiteral("file_write_failed");
        }
        file.cancelWriting();
        return false;
    }

    if (!file.commit()) {
        if (error) {
            *error = QStringLiteral("file_commit_failed");
        }
        return false;
    }

    return true;
}

LicenseRecord licenseRecordFromJson(const QJsonObject &obj)
{
    LicenseRecord record;
    record.schemaVersion = obj.value(QStringLiteral("schema_version")).toInt(3);
    record.channel = obj.value(QStringLiteral("channel")).toString(QStringLiteral("stable"));
    record.keyPlaintext = obj.value(QStringLiteral("key_plaintext")).toString();
    record.keyId = obj.value(QStringLiteral("key_id")).toString();
    record.keyHash = obj.value(QStringLiteral("key_hash")).toString();
    record.deviceId = obj.value(QStringLiteral("device_id")).toString();
    record.devicePublicKeyPem = obj.value(QStringLiteral("device_public_key_pem")).toString();
    record.devicePrivateKeyDpapiBlob = QByteArray::fromBase64(
        obj.value(QStringLiteral("device_private_key_dpapi_blob_b64")).toString().toUtf8());
    record.serverPubkeyVersion = obj.value(QStringLiteral("server_pubkey_version")).toInt(0);
    record.serverPubkeyFingerprint =
        obj.value(QStringLiteral("server_pubkey_fingerprint")).toString();
    record.lastKnownTierName =
        obj.value(QStringLiteral("last_known_tier")).toString(QStringLiteral("None"));
    record.lastKnownTierValue = obj.value(QStringLiteral("last_known_tier_value")).toInt(0);
    record.lastKnownFeatureFlags =
        static_cast<quint32>(obj.value(QStringLiteral("last_known_feature_flags"))
                                 .toVariant()
                                 .toUInt());
    record.lastKnownDllName = obj.value(QStringLiteral("last_known_dll_name")).toString();
    record.lastKnownDllSha256 = obj.value(QStringLiteral("last_known_dll_sha256")).toString();
    record.lastVerifiedAtUtc = obj.value(QStringLiteral("last_verified_at_utc")).toString();
    record.onlineStateCache = obj.value(QStringLiteral("online_state_cache")).toObject();
    return record;
}

QJsonObject licenseRecordToJson(const LicenseRecord &record)
{
    QJsonObject obj;
    obj[QStringLiteral("schema_version")] = record.schemaVersion;
    obj[QStringLiteral("channel")] = record.channel;
    obj[QStringLiteral("key_plaintext")] = record.keyPlaintext;
    obj[QStringLiteral("key_id")] = record.keyId;
    obj[QStringLiteral("key_hash")] = record.keyHash;
    obj[QStringLiteral("device_id")] = record.deviceId;
    obj[QStringLiteral("device_public_key_pem")] = record.devicePublicKeyPem;
    obj[QStringLiteral("device_private_key_dpapi_blob_b64")] =
        QString::fromLatin1(record.devicePrivateKeyDpapiBlob.toBase64());
    obj[QStringLiteral("server_pubkey_version")] = record.serverPubkeyVersion;
    obj[QStringLiteral("server_pubkey_fingerprint")] = record.serverPubkeyFingerprint;
    obj[QStringLiteral("last_known_tier")] = record.lastKnownTierName;
    obj[QStringLiteral("last_known_tier_value")] = record.lastKnownTierValue;
    obj[QStringLiteral("last_known_feature_flags")] =
        static_cast<qint64>(record.lastKnownFeatureFlags);
    obj[QStringLiteral("last_known_dll_name")] = record.lastKnownDllName;
    obj[QStringLiteral("last_known_dll_sha256")] = record.lastKnownDllSha256;
    obj[QStringLiteral("last_verified_at_utc")] = record.lastVerifiedAtUtc;
    obj[QStringLiteral("online_state_cache")] = record.onlineStateCache;
    return obj;
}

int tierTextToCode(const QString &tier)
{
    const QString normalized = tier.trimmed().toLower();
    if (normalized == QStringLiteral("dev")) {
        return 3;
    }
    if (normalized == QStringLiteral("premium")) {
        return 2;
    }
    if (normalized == QStringLiteral("trial")) {
        return 1;
    }
    return 0;
}

} // namespace

StateManager::StateManager()
{
    m_paths = buildDefaultPaths();
    m_snapshot.protocolVersion = kProtocolVersion;
    m_snapshot.abiVersion = kAbiVersion;
    m_snapshot.localStateRoot = m_paths.stateDir;
    m_snapshot.authDllVersion = QString::fromLatin1(kAuthDllVersion);
    m_hostUpdateSnapshot = QJsonObject{
        {QStringLiteral("main_exe"), QJsonObject{
             {QStringLiteral("version"), QString::fromLatin1(kMainExeVersion)},
             {QStringLiteral("version_code"), kMainExeVersionCode},
             {QStringLiteral("required"), false},
             {QStringLiteral("pending_replace"), false},
             {QStringLiteral("status"), QStringLiteral("idle")},
         }},
        {QStringLiteral("auth_dll"), QJsonObject{
            {QStringLiteral("version"), QString::fromLatin1(kAuthDllVersion)},
            {QStringLiteral("version_code"), kAuthDllVersionCode},
             {QStringLiteral("required"), false},
             {QStringLiteral("pending_replace"), false},
             {QStringLiteral("status"), QStringLiteral("idle")},
         }},
        {QStringLiteral("updater_exe"), QJsonObject{
             {QStringLiteral("version"), QString::fromLatin1(kUpdaterExeVersion)},
             {QStringLiteral("version_code"), kUpdaterExeVersionCode},
             {QStringLiteral("required"), false},
             {QStringLiteral("pending_replace"), false},
             {QStringLiteral("status"), QStringLiteral("idle")},
         }},
        {QStringLiteral("update_state"), QStringLiteral("idle")},
    };
}

bool StateManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QString error;
    if (!ensureDirectoriesLocked(&error)) {
        m_snapshot.lastError = error;
        m_snapshot.updateState = QStringLiteral("error");
        setStatusLocked(QStringLiteral("本地状态错误"), false);
        return false;
    }

    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    m_licenseKey = Protected::NBVmp_V3_NormalizeKey(
        settings.value(QStringLiteral("licenseKey")).toString());

    loadNameConfigLocked();
    loadLicenseRecordLocked(nullptr);
    m_initialized = true;
    return true;
}

void StateManager::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
}

void StateManager::setLicenseKey(const QString &key)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_licenseKey = Protected::NBVmp_V3_NormalizeKey(key);
}

QString StateManager::licenseKey() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_licenseKey;
}

int StateManager::activatePlaceholder(const QString &key, QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const QString normalizedKey = Protected::NBVmp_V3_NormalizeKey(key);
    m_licenseKey = normalizedKey;
    if (normalizedKey.isEmpty()) {
        m_snapshot.lastError = QStringLiteral("invalid_argument_key_missing");
        setStatusLocked(QStringLiteral("未输入密钥"), false);
        if (message) {
            *message = QStringLiteral("未输入密钥");
        }
        return kRcInvalidArgument;
    }

    QString statusMessage;
    QString error;
    if (!createOrRefreshPendingLicenseLocked(normalizedKey, &statusMessage, &error)) {
        m_snapshot.lastError = error;
        setStatusLocked(QStringLiteral("本地设备身份生成失败"), false);
        if (message) {
            *message = QStringLiteral("本地设备身份生成失败");
        }
        return error.startsWith(QStringLiteral("dpapi_"))
            ? kRcDpapiDecryptFailed
            : kRcDeviceKeyMissing;
    }

    m_snapshot.lastError = QStringLiteral("network_unavailable_pending_activation");
    setStatusLocked(statusMessage, false);
    if (message) {
        *message = QStringLiteral("未激活");
    }
    return kRcNetworkUnavailable;
}

int StateManager::verifyLocalState(QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return verifyLocalStateLocked(message);
}

int StateManager::verifyLocalStateLocked(QString *message)
{
    QString error;
    if (!ensureDirectoriesLocked(&error)) {
        m_snapshot.lastError = error;
        m_snapshot.updateState = QStringLiteral("error");
        setStatusLocked(QStringLiteral("本地状态错误"), false);
        if (message) {
            *message = QStringLiteral("V3 状态目录初始化失败");
        }
        return kRcLocalStateCorrupt;
    }

    if (!QFile::exists(m_paths.licensePath)) {
        m_snapshot.hasLicenseFile = false;
        m_snapshot.lastError = QStringLiteral("license_v3_missing");
        clearDerivedStateLocked();
        setStatusLocked(QStringLiteral("未激活"), false);
        if (message) {
            *message = QStringLiteral("未发现 license_v3.dat");
        }
        return kRcLocalStateMissing;
    }

    if (!loadLicenseRecordLocked(&error)) {
        m_snapshot.lastError = error;
        clearDerivedStateLocked();
        m_snapshot.updateState = QStringLiteral("error");
        setStatusLocked(QStringLiteral("本地状态错误"), false);
        if (message) {
            *message = QStringLiteral("license_v3.dat 损坏");
        }
        return error.startsWith(QStringLiteral("dpapi_"))
            ? kRcDpapiDecryptFailed
            : kRcLocalStateCorrupt;
    }

    m_snapshot.lastError.clear();
    m_snapshot.updateState =
        m_hostUpdateSnapshot.value(QStringLiteral("update_state")).toString(QStringLiteral("idle"));
    if (hasVerifiedLicenseLocked()) {
        setStatusLocked(QStringLiteral("已激活"), true);
        if (message) {
            *message = QStringLiteral("已激活");
        }
    } else {
        setStatusLocked(QStringLiteral("未激活"), false);
        if (message) {
            *message = QStringLiteral("未激活");
        }
    }
    return kRcOk;
}

int StateManager::activateWithServer(const QString &key, QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const QString normalizedKey = Protected::NBVmp_V3_NormalizeKey(key);
    m_licenseKey = normalizedKey;
    if (normalizedKey.isEmpty()) {
        m_snapshot.lastError = QStringLiteral("invalid_argument_key_missing");
        setStatusLocked(QStringLiteral("未输入密钥"), false);
        if (message) {
            *message = QStringLiteral("未输入密钥");
        }
        return kRcInvalidArgument;
    }

    QString statusMessage;
    QString error;
    if (!createOrRefreshPendingLicenseLocked(normalizedKey, &statusMessage, &error)) {
        m_snapshot.lastError = error;
        setStatusLocked(QStringLiteral("本地设备身份生成失败"), false);
        if (message) {
            *message = QStringLiteral("本地设备身份生成失败");
        }
        return error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                          : kRcDeviceKeyMissing;
    }

    RpcActivateRequest request;
    request.keyPlaintext = normalizedKey;
    request.deviceId = m_licenseRecord.deviceId;
    request.devicePublicKeyPem = m_licenseRecord.devicePublicKeyPem;
    request.timestampUtc = nowUtcIso();
    request.nonceHex = randomNonceHex();
    request.clientVersion = fixedClientVersion();

    RpcActivateResponse response;
    if (!m_rpcClient.activateDevice(request, response, error)) {
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        setStatusLocked(QStringLiteral("联网失败，未完成激活"), false);
        if (message) {
            *message = QStringLiteral("无法连接服务器");
        }
        return kRcNetworkUnavailable;
    }

    const int statusRc = classifyServerStatusLocked(response.status);
    if (statusRc != kRcOk) {
        const QString failedStatus = statusRc == kRcDeviceReplaceCooldown
            ? QStringLiteral("设备切换过于频繁")
            : statusRc == kRcProtocolMismatch
                ? QStringLiteral("协议版本不匹配")
                : QStringLiteral("激活失败：密钥无效");
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = response.status;
        m_licenseRecord.onlineStateCache[QStringLiteral("online")] = false;
        m_licenseRecord.onlineStateCache[QStringLiteral("kicked")] = false;
        m_licenseRecord.onlineStateCache[QStringLiteral("last_server_message")] =
            QStringLiteral("activation_rejected");
        saveLicenseRecordLocked(m_licenseRecord, nullptr);
        setStatusLocked(failedStatus, false);
        if (message) {
            *message = failedStatus;
        }
        return statusRc;
    }

    m_licenseRecord.keyId = response.keyId;
    m_licenseRecord.keyHash = response.keyHash;
    m_licenseRecord.deviceId = response.deviceId.isEmpty() ? m_licenseRecord.deviceId
                                                           : response.deviceId;
    m_licenseRecord.serverPubkeyVersion =
        response.serverPubkeyVersion > 0
            ? response.serverPubkeyVersion
            : formalServerPublicKeyVersion();
    m_licenseRecord.serverPubkeyFingerprint =
        response.serverPubkeyFingerprint.trimmed().isEmpty()
            ? formalServerPublicKeyFingerprint()
            : response.serverPubkeyFingerprint.trimmed().toLower();
    m_licenseRecord.lastKnownTierName = response.tier.isEmpty()
        ? tierNameFromValue(response.tierValue)
        : response.tier;
    m_licenseRecord.lastKnownTierValue = response.tierValue;
    m_licenseRecord.lastKnownFeatureFlags = response.featureFlags;
    m_licenseRecord.lastKnownDllName = response.boundDllName;
    m_licenseRecord.lastKnownDllSha256 = response.boundDllSha256;
    m_licenseRecord.lastVerifiedAtUtc = request.timestampUtc;
    m_licenseRecord.onlineStateCache = QJsonObject{
        {QStringLiteral("online"), true},
        {QStringLiteral("kicked"), false},
        {QStringLiteral("last_heartbeat_at_utc"), request.timestampUtc},
        {QStringLiteral("last_server_message"), QStringLiteral("ok")},
        {QStringLiteral("replace_cooldown_remaining_sec"), 0},
    };

    if (!saveLicenseRecordLocked(m_licenseRecord, &error)) {
        m_snapshot.lastError = error;
        setStatusLocked(QStringLiteral("写入 license_v3.dat 失败"), false);
        if (message) {
            *message = QStringLiteral("写入 license_v3.dat 失败");
        }
        return error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                          : kRcLocalStateCorrupt;
    }

    applyLicenseRecordLocked(m_licenseRecord);
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    setStatusLocked(QStringLiteral("已激活"), true);
    m_heartbeatFailureCount = 0;
    startHeartbeatLocked();
    if (message) {
        *message = QStringLiteral("激活成功");
    }
    return kRcOk;
}

int StateManager::checkWithServer(QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QString localMessage;
    const int localRc = verifyLocalStateLocked(&localMessage);
    if (localRc != kRcOk) {
        if (message) {
            *message = localMessage;
        }
        return localRc;
    }
    if (!hasVerifiedLicenseLocked()) {
        if (message) {
            *message = QStringLiteral("当前设备尚未完成联网激活");
        }
        return kRcServerRejected;
    }

    QString payloadJson;
    QString signatureHex;
    QString nonceHex;
    QString timestampUtc;
    QString error;
    if (!makeSignedPayloadLocked(QStringLiteral("device_check_v3"),
                                 payloadJson,
                                 signatureHex,
                                 nonceHex,
                                 timestampUtc,
                                 &error)) {
        m_snapshot.lastError = error;
        if (message) {
            *message = QStringLiteral("设备签名失败");
        }
        return error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                          : kRcDeviceKeyMissing;
    }

    RpcCheckRequest request;
    request.keyId = m_licenseRecord.keyId;
    request.deviceId = m_licenseRecord.deviceId;
    request.timestampUtc = timestampUtc;
    request.nonceHex = nonceHex;
    request.clientVersion = fixedClientVersion();
    request.payloadJson = payloadJson;
    request.deviceSignatureHex = signatureHex;

    RpcCheckResponse response;
    if (!m_rpcClient.deviceCheck(request, response, error)) {
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        setStatusLocked(QStringLiteral("当前离线，显示缓存状态"), m_snapshot.active);
        if (message) {
            *message = QStringLiteral("无法连接服务器");
        }
        return kRcNetworkUnavailable;
    }

    const int statusRc = classifyServerStatusLocked(response.status, response.kicked);
    if (statusRc == kRcDeviceKicked) {
        m_snapshot.networkAvailable = true;
        m_snapshot.kicked = true;
        m_licenseRecord.onlineStateCache[QStringLiteral("online")] = false;
        m_licenseRecord.onlineStateCache[QStringLiteral("kicked")] = true;
        m_licenseRecord.onlineStateCache[QStringLiteral("last_server_message")] = QStringLiteral("kicked");
        saveLicenseRecordLocked(m_licenseRecord, nullptr);
        applyLicenseRecordLocked(m_licenseRecord);
        setStatusLocked(QStringLiteral("已被其他设备顶下线"), true);
        stopHeartbeatLocked();
        if (message) {
            *message = QStringLiteral("已被其他设备顶下线");
        }
        return statusRc;
    }
    if (statusRc == kRcDeviceReplaceCooldown || statusRc == kRcServerRejected) {
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = response.status;
        setStatusLocked(statusRc == kRcDeviceReplaceCooldown
                            ? QStringLiteral("设备切换过于频繁")
                            : QStringLiteral("已激活"),
                        true);
        if (message) {
            *message = statusRc == kRcDeviceReplaceCooldown
                ? QStringLiteral("设备切换过于频繁")
                : QStringLiteral("服务端拒绝检查");
        }
        return statusRc;
    }

    m_licenseRecord.lastKnownTierName = response.tier.isEmpty()
        ? tierNameFromValue(response.tierValue)
        : response.tier;
    m_licenseRecord.lastKnownTierValue = response.tierValue;
    m_licenseRecord.lastKnownFeatureFlags = response.featureFlags;
    m_licenseRecord.lastKnownDllName = response.boundDllName;
    m_licenseRecord.lastKnownDllSha256 = response.boundDllSha256;
    m_licenseRecord.lastVerifiedAtUtc = timestampUtc;
    m_licenseRecord.onlineStateCache = QJsonObject{
        {QStringLiteral("online"), response.online},
        {QStringLiteral("kicked"), response.kicked},
        {QStringLiteral("last_heartbeat_at_utc"),
         m_licenseRecord.onlineStateCache.value(QStringLiteral("last_heartbeat_at_utc")).toString()},
        {QStringLiteral("last_server_message"), QStringLiteral("ok")},
        {QStringLiteral("replace_cooldown_remaining_sec"), response.replaceCooldownRemainingSec},
    };

    saveLicenseRecordLocked(m_licenseRecord, nullptr);
    applyLicenseRecordLocked(m_licenseRecord);
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    setStatusLocked(QStringLiteral("已激活"), true);
    m_heartbeatFailureCount = 0;
    startHeartbeatLocked();
    if (message) {
        *message = QStringLiteral("设备检查成功");
    }
    return kRcOk;
}

int StateManager::heartbeatOnce(QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_snapshot.kicked) {
        stopHeartbeatLocked();
        if (message) {
            *message = QStringLiteral("当前设备已被顶下线");
        }
        return kRcDeviceKicked;
    }
    if (!hasVerifiedLicenseLocked()) {
        if (message) {
            *message = QStringLiteral("当前设备尚未完成联网激活");
        }
        return kRcServerRejected;
    }

    QString payloadJson;
    QString signatureHex;
    QString nonceHex;
    QString timestampUtc;
    QString error;
    if (!makeSignedPayloadLocked(QStringLiteral("device_heartbeat_v3"),
                                 payloadJson,
                                 signatureHex,
                                 nonceHex,
                                 timestampUtc,
                                 &error)) {
        m_snapshot.lastError = error;
        if (message) {
            *message = QStringLiteral("心跳签名失败");
        }
        return error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                          : kRcDeviceKeyMissing;
    }

    RpcHeartbeatRequest request;
    request.keyId = m_licenseRecord.keyId;
    request.deviceId = m_licenseRecord.deviceId;
    request.timestampUtc = timestampUtc;
    request.nonceHex = nonceHex;
    request.deviceSignatureHex = signatureHex;

    RpcHeartbeatResponse response;
    if (!m_rpcClient.deviceHeartbeat(request, response, error)) {
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        m_heartbeatFailureCount++;
        if (m_heartbeatFailureCount >= 3) {
            setStatusLocked(QStringLiteral("当前离线，显示缓存状态"), m_snapshot.active);
        }
        if (message) {
            *message = QStringLiteral("心跳失败");
        }
        return kRcHeartbeatFailed;
    }

    const int statusRc = classifyServerStatusLocked(response.status, response.kicked);
    if (statusRc == kRcDeviceKicked) {
        m_snapshot.networkAvailable = true;
        m_licenseRecord.onlineStateCache[QStringLiteral("online")] = false;
        m_licenseRecord.onlineStateCache[QStringLiteral("kicked")] = true;
        m_licenseRecord.onlineStateCache[QStringLiteral("last_server_message")] = QStringLiteral("kicked");
        saveLicenseRecordLocked(m_licenseRecord, nullptr);
        applyLicenseRecordLocked(m_licenseRecord);
        setStatusLocked(QStringLiteral("已被其他设备顶下线"), true);
        stopHeartbeatLocked();
        if (message) {
            *message = QStringLiteral("已被其他设备顶下线");
        }
        return statusRc;
    }

    m_heartbeatFailureCount = 0;
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    m_licenseRecord.onlineStateCache[QStringLiteral("online")] = response.online;
    m_licenseRecord.onlineStateCache[QStringLiteral("kicked")] = response.kicked;
    m_licenseRecord.onlineStateCache[QStringLiteral("last_heartbeat_at_utc")] =
        response.serverTimeUtc.isEmpty() ? timestampUtc : response.serverTimeUtc;
    m_licenseRecord.onlineStateCache[QStringLiteral("last_server_message")] = QStringLiteral("ok");
    saveLicenseRecordLocked(m_licenseRecord, nullptr);
    applyLicenseRecordLocked(m_licenseRecord);
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    setStatusLocked(QStringLiteral("已激活"), true);
    if (message) {
        *message = QStringLiteral("心跳成功");
    }
    return kRcOk;
}

void StateManager::resetRuntimeState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.remove(QStringLiteral("licenseKey"));
    m_licenseKey.clear();
    QFile::remove(m_paths.licensePath);
    QFile::remove(m_paths.ticketPath);
    QFile::remove(m_paths.resultPath);
    QFile::remove(m_paths.consumedTicketsPath);
    ensureConsumedTicketCacheLocked(nullptr);
    clearPendingTicketLocked();
    clearDerivedStateLocked();
    m_snapshot.lastError.clear();
    setStatusLocked(QStringLiteral("未激活"), false);
    stopHeartbeatLocked();
}

void StateManager::bindHeartbeatTimer(QTimer *timer)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_heartbeatTimer = timer;
    if (m_heartbeatTimer && hasVerifiedLicenseLocked() && !m_snapshot.kicked) {
        startHeartbeatLocked();
    }
}

bool StateManager::isActive() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot.active;
}

QString StateManager::statusText() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot.licenseStatus;
}

QString StateManager::tierText() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot.tierName;
}

int StateManager::tierCode() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot.tierValue;
}

unsigned int StateManager::featureFlagsMask() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot.featureFlags;
}

unsigned int StateManager::expiresEpoch() const
{
    return 0;
}

int StateManager::protocolVersion() const
{
    return kProtocolVersion;
}

int StateManager::abiVersion() const
{
    return kAbiVersion;
}

LocalPaths StateManager::paths() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_paths;
}

StatusSnapshot StateManager::snapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_snapshot;
}

NameConfig StateManager::nameConfig() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nameConfig;
}

int StateManager::issueInjectTicket(const QJsonObject &request,
                                    QJsonObject &data,
                                    QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = kRcOk;
    if (issueInjectTicketLocked(request, data, rc, message)) {
        return rc;
    }
    return rc;
}

int StateManager::getDllPolicy(QJsonObject &data,
                               QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = kRcOk;
    if (getDllPolicyLocked(data, rc, message)) {
        return rc;
    }
    return rc;
}

int StateManager::getModelEntitlements(QJsonObject &data,
                                       QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!hasVerifiedLicenseLocked()) {
        if (message) {
            *message = QStringLiteral("当前设备尚未完成联网激活");
        }
        return kRcServerRejected;
    }

    QString payloadJson;
    QString signatureHex;
    QString nonceHex;
    QString timestampUtc;
    QString error;
    if (!makeSignedPayloadLocked(QStringLiteral("model_entitlements_v1"),
                                 payloadJson,
                                 signatureHex,
                                 nonceHex,
                                 timestampUtc,
                                 &error)) {
        m_snapshot.lastError = error;
        if (message) {
            *message = QStringLiteral("设备签名失败");
        }
        return error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                          : kRcDeviceKeyMissing;
    }

    RpcModelEntitlementsRequest request;
    request.keyId = m_licenseRecord.keyId;
    request.deviceId = m_licenseRecord.deviceId;
    request.timestampUtc = timestampUtc;
    request.nonceHex = nonceHex;
    request.deviceSignatureHex = signatureHex;

    RpcModelEntitlementsResponse response;
    if (!m_rpcClient.modelEntitlements(request, response, error)) {
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        if (message) {
            *message = QStringLiteral("无法连接服务器");
        }
        return kRcNetworkUnavailable;
    }

    const int statusRc = classifyServerStatusLocked(response.status);
    if (statusRc != kRcOk) {
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = response.status;
        if (message) {
            *message = statusRc == kRcDeviceKicked
                ? QStringLiteral("已被其他设备顶下线")
                : QStringLiteral("模型授权请求被拒绝");
        }
        return statusRc == kRcOk ? kRcServerRejected : statusRc;
    }

    data[QStringLiteral("models")] = response.models;
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    if (message) {
        *message = QStringLiteral("模型授权获取成功");
    }
    return kRcOk;
}

int StateManager::downloadOverlayDll(const QJsonObject &request,
                                     QJsonObject &data,
                                     QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (downloadOverlayDllLocked(request, data, message)) {
        return kRcOk;
    }
    return kRcDllDownloadFailed;
}

int StateManager::consumeInjectResult(QJsonObject &data,
                                      QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = kRcOk;
    if (consumeInjectResultLocked(data, rc, message)) {
        return rc;
    }
    return rc;
}

int StateManager::reportInjectResult(QJsonObject &data,
                                     QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = kRcOk;
    if (reportInjectResultLocked(data, rc, message)) {
        return rc;
    }
    return rc;
}

void StateManager::setHostUpdateSnapshot(const QJsonObject &snapshot)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hostUpdateSnapshot = snapshot;
    const QString updateState =
        snapshot.value(QStringLiteral("update_state")).toString().trimmed();
    if (!updateState.isEmpty()) {
        m_snapshot.updateState = updateState;
    }
}

QJsonObject StateManager::hostUpdateSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hostUpdateSnapshot;
}

bool StateManager::saveNameConfig(const QString &value, bool enabled, QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    m_nameConfig.value = value.trimmed();
    m_nameConfig.enabled = enabled;
    settings.setValue(QStringLiteral("customName"), m_nameConfig.value);
    settings.setValue(QStringLiteral("customNameEnabled"), m_nameConfig.enabled);
    if (message) {
        *message = QStringLiteral("名称配置已保存");
    }
    return true;
}

bool StateManager::setNameEnabled(bool enabled, QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    m_nameConfig.enabled = enabled;
    settings.setValue(QStringLiteral("customNameEnabled"), m_nameConfig.enabled);
    if (message) {
        *message = enabled ? QStringLiteral("自定义名称已启用")
                           : QStringLiteral("自定义名称已关闭");
    }
    return true;
}

bool StateManager::issueInjectTicketLocked(const QJsonObject &request,
                                           QJsonObject &data,
                                           int &rc,
                                           QString *message)
{
    if (!hasVerifiedLicenseLocked()) {
        rc = kRcServerRejected;
        if (message) {
            *message = QStringLiteral("当前设备尚未完成联网激活");
        }
        return false;
    }

    const quint32 targetPid =
        request.value(QStringLiteral("target_pid")).toVariant().toUInt();
    if (targetPid == 0) {
        rc = kRcInvalidArgument;
        if (message) {
            *message = QStringLiteral("缺少 target_pid");
        }
        return false;
    }

    const QString dllName = request.value(QStringLiteral("dll_name"))
                                .toString(m_licenseRecord.lastKnownDllName)
                                .trimmed();
    const QString dllSha256 = request.value(QStringLiteral("dll_sha256"))
                                  .toString(m_licenseRecord.lastKnownDllSha256)
                                  .trimmed()
                                  .toLower();
    if (dllName.isEmpty() || dllSha256.isEmpty()) {
        rc = kRcDllPolicyInvalid;
        if (message) {
            *message = QStringLiteral("缺少 DLL 策略缓存");
        }
        return false;
    }

    QString sessionId = request.value(QStringLiteral("session_id")).toString().trimmed();
    QString ticketId = request.value(QStringLiteral("ticket_id")).toString().trimmed();
    QString serverTicketPayload =
        request.value(QStringLiteral("server_ticket_payload")).toString().trimmed();
    QString serverTicketSignature =
        request.value(QStringLiteral("server_ticket_signature")).toString().trimmed().toLower();

    if (serverTicketPayload.isEmpty() || serverTicketSignature.isEmpty()) {
        QString signatureHex;
        QString nonceHex;
        QString timestampUtc;
        QString error;
        timestampUtc = nowUtcIso();
        nonceHex = randomNonceHex();
        if (nonceHex.isEmpty()) {
            rc = kRcTicketRequestFailed;
            if (message) {
                *message = QStringLiteral("设备签名失败");
            }
            return false;
        }
        const QString payloadJson = canonicalJsonString(
            {
                {QStringLiteral("cmd"), QStringLiteral("issue_inject_ticket_v3")},
                {QStringLiteral("key_id"), m_licenseRecord.keyId},
                {QStringLiteral("device_id"), m_licenseRecord.deviceId},
                {QStringLiteral("dll_name"), dllName},
                {QStringLiteral("dll_sha256"), dllSha256},
                {QStringLiteral("timestamp_utc"), timestampUtc},
                {QStringLiteral("nonce"), nonceHex},
            },
            {
                {QStringLiteral("target_pid"), static_cast<qint64>(targetPid)},
            });
        if (!signCanonicalPayloadLocked(payloadJson, signatureHex, &error)) {
            rc = error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                            : kRcDeviceKeyMissing;
            if (message) {
                *message = QStringLiteral("设备签名失败");
            }
            return false;
        }

        RpcIssueTicketRequest rpcReq;
        rpcReq.keyId = m_licenseRecord.keyId;
        rpcReq.deviceId = m_licenseRecord.deviceId;
        rpcReq.dllName = dllName;
        rpcReq.dllSha256 = dllSha256;
        rpcReq.targetPid = targetPid;
        rpcReq.timestampUtc = timestampUtc;
        rpcReq.nonceHex = nonceHex;
        rpcReq.deviceSignatureHex = signatureHex;

        RpcIssueTicketResponse rpcResp;
        if (!m_rpcClient.issueInjectTicket(rpcReq, rpcResp, error)) {
            rc = kRcTicketRequestFailed;
            if (message) {
                *message = QStringLiteral("票据请求失败");
            }
            m_snapshot.networkAvailable = false;
            m_snapshot.lastError = error;
            return false;
        }

        const int statusRc = classifyServerStatusLocked(rpcResp.status);
        if (statusRc != kRcOk) {
            rc = statusRc == kRcOk ? kRcTicketRequestFailed : statusRc;
            if (message) {
                *message = QStringLiteral("票据请求失败");
            }
            m_snapshot.networkAvailable = true;
            m_snapshot.lastError = rpcResp.status;
            return false;
        }

        serverTicketPayload = rpcResp.serverTicketPayload;
        serverTicketSignature = rpcResp.serverTicketSignature;
        sessionId = rpcResp.sessionId;
        ticketId = rpcResp.ticketId;
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError.clear();
        if (serverTicketPayload.isEmpty() || serverTicketSignature.isEmpty()) {
            rc = kRcTicketRequestFailed;
            if (message) {
                *message = QStringLiteral("缺少服务端票据");
            }
            return false;
        }
    }

    const QString ticketSha256 =
        Protected::NBVmp_Ticket_ComputeTicketSha256(serverTicketPayload);
    const QByteArray wrapperKey =
        Protected::NBVmp_Ticket_DeriveWrapperKey(sessionId, ticketSha256);
    if (wrapperKey.size() != static_cast<int>(::NBAuth::AES_GCM_KEY_SIZE)) {
        rc = kRcTicketWriteFailed;
        if (message) {
            *message = QStringLiteral("票据包装密钥派生失败");
        }
        return false;
    }

    const QString exeVersion =
        request.value(QStringLiteral("exe_version")).toString(QStringLiteral("3.5.98"));
    const quint64 receivedTickMs = static_cast<quint64>(GetTickCount64());
    const QString wrapperJson = canonicalWrapperJson(
        nowUtcIso(),
        receivedTickMs,
        targetPid,
        exeVersion,
        kProtocolVersion,
        ticketSha256,
        serverTicketPayload,
        serverTicketSignature);

    uint8_t iv[::NBAuth::AES_GCM_IV_SIZE]{};
    uint8_t tag[::NBAuth::AES_GCM_TAG_SIZE]{};
    if (!::NBAuth::SecureRandomBytes(iv, sizeof(iv))) {
        rc = kRcTicketWriteFailed;
        if (message) {
            *message = QStringLiteral("票据随机数生成失败");
        }
        return false;
    }

    ::NBAuth::ByteVector ciphertext;
    const QByteArray wrapperBytes = wrapperJson.toUtf8();
    if (!::NBAuth::AesGcmEncrypt(
            reinterpret_cast<const uint8_t *>(wrapperKey.constData()),
            iv,
            reinterpret_cast<const uint8_t *>(wrapperBytes.constData()),
            static_cast<size_t>(wrapperBytes.size()),
            nullptr,
            0,
            ciphertext,
            tag)) {
        rc = kRcTicketWriteFailed;
        if (message) {
            *message = QStringLiteral("票据包装失败");
        }
        return false;
    }

    const QByteArray sessionIdBytes = sessionId.toUtf8();
    const QByteArray ticketShaBytes = ticketSha256.toUtf8();

    QByteArray fileBytes;
    fileBytes.append(kTicketFileMagic, 16);
    appendBe32(fileBytes, 3);
    appendBe32(fileBytes, static_cast<quint32>(sessionIdBytes.size()));
    fileBytes.append(sessionIdBytes);
    appendBe32(fileBytes, static_cast<quint32>(ticketShaBytes.size()));
    fileBytes.append(ticketShaBytes);
    fileBytes.append(reinterpret_cast<const char *>(iv), sizeof(iv));
    fileBytes.append(reinterpret_cast<const char *>(tag), sizeof(tag));
    appendBe32(fileBytes, static_cast<quint32>(ciphertext.size()));
    fileBytes.append(reinterpret_cast<const char *>(ciphertext.data()),
                     static_cast<int>(ciphertext.size()));

    QString error;
    if (!saveBinaryFile(m_paths.ticketPath, fileBytes, &error)) {
        rc = kRcTicketWriteFailed;
        if (message) {
            *message = QStringLiteral("写入本地票据文件失败");
        }
        return false;
    }

    m_pendingTicket.valid = true;
    m_pendingTicket.sessionId = sessionId;
    m_pendingTicket.ticketId = ticketId;
    m_pendingTicket.ticketSha256 = ticketSha256;
    m_pendingTicket.serverTicketPayload = serverTicketPayload;
    m_pendingTicket.serverTicketSignature = serverTicketSignature;
    m_pendingTicket.dllName = dllName;
    m_pendingTicket.dllSha256 = dllSha256;
    m_pendingTicket.createdAtUtc = nowUtcIso();
    m_pendingTicket.targetPid = targetPid;
    m_pendingTicket.issuedTickMs = static_cast<quint64>(GetTickCount64());
    m_pendingTicket.wrapperKey = wrapperKey;
    m_pendingReport = PendingReportContext{};

    data[QStringLiteral("session_id")] = sessionId;
    data[QStringLiteral("ticket_id")] = ticketId;
    data[QStringLiteral("ticket_sha256")] = ticketSha256;
    data[QStringLiteral("ticket_path")] = m_paths.ticketPath;
    rc = kRcOk;
    if (message) {
        *message = QStringLiteral("已写入本地注入票据");
    }
    return true;
}

bool StateManager::downloadOverlayDllLocked(const QJsonObject &request,
                                            QJsonObject &data,
                                            QString *message)
{
    Q_UNUSED(request);
    if (!ensureDirectoriesLocked(message)) {
        return false;
    }

    QJsonObject policyData;
    int policyRc = kRcOk;
    QString policyMessage;
    if (!getDllPolicyLocked(policyData, policyRc, &policyMessage)) {
        if (message) {
            *message = policyMessage.isEmpty() ? QStringLiteral("当前 DLL 策略无效")
                                               : policyMessage;
        }
        return false;
    }

    const QString dllName =
        safeArtifactFileName(policyData.value(QStringLiteral("dll_name")).toString().trimmed());
    const QString expectedSha256 =
        policyData.value(QStringLiteral("dll_sha256")).toString().trimmed().toLower();
    const QString expectedMd5 =
        policyData.value(QStringLiteral("dll_md5")).toString().trimmed().toLower();
    const qint64 expectedSize =
        policyData.value(QStringLiteral("dll_size")).toVariant().toLongLong();
    const QUrl downloadUrl(
        policyData.value(QStringLiteral("download_url")).toString().trimmed());
    if (dllName.isEmpty() || expectedSha256.isEmpty() || expectedSize <= 0 || !downloadUrl.isValid()) {
        if (message) {
            *message = QStringLiteral("当前 DLL 下载元数据无效");
        }
        return false;
    }

    const QString targetPath = QDir(m_paths.dllDir).absoluteFilePath(dllName);

    bool needDownload = !QFile::exists(targetPath);
    if (!needDownload) {
        const QString targetSha256 = computeFileSha256Path(targetPath);
        const qint64 targetSize = fileSizePath(targetPath);
        needDownload = targetSha256.isEmpty() ||
                       targetSha256 != expectedSha256 ||
                       targetSize != expectedSize;
    }

    if (needDownload) {
        const HttpDownloadResult http = httpGetNoProxy(downloadUrl, expectedSize, 30000);
        if (http.statusCode < 200 || http.statusCode >= 300) {
            if (message) {
                *message = QStringLiteral("业务 DLL 下载失败");
            }
            return false;
        }
        if (http.bytes.size() != expectedSize) {
            if (message) {
                *message = QStringLiteral("业务 DLL 下载大小不匹配");
            }
            return false;
        }
        const QString actualSha256 = hashBytesHex(http.bytes, QCryptographicHash::Sha256);
        if (actualSha256 != expectedSha256) {
            if (message) {
                *message = QStringLiteral("业务 DLL sha256 校验失败");
            }
            return false;
        }
        if (!expectedMd5.isEmpty()) {
            const QString actualMd5 = hashBytesHex(http.bytes, QCryptographicHash::Md5);
            if (actualMd5 != expectedMd5) {
                if (message) {
                    *message = QStringLiteral("业务 DLL md5 校验失败");
                }
                return false;
            }
        }
        if (!saveBinaryFile(targetPath, http.bytes, message)) {
            if (message && message->isEmpty()) {
                *message = QStringLiteral("写入业务 DLL 失败");
            }
            return false;
        }
    }

    const QString sha256 = computeFileSha256Path(targetPath);
    const qint64 size = fileSizePath(targetPath);
    if (sha256.isEmpty() || size <= 0 || sha256 != expectedSha256 || size != expectedSize) {
        if (message) {
            *message = QStringLiteral("业务 DLL 校验信息获取失败");
        }
        return false;
    }

    m_licenseRecord.lastKnownDllName = dllName;
    m_licenseRecord.lastKnownDllSha256 = sha256;
    if (m_licenseRecord.schemaVersion == 3 && !m_licenseRecord.deviceId.isEmpty()) {
        saveLicenseRecordLocked(m_licenseRecord, nullptr);
        applyLicenseRecordLocked(m_licenseRecord);
    } else {
        m_snapshot.boundDllName = dllName;
        m_snapshot.boundDllSha256 = sha256;
    }

    data[QStringLiteral("dll_name")] = dllName;
    data[QStringLiteral("dll_sha256")] = sha256;
    data[QStringLiteral("dll_size")] = size;
    data[QStringLiteral("download_url")] = downloadUrl.toString();
    data[QStringLiteral("local_path")] = targetPath;
    if (message) {
        *message = QStringLiteral("已准备业务 DLL");
    }
    return true;
}

bool StateManager::consumeInjectResultLocked(QJsonObject &data,
                                             int &rc,
                                             QString *message)
{
    if (!m_pendingTicket.valid) {
        rc = kRcResultInvalid;
        if (message) {
            *message = QStringLiteral("当前没有待消费的注入结果");
        }
        return false;
    }

    if (!QFile::exists(m_paths.resultPath)) {
        rc = kRcResultTimeout;
        if (message) {
            *message = QStringLiteral("未发现注入结果文件");
        }
        return false;
    }

    QString error;
    QJsonObject obj;
    if (!parseObjectFile(m_paths.resultPath, obj, &error)) {
        rc = kRcResultInvalid;
        if (message) {
            *message = QStringLiteral("结果文件损坏");
        }
        return false;
    }

    const QString magic = obj.value(QStringLiteral("magic")).toString();
    const int version = obj.value(QStringLiteral("version")).toInt(0);
    const QString sessionId = obj.value(QStringLiteral("session_id")).toString().trimmed();
    const QString ticketId = obj.value(QStringLiteral("ticket_id")).toString().trimmed();
    const QString ticketSha256 =
        obj.value(QStringLiteral("ticket_sha256")).toString().trimmed().toLower();
    const QString status = obj.value(QStringLiteral("status")).toString().trimmed();
    const QString reason = obj.value(QStringLiteral("reason")).toString();
    const QString dllVersion = obj.value(QStringLiteral("dll_version")).toString();
    const qint64 processedTickMs =
        obj.value(QStringLiteral("processed_tick_ms")).toVariant().toLongLong();
    const QString grantedTier = obj.value(QStringLiteral("granted_tier")).toString().trimmed();
    const qint64 grantedFeatureFlags =
        obj.value(QStringLiteral("granted_feature_flags")).toVariant().toLongLong();
    const QString verifiedDllSha256 =
        obj.value(QStringLiteral("verified_dll_sha256")).toString().trimmed().toLower();
    const QString issuedAtServer =
        obj.value(QStringLiteral("issued_at_server")).toString().trimmed();
    const QString expiresAtServer =
        obj.value(QStringLiteral("expires_at_server")).toString().trimmed();
    const int serverPubkeyVersion =
        obj.value(QStringLiteral("server_pubkey_version")).toInt(0);
    const QString serverPubkeyFingerprint =
        obj.value(QStringLiteral("server_pubkey_fingerprint")).toString().trimmed();
    const QString resultHmac =
        obj.value(QStringLiteral("result_hmac")).toString().trimmed().toLower();

    if (magic != QString::fromLatin1(kResultMagic) ||
        version != 3 ||
        sessionId != m_pendingTicket.sessionId ||
        ticketId != m_pendingTicket.ticketId ||
        ticketSha256 != m_pendingTicket.ticketSha256 ||
        resultHmac.isEmpty()) {
        rc = kRcResultInvalid;
        if (message) {
            *message = QStringLiteral("结果文件无效");
        }
        return false;
    }

    const QString canonical =
        canonicalResultJsonForHmac(sessionId,
                                   ticketId,
                                   ticketSha256,
                                   status,
                                   reason,
                                   dllVersion,
                                   processedTickMs,
                                   grantedTier,
                                   grantedFeatureFlags,
                                   verifiedDllSha256,
                                   issuedAtServer,
                                   expiresAtServer,
                                   serverPubkeyVersion,
                                   serverPubkeyFingerprint);
    const QString expectedHmac =
        hmacHex(m_pendingTicket.wrapperKey, canonical.toUtf8()).toLower();
    if (!Protected::NBVmp_Verify_ResultHmacEquals(expectedHmac, resultHmac)) {
        rc = kRcResultHmacFailed;
        if (message) {
            *message = QStringLiteral("结果文件 HMAC 校验失败");
        }
        return false;
    }

    QString replayError;
    if (!replayCacheContainsLocked(ticketId, ticketSha256, &replayError)) {
        if (!appendReplayCacheLocked(ticketId, ticketSha256, nowUtcIso(), &replayError)) {
            rc = kRcResultInvalid;
            if (message) {
                *message = QStringLiteral("结果文件已读取，但本地反重放写入失败");
            }
            return false;
        }
    }

    QFile::remove(m_paths.resultPath);
    QFile::remove(m_paths.ticketPath);

    data[QStringLiteral("session_id")] = sessionId;
    data[QStringLiteral("ticket_id")] = ticketId;
    data[QStringLiteral("status")] = status;
    data[QStringLiteral("reason")] = reason;
    data[QStringLiteral("dll_version")] = dllVersion;
    data[QStringLiteral("processed_tick_ms")] = processedTickMs;
    data[QStringLiteral("granted_tier")] = grantedTier;
    data[QStringLiteral("granted_feature_flags")] = grantedFeatureFlags;
    data[QStringLiteral("verified_dll_sha256")] = verifiedDllSha256;
    data[QStringLiteral("issued_at_server")] = issuedAtServer;
    data[QStringLiteral("expires_at_server")] = expiresAtServer;
    data[QStringLiteral("server_pubkey_version")] = serverPubkeyVersion;
    data[QStringLiteral("server_pubkey_fingerprint")] = serverPubkeyFingerprint;
    data[QStringLiteral("verified_at_utc")] = nowUtcIso();
    m_pendingReport.valid = true;
    m_pendingReport.sessionId = sessionId;
    m_pendingReport.ticketId = ticketId;
    m_pendingReport.ticketSha256 = ticketSha256;
    m_pendingReport.status = status;
    m_pendingReport.reason = reason;
    m_pendingReport.dllVersion = dllVersion;
    m_pendingReport.processedTickMs = processedTickMs;
    m_pendingReport.grantedTier = grantedTier;
    m_pendingReport.grantedFeatureFlags = grantedFeatureFlags;
    m_pendingReport.verifiedDllSha256 = verifiedDllSha256;
    m_pendingReport.issuedAtServer = issuedAtServer;
    m_pendingReport.expiresAtServer = expiresAtServer;
    m_pendingReport.serverPubkeyVersion = serverPubkeyVersion;
    m_pendingReport.serverPubkeyFingerprint = serverPubkeyFingerprint;
    m_pendingReport.verifiedAtUtc = data.value(QStringLiteral("verified_at_utc")).toString();
    clearPendingTicketLocked();

    rc = kRcOk;
    if (message) {
        *message = QStringLiteral("已消费注入结果");
    }
    return true;
}

int StateManager::finalizePendingInjectFailure(const QString &status,
                                               const QString &reason,
                                               QJsonObject &data,
                                               QString *message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = kRcInjectFailed;
    finalizePendingInjectFailureLocked(status, reason, data, rc, message);
    return rc;
}

bool StateManager::finalizePendingInjectFailureLocked(const QString &status,
                                                      const QString &reason,
                                                      QJsonObject &data,
                                                      int &rc,
                                                      QString *message)
{
    if (!m_pendingTicket.valid) {
        rc = kRcResultInvalid;
        if (message) {
            *message = QStringLiteral("当前没有待处理的注入票据");
        }
        return false;
    }

    const QString normalizedStatus = status.trimmed().isEmpty()
        ? QStringLiteral("inject_failed")
        : status.trimmed();
    const QString normalizedReason = reason.trimmed().isEmpty()
        ? QStringLiteral("inject_failed:no_result_file")
        : reason.trimmed();
    QString replayError;
    if (!replayCacheContainsLocked(m_pendingTicket.ticketId, m_pendingTicket.ticketSha256, &replayError)) {
        if (!appendReplayCacheLocked(m_pendingTicket.ticketId,
                                     m_pendingTicket.ticketSha256,
                                     nowUtcIso(),
                                     &replayError)) {
            rc = kRcResultInvalid;
            if (message) {
                *message = QStringLiteral("待处理票据作废失败");
            }
            return false;
        }
    }

    QFile::remove(m_paths.ticketPath);
    QFile::remove(m_paths.resultPath);

    data[QStringLiteral("session_id")] = m_pendingTicket.sessionId;
    data[QStringLiteral("ticket_id")] = m_pendingTicket.ticketId;
    data[QStringLiteral("ticket_sha256")] = m_pendingTicket.ticketSha256;
    data[QStringLiteral("status")] = normalizedStatus;
    data[QStringLiteral("reason")] = normalizedReason;
    data[QStringLiteral("processed_tick_ms")] = static_cast<qint64>(GetTickCount64());
    data[QStringLiteral("verified_at_utc")] = nowUtcIso();

    m_pendingReport.valid = true;
    m_pendingReport.sessionId = m_pendingTicket.sessionId;
    m_pendingReport.ticketId = m_pendingTicket.ticketId;
    m_pendingReport.ticketSha256 = m_pendingTicket.ticketSha256;
    m_pendingReport.status = normalizedStatus;
    m_pendingReport.reason = normalizedReason;
    m_pendingReport.dllVersion.clear();
    m_pendingReport.processedTickMs = data.value(QStringLiteral("processed_tick_ms")).toVariant().toLongLong();
    m_pendingReport.grantedTier.clear();
    m_pendingReport.grantedFeatureFlags = 0;
    m_pendingReport.verifiedDllSha256.clear();
    m_pendingReport.issuedAtServer.clear();
    m_pendingReport.expiresAtServer.clear();
    m_pendingReport.serverPubkeyVersion = m_licenseRecord.serverPubkeyVersion;
    m_pendingReport.serverPubkeyFingerprint = m_licenseRecord.serverPubkeyFingerprint;
    m_pendingReport.verifiedAtUtc = data.value(QStringLiteral("verified_at_utc")).toString();
    clearPendingTicketLocked();

    rc = kRcOk;
    if (message) {
        *message = QStringLiteral("已作废待处理注入票据");
    }
    return true;
}

bool StateManager::reportInjectResultLocked(QJsonObject &data,
                                            int &rc,
                                            QString *message)
{
    if (!m_pendingReport.valid) {
        rc = kRcResultInvalid;
        if (message) {
            *message = QStringLiteral("当前没有待上报的注入结果");
        }
        return false;
    }

    data[QStringLiteral("session_id")] = m_pendingReport.sessionId;
    data[QStringLiteral("ticket_id")] = m_pendingReport.ticketId;
    data[QStringLiteral("ticket_sha256")] = m_pendingReport.ticketSha256;
    data[QStringLiteral("status")] = m_pendingReport.status;
    data[QStringLiteral("reason")] = m_pendingReport.reason;
    data[QStringLiteral("dll_version")] = m_pendingReport.dllVersion;
    data[QStringLiteral("processed_tick_ms")] = m_pendingReport.processedTickMs;
    data[QStringLiteral("granted_tier")] = m_pendingReport.grantedTier;
    data[QStringLiteral("granted_feature_flags")] = m_pendingReport.grantedFeatureFlags;
    data[QStringLiteral("verified_dll_sha256")] = m_pendingReport.verifiedDllSha256;
    data[QStringLiteral("issued_at_server")] = m_pendingReport.issuedAtServer;
    data[QStringLiteral("expires_at_server")] = m_pendingReport.expiresAtServer;
    data[QStringLiteral("server_pubkey_version")] = m_pendingReport.serverPubkeyVersion;
    data[QStringLiteral("server_pubkey_fingerprint")] = m_pendingReport.serverPubkeyFingerprint;
    data[QStringLiteral("verified_at_utc")] = m_pendingReport.verifiedAtUtc;

    QString payloadJson;
    QString signatureHex;
    QString nonceHex;
    QString timestampUtc;
    QString error;
    timestampUtc = nowUtcIso();
    nonceHex = randomNonceHex();
    if (nonceHex.isEmpty()) {
        rc = kRcReportFailed;
        if (message) {
            *message = QStringLiteral("结果上报失败");
        }
        return false;
    }
    payloadJson = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("report_inject_result_v3")},
            {QStringLiteral("key_id"), m_licenseRecord.keyId},
            {QStringLiteral("device_id"), m_licenseRecord.deviceId},
            {QStringLiteral("session_id"), m_pendingReport.sessionId},
            {QStringLiteral("ticket_id"), m_pendingReport.ticketId},
            {QStringLiteral("status"), m_pendingReport.status},
            {QStringLiteral("reason"), m_pendingReport.reason},
            {QStringLiteral("verified_tier"), m_pendingReport.grantedTier},
            {QStringLiteral("verified_dll_sha256"), m_pendingReport.verifiedDllSha256},
            {QStringLiteral("verified_at_utc"), m_pendingReport.verifiedAtUtc},
            {QStringLiteral("issued_at_server"), m_pendingReport.issuedAtServer},
            {QStringLiteral("expires_at_server"), m_pendingReport.expiresAtServer},
            {QStringLiteral("server_pubkey_fingerprint"), m_pendingReport.serverPubkeyFingerprint},
            {QStringLiteral("timestamp_utc"), timestampUtc},
            {QStringLiteral("nonce"), nonceHex},
        },
        {
            {QStringLiteral("verified_tier_value"),
             static_cast<qint64>(tierTextToCode(m_pendingReport.grantedTier))},
            {QStringLiteral("verified_feature_flags"), m_pendingReport.grantedFeatureFlags},
            {QStringLiteral("server_pubkey_version"),
             static_cast<qint64>(m_pendingReport.serverPubkeyVersion)},
        });
    if (!signCanonicalPayloadLocked(payloadJson, signatureHex, &error)) {
        rc = error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                        : kRcDeviceKeyMissing;
        if (message) {
            *message = QStringLiteral("结果上报失败");
        }
        return false;
    }

    RpcReportResultRequest rpcReq;
    rpcReq.keyId = m_licenseRecord.keyId;
    rpcReq.deviceId = m_licenseRecord.deviceId;
    rpcReq.sessionId = m_pendingReport.sessionId;
    rpcReq.ticketId = m_pendingReport.ticketId;
    rpcReq.status = m_pendingReport.status;
    rpcReq.reason = m_pendingReport.reason;
    rpcReq.verifiedTier = m_pendingReport.grantedTier;
    rpcReq.verifiedTierValue = tierTextToCode(m_pendingReport.grantedTier);
    rpcReq.verifiedFeatureFlags = static_cast<quint32>(m_pendingReport.grantedFeatureFlags);
    rpcReq.verifiedDllSha256 = m_pendingReport.verifiedDllSha256;
    rpcReq.verifiedAtUtc = m_pendingReport.verifiedAtUtc;
    rpcReq.issuedAtServer = m_pendingReport.issuedAtServer;
    rpcReq.expiresAtServer = m_pendingReport.expiresAtServer;
    rpcReq.serverPubkeyVersion = m_pendingReport.serverPubkeyVersion;
    rpcReq.serverPubkeyFingerprint = m_pendingReport.serverPubkeyFingerprint;
    rpcReq.timestampUtc = timestampUtc;
    rpcReq.nonceHex = nonceHex;
    rpcReq.deviceSignatureHex = signatureHex;

    RpcReportResultResponse rpcResp;
    if (!m_rpcClient.reportInjectResult(rpcReq, rpcResp, error)) {
        rc = kRcReportFailed;
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        data[QStringLiteral("accepted")] = false;
        if (message) {
            *message = QStringLiteral("结果上报失败");
        }
        return false;
    }

    if (rpcResp.status.compare(QStringLiteral("ok"), Qt::CaseInsensitive) != 0 ||
        !rpcResp.accepted) {
        rc = kRcReportFailed;
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = rpcResp.status;
        data[QStringLiteral("accepted")] = false;
        if (message) {
            *message = QStringLiteral("结果上报失败");
        }
        return false;
    }

    data[QStringLiteral("accepted")] = true;
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();
    m_pendingReport = PendingReportContext{};
    rc = kRcOk;
    if (message) {
        *message = QStringLiteral("结果上报成功");
    }
    return true;
}

bool StateManager::ensureDirectoriesLocked(QString *error)
{
    const QStringList dirs{m_paths.appRootDir, m_paths.stateDir, m_paths.dllDir};
    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        if (dir.exists()) {
            continue;
        }
        if (!QDir().mkpath(dirPath)) {
            if (error) {
                *error = QStringLiteral("mkdir_failed");
            }
            return false;
        }
    }

    return ensureConsumedTicketCacheLocked(error);
}

bool StateManager::ensureConsumedTicketCacheLocked(QString *error)
{
    if (QFile::exists(m_paths.consumedTicketsPath)) {
        return true;
    }

    QJsonObject cache;
    cache[QStringLiteral("schema_version")] = 3;
    cache[QStringLiteral("max_entries")] = 256;
    cache[QStringLiteral("retention_hours")] = 24;
    cache[QStringLiteral("entries")] = QJsonArray{};

    QSaveFile file(m_paths.consumedTicketsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }

    const QByteArray bytes = QJsonDocument(cache).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        if (error) {
            *error = QStringLiteral("file_write_failed");
        }
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool StateManager::replayCacheContainsLocked(const QString &ticketId,
                                             const QString &ticketSha256,
                                             QString *error) const
{
    QJsonObject cache;
    if (!parseObjectFile(m_paths.consumedTicketsPath, cache, error)) {
        return false;
    }
    const QJsonArray entries = cache.value(QStringLiteral("entries")).toArray();
    for (const QJsonValue &value : entries) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject entry = value.toObject();
        if (Protected::NBVmp_Verify_ReplayEntryMatches(
                ticketId,
                ticketSha256,
                entry.value(QStringLiteral("ticket_id")).toString().trimmed(),
                entry.value(QStringLiteral("ticket_sha256")).toString().trimmed().toLower())) {
            return true;
        }
    }
    return false;
}

void StateManager::normalizeReplayEntriesLocked(QJsonArray &entries) const
{
    struct ReplayEntry {
        QString ticketId;
        QString ticketSha256;
        QString consumedAtUtc;
        qint64 sortKey = 0;
    };

    std::vector<ReplayEntry> normalized;
    normalized.reserve(static_cast<size_t>(entries.size()));
    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addSecs(-24 * 60 * 60);

    for (const QJsonValue &value : entries) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        const QString ticketId = obj.value(QStringLiteral("ticket_id")).toString().trimmed();
        const QString ticketSha256 =
            obj.value(QStringLiteral("ticket_sha256")).toString().trimmed().toLower();
        const QString consumedAtUtc =
            obj.value(QStringLiteral("consumed_at_utc")).toString().trimmed();
        if (ticketId.isEmpty() || ticketSha256.isEmpty() || consumedAtUtc.isEmpty()) {
            continue;
        }
        const QDateTime ts = QDateTime::fromString(consumedAtUtc, Qt::ISODate);
        if (!ts.isValid() || ts < cutoff) {
            continue;
        }
        normalized.push_back(ReplayEntry{
            ticketId,
            ticketSha256,
            consumedAtUtc,
            ts.toSecsSinceEpoch(),
        });
    }

    std::sort(normalized.begin(), normalized.end(),
              [](const ReplayEntry &a, const ReplayEntry &b) {
                  return a.sortKey < b.sortKey;
              });

    if (normalized.size() > 256) {
        normalized.erase(normalized.begin(),
                         normalized.begin() + static_cast<ptrdiff_t>(normalized.size() - 256));
    }

    entries = QJsonArray{};
    for (const ReplayEntry &entry : normalized) {
        entries.append(QJsonObject{
            {QStringLiteral("ticket_id"), entry.ticketId},
            {QStringLiteral("ticket_sha256"), entry.ticketSha256},
            {QStringLiteral("consumed_at_utc"), entry.consumedAtUtc},
        });
    }
}

bool StateManager::appendReplayCacheLocked(const QString &ticketId,
                                           const QString &ticketSha256,
                                           const QString &consumedAtUtc,
                                           QString *error)
{
    QJsonObject cache;
    if (!parseObjectFile(m_paths.consumedTicketsPath, cache, error)) {
        return false;
    }

    QJsonArray entries = cache.value(QStringLiteral("entries")).toArray();
    entries.append(QJsonObject{
        {QStringLiteral("ticket_id"), ticketId},
        {QStringLiteral("ticket_sha256"), ticketSha256},
        {QStringLiteral("consumed_at_utc"), consumedAtUtc},
    });
    normalizeReplayEntriesLocked(entries);
    cache[QStringLiteral("schema_version")] = 3;
    cache[QStringLiteral("max_entries")] = 256;
    cache[QStringLiteral("retention_hours")] = 24;
    cache[QStringLiteral("entries")] = entries;

    QSaveFile file(m_paths.consumedTicketsPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("file_open_failed");
        }
        return false;
    }
    const QByteArray bytes = QJsonDocument(cache).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size()) {
        if (error) {
            *error = QStringLiteral("file_write_failed");
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = QStringLiteral("file_commit_failed");
        }
        return false;
    }
    return true;
}

bool StateManager::ensureDeviceIdentityLocked(LicenseRecord &record, QString *error)
{
    if (!record.deviceId.isEmpty() &&
        !record.devicePublicKeyPem.isEmpty() &&
        !record.devicePrivateKeyDpapiBlob.isEmpty()) {
        return true;
    }

    ::NBAuth::RsaKeyPair keyPair;
    if (!::NBAuth::RsaGenerateKeyPair(keyPair)) {
        if (error) {
            *error = QStringLiteral("rsa_generate_failed");
        }
        return false;
    }

    record.devicePublicKeyPem = toPemPublicKey(toQByteArray(keyPair.publicKey));
    record.deviceId = deriveDeviceId(record.devicePublicKeyPem);
    if (record.deviceId.isEmpty()) {
        if (error) {
            *error = QStringLiteral("device_id_derive_failed");
        }
        return false;
    }

    QByteArray protectedPrivateKey;
    if (!dpapiProtectMachine(toQByteArray(keyPair.privateKey), protectedPrivateKey, error)) {
        return false;
    }

    record.devicePrivateKeyDpapiBlob = protectedPrivateKey;
    return true;
}

bool StateManager::signCanonicalPayloadLocked(const QString &payloadJson,
                                              QString &signatureHex,
                                              QString *error)
{
    QByteArray privateBlob;
    if (!dpapiUnprotectMachine(m_licenseRecord.devicePrivateKeyDpapiBlob, privateBlob, error)) {
        return false;
    }

    ::NBAuth::FixedSig256 signature{};
    const QByteArray payloadBytes = payloadJson.toUtf8();
    const ::NBAuth::ByteVector blobVec(
        reinterpret_cast<const uint8_t *>(privateBlob.constData()),
        reinterpret_cast<const uint8_t *>(privateBlob.constData()) + privateBlob.size());
    if (!::NBAuth::RsaSignSha256Blob(blobVec,
                                     reinterpret_cast<const uint8_t *>(payloadBytes.constData()),
                                     static_cast<size_t>(payloadBytes.size()),
                                     signature)) {
        if (error) {
            *error = QStringLiteral("rsa_sign_failed");
        }
        return false;
    }

    signatureHex = signatureToHex(signature);
    return true;
}

bool StateManager::makeSignedPayloadLocked(const QString &cmd,
                                           QString &payloadJson,
                                           QString &signatureHex,
                                           QString &nonceHex,
                                           QString &timestampUtc,
                                           QString *error)
{
    timestampUtc = nowUtcIso();
    nonceHex = randomNonceHex();
    if (nonceHex.isEmpty()) {
        if (error) {
            *error = QStringLiteral("nonce_generate_failed");
        }
        return false;
    }

    payloadJson = canonicalJsonString(
        {
            {QStringLiteral("cmd"), cmd},
            {QStringLiteral("key_id"), m_licenseRecord.keyId},
            {QStringLiteral("device_id"), m_licenseRecord.deviceId},
            {QStringLiteral("timestamp_utc"), timestampUtc},
            {QStringLiteral("nonce"), nonceHex},
        },
        {
            {QStringLiteral("protocol_version"), kProtocolVersion},
        });

    return signCanonicalPayloadLocked(payloadJson, signatureHex, error);
}

bool StateManager::loadLicenseRecordLocked(QString *error)
{
    QJsonObject obj;
    if (!unwrapProtectedLicenseJson(m_paths.licensePath, obj, error)) {
        m_snapshot.hasLicenseFile = QFile::exists(m_paths.licensePath);
        if (!m_snapshot.hasLicenseFile) {
            clearDerivedStateLocked();
            setStatusLocked(QStringLiteral("未激活"), false);
        }
        return false;
    }

    applyLicenseRecordLocked(licenseRecordFromJson(obj));
    m_snapshot.hasLicenseFile = true;
    return true;
}

bool StateManager::saveLicenseRecordLocked(const LicenseRecord &record, QString *error)
{
    if (!ensureDirectoriesLocked(error)) {
        return false;
    }
    return saveProtectedLicenseJson(m_paths.licensePath, licenseRecordToJson(record), error);
}

bool StateManager::createOrRefreshPendingLicenseLocked(const QString &normalizedKey,
                                                       QString *statusMessage,
                                                       QString *error)
{
    LicenseRecord record = m_licenseRecord;
    record.schemaVersion = 3;
    record.channel = QStringLiteral("stable");
    record.keyPlaintext = normalizedKey;
    record.keyHash = Protected::NBVmp_V3_HashKeySha256(normalizedKey);
    record.keyId = Protected::NBVmp_V3_MakeKeyId(record.keyHash);
    record.lastKnownTierName = QStringLiteral("None");
    record.lastKnownTierValue = 0;
    record.lastKnownFeatureFlags = 0;
    record.lastKnownDllName.clear();
    record.lastKnownDllSha256.clear();
    record.lastVerifiedAtUtc.clear();
    record.serverPubkeyVersion = 0;
    record.serverPubkeyFingerprint.clear();

    if (!ensureDeviceIdentityLocked(record, error)) {
        return false;
    }

    record.onlineStateCache = QJsonObject{
        {QStringLiteral("online"), false},
        {QStringLiteral("kicked"), false},
        {QStringLiteral("last_heartbeat_at_utc"), QString()},
        {QStringLiteral("last_server_message"), QStringLiteral("pending_activation")},
        {QStringLiteral("replace_cooldown_remaining_sec"), 0},
    };

    if (!saveLicenseRecordLocked(record, error)) {
        return false;
    }

    applyLicenseRecordLocked(record);
    m_snapshot.hasLicenseFile = true;
    if (statusMessage) {
        *statusMessage = QStringLiteral("未激活");
    }
    return true;
}

bool StateManager::getDllPolicyLocked(QJsonObject &data,
                                      int &rc,
                                      QString *message)
{
    if (!hasVerifiedLicenseLocked()) {
        rc = kRcServerRejected;
        if (message) {
            *message = QStringLiteral("当前设备尚未完成联网激活");
        }
        return false;
    }

    QString signatureHex;
    QString nonceHex;
    QString timestampUtc;
    QString error;
    timestampUtc = nowUtcIso();
    nonceHex = randomNonceHex();
    if (nonceHex.isEmpty()) {
        rc = kRcDeviceKeyMissing;
        if (message) {
            *message = QStringLiteral("设备签名失败");
        }
        return false;
    }
    const QString payloadJson = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("dll_policy_v3")},
            {QStringLiteral("key_id"), m_licenseRecord.keyId},
            {QStringLiteral("device_id"), m_licenseRecord.deviceId},
            {QStringLiteral("timestamp_utc"), timestampUtc},
            {QStringLiteral("nonce"), nonceHex},
        },
        {});
    if (!signCanonicalPayloadLocked(payloadJson, signatureHex, &error)) {
        rc = error.startsWith(QStringLiteral("dpapi_")) ? kRcDpapiDecryptFailed
                                                        : kRcDeviceKeyMissing;
        if (message) {
            *message = QStringLiteral("设备签名失败");
        }
        return false;
    }

    RpcDllPolicyRequest request;
    request.keyId = m_licenseRecord.keyId;
    request.deviceId = m_licenseRecord.deviceId;
    request.timestampUtc = timestampUtc;
    request.nonceHex = nonceHex;
    request.deviceSignatureHex = signatureHex;

    RpcDllPolicyResponse response;
    if (!m_rpcClient.dllPolicy(request, response, error)) {
        rc = kRcNetworkUnavailable;
        m_snapshot.networkAvailable = false;
        m_snapshot.lastError = error;
        if (message) {
            *message = QStringLiteral("无法连接服务器");
        }
        return false;
    }

    const int statusRc = classifyServerStatusLocked(response.status);
    if (statusRc != kRcOk) {
        rc = statusRc == kRcOk ? kRcDllPolicyInvalid : statusRc;
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = response.status;
        if (message) {
            *message = statusRc == kRcDeviceKicked
                ? QStringLiteral("已被其他设备顶下线")
                : QStringLiteral("服务端拒绝 DLL 策略请求");
        }
        return false;
    }

    if (response.dllName.isEmpty() || response.dllSha256.isEmpty() || response.dllSize <= 0) {
        rc = kRcDllPolicyInvalid;
        m_snapshot.networkAvailable = true;
        m_snapshot.lastError = QStringLiteral("dll_policy_invalid");
        if (message) {
            *message = QStringLiteral("当前 DLL 策略无效");
        }
        return false;
    }

    m_licenseRecord.lastKnownDllName = response.dllName;
    m_licenseRecord.lastKnownDllSha256 = response.dllSha256;
    saveLicenseRecordLocked(m_licenseRecord, nullptr);
    applyLicenseRecordLocked(m_licenseRecord);
    m_snapshot.networkAvailable = true;
    m_snapshot.lastError.clear();

    data[QStringLiteral("dll_name")] = response.dllName;
    data[QStringLiteral("dll_sha256")] = response.dllSha256;
    data[QStringLiteral("dll_md5")] = response.dllMd5;
    data[QStringLiteral("dll_size")] = response.dllSize;
    data[QStringLiteral("download_url")] = response.downloadUrl;
    data[QStringLiteral("channel")] = response.channel;
    data[QStringLiteral("expires_in")] = response.expiresIn;

    rc = kRcOk;
    if (message) {
        *message = QStringLiteral("DLL 策略获取成功");
    }
    return true;
}

void StateManager::applyLicenseRecordLocked(const LicenseRecord &record)
{
    m_licenseRecord = record;
    if (m_licenseKey.isEmpty()) {
        m_licenseKey = Protected::NBVmp_V3_NormalizeKey(record.keyPlaintext);
    }

    m_snapshot.active = hasVerifiedLicenseLocked();
    m_snapshot.online = record.onlineStateCache.value(QStringLiteral("online")).toBool(false);
    m_snapshot.kicked = record.onlineStateCache.value(QStringLiteral("kicked")).toBool(false);
    m_snapshot.networkAvailable = false;
    m_snapshot.tierName = record.lastKnownTierName.isEmpty() ? QStringLiteral("None")
                                                             : record.lastKnownTierName;
    m_snapshot.tierValue = record.lastKnownTierValue;
    m_snapshot.featureFlags = record.lastKnownFeatureFlags;
    m_snapshot.keyId = record.keyId;
    m_snapshot.deviceId = record.deviceId;
    m_snapshot.boundDllName = record.lastKnownDllName;
    m_snapshot.boundDllSha256 = record.lastKnownDllSha256;
    m_snapshot.currentChannel = record.channel.isEmpty() ? QStringLiteral("stable")
                                                         : record.channel;
    m_snapshot.lastVerifiedAtUtc = record.lastVerifiedAtUtc;
    m_snapshot.lastHeartbeatAtUtc =
        record.onlineStateCache.value(QStringLiteral("last_heartbeat_at_utc")).toString();
    m_snapshot.replaceCooldownRemainingSec =
        record.onlineStateCache.value(QStringLiteral("replace_cooldown_remaining_sec"))
            .toVariant()
            .toLongLong();
    m_snapshot.localStateRoot = m_paths.stateDir;
    m_snapshot.updateState =
        m_hostUpdateSnapshot.value(QStringLiteral("update_state")).toString(QStringLiteral("idle"));
    m_snapshot.authDllVersion = QString::fromLatin1(kAuthDllVersion);
    if (m_snapshot.kicked) {
        m_snapshot.licenseStatus = QStringLiteral("已被其他设备顶下线");
    } else {
        const QString lastServerMessage =
            record.onlineStateCache.value(QStringLiteral("last_server_message")).toString();
        if (!m_snapshot.active &&
            lastServerMessage == QStringLiteral("activation_rejected")) {
            m_snapshot.licenseStatus = QStringLiteral("激活失败：密钥无效");
        } else {
            m_snapshot.licenseStatus = m_snapshot.active
                ? QStringLiteral("已激活")
                : QStringLiteral("未激活");
        }
    }
}

void StateManager::clearDerivedStateLocked()
{
    m_licenseRecord = LicenseRecord{};
    clearPendingTicketLocked();
    m_snapshot.active = false;
    m_snapshot.online = false;
    m_snapshot.kicked = false;
    m_snapshot.networkAvailable = false;
    m_snapshot.tierName = QStringLiteral("None");
    m_snapshot.tierValue = 0;
    m_snapshot.featureFlags = 0;
    m_snapshot.keyId.clear();
    m_snapshot.deviceId.clear();
    m_snapshot.boundDllName.clear();
    m_snapshot.boundDllSha256.clear();
    m_snapshot.lastVerifiedAtUtc.clear();
    m_snapshot.lastHeartbeatAtUtc.clear();
    m_snapshot.replaceCooldownRemainingSec = 0;
    m_snapshot.hasLicenseFile = QFile::exists(m_paths.licensePath);
    m_snapshot.localStateRoot = m_paths.stateDir;
    m_snapshot.currentChannel = QStringLiteral("stable");
    m_snapshot.updateState =
        m_hostUpdateSnapshot.value(QStringLiteral("update_state")).toString(QStringLiteral("idle"));
    m_snapshot.authDllVersion = QString::fromLatin1(kAuthDllVersion);
    m_snapshot.licenseStatus = QStringLiteral("未激活");
    m_pendingReport = PendingReportContext{};
}

void StateManager::setStatusLocked(const QString &status, bool active)
{
    m_snapshot.licenseStatus = status;
    m_snapshot.active = active;
}

int StateManager::classifyServerStatusLocked(const QString &status, bool kicked) const
{
    if (kicked || status.compare(QStringLiteral("kicked"), Qt::CaseInsensitive) == 0) {
        return kRcDeviceKicked;
    }
    if (status.compare(QStringLiteral("replace_cooldown"), Qt::CaseInsensitive) == 0) {
        return kRcDeviceReplaceCooldown;
    }
    if (status.compare(QStringLiteral("protocol_mismatch"), Qt::CaseInsensitive) == 0) {
        return kRcProtocolMismatch;
    }
    if (status.compare(QStringLiteral("ok"), Qt::CaseInsensitive) == 0) {
        return kRcOk;
    }
    return kRcServerRejected;
}

void StateManager::startHeartbeatLocked()
{
    if (!m_heartbeatTimer || m_snapshot.kicked) {
        return;
    }
    if (!m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start(30000);
    }
}

void StateManager::stopHeartbeatLocked()
{
    if (m_heartbeatTimer && m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
    }
}

bool StateManager::hasVerifiedLicenseLocked() const
{
    return !m_licenseRecord.keyId.isEmpty() &&
           !m_licenseRecord.deviceId.isEmpty() &&
           !m_licenseRecord.lastVerifiedAtUtc.isEmpty() &&
           m_licenseRecord.serverPubkeyVersion > 0 &&
           !m_licenseRecord.serverPubkeyFingerprint.isEmpty();
}

void StateManager::clearPendingTicketLocked()
{
    m_pendingTicket = PendingTicketContext{};
}

void StateManager::loadNameConfigLocked()
{
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    m_nameConfig.available = true;
    m_nameConfig.enabled = settings.value(QStringLiteral("customNameEnabled"), false).toBool();
    m_nameConfig.value = settings.value(QStringLiteral("customName")).toString().trimmed();
}

QJsonObject statusSnapshotToJson(const StatusSnapshot &snapshot)
{
    QJsonObject obj;
    obj[QStringLiteral("active")] = snapshot.active;
    obj[QStringLiteral("online")] = snapshot.online;
    obj[QStringLiteral("kicked")] = snapshot.kicked;
    obj[QStringLiteral("license_status")] = snapshot.licenseStatus;
    obj[QStringLiteral("tier_name")] = snapshot.tierName;
    obj[QStringLiteral("tier_value")] = snapshot.tierValue;
    obj[QStringLiteral("feature_flags")] = static_cast<qint64>(snapshot.featureFlags);
    obj[QStringLiteral("key_id")] = snapshot.keyId;
    obj[QStringLiteral("device_id")] = snapshot.deviceId;
    obj[QStringLiteral("bound_dll_name")] = snapshot.boundDllName;
    obj[QStringLiteral("bound_dll_sha256")] = snapshot.boundDllSha256;
    obj[QStringLiteral("last_verified_at_utc")] = snapshot.lastVerifiedAtUtc;
    obj[QStringLiteral("last_heartbeat_at_utc")] = snapshot.lastHeartbeatAtUtc;
    obj[QStringLiteral("replace_cooldown_remaining_sec")] = snapshot.replaceCooldownRemainingSec;
    obj[QStringLiteral("network_available")] = snapshot.networkAvailable;
    obj[QStringLiteral("update_state")] = snapshot.updateState;
    obj[QStringLiteral("auth_dll_version")] = snapshot.authDllVersion;
    obj[QStringLiteral("protocol_version")] = snapshot.protocolVersion;
    obj[QStringLiteral("abi_version")] = snapshot.abiVersion;
    return obj;
}

QJsonObject localPathsToJson(const LocalPaths &paths)
{
    QJsonObject obj;
    obj[QStringLiteral("app_root_dir")] = paths.appRootDir;
    obj[QStringLiteral("state_dir")] = paths.stateDir;
    obj[QStringLiteral("dll_dir")] = paths.dllDir;
    obj[QStringLiteral("license_path")] = paths.licensePath;
    obj[QStringLiteral("ticket_path")] = paths.ticketPath;
    obj[QStringLiteral("result_path")] = paths.resultPath;
    obj[QStringLiteral("consumed_tickets_path")] = paths.consumedTicketsPath;
    return obj;
}

QJsonObject nameConfigToJson(const NameConfig &config)
{
    QJsonObject obj;
    obj[QStringLiteral("available")] = config.available;
    obj[QStringLiteral("enabled")] = config.enabled;
    obj[QStringLiteral("name")] = config.value;
    return obj;
}

} // namespace NBAuth::V3
