#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QTimer>

#include <mutex>

#include "v3_rpc_client.h"

namespace NBAuth::V3 {

constexpr int kRcOk = 0;
constexpr int kRcInvalidArgument = 1001;
constexpr int kRcLocalStateMissing = 1002;
constexpr int kRcLocalStateCorrupt = 1003;
constexpr int kRcDpapiDecryptFailed = 1004;
constexpr int kRcDeviceKeyMissing = 1005;
constexpr int kRcNetworkUnavailable = 1006;
constexpr int kRcServerRejected = 1007;
constexpr int kRcDeviceKicked = 1008;
constexpr int kRcDeviceReplaceCooldown = 1009;
constexpr int kRcHeartbeatFailed = 1010;
constexpr int kRcDllPolicyInvalid = 1011;
constexpr int kRcDllDownloadFailed = 1012;
constexpr int kRcDllHashMismatch = 1013;
constexpr int kRcTicketRequestFailed = 1014;
constexpr int kRcTicketWriteFailed = 1015;
constexpr int kRcInjectFailed = 1016;
constexpr int kRcResultTimeout = 1017;
constexpr int kRcResultInvalid = 1018;
constexpr int kRcResultHmacFailed = 1019;
constexpr int kRcReportFailed = 1020;
constexpr int kRcProtocolMismatch = 1021;
constexpr int kRcSelfCheckFailed = 1022;

struct LocalPaths {
    QString appRootDir;
    QString stateDir;
    QString dllDir;
    QString licensePath;
    QString ticketPath;
    QString resultPath;
    QString consumedTicketsPath;
};

struct LicenseRecord {
    int schemaVersion = 3;
    QString channel = QStringLiteral("stable");
    QString keyPlaintext;
    QString keyId;
    QString keyHash;
    QString deviceId;
    QString devicePublicKeyPem;
    QByteArray devicePrivateKeyDpapiBlob;
    int serverPubkeyVersion = 0;
    QString serverPubkeyFingerprint;
    QString lastKnownTierName = QStringLiteral("None");
    int lastKnownTierValue = 0;
    quint32 lastKnownFeatureFlags = 0;
    QString lastKnownDllName;
    QString lastKnownDllSha256;
    QString lastVerifiedAtUtc;
    QJsonObject onlineStateCache;
};

struct StatusSnapshot {
    bool active = false;
    bool online = false;
    bool kicked = false;
    bool networkAvailable = false;
    QString tierName = QStringLiteral("None");
    int tierValue = 0;
    quint32 featureFlags = 0;
    QString licenseStatus = QStringLiteral("未激活");
    QString keyId;
    QString deviceId;
    QString boundDllName;
    QString boundDllSha256;
    int protocolVersion = 3;
    int abiVersion = 1;
    QString lastVerifiedAtUtc;
    QString lastHeartbeatAtUtc;
    QString currentChannel = QStringLiteral("stable");
    QString updateState = QStringLiteral("idle");
    QString authDllVersion;
    qint64 replaceCooldownRemainingSec = 0;
    bool hasLicenseFile = false;
    QString localStateRoot;
    QString lastError;
};

struct NameConfig {
    bool available = true;
    bool enabled = false;
    QString value;
};

struct PendingTicketContext {
    bool valid = false;
    QString sessionId;
    QString ticketId;
    QString ticketSha256;
    QString serverTicketPayload;
    QString serverTicketSignature;
    QString dllName;
    QString dllSha256;
    QString createdAtUtc;
    quint32 targetPid = 0;
    quint64 issuedTickMs = 0;
    QByteArray wrapperKey;
};

struct PendingReportContext {
    bool valid = false;
    QString sessionId;
    QString ticketId;
    QString ticketSha256;
    QString status;
    QString reason;
    QString dllVersion;
    qint64 processedTickMs = 0;
    QString grantedTier;
    qint64 grantedFeatureFlags = 0;
    QString verifiedDllSha256;
    QString issuedAtServer;
    QString expiresAtServer;
    int serverPubkeyVersion = 0;
    QString serverPubkeyFingerprint;
    QString verifiedAtUtc;
};

class StateManager {
public:
    StateManager();

    bool initialize();
    void shutdown();

    void setLicenseKey(const QString &key);
    QString licenseKey() const;

    int activatePlaceholder(const QString &key, QString *message = nullptr);
    int verifyLocalState(QString *message = nullptr);
    int activateWithServer(const QString &key, QString *message = nullptr);
    int checkWithServer(QString *message = nullptr);
    int heartbeatOnce(QString *message = nullptr);
    void resetRuntimeState();
    void bindHeartbeatTimer(QTimer *timer);

    bool isActive() const;
    QString statusText() const;
    QString tierText() const;
    int tierCode() const;
    unsigned int featureFlagsMask() const;
    unsigned int expiresEpoch() const;
    int protocolVersion() const;
    int abiVersion() const;

    LocalPaths paths() const;
    StatusSnapshot snapshot() const;
    NameConfig nameConfig() const;
    int issueInjectTicket(const QJsonObject &request,
                          QJsonObject &data,
                          QString *message = nullptr);
    int getDllPolicy(QJsonObject &data,
                     QString *message = nullptr);
    int getModelEntitlements(QJsonObject &data,
                             QString *message = nullptr);
    int downloadOverlayDll(const QJsonObject &request,
                           QJsonObject &data,
                           QString *message = nullptr);
    int consumeInjectResult(QJsonObject &data,
                            QString *message = nullptr);
    int finalizePendingInjectFailure(const QString &status,
                                     const QString &reason,
                                     QJsonObject &data,
                                     QString *message = nullptr);
    int reportInjectResult(QJsonObject &data,
                           QString *message = nullptr);
    void setHostUpdateSnapshot(const QJsonObject &snapshot);
    QJsonObject hostUpdateSnapshot() const;
    bool saveNameConfig(const QString &value, bool enabled, QString *message = nullptr);
    bool setNameEnabled(bool enabled, QString *message = nullptr);

private:
    int verifyLocalStateLocked(QString *message = nullptr);
    bool ensureDirectoriesLocked(QString *error = nullptr);
    bool ensureConsumedTicketCacheLocked(QString *error = nullptr);
    bool ensureDeviceIdentityLocked(LicenseRecord &record, QString *error = nullptr);
    bool signCanonicalPayloadLocked(const QString &payloadJson,
                                    QString &signatureHex,
                                    QString *error = nullptr);
    bool makeSignedPayloadLocked(const QString &cmd,
                                 QString &payloadJson,
                                 QString &signatureHex,
                                 QString &nonceHex,
                                 QString &timestampUtc,
                                 QString *error = nullptr);
    bool getDllPolicyLocked(QJsonObject &data,
                            int &rc,
                            QString *message = nullptr);
    bool loadLicenseRecordLocked(QString *error = nullptr);
    bool saveLicenseRecordLocked(const LicenseRecord &record, QString *error = nullptr);
    bool createOrRefreshPendingLicenseLocked(const QString &normalizedKey,
                                             QString *statusMessage,
                                             QString *error = nullptr);
    bool issueInjectTicketLocked(const QJsonObject &request,
                                 QJsonObject &data,
                                 int &rc,
                                 QString *message = nullptr);
    bool downloadOverlayDllLocked(const QJsonObject &request,
                                  QJsonObject &data,
                                  QString *message = nullptr);
    bool consumeInjectResultLocked(QJsonObject &data,
                                   int &rc,
                                   QString *message = nullptr);
    bool reportInjectResultLocked(QJsonObject &data,
                                  int &rc,
                                  QString *message = nullptr);
    int classifyServerStatusLocked(const QString &status, bool kicked = false) const;
    void startHeartbeatLocked();
    void stopHeartbeatLocked();
    void applyLicenseRecordLocked(const LicenseRecord &record);
    void clearDerivedStateLocked();
    void setStatusLocked(const QString &status, bool active);
    bool hasVerifiedLicenseLocked() const;
    void loadNameConfigLocked();
    bool replayCacheContainsLocked(const QString &ticketId,
                                   const QString &ticketSha256,
                                   QString *error = nullptr) const;
    bool appendReplayCacheLocked(const QString &ticketId,
                                 const QString &ticketSha256,
                                 const QString &consumedAtUtc,
                                 QString *error = nullptr);
    bool finalizePendingInjectFailureLocked(const QString &status,
                                            const QString &reason,
                                            QJsonObject &data,
                                            int &rc,
                                            QString *message = nullptr);
    void normalizeReplayEntriesLocked(QJsonArray &entries) const;
    void clearPendingTicketLocked();

    mutable std::mutex m_mutex;
    LocalPaths m_paths;
    QString m_licenseKey;
    LicenseRecord m_licenseRecord;
    StatusSnapshot m_snapshot;
    NameConfig m_nameConfig;
    PendingTicketContext m_pendingTicket;
    PendingReportContext m_pendingReport;
    QJsonObject m_hostUpdateSnapshot;
    RpcClientV3 m_rpcClient;
    QTimer *m_heartbeatTimer = nullptr;
    int m_heartbeatFailureCount = 0;
    bool m_initialized = false;
};

QJsonObject statusSnapshotToJson(const StatusSnapshot &snapshot);
QJsonObject localPathsToJson(const LocalPaths &paths);
QJsonObject nameConfigToJson(const NameConfig &config);

} // namespace NBAuth::V3
