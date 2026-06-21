#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QPointer>
#include <functional>
#include "processmodel.h"
#include "logmodel.h"
#include "updater.h"

/* 前向声明 DLL 加载器 */
struct AuthDllFuncs;
struct ModelDllFuncs;
class ModelCatalogModel;

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int processCount READ processCount NOTIFY processCountChanged)
    Q_PROPERTY(int selectedPid READ selectedPid NOTIFY selectedPidChanged)
    Q_PROPERTY(QString injectState READ injectState NOTIFY injectStateChanged)
    Q_PROPERTY(QString scanStatus READ scanStatus NOTIFY scanStatusChanged)
    Q_PROPERTY(QString licenseStatus READ licenseStatus NOTIFY licenseStatusChanged)
    Q_PROPERTY(QString licenseTier READ licenseTier NOTIFY licenseStatusChanged)
    Q_PROPERTY(QString licenseKey READ licenseKey WRITE setLicenseKeyProperty NOTIFY licenseKeyChanged)
    Q_PROPERTY(bool isDevMode READ isDevMode NOTIFY licenseStatusChanged)
    Q_PROPERTY(bool isActivated READ isActivated NOTIFY licenseStatusChanged)
    Q_PROPERTY(bool authSessionVerified READ authSessionVerified NOTIFY authSessionVerifiedChanged)
    Q_PROPERTY(bool injectCooldown READ injectCooldown NOTIFY injectCooldownChanged)
    Q_PROPERTY(int downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)
    Q_PROPERTY(int injectProgress READ injectProgress NOTIFY injectProgressChanged)
    Q_PROPERTY(QString injectStageText READ injectStageText NOTIFY injectStageTextChanged)
    Q_PROPERTY(bool modelModificationEnabled READ modelModificationEnabled WRITE setModelModificationEnabled NOTIFY modelModificationEnabledChanged)
    Q_PROPERTY(bool modelRuntimeAvailable READ modelRuntimeAvailable NOTIFY modelRuntimeAvailableChanged)
    Q_PROPERTY(bool modelReplacementRunning READ modelReplacementRunning NOTIFY modelReplacementRunningChanged)
    Q_PROPERTY(QString modelReplacementStatus READ modelReplacementStatus NOTIFY modelReplacementStatusChanged)
    Q_PROPERTY(bool initializing READ initializing NOTIFY initializingChanged)
    Q_PROPERTY(int initStep READ initStep NOTIFY initStepChanged)
    Q_PROPERTY(QString initStatus READ initStatus NOTIFY initStatusChanged)

public:
    explicit Backend(ProcessModel *procModel,
                     LogModel *logModel,
                     ModelCatalogModel *modelCatalogModel,
                     QObject *parent = nullptr);
    ~Backend();

    int processCount() const { return m_processCount; }
    int selectedPid() const { return static_cast<int>(m_selectedPid); }
    QString injectState() const { return m_injectState; }
    QString scanStatus() const { return m_scanStatus; }
    QString licenseStatus() const { return m_licenseStatus; }
    QString licenseTier() const { return m_licenseTier; }
    QString licenseKey() const { return m_licenseKey; }
    bool isDevMode() const { return m_licenseTier == "Dev"; }
    bool isActivated() const { return m_isActivated; }
    bool authSessionVerified() const { return m_authSessionVerified; }
    bool injectCooldown() const { return m_injectCooldown; }
    int downloadProgress() const { return m_downloadProgress; }
    int injectProgress() const { return m_injectProgress; }
    QString injectStageText() const { return m_injectStageText; }
    bool modelModificationEnabled() const { return m_modelModificationEnabled; }
    bool modelRuntimeAvailable() const { return m_modelRuntimeAvailable; }
    bool modelReplacementRunning() const { return m_modelReplacementRunning; }
    QString modelReplacementStatus() const { return m_modelReplacementStatus; }
    void setModelModificationEnabled(bool enabled);
    bool initializing() const { return m_initializing; }
    int initStep() const { return m_initStep; }
    QString initStatus() const { return m_initStatus; }
    void setLicenseKeyProperty(const QString &v);
    Q_INVOKABLE QString callDll(const QString &action, const QString &json = QString());
    Q_INVOKABLE void callDllAsync(const QString &action, const QString &json = QString());
    Q_INVOKABLE void appendLog(const QString &msg);
    Q_INVOKABLE void activateModel(const QString &modelId);
    Q_INVOKABLE void startModelReplacementWait();
    Q_INVOKABLE void stopModelReplacementWait();

    // 同步加载 DLL（只 LoadLibrary + 解析导出，不做验证）
    bool loadAuthDll();

    // 同步初始化壳，仅加载 DLL
    bool initialize();

    // 异步初始化：状态同步 + 更新检查（在 QML 加载后调用）
    void initializeAsync();

    // 异步检查 DLL 更新（后台线程）
    void checkDllUpdateAsync();

signals:
    void processCountChanged();
    void selectedPidChanged();
    void injectStateChanged();
    void scanStatusChanged();
    void licenseStatusChanged();
    void licenseKeyChanged();
    void authSessionVerifiedChanged();
    void injectCooldownChanged();
    void downloadProgressChanged();
    void injectProgressChanged();
    void injectStageTextChanged();
    void modelModificationEnabledChanged();
    void modelRuntimeAvailableChanged();
    void modelReplacementRunningChanged();
    void modelReplacementStatusChanged();
    void splashFinished();
    void initializingChanged();
    void initStepChanged();
    void initStatusChanged();
    void initializationFinished();
    void initializationFailed(const QString &reason);

    void _logSignal(const QString &msg);

public slots:
    void startScanning();
    void selectProcess(int pid);
    void bringToFront(int pid);
    void setScanTarget(const QString &name);
    void doInject();

public slots:
    void setLicenseKey(const QString &key);
    void verifyLicense();
    void doActivate();
    void doLocalVerify();
    void doDiagnose();
    void resetActivation();

public slots:
    void onDllStateChanged(const QString &key, const QString &value);
    void handleModelDllState(const QString &key, const QString &value);

private slots:
    void onLogMsg(const QString &msg);
    void autoScan();

private:
    void unloadAuthDll();
    bool loadModelDll();
    void unloadModelDll();
    void setModelReplacementRunning(bool running);
    void setModelRuntimeAvailable(bool available);
    void setModelReplacementStatus(const QString &status);
    void requestModelReplacementRestart();
    void syncStatusFromDll();
    void syncHostUpdateSnapshot(const QString &state, bool authDllPendingReplace = false);
    void checkDllUpdateAsyncInternal();
    void finishInitialization();
    void setAuthSessionVerified(bool verified);
    void refreshModelEntitlementsAsync();
    void refreshModelRuntimeAsync();
    void clearModelCatalog();
    bool authActionsBlocked(QString *reason = nullptr) const;
    bool injectBlocked(QString *reason = nullptr) const;
    bool prepareUpdaterExecutable(const QString &downloadedUpdaterPath,
                                  QString *readyPath,
                                  QString *errorMessage);
    bool launchMainExeReplacement(const QString &updaterPath,
                                  const QString &downloadedMainPath,
                                  QString *errorMessage);
    void setInitStatus(const QString &status, int step = -1);
    void logFromThread(const QString &msg);
    void updateInjectUiState(const QString &state,
                             const QString &stageText,
                             int progress,
                             bool keepDownloadProgress = false);
    void finishInjectUiState(const QString &state,
                             const QString &stageText,
                             int progress);

    ProcessModel *m_procModel;
    LogModel *m_logModel;
    QPointer<ModelCatalogModel> m_modelCatalogModel;
    uint m_selectedPid = 0;
    QString m_injectState = "idle";
    QString m_scanStatus = "等待启动";
    QString m_licenseStatus = "未授权";
    QString m_licenseTier = "None";
    int m_processCount = 0;
    QString m_scanTarget = "Minecraft.Windows.exe";
    QSet<uint> m_prevPids;
    QTimer *m_scanTimer;
    bool m_injectCooldown = false;
    int m_downloadProgress = -1;
    int m_injectProgress = -1;
    QString m_injectStageText;
    bool m_injectRunning = false;
    bool m_modelModificationEnabled = false;
    bool m_modelRuntimeAvailable = false;
    bool m_modelReplacementRunning = false;
    bool m_modelReplacementRestartPending = false;
    QString m_modelReplacementStatus = "已关闭";
    bool m_isActivated = false;
    bool m_authSessionVerified = false;
    QString m_licenseKey;
    QString m_activeModelId;

    // 初始化状态
    bool m_initializing = false;
    int m_initStep = 0;
    QString m_initStatus;
    QString m_hostUpdateState = "idle";
    bool m_authUpdateRequired = false;

    // DLL 句柄和函数表
    AuthDllFuncs *m_funcs = nullptr;
    void *m_hDll = nullptr;
    ModelDllFuncs *m_modelFuncs = nullptr;
    void *m_hModelDll = nullptr;
};

#endif // BACKEND_H
