#include "protected_verify_ops.h"

#include <algorithm>
#include <cstring>

namespace NBAuth::Protected {

NB_NOINLINE bool NBVmp_Verify_SignatureHexToFixed(const QString &signatureHex,
                                                  ::NBAuth::FixedSig256 &out,
                                                  QString *error)
{
    NB_VMP_ULTRA("NB.Verify.SignatureHexToFixed");
    const QByteArray bytes = QByteArray::fromHex(signatureHex.trimmed().toLatin1());
    if (bytes.size() != static_cast<int>(out.size())) {
        if (error) {
            *error = QStringLiteral("signature_invalid:signature_length_invalid");
        }
        NB_VMP_END();
        return false;
    }
    std::memcpy(out.data(), bytes.constData(), out.size());
    NB_VMP_END();
    return true;
}

NB_NOINLINE bool NBVmp_Verify_FormalServerKeyReady(int serverPubkeyVersion,
                                                   const QString &serverPubkeyFingerprint,
                                                   const ::NBAuth::ByteVector &publicKeyDer,
                                                   QString *error)
{
    NB_VMP_ULTRA("NB.Verify.FormalServerKeyReady");
    const bool ok = serverPubkeyVersion > 0 &&
                    !serverPubkeyFingerprint.trimmed().isEmpty() &&
                    !publicKeyDer.empty();
    if (!ok && error) {
        *error = QStringLiteral("signature_invalid:pubkey_metadata_invalid");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_ServerTicketSignature(const ::NBAuth::ByteVector &publicKeyDer,
                                                    const QString &payload,
                                                    const ::NBAuth::FixedSig256 &signature,
                                                    QString *error)
{
    NB_VMP_ULTRA("NB.Verify.ServerTicketSignature");
    const QByteArray payloadBytes = payload.toUtf8();
    const bool ok = ::NBAuth::RsaVerifySha256(
        publicKeyDer,
        reinterpret_cast<const uint8_t *>(payloadBytes.constData()),
        static_cast<size_t>(payloadBytes.size()),
        signature);
    if (!ok && error) {
        *error = QStringLiteral("signature_invalid:rsa_verify_failed");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_TicketShaMatches(const QString &expectedTicketSha256,
                                               const QString &actualTicketSha256,
                                               QString *error)
{
    NB_VMP_MUTATE("NB.Verify.TicketShaMatches");
    const bool ok = !expectedTicketSha256.isEmpty() &&
                    expectedTicketSha256 == actualTicketSha256;
    if (!ok && error) {
        *error = QStringLiteral("wrapper_invalid:ticket_sha_mismatch");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_ServerKeyMetadataMatches(int expectedVersion,
                                                       const QString &expectedFingerprint,
                                                       int actualVersion,
                                                       const QString &actualFingerprint,
                                                       QString *error)
{
    NB_VMP_MUTATE("NB.Verify.ServerKeyMetadataMatches");
    const bool ok = expectedVersion > 0 &&
                    actualVersion > 0 &&
                    !expectedFingerprint.trimmed().isEmpty() &&
                    !actualFingerprint.trimmed().isEmpty() &&
                    expectedVersion == actualVersion &&
                    expectedFingerprint.trimmed().compare(actualFingerprint.trimmed(),
                                                         Qt::CaseInsensitive) == 0;
    if (!ok && error) {
        *error = QStringLiteral("signature_invalid:license_pubkey_metadata_mismatch");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_TargetPid(quint32 expectedTargetPid,
                                        quint32 currentPid,
                                        QString *error)
{
    NB_VMP_MUTATE("NB.Verify.TargetPid");
    const bool ok = expectedTargetPid != 0 && expectedTargetPid == currentPid;
    if (!ok && error) {
        *error = QStringLiteral("wrapper_invalid:target_pid_mismatch");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_DllShaMatches(const QString &expectedDllSha256,
                                            const QString &actualDllSha256,
                                            QString *error)
{
    NB_VMP_MUTATE("NB.Verify.DllShaMatches");
    const bool ok = !expectedDllSha256.isEmpty() &&
                    expectedDllSha256 == actualDllSha256;
    if (!ok && error) {
        *error = QStringLiteral("dll_hash_mismatch:bound_dll_sha256_mismatch");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_TtlElapsed(quint64 receivedTickMs,
                                         quint64 currentTickMs,
                                         quint64 ttlMs,
                                         QString *error)
{
    NB_VMP_ULTRA("NB.Verify.TtlElapsed");
    const bool ok = ttlMs > 0 &&
                    currentTickMs >= receivedTickMs &&
                    (currentTickMs - receivedTickMs) <= ttlMs;
    if (!ok && error) {
        *error = QStringLiteral("ticket_expired:wrapper_ttl_elapsed");
    }
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_ReplayEntryMatches(const QString &ticketId,
                                                 const QString &ticketSha256,
                                                 const QString &entryTicketId,
                                                 const QString &entryTicketSha256)
{
    NB_VMP_ULTRA("NB.Verify.ReplayEntryMatches");
    const bool ok = ticketId == entryTicketId && ticketSha256 == entryTicketSha256;
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_ResultHmacEquals(const QString &expectedLowerHex,
                                               const QString &actualLowerHex)
{
    NB_VMP_ULTRA("NB.Verify.ResultHmacEquals");
    const bool ok = !expectedLowerHex.isEmpty() && expectedLowerHex == actualLowerHex;
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_SignDevicePayload(const QByteArray &privateKeyBlob,
                                                const QByteArray &payloadBytes,
                                                ::NBAuth::FixedSig256 &outSignature)
{
    NB_VMP_ULTRA("NB.Verify.SignDevicePayload");
    if (privateKeyBlob.isEmpty() || payloadBytes.isEmpty()) {
        NB_VMP_END();
        return false;
    }
    ::NBAuth::ByteVector key(
        reinterpret_cast<const uint8_t *>(privateKeyBlob.constData()),
        reinterpret_cast<const uint8_t *>(privateKeyBlob.constData()) + privateKeyBlob.size());
    const bool ok = ::NBAuth::RsaSignSha256Blob(
        key,
        reinterpret_cast<const uint8_t *>(payloadBytes.constData()),
        static_cast<size_t>(payloadBytes.size()),
        outSignature);
    std::fill(key.begin(), key.end(), 0);
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_TicketResponseReady(const QString &serverTicketPayload,
                                                  const QString &serverTicketSignature)
{
    NB_VMP_ULTRA("NB.Verify.TicketResponseReady");
    const bool ok = !serverTicketPayload.isEmpty() && !serverTicketSignature.isEmpty();
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_WrapperKeyReady(const QByteArray &wrapperKey)
{
    NB_VMP_ULTRA("NB.Verify.WrapperKeyReady");
    const bool ok = wrapperKey.size() == static_cast<int>(::NBAuth::AES_GCM_KEY_SIZE);
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_InjectResultEnvelopeMatches(const QString &magic,
                                                          int version,
                                                          const QString &expectedSessionId,
                                                          const QString &sessionId,
                                                          const QString &expectedTicketId,
                                                          const QString &ticketId,
                                                          const QString &expectedTicketSha256,
                                                          const QString &ticketSha256,
                                                          const QString &resultHmac)
{
    NB_VMP_ULTRA("NB.Verify.InjectResultEnvelopeMatches");
    const bool ok = magic == QStringLiteral("NB_TICKET_RESULT_V3") &&
                    version == 3 &&
                    sessionId == expectedSessionId &&
                    ticketId == expectedTicketId &&
                    ticketSha256 == expectedTicketSha256 &&
                    !resultHmac.isEmpty();
    NB_VMP_END();
    return ok;
}

NB_NOINLINE bool NBVmp_Verify_FinalAllow(bool signatureOk,
                                         bool targetPidOk,
                                         bool hashOk,
                                         bool ttlOk,
                                         bool replayOk,
                                         QString *error)
{
    NB_VMP_ULTRA("NB.Verify.FinalAllow");
    const bool ok = signatureOk && targetPidOk && hashOk && ttlOk && replayOk;
    if (!ok && error && error->isEmpty()) {
        *error = QStringLiteral("inject_failed:final_allow_blocked");
    }
    NB_VMP_END();
    return ok;
}

} // namespace NBAuth::Protected
