#include "updater.h"
#include "crypto_utils.h"

#include <QTcpSocket>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtGlobal>
#include <climits>
#include <windows.h>
#include <winhttp.h>

static const uint8_t g_psk[32] = {
    0xbb, 0xfd, 0xb6, 0x21, 0x90, 0xaf, 0x1e, 0x5e,
    0x62, 0xc7, 0x2b, 0xf9, 0xa0, 0x28, 0x26, 0x57,
    0xfb, 0x0f, 0x95, 0xe9, 0xc7, 0xbd, 0xec, 0x7f,
    0x70, 0x70, 0x83, 0xa3, 0xd4, 0xff, 0x58, 0x23
};

static const char *DEFAULT_HOST = "notebot-api.fucku.top";
static const quint16 DEFAULT_PORT = 30165;
static const char *PINNED_DOWNLOAD_CERT_SHA256 =
    "755a7e09f472a45ef9b6a50c87c845f7059fd1be825cfcdf61c7c503ba109126";

namespace {

struct HttpDownloadResult {
    int statusCode = 0;
    QByteArray bytes;
    QString error;
    bool timedOut = false;
};

struct ManifestAuthContext {
    bool hasVerifiedLicense = false;
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
};

constexpr qint64 kDownloadTokenRefreshWindowSec = 15;

QString winHttpError(const QString &where);

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

QString nowUtcIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString randomNonceHex(int bytes = 16)
{
    QByteArray nonce(bytes, Qt::Uninitialized);
    if (!NBAuth::SecureRandomBytes(reinterpret_cast<uint8_t *>(nonce.data()),
                                   static_cast<size_t>(nonce.size()))) {
        return QString();
    }
    return QString::fromLatin1(nonce.toHex()).toLower();
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
    if (actualSha256 != QString::fromLatin1(PINNED_DOWNLOAD_CERT_SHA256)) {
        if (error) {
            *error = QStringLiteral("服务器下载证书指纹不匹配 expected=%1 actual=%2")
                         .arg(QString::fromLatin1(PINNED_DOWNLOAD_CERT_SHA256),
                              actualSha256);
        }
        return false;
    }
    return true;
}

QString hashFileHex(const QString &path, QCryptographicHash::Algorithm alg)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(alg);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(64 * 1024);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            return QString();
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex()).toLower();
}

QString signatureToHex(const NBAuth::FixedSig256 &signature)
{
    return QByteArray(reinterpret_cast<const char *>(signature.data()),
                      static_cast<int>(signature.size())).toHex().toLower();
}

QString canonicalJsonString(const QList<QPair<QString, QString>> &stringFields,
                            const QList<QPair<QString, qint64>> &integerFields)
{
    QStringList parts;
    parts.reserve(stringFields.size() + integerFields.size());
    for (const auto &field : stringFields) {
        parts.append(QStringLiteral("\"%1\":\"%2\"")
                         .arg(field.first, escapeJson(field.second)));
    }
    for (const auto &field : integerFields) {
        parts.append(QStringLiteral("\"%1\":%2")
                         .arg(field.first, QString::number(field.second)));
    }
    return QStringLiteral("{%1}").arg(parts.join(QLatin1Char(',')));
}

QString updateStateLicensePath()
{
    const QString localDir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(localDir).absoluteFilePath(QStringLiteral("state_v3/license_v3.dat"));
}

bool dpapiUnprotectMachine(const QByteArray &ciphertext, QByteArray &plaintext)
{
    DATA_BLOB inBlob{};
    inBlob.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(ciphertext.constData()));
    inBlob.cbData = static_cast<DWORD>(ciphertext.size());

    DATA_BLOB outBlob{};
    if (!CryptUnprotectData(&inBlob,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            CRYPTPROTECT_LOCAL_MACHINE,
                            &outBlob)) {
        return false;
    }

    plaintext = QByteArray(reinterpret_cast<const char *>(outBlob.pbData),
                           static_cast<int>(outBlob.cbData));
    LocalFree(outBlob.pbData);
    return true;
}

bool loadProtectedLicenseObject(QJsonObject &out)
{
    QFile file(updateStateLicensePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QByteArray raw = file.readAll();
    file.close();
    if (raw.size() < 8 || std::memcmp(raw.constData(), "NBV3", 4) != 0) {
        return false;
    }

    QByteArray plain;
    if (!dpapiUnprotectMachine(raw.mid(8), plain)) {
        return false;
    }

    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(plain, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    out = doc.object();
    return true;
}

bool buildManifestAuthContext(const QString &currentMainVersion,
                              int currentMainVersionCode,
                              const QString &currentAuthDllVersion,
                              int currentAuthDllVersionCode,
                              const QString &currentUpdaterVersion,
                              int currentUpdaterVersionCode,
                              ManifestAuthContext &out)
{
    QJsonObject license;
    if (!loadProtectedLicenseObject(license)) {
        return false;
    }

    const QString keyId = license.value(QStringLiteral("key_id")).toString().trimmed();
    const QString deviceId = license.value(QStringLiteral("device_id")).toString().trimmed();
    const QString lastVerifiedAtUtc =
        license.value(QStringLiteral("last_verified_at_utc")).toString().trimmed();
    const int serverPubkeyVersion =
        license.value(QStringLiteral("server_pubkey_version")).toInt(0);
    const QString serverPubkeyFingerprint =
        license.value(QStringLiteral("server_pubkey_fingerprint")).toString().trimmed();
    const QByteArray privateBlob = QByteArray::fromBase64(
        license.value(QStringLiteral("device_private_key_dpapi_blob_b64")).toString().toUtf8());
    if (keyId.isEmpty() || deviceId.isEmpty() || lastVerifiedAtUtc.isEmpty() ||
        serverPubkeyVersion <= 0 || serverPubkeyFingerprint.isEmpty() || privateBlob.isEmpty()) {
        return false;
    }

    QByteArray privateKeyBlob;
    if (!dpapiUnprotectMachine(privateBlob, privateKeyBlob)) {
        return false;
    }

    out.keyId = keyId;
    out.deviceId = deviceId;
    out.timestampUtc = nowUtcIso();
    out.nonceHex = randomNonceHex();
    if (out.nonceHex.isEmpty()) {
        return false;
    }

    const QString canonical = canonicalJsonString(
        {
            {QStringLiteral("cmd"), QStringLiteral("update_manifest_v3")},
            {QStringLiteral("client_kind"), QStringLiteral("NoteBotInjector")},
            {QStringLiteral("channel"), QStringLiteral("stable")},
            {QStringLiteral("current_main_version"), currentMainVersion},
            {QStringLiteral("current_auth_dll_version"), currentAuthDllVersion},
            {QStringLiteral("current_updater_version"), currentUpdaterVersion},
            {QStringLiteral("key_id"), keyId},
            {QStringLiteral("device_id"), deviceId},
            {QStringLiteral("timestamp_utc"), out.timestampUtc},
            {QStringLiteral("nonce"), out.nonceHex},
        },
        {
            {QStringLiteral("current_main_version_code"), currentMainVersionCode},
            {QStringLiteral("current_auth_dll_version_code"), currentAuthDllVersionCode},
            {QStringLiteral("current_updater_version_code"), currentUpdaterVersionCode},
            {QStringLiteral("protocol_version"), 3},
        });

    const NBAuth::ByteVector privateVec(
        reinterpret_cast<const uint8_t *>(privateKeyBlob.constData()),
        reinterpret_cast<const uint8_t *>(privateKeyBlob.constData()) + privateKeyBlob.size());
    NBAuth::FixedSig256 signature{};
    const QByteArray canonicalUtf8 = canonical.toUtf8();
    if (!NBAuth::RsaSignSha256Blob(privateVec,
                                   reinterpret_cast<const uint8_t *>(canonicalUtf8.constData()),
                                   static_cast<size_t>(canonicalUtf8.size()),
                                   signature)) {
        return false;
    }

    out.deviceSignatureHex = signatureToHex(signature);
    out.hasVerifiedLicense = true;
    return true;
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

bool writeFileAtomic(const QString &path, const QByteArray &bytes, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    if (file.write(bytes) != bytes.size()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

qint64 secondsUntilExpiry(qint64 receivedAtUtcSec, int expiresInSec)
{
    if (receivedAtUtcSec <= 0 || expiresInSec <= 0) {
        return static_cast<qint64>(LLONG_MAX);
    }
    return (receivedAtUtcSec + static_cast<qint64>(expiresInSec)) -
           QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
}

bool shouldRefreshDownloadToken(qint64 receivedAtUtcSec, int expiresInSec)
{
    return secondsUntilExpiry(receivedAtUtcSec, expiresInSec) <=
           kDownloadTokenRefreshWindowSec;
}

HttpDownloadResult httpGetNoProxy(const QUrl &url,
                                  qint64 expectedSize,
                                  const std::function<void(int)> &progressCb,
                                  int timeoutMs)
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

    HINTERNET session = WinHttpOpen(L"NoteBotInjector",
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

    if (progressCb) {
        progressCb(1);
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

        if (progressCb && totalSize > 0) {
            const int pct = static_cast<int>((static_cast<qint64>(out.size()) * 100) / totalSize);
            progressCb(qBound(1, pct, 100));
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    result.bytes = out;
    return result;
}

Updater::ArtifactInfo parseArtifact(const QJsonObject &obj)
{
    Updater::ArtifactInfo artifact;
    artifact.artifactType = obj.value(QStringLiteral("artifact_type")).toString().trimmed();
    artifact.channel = obj.value(QStringLiteral("channel")).toString().trimmed();
    artifact.version = obj.value(QStringLiteral("version")).toString().trimmed();
    artifact.versionCode = obj.value(QStringLiteral("version_code")).toInt(0);
    artifact.fileName = obj.value(QStringLiteral("file_name")).toString().trimmed();
    artifact.fileSha256 = obj.value(QStringLiteral("sha256")).toString().trimmed().toLower();
    artifact.fileMd5 = obj.value(QStringLiteral("md5")).toString().trimmed().toLower();
    artifact.fileSize = obj.value(QStringLiteral("size")).toVariant().toLongLong();
    artifact.downloadUrl = QUrl(obj.value(QStringLiteral("download_url")).toString().trimmed());
    artifact.required = obj.value(QStringLiteral("required")).toBool(false);
    artifact.protocolMin = obj.value(QStringLiteral("protocol_min")).toInt(0);
    artifact.protocolMax = obj.value(QStringLiteral("protocol_max")).toInt(0);
    artifact.keyScope = obj.value(QStringLiteral("key_scope")).toString().trimmed();
    artifact.publishedAtUtc = obj.value(QStringLiteral("published_at_utc")).toString().trimmed();
    return artifact;
}

QString buildManifestRequestJson(const QString &currentMainVersion,
                                 int currentMainVersionCode,
                                 const QString &currentAuthDllVersion,
                                 int currentAuthDllVersionCode,
                                 const QString &currentUpdaterVersion,
                                 int currentUpdaterVersionCode,
                                 const ManifestAuthContext &auth)
{
    return QStringLiteral(
               "{\"cmd\":\"update_manifest_v3\",\"client_kind\":\"NoteBotInjector\","
               "\"channel\":\"stable\",\"current_main_version\":\"%1\","
               "\"current_main_version_code\":%2,"
               "\"current_auth_dll_version\":\"%3\","
               "\"current_auth_dll_version_code\":%4,"
               "\"current_updater_version\":\"%5\","
               "\"current_updater_version_code\":%6,"
               "\"protocol_version\":3,"
               "\"key_id\":\"%7\",\"device_id\":\"%8\","
               "\"timestamp_utc\":\"%9\",\"nonce\":\"%10\",\"device_signature\":\"%11\"}")
        .arg(escapeJson(currentMainVersion),
             QString::number(currentMainVersionCode),
             escapeJson(currentAuthDllVersion),
             QString::number(currentAuthDllVersionCode),
             escapeJson(currentUpdaterVersion),
             QString::number(currentUpdaterVersionCode),
             escapeJson(auth.keyId),
             escapeJson(auth.deviceId),
             escapeJson(auth.timestampUtc.isEmpty() ? nowUtcIso() : auth.timestampUtc),
             escapeJson(auth.nonceHex.isEmpty() ? randomNonceHex() : auth.nonceHex),
             escapeJson(auth.deviceSignatureHex));
}

} // namespace

Updater::Updater(QObject *parent)
    : QObject(parent)
    , m_host(QString::fromLatin1(DEFAULT_HOST))
    , m_port(DEFAULT_PORT)
{
}

void Updater::setLogCallback(std::function<void(const QString&)> cb)
{
    m_logCallback = std::move(cb);
}

void Updater::setProgressCallback(std::function<void(int)> cb)
{
    m_progressCallback = std::move(cb);
}

QByteArray Updater::aesGcmEncrypt(const QByteArray &plaintext)
{
    uint8_t iv[NBAuth::AES_GCM_IV_SIZE];
    uint8_t tag[NBAuth::AES_GCM_TAG_SIZE];
    NBAuth::ByteVector ct;
    if (!NBAuth::SecureRandomBytes(iv, NBAuth::AES_GCM_IV_SIZE)) {
        return {};
    }
    if (!NBAuth::AesGcmEncrypt(g_psk,
                               iv,
                               reinterpret_cast<const uint8_t *>(plaintext.constData()),
                               static_cast<size_t>(plaintext.size()),
                               nullptr,
                               0,
                               ct,
                               tag)) {
        return {};
    }
    QByteArray result;
    result.append(reinterpret_cast<const char *>(iv), NBAuth::AES_GCM_IV_SIZE);
    result.append(reinterpret_cast<const char *>(ct.data()), static_cast<int>(ct.size()));
    result.append(reinterpret_cast<const char *>(tag), NBAuth::AES_GCM_TAG_SIZE);
    return result;
}

QByteArray Updater::aesGcmDecrypt(const QByteArray &ciphertext)
{
    if (ciphertext.size() <
        static_cast<int>(NBAuth::AES_GCM_IV_SIZE + NBAuth::AES_GCM_TAG_SIZE)) {
        return {};
    }

    const uint8_t *iv = reinterpret_cast<const uint8_t *>(ciphertext.constData());
    const uint8_t *ct = iv + NBAuth::AES_GCM_IV_SIZE;
    const size_t ctLen = static_cast<size_t>(ciphertext.size()) -
                         NBAuth::AES_GCM_IV_SIZE - NBAuth::AES_GCM_TAG_SIZE;
    const uint8_t *tag = ct + ctLen;

    NBAuth::ByteVector pt;
    if (!NBAuth::AesGcmDecrypt(g_psk, iv, ct, ctLen, nullptr, 0, tag, pt)) {
        return {};
    }
    return QByteArray(reinterpret_cast<const char *>(pt.data()), static_cast<int>(pt.size()));
}

QByteArray Updater::sendEncryptedJson(const QByteArray &requestJson, int timeoutMs)
{
    if (requestJson.isEmpty()) {
        return {};
    }

    const QByteArray encrypted = aesGcmEncrypt(requestJson);
    if (encrypted.isEmpty()) {
        return {};
    }

    auto tryOnce = [&](bool forceDirect) -> QByteArray {
        QTcpSocket socket;
        if (forceDirect) {
            socket.setProxy(QNetworkProxy::NoProxy);
        }

        socket.connectToHost(m_host, m_port);
        if (!socket.waitForConnected(timeoutMs)) {
            if (m_logCallback) {
                m_logCallback(QStringLiteral("[UPD] [ERR] 连接服务端失败: ") + socket.errorString());
            }
            return {};
        }

        QByteArray packet;
        QDataStream ds(&packet, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << static_cast<quint32>(encrypted.size());
        packet.append(encrypted);

        socket.write(packet);
        if (!socket.waitForBytesWritten(5000)) {
            socket.close();
            return {};
        }
        if (!socket.waitForReadyRead(timeoutMs)) {
            if (m_logCallback) {
                m_logCallback(QStringLiteral("[UPD] [ERR] 等待服务端响应超时"));
            }
            socket.close();
            return {};
        }

        QByteArray lenBytes = socket.read(4);
        if (lenBytes.size() != 4) {
            socket.close();
            return {};
        }

        QDataStream lenStream(lenBytes);
        lenStream.setByteOrder(QDataStream::BigEndian);
        quint32 respLen = 0;
        lenStream >> respLen;

        QByteArray respEncrypted;
        while (respEncrypted.size() < static_cast<int>(respLen)) {
            const qint64 avail = socket.bytesAvailable();
            if (avail > 0) {
                respEncrypted.append(socket.read(respLen - respEncrypted.size()));
            } else if (!socket.waitForReadyRead(5000)) {
                break;
            }
        }
        socket.close();

        if (respEncrypted.size() != static_cast<int>(respLen)) {
            return {};
        }
        return aesGcmDecrypt(respEncrypted);
    };

    QByteArray response = tryOnce(false);
    if (!response.isEmpty()) {
        return response;
    }

    if (m_logCallback) {
        m_logCallback(QStringLiteral("[UPD] 首次清单请求失败，切换强制直连模式重试..."));
    }
    response = tryOnce(true);
    if (!response.isEmpty() && m_logCallback) {
        m_logCallback(QStringLiteral("[UPD] 强制直连模式已获取更新清单"));
    }
    return response;
}

Updater::UpdateInfo Updater::checkManifest(const QString &currentMainVersion,
                                           int currentMainVersionCode,
                                           const QString &currentAuthDllVersion,
                                           int currentAuthDllVersionCode,
                                           const QString &currentUpdaterVersion,
                                           int currentUpdaterVersionCode)
{
    auto L = [this](const QString &msg) {
        if (m_logCallback) {
            m_logCallback(msg);
        }
    };

    UpdateInfo info;
    ManifestAuthContext auth;
    if (buildManifestAuthContext(currentMainVersion,
                                 currentMainVersionCode,
                                 currentAuthDllVersion,
                                 currentAuthDllVersionCode,
                                 currentUpdaterVersion,
                                 currentUpdaterVersionCode,
                                 auth)) {
    } else {
        auth.timestampUtc = nowUtcIso();
        auth.nonceHex = randomNonceHex();
    }

    const QByteArray requestJson = buildManifestRequestJson(currentMainVersion,
                                                            currentMainVersionCode,
                                                            currentAuthDllVersion,
                                                            currentAuthDllVersionCode,
                                                            currentUpdaterVersion,
                                                            currentUpdaterVersionCode,
                                                            auth)
                                       .toUtf8();

    const QByteArray respPlain = sendEncryptedJson(requestJson, 15000);
    if (respPlain.isEmpty()) {
        info.error = QStringLiteral("empty response");
        L(QStringLiteral("[UPD] [ERR] 更新清单请求失败"));
        return info;
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(respPlain, &parseError);
    if (!doc.isObject()) {
        info.error = QStringLiteral("invalid json: %1").arg(parseError.errorString());
        L(QStringLiteral("[UPD] [ERR] 更新清单响应不是合法 JSON"));
        return info;
    }

    const QJsonObject resp = doc.object();
    const QString status = resp.value(QStringLiteral("status")).toString().trimmed();
    if (status.compare(QStringLiteral("ok"), Qt::CaseInsensitive) != 0) {
        info.error = resp.value(QStringLiteral("msg")).toString(status);
        L(QStringLiteral("[UPD] [ERR] 服务端拒绝更新清单: %1").arg(info.error));
        return info;
    }

    info.expiresIn = resp.value(QStringLiteral("expires_in")).toInt(0);
    info.receivedAt = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    const QJsonArray artifacts = resp.value(QStringLiteral("artifacts")).toArray();
    for (const QJsonValue &value : artifacts) {
        if (!value.isObject()) {
            continue;
        }
        const ArtifactInfo artifact = parseArtifact(value.toObject());
        if (artifact.artifactType == QStringLiteral("main_exe")) {
            info.mainExe = artifact;
        } else if (artifact.artifactType == QStringLiteral("auth_dll")) {
            info.authDll = artifact;
        } else if (artifact.artifactType == QStringLiteral("updater_exe")) {
            info.updaterExe = artifact;
        } else if (artifact.artifactType == QStringLiteral("overlay_dll")) {
            info.overlayDll = artifact;
        }
    }

    info.hasAnyArtifact = !info.mainExe.fileName.isEmpty() ||
                          !info.authDll.fileName.isEmpty() ||
                          !info.updaterExe.fileName.isEmpty() ||
                          !info.overlayDll.fileName.isEmpty();

    return info;
}

QString Updater::downloadArtifact(const ArtifactInfo &artifact,
                                  const QString &cacheSubdir,
                                  int expiresIn,
                                  qint64 receivedAtUtcSec,
                                  ArtifactRefreshCallback refreshCallback)
{
    auto L = [this](const QString &msg) {
        if (m_logCallback) {
            m_logCallback(msg);
        }
    };
    auto P = [this](int pct) {
        if (m_progressCallback) {
            m_progressCallback(pct);
        }
    };

    ArtifactInfo effectiveArtifact = artifact;
    int effectiveExpiresIn = expiresIn;
    qint64 effectiveReceivedAtUtcSec = receivedAtUtcSec;

    auto refreshArtifact = [&](const QString &reason) -> bool {
        if (!refreshCallback) {
            return false;
        }
        L(QStringLiteral("[UPD] %1，正在重新申请下载凭证...").arg(reason));
        QString refreshError;
        ArtifactInfo refreshedArtifact = effectiveArtifact;
        int refreshedExpiresIn = effectiveExpiresIn;
        qint64 refreshedReceivedAtUtcSec = effectiveReceivedAtUtcSec;
        if (!refreshCallback(refreshedArtifact,
                             refreshedExpiresIn,
                             refreshedReceivedAtUtcSec,
                             refreshError)) {
            L(QStringLiteral("[UPD] [ERR] 下载凭证刷新失败: %1")
                  .arg(refreshError.isEmpty() ? QStringLiteral("unknown") : refreshError));
            return false;
        }
        effectiveArtifact = refreshedArtifact;
        effectiveExpiresIn = refreshedExpiresIn;
        effectiveReceivedAtUtcSec = refreshedReceivedAtUtcSec;
        const qint64 remaining =
            secondsUntilExpiry(effectiveReceivedAtUtcSec, effectiveExpiresIn);
        if (remaining == static_cast<qint64>(LLONG_MAX)) {
            L(QStringLiteral("[UPD] 下载凭证已刷新"));
        } else {
            L(QStringLiteral("[UPD] 下载凭证已刷新，剩余约 %1 秒有效期")
                  .arg(remaining < 0 ? 0 : remaining));
        }
        return true;
    };

    if (shouldRefreshDownloadToken(effectiveReceivedAtUtcSec, effectiveExpiresIn) &&
        !refreshArtifact(QStringLiteral("检测到下载凭证即将过期"))) {
        return QString();
    }

    const QString fileName = safeArtifactFileName(effectiveArtifact.fileName);
    if (fileName.isEmpty() || !effectiveArtifact.downloadUrl.isValid() ||
        effectiveArtifact.downloadUrl.host().isEmpty()) {
        L(QStringLiteral("[UPD] [ERR] 更新产物元数据无效"));
        return QString();
    }
    if (effectiveArtifact.fileSize <= 0) {
        L(QStringLiteral("[UPD] [ERR] 更新产物大小无效"));
        return QString();
    }

    L(QStringLiteral("[UPD] 正在下载：%1").arg(fileName));
    HttpDownloadResult http =
        httpGetNoProxy(effectiveArtifact.downloadUrl, effectiveArtifact.fileSize, P, 30000);
    if (http.statusCode == 403 &&
        refreshArtifact(QStringLiteral("服务端拒绝了旧下载凭证(403)"))) {
        http = httpGetNoProxy(effectiveArtifact.downloadUrl,
                              effectiveArtifact.fileSize,
                              P,
                              30000);
    }
    if (http.statusCode < 200 || http.statusCode >= 300) {
        L(QStringLiteral("[UPD] [ERR] 下载失败：%1 (status=%2)")
              .arg(fileName)
              .arg(http.statusCode));
        return QString();
    }
    if (http.bytes.size() != effectiveArtifact.fileSize) {
        L(QStringLiteral("[UPD] [ERR] 下载大小不匹配 expected=%1 actual=%2")
              .arg(effectiveArtifact.fileSize)
              .arg(http.bytes.size()));
        return QString();
    }
    if (!effectiveArtifact.fileSha256.isEmpty()) {
        const QString actualSha256 = hashBytesHex(http.bytes, QCryptographicHash::Sha256);
        if (actualSha256 != effectiveArtifact.fileSha256) {
            L(QStringLiteral("[UPD] [ERR] 文件校验失败：%1").arg(fileName));
            return QString();
        }
    }
    if (!effectiveArtifact.fileMd5.isEmpty()) {
        const QString actualMd5 = hashBytesHex(http.bytes, QCryptographicHash::Md5);
        if (actualMd5 != effectiveArtifact.fileMd5) {
            L(QStringLiteral("[UPD] [ERR] 文件校验失败：%1").arg(fileName));
            return QString();
        }
    }

    const QString shaPrefix =
        effectiveArtifact.fileSha256.isEmpty() ? QStringLiteral("nosha")
                                               : effectiveArtifact.fileSha256.left(12);
    const QString cacheName =
        QStringLiteral("%1-%2-%3.bin").arg(effectiveArtifact.artifactType,
                                           effectiveArtifact.version,
                                           shaPrefix);
    const QString saveDir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        QStringLiteral("/updates/") + cacheSubdir;
    const QString savePath = QDir(saveDir).absoluteFilePath(cacheName);
    QString writeError;
    if (!writeFileAtomic(savePath, http.bytes, &writeError)) {
        L(QStringLiteral("[UPD] [ERR] 写入更新缓存失败：%1").arg(fileName));
        return QString();
    }

    P(100);
    L(QStringLiteral("[UPD] 下载完成并校验通过：%1").arg(fileName));
    return savePath;
}
