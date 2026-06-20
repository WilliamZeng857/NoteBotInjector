#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <functional>

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = nullptr);

    struct ArtifactInfo {
        QString artifactType;
        QString channel;
        QString version;
        int versionCode = 0;
        QString fileName;
        QString fileMd5;
        QString fileSha256;
        qint64 fileSize = 0;
        QUrl downloadUrl;
        bool required = false;
        int protocolMin = 0;
        int protocolMax = 0;
        QString keyScope;
        QString publishedAtUtc;
    };

    struct UpdateInfo {
        bool hasAnyArtifact = false;
        ArtifactInfo mainExe;
        ArtifactInfo authDll;
        ArtifactInfo updaterExe;
        ArtifactInfo overlayDll;
        int expiresIn = 0;
        qint64 receivedAt = 0;
        QString error;
    };

    using ArtifactRefreshCallback =
        std::function<bool(ArtifactInfo &artifact,
                           int &expiresIn,
                           qint64 &receivedAtUtcSec,
                           QString &error)>;

    void setLogCallback(std::function<void(const QString&)> cb);
    void setProgressCallback(std::function<void(int)> cb);

    UpdateInfo checkManifest(const QString &currentMainVersion,
                             int currentMainVersionCode,
                             const QString &currentAuthDllVersion,
                             int currentAuthDllVersionCode,
                             const QString &currentUpdaterVersion,
                             int currentUpdaterVersionCode);
    QString downloadArtifact(const ArtifactInfo &artifact,
                             const QString &cacheSubdir,
                             int expiresIn = 0,
                             qint64 receivedAtUtcSec = 0,
                             ArtifactRefreshCallback refreshCallback = {});

private:
    QByteArray aesGcmEncrypt(const QByteArray &plaintext);
    QByteArray aesGcmDecrypt(const QByteArray &ciphertext);
    QByteArray sendEncryptedJson(const QByteArray &requestJson, int timeoutMs);

    QString m_host;
    quint16 m_port;
    std::function<void(const QString&)> m_logCallback;
    std::function<void(int)> m_progressCallback;
};

#endif
