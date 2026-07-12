#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace NBAuth::V3 {

struct RpcActivateRequest {
    QString keyPlaintext;
    QString deviceId;
    QString devicePublicKeyPem;
    QString timestampUtc;
    QString nonceHex;
    QString clientVersion;
    int protocolVersion = 3;
};

struct RpcActivateResponse {
    QString status;
    QString keyId;
    QString keyHash;
    QString tier;
    int tierValue = 0;
    quint32 featureFlags = 0;
    QString deviceId;
    int serverPubkeyVersion = 0;
    QString serverPubkeyFingerprint;
    QString boundDllName;
    QString boundDllSha256;
};

struct RpcCheckRequest {
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString clientVersion;
    int protocolVersion = 3;
    QString payloadJson;
    QString deviceSignatureHex;
};

struct RpcCheckResponse {
    QString status;
    bool online = false;
    bool kicked = false;
    QString tier;
    int tierValue = 0;
    quint32 featureFlags = 0;
    QString boundDllName;
    QString boundDllSha256;
    qint64 replaceCooldownRemainingSec = 0;
};

struct RpcHeartbeatRequest {
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
};

struct RpcHeartbeatResponse {
    QString status;
    bool online = false;
    bool kicked = false;
    QString serverTimeUtc;
};

struct RpcDllPolicyRequest {
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
};

struct RpcDllPolicyResponse {
    QString status;
    QString dllName;
    QString dllSha256;
    QString dllMd5;
    qint64 dllSize = 0;
    QString downloadUrl;
    QString channel;
    int expiresIn = 0;
};

struct RpcModelEntitlementsRequest {
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
    int protocolVersion = 3;
};

struct RpcModelEntitlementsResponse {
    QString status;
    QJsonArray models;
};

struct RpcModelRuntimePolicyRequest {
    QString keyId;
    QString deviceId;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
    QString currentRuntimeSha256;
    int protocolVersion = 3;
};

struct RpcModelRuntimePolicyResponse {
    QString status;
    bool runtimeEnabled = false;
    QString dllName;
    QString dllSha256;
    QString dllMd5;
    qint64 dllSize = 0;
    QString downloadUrl;
    QString channel;
    int expiresIn = 0;
    int runtimeProtocol = 1;
    int runtimeAbi = 2;
};

struct RpcIssueTicketRequest {
    QString keyId;
    QString deviceId;
    QString dllName;
    QString dllSha256;
    quint32 targetPid = 0;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
};

struct RpcIssueTicketResponse {
    QString status;
    QString message;
    QString sessionId;
    QString ticketId;
    QString serverTicketPayload;
    QString serverTicketSignature;
    int expiresIn = 0;
};

struct RpcReportResultRequest {
    QString keyId;
    QString deviceId;
    QString sessionId;
    QString ticketId;
    QString status;
    QString reason;
    QString verifiedTier;
    int verifiedTierValue = 0;
    quint32 verifiedFeatureFlags = 0;
    QString verifiedDllSha256;
    QString verifiedAtUtc;
    QString issuedAtServer;
    QString expiresAtServer;
    int serverPubkeyVersion = 0;
    QString serverPubkeyFingerprint;
    QString timestampUtc;
    QString nonceHex;
    QString deviceSignatureHex;
};

struct RpcReportResultResponse {
    QString status;
    QString message;
    bool accepted = false;
};

class RpcClientV3 {
public:
    RpcClientV3();

    bool activateDevice(const RpcActivateRequest &request,
                        RpcActivateResponse &response,
                        QString &error) const;
    bool deviceCheck(const RpcCheckRequest &request,
                     RpcCheckResponse &response,
                     QString &error) const;
    bool deviceHeartbeat(const RpcHeartbeatRequest &request,
                         RpcHeartbeatResponse &response,
                         QString &error) const;
    bool dllPolicy(const RpcDllPolicyRequest &request,
                   RpcDllPolicyResponse &response,
                   QString &error) const;
    bool modelEntitlements(const RpcModelEntitlementsRequest &request,
                           RpcModelEntitlementsResponse &response,
                           QString &error) const;
    bool modelRuntimePolicy(const RpcModelRuntimePolicyRequest &request,
                            RpcModelRuntimePolicyResponse &response,
                            QString &error) const;
    bool issueInjectTicket(const RpcIssueTicketRequest &request,
                           RpcIssueTicketResponse &response,
                           QString &error) const;
    bool reportInjectResult(const RpcReportResultRequest &request,
                            RpcReportResultResponse &response,
                            QString &error) const;

private:
    QByteArray aesGcmEncrypt(const QByteArray &plaintext) const;
    QByteArray aesGcmDecrypt(const QByteArray &ciphertext) const;
    QByteArray sendEncryptedJson(const QByteArray &requestJson,
                                 int timeoutMs,
                                 QString &error) const;
};

QString canonicalJsonString(const QList<QPair<QString, QString>> &stringFields,
                            const QList<QPair<QString, qint64>> &integerFields,
                            const QList<QPair<QString, bool>> &boolFields = {});
QString randomNonceHex(int sizeBytes = 16);
QString tierNameFromValue(int tierValue);
int tierValueFromVariant(const QJsonValue &value);

} // namespace NBAuth::V3
