#pragma once

#include <QByteArray>
#include <QString>

#include "crypto/crypto_utils.h"
#include "crypto/vmp_defs.h"

namespace NBAuth::Protected {

NB_NOINLINE bool NBVmp_Verify_SignatureHexToFixed(const QString &signatureHex,
                                                  ::NBAuth::FixedSig256 &out,
                                                  QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_FormalServerKeyReady(int serverPubkeyVersion,
                                                   const QString &serverPubkeyFingerprint,
                                                   const ::NBAuth::ByteVector &publicKeyDer,
                                                   QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_ServerTicketSignature(const ::NBAuth::ByteVector &publicKeyDer,
                                                    const QString &payload,
                                                    const ::NBAuth::FixedSig256 &signature,
                                                    QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_TicketShaMatches(const QString &expectedTicketSha256,
                                               const QString &actualTicketSha256,
                                               QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_ServerKeyMetadataMatches(int expectedVersion,
                                                       const QString &expectedFingerprint,
                                                       int actualVersion,
                                                       const QString &actualFingerprint,
                                                       QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_TargetPid(quint32 expectedTargetPid,
                                        quint32 currentPid,
                                        QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_DllShaMatches(const QString &expectedDllSha256,
                                            const QString &actualDllSha256,
                                            QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_TtlElapsed(quint64 receivedTickMs,
                                         quint64 currentTickMs,
                                         quint64 ttlMs,
                                         QString *error = nullptr);
NB_NOINLINE bool NBVmp_Verify_ReplayEntryMatches(const QString &ticketId,
                                                 const QString &ticketSha256,
                                                 const QString &entryTicketId,
                                                 const QString &entryTicketSha256);
NB_NOINLINE bool NBVmp_Verify_ResultHmacEquals(const QString &expectedLowerHex,
                                               const QString &actualLowerHex);
NB_NOINLINE bool NBVmp_Verify_FinalAllow(bool signatureOk,
                                         bool targetPidOk,
                                         bool hashOk,
                                         bool ttlOk,
                                         bool replayOk,
                                         QString *error = nullptr);

} // namespace NBAuth::Protected
