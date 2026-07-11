#include "protected_download_ops.h"
#include "crypto/vmp_defs.h"

#include <QCryptographicHash>
#include <QFileInfo>

namespace NBAuth::Protected {

static QString hashBytesHex(const QByteArray &bytes, QCryptographicHash::Algorithm alg)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, alg).toHex()).toLower();
}

static QString safeFileNamePlain(const QString &name)
{
    QString fileName = QFileInfo(name.trimmed()).fileName();
    if (!NBVmp_Dll_SafeFileNameAllowed(fileName.endsWith(".dll", Qt::CaseInsensitive))) {
        fileName.clear();
    }
    return fileName;
}

NB_NOINLINE bool NBVmp_Dll_SafeFileNameAllowed(bool hasDllSuffix)
{
    NB_VMP_MUTATE("NB.Dll.SafeFileNameAllowed");
    NB_VMP_END();
    return hasDllSuffix;
}

NB_NOINLINE int NBVmp_Dll_MetadataIssue(bool hasFileName,
                                        bool validUrl,
                                        bool positiveSize,
                                        bool md5Matches)
{
    NB_VMP_VIRTUALIZE("NB.Dll.MetadataIssue");
    const int issue = !hasFileName ? 1 : !validUrl ? 2 : !positiveSize ? 3 : !md5Matches ? 4 : 0;
    NB_VMP_END();
    return issue;
}

NB_NOINLINE bool NBVmp_Dll_DownloadedSizeMatches(qint64 expectedSize, qint64 actualSize)
{
    NB_VMP_ULTRA("NB.Dll.DownloadedSizeMatches");
    const bool matches = expectedSize == actualSize;
    NB_VMP_END();
    return matches;
}

QString NBVmp_Dll_SafeFileName(const QString &name)
{
    return safeFileNamePlain(name);
}

NB_NOINLINE bool NBVmp_Dll_ParseSignedMetadata(const QJsonObject &resp,
                                               const QString &requestedName,
                                               const QString &expectedMd5,
                                               SignedDllMetadata &out,
                                               QString &error)
{
    SignedDllMetadata info;
    info.fileName = safeFileNamePlain(resp["dll_name"].toString().trimmed());
    if (info.fileName.isEmpty()) info.fileName = safeFileNamePlain(requestedName);
    info.md5 = resp["dll_md5"].toString().trimmed().toLower();
    info.sha256 = resp["dll_sha256"].toString().trimmed().toLower();
    info.size = resp["dll_size"].toVariant().toLongLong();
    info.url = QUrl(resp["download_url"].toString().trimmed());
    info.expiresIn = resp["expires_in"].toInt(0);

    const int issue = NBVmp_Dll_MetadataIssue(
        !info.fileName.isEmpty(),
        info.url.isValid() && !info.url.scheme().isEmpty() && !info.url.host().isEmpty(),
        info.size > 0,
        expectedMd5.isEmpty() || info.md5.isEmpty() || info.md5 == expectedMd5);
    bool ok = issue == 0;
    error.clear();
    if (issue == 1) {
        error = "invalid dll_name";
    } else if (issue == 2) {
        error = "invalid download_url";
    } else if (issue == 3) {
        error = "invalid dll_size";
    } else if (issue == 4) {
        error = QString("metadata md5 mismatch expected=%1 actual=%2").arg(expectedMd5, info.md5);
    }

    if (ok) out = info;
    return ok;
}

NB_NOINLINE bool NBVmp_Dll_VerifyDownloadedBytes(const SignedDllMetadata &info,
                                                 const QByteArray &bytes,
                                                 const QString &expectedMd5,
                                                 QString &actualSha256,
                                                 QString &actualMd5,
                                                 QString &error)
{
    bool ok = true;
    error.clear();
    actualSha256.clear();
    actualMd5.clear();

    if (!NBVmp_Dll_DownloadedSizeMatches(info.size, bytes.size())) {
        ok = false;
        error = QString("size mismatch expected=%1 actual=%2").arg(info.size).arg(bytes.size());
    }

    if (ok && !info.sha256.isEmpty()) {
        actualSha256 = hashBytesHex(bytes, QCryptographicHash::Sha256);
        if (actualSha256 != info.sha256) {
            ok = false;
            error = "sha256 mismatch";
        }
    }

    if (ok && !info.md5.isEmpty()) {
        actualMd5 = hashBytesHex(bytes, QCryptographicHash::Md5);
        if (actualMd5 != info.md5) {
            ok = false;
            error = "md5 mismatch";
        }
    } else if (ok && !expectedMd5.isEmpty()) {
        actualMd5 = hashBytesHex(bytes, QCryptographicHash::Md5);
        if (actualMd5 != expectedMd5) {
            ok = false;
            error = "expected md5 mismatch";
        }
    }

    return ok;
}

} // namespace NBAuth::Protected
