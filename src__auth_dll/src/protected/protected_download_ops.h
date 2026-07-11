#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include "crypto/vmp_defs.h"

namespace NBAuth::Protected {

struct SignedDllMetadata {
    QString fileName;
    QString md5;
    QString sha256;
    qint64 size = 0;
    QUrl url;
    int expiresIn = 0;
};

NB_NOINLINE bool NBVmp_Dll_SafeFileNameAllowed(bool hasDllSuffix);
NB_NOINLINE int NBVmp_Dll_MetadataIssue(bool hasFileName,
                                        bool validUrl,
                                        bool positiveSize,
                                        bool md5Matches);
NB_NOINLINE bool NBVmp_Dll_DownloadedSizeMatches(qint64 expectedSize,
                                                  qint64 actualSize);
QString NBVmp_Dll_SafeFileName(const QString &name);
NB_NOINLINE bool NBVmp_Dll_ParseSignedMetadata(const QJsonObject &resp,
                                               const QString &requestedName,
                                               const QString &expectedMd5,
                                               SignedDllMetadata &out,
                                               QString &error);
NB_NOINLINE bool NBVmp_Dll_VerifyDownloadedBytes(const SignedDllMetadata &info,
                                                 const QByteArray &bytes,
                                                 const QString &expectedMd5,
                                                 QString &actualSha256,
                                                 QString &actualMd5,
                                                 QString &error);

} // namespace NBAuth::Protected
