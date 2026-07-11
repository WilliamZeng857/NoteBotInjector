#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "crypto/vmp_defs.h"

namespace NBAuth::Protected {

struct ParsedWrapperTicket {
    QString magic;
    int wrapperVersion = 0;
    QString createdAtUtc;
    quint64 receivedTickMs = 0;
    quint32 targetPid = 0;
    QString exeVersion;
    int authDllProtocol = 0;
    QString ticketSha256;
    QString serverTicketPayload;
    QString serverTicketSignature;
};

struct ParsedServerTicket {
    QString magic;
    int version = 0;
    QString alg;
    QString keyId;
    QString deviceId;
    QString sessionId;
    QString ticketId;
    QString tier;
    int tierValue = 0;
    quint32 featureFlags = 0;
    QString dllName;
    QString dllSha256;
    QString issuedAtServer;
    QString expiresAtServer;
    qint64 ttlMs = 0;
    QString nonce;
};

NB_NOINLINE QByteArray NBVmp_Ticket_DeriveWrapperKey(const QString &sessionId,
                                                     const QString &ticketSha256);
NB_NOINLINE QString NBVmp_Ticket_ComputeTicketSha256(const QString &serverTicketPayload);
NB_NOINLINE bool NBVmp_Ticket_DecryptWrapper(const QByteArray &wrapperKey,
                                             const QByteArray &iv,
                                             const QByteArray &tag,
                                             const QByteArray &cipher,
                                             QByteArray &wrapperPlaintext,
                                             QString *error = nullptr);
NB_NOINLINE bool NBVmp_Ticket_WrapperFieldsAllowed(int wrapperVersion,
                                                    quint32 targetPid,
                                                    bool magicMatches,
                                                    bool hasTicketSha,
                                                    bool hasPayload,
                                                    bool hasSignature);
NB_NOINLINE bool NBVmp_Ticket_ServerFieldsAllowed(int version,
                                                   bool magicMatches,
                                                   bool algorithmMatches,
                                                   bool hasSessionId,
                                                   bool hasTicketId,
                                                   bool hasDllSha256,
                                                   bool hasIssuedAt,
                                                   bool hasExpiresAt,
                                                   qint64 ttlMs);
NB_NOINLINE qint64 NBVmp_Ticket_CanonicalResultFeatureFlags(qint64 value);
bool NBVmp_Ticket_ParseWrapperJson(const QByteArray &wrapperPlaintext,
                                   ParsedWrapperTicket &out,
                                   QString *error = nullptr);
bool NBVmp_Ticket_ParseServerTicket(const QString &payload,
                                    ParsedServerTicket &out,
                                    QString *error = nullptr);
QString NBVmp_Ticket_CanonicalResultJson(const QString &sessionId,
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
                                         const QString &serverPubkeyFingerprint);
NB_NOINLINE QString NBVmp_Ticket_ComputeResultHmacHex(const QByteArray &wrapperKey,
                                                      const QByteArray &canonicalResultUtf8,
                                                      QString *error = nullptr);

} // namespace NBAuth::Protected
