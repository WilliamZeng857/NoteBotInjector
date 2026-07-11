#include "backend.h"
#include "win32process.h"
#include "win32injector.h"
#include "updater.h"
#include "vmp_defs.h"
#include "modelcatalogmodel.h"
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <thread>
#include <QUrl>
#include <QMetaObject>
#include <QSettings>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QImage>
#include <algorithm>
#include <windows.h>
#include "../src__auth_dll/include/notebot_auth.h"
#include "../src__model_dll/include/notebot_model.h"

static QString computeFileHash(const QString &filePath, QCryptographicHash::Algorithm algorithm)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(algorithm);
    while (!file.atEnd()) {
        QByteArray chunk = file.read(64 * 1024);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            return QString();
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex()).toLower();
}

constexpr int kExpectedAuthProtocolVersion = 3;
constexpr int kExpectedAuthAbiVersion = 1;
constexpr int kExpectedModelProtocolVersion = 1;
constexpr int kExpectedModelAbiVersion = 2;

static NB_NOINLINE QString NBVmp_Injector_NormalizeLicenseKeyForIdentity(const QString &key)
{
    NB_VMP_MUTATE("NB.Injector.NormalizeLicenseKeyForIdentity");
    QString out = key.trimmed().toUpper();
    out.remove(QRegularExpression(QStringLiteral("\\s+")));
    NB_VMP_END();
    return out;
}

static NB_NOINLINE QString NBVmp_Injector_MakeLocalKeyId(const QString &key)
{
    NB_VMP_VIRTUALIZE("NB.Injector.MakeLocalKeyId");
    const QString normalized = NBVmp_Injector_NormalizeLicenseKeyForIdentity(key);
    if (normalized.isEmpty()) {
        NB_VMP_END();
        return QString();
    }
    const QByteArray hash =
        QCryptographicHash::hash(normalized.toUtf8(), QCryptographicHash::Sha256).toHex();
    const QString result = QStringLiteral("kid_%1").arg(QString::fromLatin1(hash.left(16)).toLower());
    NB_VMP_END();
    return result;
}

static QString computeFileMd5(const QString &filePath)
{
    return computeFileHash(filePath, QCryptographicHash::Md5);
}

static QString computeFileSha256(const QString &filePath)
{
    return computeFileHash(filePath, QCryptographicHash::Sha256);
}

static QString displayNameOnly(const QString &pathOrName)
{
    const QString trimmed = pathOrName.trimmed();
    const QString fileName = QFileInfo(trimmed).fileName();
    return fileName.isEmpty() ? trimmed : fileName;
}

static QString sanitizeUiLogMessage(QString msg)
{
    static const QRegularExpression urlRe(QStringLiteral("https?://\\S+"));
    static const QRegularExpression winPathRe(QStringLiteral("([A-Za-z]:[\\\\/][^\\s\\r\\n]+)"));
    static const QRegularExpression hexAddressRe(QStringLiteral("\\b0x[0-9A-Fa-f]{6,}\\b"));
    static const QRegularExpression pidRe(QStringLiteral("\\bPID=\\d+\\b"));

    msg.replace(urlRe, QStringLiteral("[下载地址已隐藏]"));
    msg.replace(hexAddressRe, QStringLiteral("0x[隐藏]"));
    msg.replace(pidRe, QStringLiteral("PID=[隐藏]"));

    QRegularExpressionMatchIterator it = winPathRe.globalMatch(msg);
    QList<QRegularExpressionMatch> matches;
    while (it.hasNext()) {
        matches.append(it.next());
    }
    for (int i = matches.size() - 1; i >= 0; --i) {
        const QRegularExpressionMatch &m = matches.at(i);
        const QString fileName = displayNameOnly(m.captured(1));
        msg.replace(m.capturedStart(1), m.capturedLength(1),
                    fileName.isEmpty() ? QStringLiteral("[本地路径已隐藏]") : fileName);
    }

    return msg;
}

static QString sanitizeModelLogMessage(QString msg)
{
    if (!msg.startsWith(QStringLiteral("[MODEL]"))) {
        return sanitizeUiLogMessage(msg);
    }

    if (msg.contains(QStringLiteral("hook resolved")) ||
        msg.contains(QStringLiteral("hook installed")) ||
        msg.contains(QStringLiteral("hook hit")) ||
        msg.contains(QStringLiteral("remote ")) ||
        msg.contains(QStringLiteral("resolver group")) ||
        msg.contains(QStringLiteral("addr=")) ||
        msg.contains(QStringLiteral("shell=")) ||
        msg.contains(QStringLiteral("actual="))) {
        return QString();
    }

    if (msg.contains(QStringLiteral("base=")) || msg.contains(QStringLiteral("size="))) {
        return QStringLiteral("[MODEL] 模块已就绪");
    }
    if (msg.contains(QStringLiteral("name-anchored resolver failed")) ||
        msg.contains(QStringLiteral("resolver failed")) ||
        msg.contains(QStringLiteral("resolver incomplete"))) {
        return QStringLiteral("[MODEL] 替换点暂不可用");
    }

    return sanitizeUiLogMessage(msg);
}

static bool fileMatchesArtifact(const QString &path, const Updater::ArtifactInfo &artifact)
{
    if (!QFile::exists(path)) {
        return false;
    }
    if (!artifact.fileSha256.isEmpty()) {
        QString localSha256 = computeFileSha256(path);
        return !localSha256.isEmpty() && localSha256 == artifact.fileSha256;
    }
    if (!artifact.fileMd5.isEmpty()) {
        QString localMd5 = computeFileMd5(path);
        return !localMd5.isEmpty() && localMd5 == artifact.fileMd5;
    }
    return false;
}

static bool authDllStillRequiresMandatoryUpdate(const QString &mainPath,
                                                const Updater::ArtifactInfo &artifact)
{
    if (artifact.fileName.isEmpty() || !artifact.required) {
        return false;
    }
    if (artifact.fileSize <= 0 ||
        (artifact.fileMd5.isEmpty() && artifact.fileSha256.isEmpty())) {
        return false;
    }
    return !fileMatchesArtifact(mainPath, artifact);
}

static QString installedUpdaterPath()
{
    const QString localDir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return QDir(localDir).absoluteFilePath(QStringLiteral("updater/NoteBotUpdater.exe"));
}

static QString injectorLocalDataDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

static QString injectorStateDir()
{
    return QDir(injectorLocalDataDir()).absoluteFilePath(QStringLiteral("state_v3"));
}

static QString injectorTicketPath()
{
    return QDir(injectorStateDir()).absoluteFilePath(QStringLiteral("inject_ticket_v3.dat"));
}

static QString injectorResultPath()
{
    return QDir(injectorStateDir()).absoluteFilePath(QStringLiteral("inject_result_v3.dat"));
}

static QString injectorGateLogPath()
{
    return QDir(injectorStateDir()).absoluteFilePath(QStringLiteral("overlay_gate_v3.log"));
}

static QString injectorUiLogPath()
{
    return QDir(injectorStateDir()).absoluteFilePath(QStringLiteral("injector_ui.log"));
}

static QString injectorBusinessDllPath(const QString &dllName)
{
    return QDir(injectorLocalDataDir()).absoluteFilePath(QStringLiteral("dlls_v3/%1").arg(dllName));
}

static QString injectorModelsCacheDir()
{
    return QDir(injectorLocalDataDir()).absoluteFilePath(QStringLiteral("models_v1"));
}

static QString injectorModelRuntimeDllPath()
{
    return QDir(injectorLocalDataDir())
        .absoluteFilePath(QStringLiteral("model_runtime_v1/NoteBotModel.dll"));
}

static QString injectorModelRuntimeCacheSubdir()
{
    return QStringLiteral("model_runtime_v1");
}

static QString activeModelSettingKey(const QString &licenseKey)
{
    const QString keyId = NBVmp_Injector_MakeLocalKeyId(licenseKey);
    return keyId.isEmpty()
        ? QStringLiteral("activeModelId")
        : QStringLiteral("activeModelId/%1").arg(keyId);
}

static QString normalizeModelArmSize(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("wide")
        ? QStringLiteral("wide")
        : QStringLiteral("slim");
}

static QString modelArmSizeLabel(const QString &value)
{
    return normalizeModelArmSize(value) == QStringLiteral("wide")
        ? QStringLiteral("Steve")
        : QStringLiteral("Alex");
}

static QString modelAssetCachePath(const QString &modelId,
                                   const QString &kind,
                                   const QString &sha256,
                                   const QString &fileName)
{
    const QString ext = QFileInfo(fileName).suffix().trimmed().toLower();
    const QString suffix = ext.isEmpty() ? QString() : QStringLiteral(".") + ext;
    return QDir(injectorModelsCacheDir())
        .absoluteFilePath(QStringLiteral("%1/%2_%3%4")
                              .arg(modelId, kind, sha256.toLower(), suffix));
}

struct RemoteModelAsset {
    QString fileName;
    QString sha256;
    qint64 size = 0;
    QUrl downloadUrl;
};

struct RemoteModelEntry {
    QString modelId;
    QString displayName;
    QString subtitle;
    QString entitlementState;
    QString stateLabel;
    int orderIndex = 0;
    RemoteModelAsset geometry;
    RemoteModelAsset texture;
    RemoteModelAsset cover;
    bool hasGeometry = false;
    bool hasTexture = false;
    bool hasCover = false;
};

static NB_NOINLINE bool NBVmp_Injector_ParseRemoteModelAsset(const QJsonObject &obj, RemoteModelAsset *asset)
{
    NB_VMP_VIRTUALIZE("NB.Injector.ParseRemoteModelAsset");
    if (!asset) {
        NB_VMP_END();
        return false;
    }
    asset->fileName = obj.value(QStringLiteral("file_name")).toString().trimmed();
    asset->sha256 = obj.value(QStringLiteral("sha256")).toString().trimmed().toLower();
    asset->size = obj.value(QStringLiteral("size")).toVariant().toLongLong();
    asset->downloadUrl = QUrl(obj.value(QStringLiteral("download_url")).toString().trimmed());
    const bool valid = !asset->fileName.isEmpty() &&
           !asset->sha256.isEmpty() &&
           asset->size > 0 &&
           asset->downloadUrl.isValid();
    NB_VMP_END();
    return valid;
}

static NB_NOINLINE bool NBVmp_Injector_ParseRemoteModelEntry(const QJsonObject &obj, RemoteModelEntry *entry)
{
    NB_VMP_VIRTUALIZE("NB.Injector.ParseRemoteModelEntry");
    if (!entry) {
        NB_VMP_END();
        return false;
    }
    entry->modelId = obj.value(QStringLiteral("model_id")).toString().trimmed();
    entry->displayName = obj.value(QStringLiteral("display_name")).toString().trimmed();
    entry->subtitle = obj.value(QStringLiteral("subtitle")).toString().trimmed();
    entry->entitlementState = obj.value(QStringLiteral("entitlement_state")).toString().trimmed().toLower();
    entry->stateLabel = obj.value(QStringLiteral("state_label")).toString().trimmed();
    entry->orderIndex = obj.value(QStringLiteral("order_index")).toInt(0);
    entry->hasGeometry =
        NBVmp_Injector_ParseRemoteModelAsset(obj.value(QStringLiteral("geometry_asset")).toObject(), &entry->geometry);
    entry->hasTexture =
        NBVmp_Injector_ParseRemoteModelAsset(obj.value(QStringLiteral("texture_asset")).toObject(), &entry->texture);
    entry->hasCover =
        NBVmp_Injector_ParseRemoteModelAsset(obj.value(QStringLiteral("cover_asset")).toObject(), &entry->cover);

    if (entry->modelId.isEmpty()) {
        NB_VMP_END();
        return false;
    }
    if (entry->entitlementState == QStringLiteral("owned")) {
        const bool valid = entry->hasGeometry && entry->hasTexture;
        NB_VMP_END();
        return valid;
    }
    if (entry->entitlementState == QStringLiteral("unowned")) {
        const bool valid = entry->hasCover;
        NB_VMP_END();
        return valid;
    }
    NB_VMP_END();
    return false;
}

static bool ensureParentDir(const QString &filePath)
{
    return QDir().mkpath(QFileInfo(filePath).absolutePath());
}

static bool fileMatchesSha256AndSize(const QString &path, const QString &sha256, qint64 size)
{
    QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return false;
    }
    if (size > 0 && info.size() != size) {
        return false;
    }
    const QString localSha256 = computeFileSha256(path);
    return !localSha256.isEmpty() && localSha256 == sha256.toLower();
}

static QPair<QString, QString> summarizeModelGeometry(const QString &modelPath)
{
    QFile file(modelPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {QStringLiteral("geometry"), QStringLiteral("未读到几何")};
    }

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (!doc.isObject()) {
        Q_UNUSED(parseError);
        return {QStringLiteral("geometry"), QStringLiteral("结构未识别")};
    }

    const QJsonArray geometries =
        doc.object().value(QStringLiteral("minecraft:geometry")).toArray();
    if (geometries.isEmpty()) {
        return {QStringLiteral("geometry"), QStringLiteral("几何为空")};
    }

    const QJsonArray bones =
        geometries.at(0).toObject().value(QStringLiteral("bones")).toArray();
    int cubeCount = 0;
    for (const QJsonValue &boneValue : bones) {
        cubeCount += boneValue.toObject().value(QStringLiteral("cubes")).toArray().size();
    }
    return {QStringLiteral("%1 bones").arg(bones.size()),
            QStringLiteral("%1 cubes").arg(cubeCount)};
}

static constexpr char kCurrentMainVersion[] = "3.6.76";
static constexpr int kCurrentMainVersionCode = 30676;
static constexpr char kCurrentAuthDllVersion[] = "3.5.51";
static constexpr int kCurrentAuthDllVersionCode = 30551;
static constexpr char kCurrentUpdaterVersion[] = "3.5.71";
static constexpr int kCurrentUpdaterVersionCode = 30571;

static QByteArray embeddedUpdaterBytes()
{
    QFile file(QStringLiteral(":/embedded/NoteBotUpdater.exe"));
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

/* ============================================================
 *  DLL 函数表
 * ============================================================ */
struct AuthDllFuncs {
    HMODULE hDll = nullptr;

    decltype(nb_init)*                fn_init = nullptr;
    decltype(nb_shutdown)*            fn_shutdown = nullptr;
    decltype(nb_set_log_callback)*    fn_set_log_callback = nullptr;
    decltype(nb_set_state_callback)*  fn_set_state_callback = nullptr;
    decltype(nb_set_key)*             fn_set_key = nullptr;
    decltype(nb_get_key)*             fn_get_key = nullptr;
    decltype(nb_download)*            fn_download = nullptr;
    decltype(nb_inject)*              fn_inject = nullptr;
    decltype(nb_inject_async)*        fn_inject_async = nullptr;
    decltype(nb_is_active)*           fn_is_active = nullptr;
    decltype(nb_get_tier)*            fn_get_tier = nullptr;
    decltype(nb_get_status)*          fn_get_status = nullptr;
    decltype(nb_get_expires)*         fn_get_expires = nullptr;
    decltype(nb_get_version)*         fn_get_version = nullptr;
    decltype(nb_call)*                fn_call = nullptr;
    decltype(nb_self_check)*          fn_self_check = nullptr;
    decltype(nb_get_protocol_version)* fn_get_protocol_version = nullptr;
    decltype(nb_get_abi_version)*      fn_get_abi_version = nullptr;

    bool resolve(HMODULE h)
    {
        hDll = h;
        auto getAndCheck = [&](const char* name, void** out) -> bool {
            *out = GetProcAddress(h, name);
            if (!*out) return false;
            return true;
        };
#define RESOLVE(name) if (!getAndCheck("nb_" #name, (void**)&fn_##name)) return false;
        RESOLVE(init)
        RESOLVE(shutdown)
        RESOLVE(set_log_callback)
        RESOLVE(set_state_callback)
        RESOLVE(set_key)
        RESOLVE(get_key)
        RESOLVE(download)
        RESOLVE(inject)
        RESOLVE(inject_async)
        RESOLVE(is_active)
        RESOLVE(get_tier)
        RESOLVE(get_status)
        RESOLVE(get_expires)
        RESOLVE(get_version)
        RESOLVE(self_check)
#undef RESOLVE
        fn_call = reinterpret_cast<decltype(nb_call)*>(GetProcAddress(h, "nb_call"));
        fn_get_protocol_version = reinterpret_cast<decltype(nb_get_protocol_version)*>(
            GetProcAddress(h, "nb_get_protocol_version"));
        fn_get_abi_version = reinterpret_cast<decltype(nb_get_abi_version)*>(
            GetProcAddress(h, "nb_get_abi_version"));
        return true;
    }
};

struct ModelDllFuncs {
    HMODULE hDll = nullptr;

    decltype(nbm_get_protocol_version)* fn_get_protocol_version = nullptr;
    decltype(nbm_get_abi_version)*      fn_get_abi_version = nullptr;
    decltype(nbm_init)*                 fn_init = nullptr;
    decltype(nbm_shutdown)*             fn_shutdown = nullptr;
    decltype(nbm_set_log_callback)*     fn_set_log_callback = nullptr;
    decltype(nbm_set_state_callback)*   fn_set_state_callback = nullptr;
    decltype(nbm_start_wait)*           fn_start_wait = nullptr;
    decltype(nbm_stop_wait)*            fn_stop_wait = nullptr;
    decltype(nbm_is_running)*           fn_is_running = nullptr;
    decltype(nbm_is_waiting_for_process)* fn_is_waiting_for_process = nullptr;

    bool resolve(HMODULE h)
    {
        hDll = h;
        auto getAndCheck = [&](const char *name, void **out) -> bool {
            *out = GetProcAddress(h, name);
            return *out != nullptr;
        };
#define RESOLVE_MODEL(name) if (!getAndCheck("nbm_" #name, (void**)&fn_##name)) return false;
        RESOLVE_MODEL(get_protocol_version)
        RESOLVE_MODEL(get_abi_version)
        RESOLVE_MODEL(init)
        RESOLVE_MODEL(shutdown)
        RESOLVE_MODEL(set_log_callback)
        RESOLVE_MODEL(set_state_callback)
        RESOLVE_MODEL(start_wait)
        RESOLVE_MODEL(stop_wait)
        RESOLVE_MODEL(is_running)
        RESOLVE_MODEL(is_waiting_for_process)
#undef RESOLVE_MODEL
        return true;
    }
};

static QString g_dllLogBuffer;
static Backend *s_backendInstance = nullptr;

static QJsonObject parseDllJsonObject(const QString &json)
{
    QJsonParseError err{};
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return QJsonObject{};
    }
    return doc.object();
}

static QJsonObject extractStatusSnapshot(const QJsonObject &resp)
{
    const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
    const QJsonObject snap = data.value(QStringLiteral("status_snapshot")).toObject();
    if (!snap.isEmpty()) {
        return snap;
    }
    return resp.value(QStringLiteral("status_snapshot")).toObject();
}

static QString describeV3Rc(int rc, const QString &fallbackMessage)
{
    if (!fallbackMessage.trimmed().isEmpty()) {
        return fallbackMessage.trimmed();
    }

    switch (rc) {
    case 0: return QStringLiteral("操作成功");
    case 1001: return QStringLiteral("本地参数无效");
    case 1002: return QStringLiteral("缺少本地 V3 状态文件");
    case 1003: return QStringLiteral("本地 V3 状态文件损坏");
    case 1004: return QStringLiteral("本地 DPAPI 解密失败");
    case 1005: return QStringLiteral("设备私钥缺失");
    case 1006: return QStringLiteral("无法连接服务器");
    case 1007: return QStringLiteral("服务端拒绝请求");
    case 1008: return QStringLiteral("当前设备已被顶下线");
    case 1009: return QStringLiteral("设备替换冷却中");
    case 1010: return QStringLiteral("心跳失败");
    default: return QStringLiteral("未完成的 V3 操作");
    }
}

static void s_dllLogCallback(const char *msg)
{
    g_dllLogBuffer = QString::fromUtf8(msg);
    if (s_backendInstance) {
        QString m = QString::fromUtf8(msg);
        QMetaObject::invokeMethod(s_backendInstance, [m]() {
            if (s_backendInstance) {
                emit s_backendInstance->_logSignal(m);
            }
        }, Qt::QueuedConnection);
    }
}

static void s_dllStateCallback(const char *key, const char *value)
{
    if (s_backendInstance) {
        QString k = QString::fromUtf8(key);
        QString v = QString::fromUtf8(value);
        QMetaObject::invokeMethod(s_backendInstance, [k, v]() {
            if (s_backendInstance) {
                s_backendInstance->onDllStateChanged(k, v);
            }
        }, Qt::QueuedConnection);
    }
}

/* ============================================================
 *  Backend 实现
 * ============================================================ */
Backend::Backend(ProcessModel *procModel,
                 LogModel *logModel,
                 ModelCatalogModel *modelCatalogModel,
                 QObject *parent)
    : QObject(parent)
    , m_procModel(procModel)
    , m_logModel(logModel)
    , m_modelCatalogModel(modelCatalogModel)
{
    qInfo() << "Backend constructor: connecting signals...";
    s_backendInstance = this;
    connect(this, &Backend::_logSignal, this, &Backend::onLogMsg, Qt::QueuedConnection);

    m_scanTimer = new QTimer(this);
    connect(m_scanTimer, &QTimer::timeout, this, &Backend::autoScan);

    // 从 QSettings 读取上次保存的密钥
    QSettings settings("NoteBot", "Injector");
    m_licenseKey = settings.value("licenseKey").toString();
    m_modelModificationEnabled = settings.value(QStringLiteral("modelModificationEnabled"), false).toBool();
    m_modelArmOverrideEnabled = false;
    settings.remove(QStringLiteral("modelArmOverrideEnabled"));
    m_modelArmSize = normalizeModelArmSize(settings.value(QStringLiteral("modelArmSize"), QStringLiteral("slim")).toString());
    m_skinPngPath = settings.value(QStringLiteral("skinPngPath")).toString();
    m_classicModeEnabled = settings.value(QStringLiteral("classicModeEnabled"), false).toBool();
    m_modelReplacementStatus = modelRuntimeRequested()
        ? QStringLiteral("等待密钥验证")
        : QStringLiteral("已关闭");
    m_initStatus = "准备启动...";
    m_initStep = 3;
    qInfo() << "Backend constructor: done";
}

bool Backend::initialize()
{
    // 保留旧接口兼容，只做同步 DLL 加载。
    return loadAuthDll();
}

void Backend::initializeAsync()
{
    qInfo() << "Backend::initializeAsync() started";

    m_initializing = true;
    emit initializingChanged();
    setInitStatus("准备检查更新...", 5);
    // 让 QML 先完成首帧渲染，再开始可能较慢的网络检查。
    QTimer::singleShot(60, this, [this]() {
        setInitStatus("检查 DLL 更新...", 15);
        checkDllUpdateAsync();
    });
}

void Backend::finishInitialization()
{
    setInitStatus("加载授权模块...", 55);

    if (m_funcs) {
        unloadAuthDll();
    }

    if (!loadAuthDll()) {
        setInitStatus("授权模块加载失败", 0);
        m_initializing = false;
        emit initializingChanged();
        emit initializationFailed("未找到 NoteBotAuth.dll，程序无法运行");
        return;
    }

    setInitStatus("读取本地状态...", 60);
    syncHostUpdateSnapshot(m_hostUpdateState, false);
    syncStatusFromDll();
    emit licenseStatusChanged();
    emit licenseKeyChanged();
    if (m_injectProgress != -1) {
        m_injectProgress = -1;
        emit injectProgressChanged();
    }
    if (!m_injectStageText.isEmpty()) {
        m_injectStageText.clear();
        emit injectStageTextChanged();
    }
    m_injectRunning = false;

    // 已有激活快照时，启动后自动续一次当前会话验证，
    // 避免主界面显示“已激活”但注入按钮仍因未验证而灰着。
    if (m_isActivated && !m_licenseKey.trimmed().isEmpty() && !m_authSessionVerified) {
        QTimer::singleShot(0, this, [this]() {
            verifyLicense();
        });
    }

    setInitStatus("就绪", 100);
    m_downloadProgress = -1;
    emit downloadProgressChanged();
    m_initializing = false;
    emit initializingChanged();

    emit initializationFinished();
}

bool Backend::authActionsBlocked(QString *reason) const
{
    if (m_authUpdateRequired) {
        if (reason) {
            *reason = QStringLiteral("Auth DLL 存在必需更新，当前仅允许进入最小壳");
        }
        return true;
    }
    return false;
}

bool Backend::injectBlocked(QString *reason) const
{
    if (m_licenseKey.trimmed().isEmpty()) {
        if (reason) {
            *reason = QStringLiteral("请先输入密钥并完成检查/激活");
        }
        return true;
    }
    if (!m_authSessionVerified) {
        if (reason) {
            *reason = QStringLiteral("本次会话尚未完成密钥检查，当前禁止注入");
        }
        return true;
    }
    if (m_authUpdateRequired) {
        if (reason) {
            *reason = QStringLiteral("Auth DLL 存在必需更新，当前禁止注入");
        }
        return true;
    }
    return false;
}

void Backend::setAuthSessionVerified(bool verified)
{
    if (m_authSessionVerified == verified) {
        return;
    }
    m_authSessionVerified = verified;
    emit authSessionVerifiedChanged();
    if (verified) {
        QTimer::singleShot(0, this, [this]() {
            refreshModelRuntimeAsync();
            refreshModelEntitlementsAsync();
        });
    } else {
        disableModelRuntimeAndRemoveLocal(QStringLiteral("等待密钥验证"), true);
        clearModelCatalog();
    }
}

static void s_modelLogCallback(const char *msg)
{
    if (s_backendInstance) {
        QString m = sanitizeModelLogMessage(QString::fromUtf8(msg));
        if (m.isEmpty()) {
            return;
        }
        QMetaObject::invokeMethod(s_backendInstance, [m]() {
            if (s_backendInstance) {
                emit s_backendInstance->_logSignal(m);
            }
        }, Qt::QueuedConnection);
    }
}

static void s_modelStateCallback(const char *key, const char *value)
{
    if (s_backendInstance) {
        QString k = QString::fromUtf8(key);
        QString v = QString::fromUtf8(value);
        QMetaObject::invokeMethod(s_backendInstance, [k, v]() {
            if (s_backendInstance) {
                s_backendInstance->handleModelDllState(k, v);
            }
        }, Qt::QueuedConnection);
    }
}

void Backend::clearModelCatalog()
{
    if (m_modelCatalogModel) {
        m_modelCatalogModel->clear();
    }
}

void Backend::activateModel(const QString &modelId)
{
    const QString trimmed = modelId.trimmed();
    if (trimmed.isEmpty() || !m_modelCatalogModel) {
        return;
    }
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(activeModelSettingKey(m_licenseKey), trimmed);
    m_activeModelId = trimmed;
    m_modelCatalogModel->setActiveModelId(trimmed);
    m_logModel->append(QStringLiteral("[MODEL] 当前激活模型：%1").arg(trimmed));
    if (m_modelModificationEnabled && m_authSessionVerified) {
        requestModelReplacementRestart();
    }
}

void Backend::setModelModificationEnabled(bool enabled)
{
    if (enabled && !m_modelRuntimeAvailable) {
        setModelReplacementStatus(QStringLiteral("不可用"));
        m_logModel->append(QStringLiteral("[MODEL] 模型替换运行时尚未授权或下载完成"));
        return;
    }
    if (m_modelModificationEnabled == enabled) {
        return;
    }
    m_modelModificationEnabled = enabled;
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(QStringLiteral("modelModificationEnabled"), enabled);
    emit modelModificationEnabledChanged();

    if (!enabled) {
        if (modelRuntimeRequested()) {
            requestModelReplacementRestart();
            setModelReplacementStatus(QStringLiteral("正在运行"));
        } else {
            stopModelReplacementWait();
            setModelReplacementStatus(QStringLiteral("已关闭"));
        }
        m_logModel->append(QStringLiteral("[MODEL] 启动阶段模型替换已关闭"));
    } else {
        setModelReplacementStatus(m_authSessionVerified
            ? QStringLiteral("等待模型资源")
            : QStringLiteral("等待密钥验证"));
        m_logModel->append(QStringLiteral("[MODEL] 启动阶段模型替换已启用，将自动等待新的 Minecraft 进程"));
        if (m_authSessionVerified) {
            QTimer::singleShot(0, this, [this]() {
                startModelReplacementWait();
            });
        }
    }
}

void Backend::setModelArmOverrideEnabled(bool enabled)
{
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.remove(QStringLiteral("modelArmOverrideEnabled"));
    Q_UNUSED(enabled);
    if (!m_modelArmOverrideEnabled) {
        return;
    }
    m_modelArmOverrideEnabled = false;
    emit modelArmOverrideEnabledChanged();
}

void Backend::setModelArmSize(const QString &size)
{
    const QString normalized = normalizeModelArmSize(size);
    if (m_modelArmSize == normalized) {
        return;
    }
    m_modelArmSize = normalized;
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(QStringLiteral("modelArmSize"), normalized);
    emit modelArmSizeChanged();

    if (m_modelModificationEnabled && m_classicModeEnabled) {
        m_logModel->append(QStringLiteral("[MODEL] 经典方块人手臂切换为：%1")
                               .arg(modelArmSizeLabel(m_modelArmSize)));
        requestModelReplacementRestart();
    }
}

void Backend::startModelReplacementWait()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (!m_modelRuntimeAvailable) {
        setModelReplacementStatus(QStringLiteral("不可用"));
        return;
    }
    if (!QFile::exists(injectorModelRuntimeDllPath())) {
        setModelRuntimeAvailable(false);
        setModelReplacementStatus(QStringLiteral("不可用"));
        return;
    }
    if (!modelRuntimeRequested()) {
        m_logModel->append(QStringLiteral("[MODEL] 模型运行时已关闭"));
        setModelReplacementStatus(QStringLiteral("已关闭"));
        return;
    }
    if (!m_authSessionVerified) {
        m_logModel->append(QStringLiteral("[MODEL] 等待密钥验证后自动开始模型替换"));
        setModelReplacementStatus(QStringLiteral("等待密钥验证"));
        return;
    }
    if (m_modelReplacementRunning) {
        m_logModel->append(QStringLiteral("[MODEL] 当前已经在等待下次启动"));
        return;
    }
    ModelCatalogEntry active;
    const bool classicSkin = m_modelModificationEnabled && m_classicModeEnabled;
    const QString arm = effectiveModelArmSize();
    if (m_modelModificationEnabled) {
        if (classicSkin) {
            // Classic square skin: use hardcoded vanilla geometry + user PNG
            if (m_skinPngPath.isEmpty()) {
                m_logModel->append(QStringLiteral("[MODEL] 经典方块人覆盖已开启，请先选择皮肤 PNG"));
                setModelReplacementStatus(QStringLiteral("等待选择皮肤文件"));
                return;
            }
            QImage png(m_skinPngPath);
            if (png.isNull()) {
                m_logModel->append(QStringLiteral("[MODEL] 皮肤 PNG 无法读取，请重新选择"));
                setModelReplacementStatus(QStringLiteral("等待选择皮肤文件"));
                return;
            }
            const QSize size = png.size();
            if (!(size == QSize(64, 64) || size == QSize(128, 128) || size == QSize(256, 256))) {
                m_logModel->append(QStringLiteral("[MODEL] 皮肤 PNG 必须是 64/128/256 像素，当前 %1x%2")
                                       .arg(size.width()).arg(size.height()));
                setModelReplacementStatus(QStringLiteral("等待选择皮肤文件"));
                return;
            }
            m_skinPngWidth = size.width();
            m_skinPngHeight = size.height();
            emit skinPngInfoChanged();
            const QString geomPath = classicGeometryTempPath();
            if (!writeClassicGeometry(geomPath, arm)) {
                m_logModel->append(QStringLiteral("[MODEL] 无法生成方块人几何文件"));
                setModelReplacementStatus(QStringLiteral("失败"));
                return;
            }
            active.modelFile = geomPath;
            active.textureFile = m_skinPngPath;
            active.name = QStringLiteral("经典方块人 %1").arg(modelArmSizeLabel(arm));
        } else {
            // Server model mode: requires ModelCatalogModel
            if (!m_modelCatalogModel) {
                m_logModel->append(QStringLiteral("[MODEL] 模型列表不可用"));
                setModelReplacementStatus(QStringLiteral("失败"));
                return;
            }
            if (!m_modelCatalogModel->activeEntry(&active)) {
                m_logModel->append(QStringLiteral("[MODEL] 请选择一个已拥有的模型，总闸保持开启后会自动等待"));
                setModelReplacementStatus(QStringLiteral("等待选择模型"));
                return;
            }
            if (active.modelFile.isEmpty() || active.textureFile.isEmpty() ||
                !QFile::exists(active.modelFile) || !QFile::exists(active.textureFile)) {
                m_logModel->append(QStringLiteral("[MODEL] 当前模型资源缺失，请重新检查密钥或刷新模型列表"));
                setModelReplacementStatus(QStringLiteral("等待模型资源"));
                return;
            }
        }
    }
    if (!loadModelDll()) {
        return;
    }

    QJsonObject cfg;
    cfg[QStringLiteral("model_enabled")] = m_modelModificationEnabled;
    cfg[QStringLiteral("arm_override_enabled")] = m_modelModificationEnabled && m_classicModeEnabled;
    cfg[QStringLiteral("arm_size")] = effectiveModelArmSize();
    if (m_modelModificationEnabled) {
        cfg[QStringLiteral("geometry_path")] = active.modelFile;
        cfg[QStringLiteral("texture_path")] = active.textureFile;
        cfg[QStringLiteral("skin_id")] =
            QStringLiteral("c18e65aa-7b21-4637-9b63-8ad63622ef01.CustomSlimaf8fd34e3bc6df55dfee6dd80d80a1bb");
        cfg[QStringLiteral("trusted")] = true;
        cfg[QStringLiteral("premium")] = true;
        cfg[QStringLiteral("persona")] = true;
    }
    cfg[QStringLiteral("process_timeout_ms")] = 86400 * 1000;
    cfg[QStringLiteral("hook_timeout_ms")] = 90000;
    cfg[QStringLiteral("hit_timeout_ms")] = 45000;

    const QByteArray json = QJsonDocument(cfg).toJson(QJsonDocument::Compact);
    const int rc = m_modelFuncs->fn_start_wait(json.constData());
    if (rc == 0) {
        m_modelReplacementRestartPending = false;
        setModelReplacementRunning(true);
        setModelReplacementStatus(QStringLiteral("正在运行"));
        m_logModel->append(QStringLiteral("[MODEL] 开始等待下次 Minecraft 启动：%1").arg(active.name));
    } else if (rc == 2) {
        setModelReplacementRunning(true);
        m_logModel->append(QStringLiteral("[MODEL] 当前已经在等待下次启动"));
    } else {
        setModelReplacementRunning(false);
        setModelReplacementStatus(QStringLiteral("失败"));
        m_logModel->append(QStringLiteral("[MODEL] 等待启动失败，错误码=%1").arg(rc));
    }
}

void Backend::stopModelReplacementWait()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    m_modelReplacementRestartPending = false;
    if (m_modelFuncs && !m_hModelDll) {
        m_modelFuncs = nullptr;
    }
    if (m_modelFuncs && m_hModelDll && m_modelFuncs->fn_stop_wait) {
        m_modelFuncs->fn_stop_wait();
    }
    if (m_modelReplacementRunning) {
        setModelReplacementStatus(QStringLiteral("停止中"));
        m_logModel->append(QStringLiteral("[MODEL] 已请求停止等待"));
    } else if (modelRuntimeRequested()) {
        setModelReplacementStatus(m_authSessionVerified
            ? QStringLiteral("等待选择模型")
            : QStringLiteral("等待密钥验证"));
    } else {
        setModelReplacementStatus(QStringLiteral("已关闭"));
    }
}

void Backend::refreshModelEntitlementsAsync()
{
    if (!m_modelCatalogModel || !m_authSessionVerified || !m_funcs) {
        clearModelCatalog();
        return;
    }

    const QString currentLicenseKey = m_licenseKey;

    std::thread([this, currentLicenseKey]() {
        const QJsonObject resp =
            parseDllJsonObject(callDll(QStringLiteral("model_entitlements_v1")));
        const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString message = resp.value(QStringLiteral("message")).toString();
        if (rc != NB_OK) {
            logFromThread(QStringLiteral("[MODEL] 模型授权获取失败：%1")
                              .arg(describeV3Rc(rc, message)));
            QMetaObject::invokeMethod(this, [this]() {
                clearModelCatalog();
            }, Qt::QueuedConnection);
            return;
        }

        const QJsonArray models =
            resp.value(QStringLiteral("data")).toObject().value(QStringLiteral("models")).toArray();
        QVector<ModelCatalogEntry> entries;
        entries.reserve(models.size());
        QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
        const QString activeModelId =
            settings.value(activeModelSettingKey(currentLicenseKey)).toString().trimmed();

        Updater updater;
        updater.setLogCallback([this](const QString &msg) {
            if (msg.contains(QStringLiteral("[ERR]"))) {
                logFromThread(QStringLiteral("[MODEL] %1").arg(msg));
            }
        });

        for (const QJsonValue &value : models) {
            RemoteModelEntry remote;
            if (!NBVmp_Injector_ParseRemoteModelEntry(value.toObject(), &remote)) {
                continue;
            }

            auto ensureAssetReady = [this, &updater, &remote](
                                        const QString &kind,
                                        const RemoteModelAsset &asset,
                                        QString *readyPath) -> bool {
                const QString finalPath =
                    modelAssetCachePath(remote.modelId, kind, asset.sha256, asset.fileName);
                if (fileMatchesSha256AndSize(finalPath, asset.sha256, asset.size)) {
                    *readyPath = finalPath;
                    return true;
                }

                Updater::ArtifactInfo artifact;
                artifact.artifactType = kind == QStringLiteral("geometry")
                    ? QStringLiteral("model_geometry")
                    : QStringLiteral("model_texture");
                artifact.channel = QStringLiteral("stable");
                artifact.version = asset.sha256.left(12);
                artifact.fileName = asset.fileName;
                artifact.fileSha256 = asset.sha256;
                artifact.fileSize = asset.size;
                artifact.downloadUrl = asset.downloadUrl;

                logFromThread(QStringLiteral("[MODEL] 下载模型资源：%1")
                                  .arg(displayNameOnly(asset.fileName)));
                const QString cachedPath =
                    updater.downloadArtifact(artifact,
                                             QStringLiteral("models/%1").arg(remote.modelId));
                if (cachedPath.isEmpty() || !ensureParentDir(finalPath)) {
                    return false;
                }
                QFile::remove(finalPath);
                if (!QFile::copy(cachedPath, finalPath)) {
                    return false;
                }
                if (!fileMatchesSha256AndSize(finalPath, asset.sha256, asset.size)) {
                    QFile::remove(finalPath);
                    return false;
                }
                *readyPath = finalPath;
                return true;
            };

            QString geometryPath;
            QString texturePath;
            QString coverPath;
            const bool owned = remote.entitlementState == QStringLiteral("owned");

            if (owned) {
                if (!ensureAssetReady(QStringLiteral("geometry"), remote.geometry, &geometryPath) ||
                    !ensureAssetReady(QStringLiteral("texture"), remote.texture, &texturePath)) {
                    logFromThread(QStringLiteral("[MODEL] 资源准备失败：%1")
                                      .arg(remote.displayName.isEmpty() ? remote.modelId
                                                                        : remote.displayName));
                    continue;
                }
                if (remote.hasCover) {
                    ensureAssetReady(QStringLiteral("cover"), remote.cover, &coverPath);
                }
            } else {
                if (!ensureAssetReady(QStringLiteral("cover"), remote.cover, &coverPath)) {
                    logFromThread(QStringLiteral("[MODEL] 封面准备失败：%1")
                                      .arg(remote.displayName.isEmpty() ? remote.modelId
                                                                        : remote.displayName));
                    continue;
                }
            }

            ModelCatalogEntry entry;
            entry.modelId = remote.modelId;
            entry.name = remote.displayName.isEmpty() ? remote.modelId : remote.displayName;
            entry.subtitle = remote.subtitle;
            entry.owned = owned;
            entry.active = owned && !activeModelId.isEmpty() && activeModelId == remote.modelId;
            entry.stateCode = entry.active
                ? QStringLiteral("active")
                : owned
                  ? QStringLiteral("owned")
                  : QStringLiteral("unowned");
            entry.stateLabel = entry.active
                ? QStringLiteral("已激活")
                : !remote.stateLabel.isEmpty()
                  ? remote.stateLabel
                  : owned
                    ? QStringLiteral("已拥有")
                    : QStringLiteral("未拥有");
            entry.modelFile = geometryPath;
            entry.textureFile = texturePath;
            entry.coverFile = coverPath;
            entry.startYaw = 180.0;
            if (owned) {
                const QPair<QString, QString> summary = summarizeModelGeometry(geometryPath);
                entry.footerLeft = summary.first;
                entry.footerRight = summary.second;
            } else {
                entry.footerLeft = QStringLiteral("cover");
                entry.footerRight = QStringLiteral("preview");
            }
            entries.push_back(entry);
        }

        QMetaObject::invokeMethod(this, [this, entries]() {
            if (!m_modelCatalogModel) {
                return;
            }
            m_modelCatalogModel->setEntries(entries);
            if (entries.isEmpty()) {
                m_logModel->append(QStringLiteral("[MODEL] 当前密钥未分配可用模型"));
            } else {
                m_logModel->append(QStringLiteral("[MODEL] 模型列表已刷新：%1 个")
                                       .arg(entries.size()));
            }
            if (modelRuntimeRequested() && m_modelRuntimeAvailable) {
                startModelReplacementWait();
            }
        }, Qt::QueuedConnection);
    }).detach();
}

void Backend::refreshModelRuntimeAsync()
{
    if (!m_authSessionVerified || !m_funcs) {
        disableModelRuntimeAndRemoveLocal(QStringLiteral("等待密钥验证"), true);
        return;
    }

    std::thread([this]() {
        const QString runtimePath = injectorModelRuntimeDllPath();
        const QString currentSha256 = QFile::exists(runtimePath)
            ? computeFileSha256(runtimePath)
            : QString();

        auto markRuntimeUnavailable = [this](
                                          const QString &status = QStringLiteral("不可用"),
                                          bool removeLocalDll = false) {
            QMetaObject::invokeMethod(this, [this, status, removeLocalDll]() {
                disableModelRuntimeAndRemoveLocal(status, removeLocalDll);
            }, Qt::QueuedConnection);
        };

        QJsonObject req;
        req[QStringLiteral("current_runtime_sha256")] = currentSha256;
        const QJsonObject resp =
            parseDllJsonObject(callDll(QStringLiteral("model_runtime_policy_v1"),
                QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact))));

        const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString message = resp.value(QStringLiteral("message")).toString();
        if (rc != NB_OK) {
            logFromThread(QStringLiteral("[MODEL] 模型替换运行时暂不可用：%1")
                              .arg(describeV3Rc(rc, message)));
            markRuntimeUnavailable();
            return;
        }

        const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
        if (!data.value(QStringLiteral("runtime_enabled")).toBool(false)) {
            markRuntimeUnavailable(QStringLiteral("未授权"), true);
            return;
        }

        Updater::ArtifactInfo artifact;
        artifact.artifactType = QStringLiteral("model_runtime");
        artifact.channel = data.value(QStringLiteral("channel")).toString(QStringLiteral("stable"));
        artifact.version = data.value(QStringLiteral("dll_sha256")).toString().left(12);
        artifact.fileName = data.value(QStringLiteral("dll_name")).toString().trimmed();
        artifact.fileSha256 =
            data.value(QStringLiteral("dll_sha256")).toString().trimmed().toLower();
        artifact.fileMd5 = data.value(QStringLiteral("dll_md5")).toString().trimmed().toLower();
        artifact.fileSize = data.value(QStringLiteral("dll_size")).toVariant().toLongLong();
        artifact.downloadUrl = QUrl(data.value(QStringLiteral("download_url")).toString().trimmed());
        const int expiresIn = data.value(QStringLiteral("expires_in")).toInt(0);
        const int runtimeProtocol = data.value(QStringLiteral("runtime_protocol")).toInt(1);
        const int runtimeAbi = data.value(QStringLiteral("runtime_abi")).toInt(2);
        const bool runtimeCurrent = data.value(QStringLiteral("runtime_current")).toBool(false);

        if (artifact.fileSha256.isEmpty() ||
            runtimeProtocol != kExpectedModelProtocolVersion ||
            runtimeAbi != kExpectedModelAbiVersion ||
            (!runtimeCurrent && (artifact.fileName.isEmpty() ||
                                 artifact.fileSize <= 0 ||
                                 !artifact.downloadUrl.isValid()))) {
            logFromThread(QStringLiteral("[MODEL] 模型替换运行时策略无效"));
            markRuntimeUnavailable();
            return;
        }

        bool ready = runtimeCurrent
            ? fileMatchesSha256AndSize(runtimePath, artifact.fileSha256, 0)
            : fileMatchesSha256AndSize(runtimePath, artifact.fileSha256, artifact.fileSize);
        if (!ready) {
            if (runtimeCurrent) {
                logFromThread(QStringLiteral("[MODEL] 本地模型替换运行时缺失或校验失败"));
                markRuntimeUnavailable();
                return;
            }

            Updater updater;
            updater.setLogCallback([this](const QString &msg) {
                if (msg.contains(QStringLiteral("[ERR]"))) {
                    logFromThread(QStringLiteral("[MODEL] %1").arg(msg));
                }
            });
            logFromThread(QStringLiteral("[MODEL] 正在准备模型替换运行时"));
            const QString cachedPath =
                updater.downloadArtifact(artifact, injectorModelRuntimeCacheSubdir(), expiresIn);
            if (cachedPath.isEmpty() || !ensureParentDir(runtimePath)) {
                logFromThread(QStringLiteral("[MODEL] 模型替换运行时下载失败"));
                markRuntimeUnavailable();
                return;
            }
            QFile::remove(runtimePath);
            if (!QFile::copy(cachedPath, runtimePath)) {
                logFromThread(QStringLiteral("[MODEL] 模型替换运行时写入失败"));
                markRuntimeUnavailable();
                return;
            }
            ready = fileMatchesSha256AndSize(runtimePath, artifact.fileSha256, artifact.fileSize);
            if (!ready) {
                QFile::remove(runtimePath);
                logFromThread(QStringLiteral("[MODEL] 模型替换运行时校验失败"));
                markRuntimeUnavailable();
                return;
            }
        }

        QMetaObject::invokeMethod(this, [this]() {
            setModelRuntimeAvailable(true);
            if (modelRuntimeRequested()) {
                setModelReplacementStatus(m_modelModificationEnabled
                    ? QStringLiteral("等待模型资源")
                    : QStringLiteral("等待密钥验证"));
                if (m_authSessionVerified) {
                    startModelReplacementWait();
                }
            }
        }, Qt::QueuedConnection);
    }).detach();
}

Backend::~Backend()
{
    if (s_backendInstance == this) {
        s_backendInstance = nullptr;
    }
    unloadModelDll();
    unloadAuthDll();
}

bool Backend::loadAuthDll()
{
    if (m_funcs) return true;

    QString localDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString dllPath = QDir(localDir).absoluteFilePath("NoteBotAuth.dll");

    if (!QFile::exists(dllPath)) {
        m_logModel->append("[ERR] 未找到已更新的 NoteBotAuth.dll，停止启动");
        QString msg = "未找到 NoteBotAuth.dll。\n"
                      "\n程序只允许加载更新器准备好的 DLL：\n"
                      "%LOCALAPPDATA%\\NoteBotInjector\\NoteBotAuth.dll\n"
                      "\n当前不会从 EXE 同级目录降级加载。";
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(msg.utf16()),
            L"NoteBot - DLL 缺失",
            MB_OK | MB_ICONERROR);
        return false;
    }

    m_logModel->append("[DLL] 正在加载授权模块");

    HMODULE h = LoadLibraryW(reinterpret_cast<const wchar_t*>(dllPath.utf16()));
    if (!h) {
        DWORD err = GetLastError();
        m_logModel->append(QString("[ERR] LoadLibrary 失败: %1").arg(err));
        QString msg = QString("加载 NoteBotAuth.dll 失败。\n"
                              "错误码: %1\n"
                              "文件: %2\n"
                              "\n常见原因：\n"
                              "- DLL 位数与 EXE 不匹配（x64 vs x86）\n"
                              "- DLL 依赖缺失（Qt 运行时等）\n"
                              "- DLL 文件损坏").arg(err).arg(displayNameOnly(dllPath));
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(msg.utf16()),
            L"NoteBot - DLL 加载失败",
            MB_OK | MB_ICONERROR);
        return false;
    }

    m_funcs = new AuthDllFuncs();
    if (!m_funcs->resolve(h)) {
        m_logModel->append("[ERR] DLL 导出函数解析失败");
        QString msg = "NoteBotAuth.dll 导出函数解析失败。\n"
                      "\n可能原因：\n"
                      "- DLL 版本太旧，缺少新接口\n"
                      "- DLL 不是 NoteBot 授权模块\n"
                      "\n请使用最新版本的 DLL。";
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(msg.utf16()),
            L"NoteBot - DLL 不兼容",
            MB_OK | MB_ICONERROR);
        delete m_funcs;
        m_funcs = nullptr;
        FreeLibrary(h);
        return false;
    }

    if (m_funcs->fn_self_check && m_funcs->fn_self_check() != NB_OK) {
        m_logModel->append("[ERR] Auth DLL 自校验失败");
        QString msg = "NoteBotAuth.dll 自校验失败，已拒绝加载。\n"
                      "\n可能原因：\n"
                      "- DLL 被篡改\n"
                      "- DLL 文件损坏\n"
                      "- 当前产物与发布内容不一致";
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(msg.utf16()),
            L"NoteBot - DLL 自检失败",
            MB_OK | MB_ICONERROR);
        delete m_funcs;
        m_funcs = nullptr;
        FreeLibrary(h);
        return false;
    }

    if (!m_funcs->fn_get_protocol_version ||
        !m_funcs->fn_get_abi_version ||
        m_funcs->fn_get_protocol_version() != kExpectedAuthProtocolVersion ||
        m_funcs->fn_get_abi_version() != kExpectedAuthAbiVersion) {
        m_logModel->append("[ERR] Auth DLL 协议版本不兼容");
        QString msg = QString("NoteBotAuth.dll 协议版本不兼容。\n"
                              "\n期望协议: %1 / ABI: %2\n"
                              "当前 DLL: 协议 %3 / ABI %4")
                          .arg(kExpectedAuthProtocolVersion)
                          .arg(kExpectedAuthAbiVersion)
                          .arg(m_funcs->fn_get_protocol_version
                                   ? m_funcs->fn_get_protocol_version()
                                   : -1)
                          .arg(m_funcs->fn_get_abi_version
                                   ? m_funcs->fn_get_abi_version()
                                   : -1);
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(msg.utf16()),
            L"NoteBot - DLL 协议不兼容",
            MB_OK | MB_ICONERROR);
        delete m_funcs;
        m_funcs = nullptr;
        FreeLibrary(h);
        return false;
    }

    m_funcs->fn_set_log_callback(s_dllLogCallback);
    m_funcs->fn_set_state_callback(s_dllStateCallback);
    m_funcs->fn_init();

    // 如果之前有保存的密钥，同步到 DLL
    if (!m_licenseKey.isEmpty()) {
        m_funcs->fn_set_key(m_licenseKey.toUtf8().constData());
    }

    m_logModel->append("[DLL] 授权模块加载成功");
    return true;
}

void Backend::unloadAuthDll()
{
    if (!m_funcs) return;
    if (m_funcs->fn_shutdown) m_funcs->fn_shutdown();
    HMODULE h = m_funcs->hDll;
    delete m_funcs;
    m_funcs = nullptr;
    FreeLibrary(h);
}

bool Backend::loadModelDll()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (m_modelFuncs && m_hModelDll) {
        return true;
    }
    if (m_modelFuncs && !m_hModelDll) {
        m_modelFuncs = nullptr;
    }

    const QString dllPath = injectorModelRuntimeDllPath();
    if (!QFile::exists(dllPath)) {
        setModelRuntimeAvailable(false);
        m_logModel->append(QStringLiteral("[MODEL] 模型替换运行时未准备好"));
        setModelReplacementStatus(QStringLiteral("失败"));
        return false;
    }

    HMODULE h = LoadLibraryW(reinterpret_cast<const wchar_t*>(dllPath.utf16()));
    if (!h) {
        const DWORD err = GetLastError();
        m_logModel->append(QStringLiteral("[MODEL] NoteBotModel.dll 加载失败：%1").arg(err));
        setModelReplacementStatus(QStringLiteral("失败"));
        return false;
    }

    m_modelFuncs = new ModelDllFuncs();
    m_hModelDll = h;
    if (!m_modelFuncs->resolve(h)) {
        m_logModel->append(QStringLiteral("[MODEL] NoteBotModel.dll 导出函数解析失败"));
        delete m_modelFuncs;
        m_modelFuncs = nullptr;
        m_hModelDll = nullptr;
        FreeLibrary(h);
        setModelReplacementStatus(QStringLiteral("失败"));
        return false;
    }
    if (m_modelFuncs->fn_get_protocol_version() != kExpectedModelProtocolVersion ||
        m_modelFuncs->fn_get_abi_version() != kExpectedModelAbiVersion) {
        m_logModel->append(QStringLiteral("[MODEL] NoteBotModel.dll 协议版本不兼容"));
        delete m_modelFuncs;
        m_modelFuncs = nullptr;
        m_hModelDll = nullptr;
        FreeLibrary(h);
        setModelReplacementStatus(QStringLiteral("失败"));
        return false;
    }
    m_modelFuncs->fn_set_log_callback(s_modelLogCallback);
    m_modelFuncs->fn_set_state_callback(s_modelStateCallback);
    if (m_modelFuncs->fn_init() != 0) {
        m_logModel->append(QStringLiteral("[MODEL] NoteBotModel.dll 初始化失败"));
        delete m_modelFuncs;
        m_modelFuncs = nullptr;
        m_hModelDll = nullptr;
        FreeLibrary(h);
        setModelReplacementStatus(QStringLiteral("失败"));
        return false;
    }
    m_logModel->append(QStringLiteral("[MODEL] 模型替换模块加载成功"));
    setModelRuntimeAvailable(true);
    return true;
}

void Backend::unloadModelDll()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (!m_modelFuncs || !m_hModelDll) {
        m_modelFuncs = nullptr;
        m_hModelDll = nullptr;
        setModelReplacementRunning(false);
        return;
    }
    detachModelDllCallbacks();
    if (m_modelFuncs->fn_stop_wait) {
        m_modelFuncs->fn_stop_wait();
    }
    if (m_modelFuncs->fn_shutdown) {
        m_modelFuncs->fn_shutdown();
    }
    HMODULE h = static_cast<HMODULE>(m_hModelDll);
    delete m_modelFuncs;
    m_modelFuncs = nullptr;
    m_hModelDll = nullptr;
    FreeLibrary(h);
    setModelReplacementRunning(false);
}

void Backend::detachModelDllCallbacks()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (!m_modelFuncs || !m_hModelDll) {
        return;
    }
    if (m_modelFuncs->fn_set_log_callback) {
        m_modelFuncs->fn_set_log_callback(nullptr);
    }
    if (m_modelFuncs->fn_set_state_callback) {
        m_modelFuncs->fn_set_state_callback(nullptr);
    }
}

void Backend::requestModelReplacementRestart()
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (!modelRuntimeRequested() || !m_authSessionVerified) {
        return;
    }
    if (!m_modelReplacementRunning) {
        QTimer::singleShot(0, this, [this]() {
            startModelReplacementWait();
        });
        return;
    }

    m_modelReplacementRestartPending = true;
    if (m_modelFuncs && m_hModelDll && m_modelFuncs->fn_is_waiting_for_process &&
        m_modelFuncs->fn_is_waiting_for_process() == 1) {
        if (m_modelFuncs->fn_stop_wait) {
            m_modelFuncs->fn_stop_wait();
        }
        m_logModel->append(QStringLiteral("[MODEL] 模型已切换，正在重新等待新游戏进程"));
    } else {
        m_logModel->append(QStringLiteral("[MODEL] 模型已切换，本轮替换完成后生效"));
    }
}

void Backend::disableModelRuntimeAndRemoveLocal(const QString &status, bool removeLocalDll)
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    m_modelReplacementRestartPending = false;
    if (m_modelFuncs && !m_hModelDll) {
        m_modelFuncs = nullptr;
    }
    const bool hasLoadedModelDll = m_modelFuncs && m_hModelDll;
    if (m_modelReplacementRunning || hasLoadedModelDll) {
        stopModelReplacementWait();
    }
    if (hasLoadedModelDll) {
        unloadModelDll();
    }
    setModelRuntimeAvailable(false);
    if (modelRuntimeRequested()) {
        setModelReplacementStatus(status);
    } else {
        setModelReplacementStatus(QStringLiteral("已关闭"));
    }

    if (!removeLocalDll) {
        return;
    }

    const QString runtimePath = injectorModelRuntimeDllPath();
    if (!QFile::exists(runtimePath)) {
        return;
    }
    if (QFile::remove(runtimePath)) {
        m_logModel->append(QStringLiteral("[MODEL] 云端未授权，已移除本地模型运行时"));
    } else {
        m_logModel->append(QStringLiteral("[MODEL] 云端未授权，本地模型运行时将在重启后彻底失效"));
    }
}

void Backend::setModelReplacementRunning(bool running)
{
    if (m_modelReplacementRunning == running) {
        return;
    }
    m_modelReplacementRunning = running;
    emit modelReplacementRunningChanged();
}

void Backend::setModelRuntimeAvailable(bool available)
{
    if (m_modelRuntimeAvailable == available) {
        return;
    }
    m_modelRuntimeAvailable = available;
    emit modelRuntimeAvailableChanged();
}

void Backend::setModelReplacementStatus(const QString &status)
{
    if (m_modelReplacementStatus == status) {
        return;
    }
    m_modelReplacementStatus = status;
    emit modelReplacementStatusChanged();
}

bool Backend::modelRuntimeRequested() const
{
    return m_modelModificationEnabled;
}

QString Backend::effectiveModelArmSize() const
{
    return normalizeModelArmSize(m_modelArmSize);
}

QString Backend::effectiveClassicSkinId() const
{
    const QString arm = effectiveModelArmSize();
    return arm == QStringLiteral("wide")
        ? QStringLiteral("00000000-0000-0000-0000-000000000000.NonsyncCustom")
        : QStringLiteral("00000000-0000-0000-0000-000000000000.NonsyncCustomSlim");
}

void Backend::setClassicModeEnabled(bool enabled)
{
    if (m_classicModeEnabled == enabled) {
        return;
    }
    m_classicModeEnabled = enabled;
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(QStringLiteral("classicModeEnabled"), enabled);
    emit classicModeEnabledChanged();
    m_logModel->append(enabled
        ? QStringLiteral("[MODEL] 经典方块人覆盖已开启")
        : QStringLiteral("[MODEL] 经典方块人覆盖已关闭"));
    if (modelRuntimeRequested() && m_authSessionVerified) {
        requestModelReplacementRestart();
    }
}

QString Backend::classicGeometryTempPath() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
        .absoluteFilePath(QStringLiteral("classic_player_geometry.json"));
}

bool Backend::writeClassicGeometry(const QString &path, const QString &armSize) const
{
    // Hardcoded vanilla square-person geometry JSON.
    // geometry.humanoid.custom = Steve (wide, 4px arms)
    // geometry.humanoid.customSlim = Alex (slim, 3px arms)
    // Bones match the verified geometry from minecraft_player_geometry.json.
    static const QString kSlimGeometry =
        QStringLiteral("{\"format_version\":\"1.12.0\",\"minecraft:geometry\":["
        "{\"description\":{\"identifier\":\"geometry.humanoid.customSlim\","
        "\"texture_width\":64,\"texture_height\":64,\"visible_bounds_width\":1,"
        "\"visible_bounds_height\":2,\"visible_bounds_offset\":[0,1,0]},"
        "\"bones\":["
        "{\"name\":\"root\",\"pivot\":[0,0,0]},"
        "{\"name\":\"waist\",\"parent\":\"root\",\"pivot\":[0,12,0]},"
        "{\"name\":\"body\",\"parent\":\"waist\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,12,-2],\"size\":[8,12,4],\"uv\":[16,16]}]},"
        "{\"name\":\"head\",\"parent\":\"body\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,24,-4],\"size\":[8,8,8],\"uv\":[0,0]}]},"
        "{\"name\":\"hat\",\"parent\":\"head\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,24,-4],\"size\":[8,8,8],\"uv\":[32,0],\"inflate\":0.5}]},"
        "{\"name\":\"rightLeg\",\"parent\":\"root\",\"pivot\":[-1.9,12,0],"
        "\"cubes\":[{\"origin\":[-3.9,0,-2],\"size\":[4,12,4],\"uv\":[0,16]}]},"
        "{\"name\":\"rightPants\",\"parent\":\"rightLeg\",\"pivot\":[-1.9,12,0],"
        "\"cubes\":[{\"origin\":[-3.9,0,-2],\"size\":[4,12,4],\"uv\":[0,32],\"inflate\":0.25}]},"
        "{\"name\":\"leftLeg\",\"parent\":\"root\",\"pivot\":[1.9,12,0],\"mirror\":true,"
        "\"cubes\":[{\"origin\":[-0.1,0,-2],\"size\":[4,12,4],\"uv\":[16,48]}]},"
        "{\"name\":\"leftPants\",\"parent\":\"leftLeg\",\"pivot\":[1.9,12,0],"
        "\"cubes\":[{\"origin\":[-0.1,0,-2],\"size\":[4,12,4],\"uv\":[0,48],\"inflate\":0.25}]},"
        "{\"name\":\"leftArm\",\"parent\":\"body\",\"pivot\":[5,21.5,0],"
        "\"cubes\":[{\"origin\":[4,11.5,-2],\"size\":[3,12,4],\"uv\":[32,48]}]},"
        "{\"name\":\"leftSleeve\",\"parent\":\"leftArm\",\"pivot\":[5,21.5,0],"
        "\"cubes\":[{\"origin\":[4,11.5,-2],\"size\":[3,12,4],\"uv\":[48,48],\"inflate\":0.25}]},"
        "{\"name\":\"leftItem\",\"parent\":\"leftArm\",\"pivot\":[6,14.5,1]},"
        "{\"name\":\"rightArm\",\"parent\":\"body\",\"pivot\":[-5,21.5,0],"
        "\"cubes\":[{\"origin\":[-7,11.5,-2],\"size\":[3,12,4],\"uv\":[40,16]}]},"
        "{\"name\":\"rightSleeve\",\"parent\":\"rightArm\",\"pivot\":[-5,21.5,0],"
        "\"cubes\":[{\"origin\":[-7,11.5,-2],\"size\":[3,12,4],\"uv\":[40,32],\"inflate\":0.25}]},"
        "{\"name\":\"rightItem\",\"parent\":\"rightArm\",\"pivot\":[-6,14.5,1],"
        "\"locators\":{\"lead_hold\":[-6,14.5,1]}},"
        "{\"name\":\"jacket\",\"parent\":\"body\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,12,-2],\"size\":[8,12,4],\"uv\":[16,32],\"inflate\":0.25}]},"
        "{\"name\":\"cape\",\"parent\":\"body\",\"pivot\":[0,24,-3]}"
        "]}]}");

    static const QString kWideGeometry =
        QStringLiteral("{\"format_version\":\"1.12.0\",\"minecraft:geometry\":["
        "{\"description\":{\"identifier\":\"geometry.humanoid.custom\","
        "\"texture_width\":64,\"texture_height\":64,\"visible_bounds_width\":1,"
        "\"visible_bounds_height\":2,\"visible_bounds_offset\":[0,1,0]},"
        "\"bones\":["
        "{\"name\":\"root\",\"pivot\":[0,0,0]},"
        "{\"name\":\"waist\",\"parent\":\"root\",\"pivot\":[0,12,0]},"
        "{\"name\":\"body\",\"parent\":\"waist\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,12,-2],\"size\":[8,12,4],\"uv\":[16,16]}]},"
        "{\"name\":\"head\",\"parent\":\"body\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,24,-4],\"size\":[8,8,8],\"uv\":[0,0]}]},"
        "{\"name\":\"hat\",\"parent\":\"head\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,24,-4],\"size\":[8,8,8],\"uv\":[32,0],\"inflate\":0.5}]},"
        "{\"name\":\"leftArm\",\"parent\":\"body\",\"pivot\":[5,22,0],"
        "\"cubes\":[{\"origin\":[4,12,-2],\"size\":[4,12,4],\"uv\":[32,48]}]},"
        "{\"name\":\"leftSleeve\",\"parent\":\"leftArm\",\"pivot\":[5,22,0],"
        "\"cubes\":[{\"origin\":[4,12,-2],\"size\":[4,12,4],\"uv\":[48,48],\"inflate\":0.25}]},"
        "{\"name\":\"leftItem\",\"parent\":\"leftArm\",\"pivot\":[6,15,1]},"
        "{\"name\":\"rightArm\",\"parent\":\"body\",\"pivot\":[-5,22,0],"
        "\"cubes\":[{\"origin\":[-8,12,-2],\"size\":[4,12,4],\"uv\":[40,16]}]},"
        "{\"name\":\"rightSleeve\",\"parent\":\"rightArm\",\"pivot\":[-5,22,0],"
        "\"cubes\":[{\"origin\":[-8,12,-2],\"size\":[4,12,4],\"uv\":[40,32],\"inflate\":0.25}]},"
        "{\"name\":\"rightItem\",\"parent\":\"rightArm\",\"pivot\":[-6,15,1],"
        "\"locators\":{\"lead_hold\":[-6,15,1]}},"
        "{\"name\":\"leftLeg\",\"parent\":\"root\",\"pivot\":[1.9,12,0],"
        "\"cubes\":[{\"origin\":[-0.1,0,-2],\"size\":[4,12,4],\"uv\":[16,48]}]},"
        "{\"name\":\"leftPants\",\"parent\":\"leftLeg\",\"pivot\":[1.9,12,0],"
        "\"cubes\":[{\"origin\":[-0.1,0,-2],\"size\":[4,12,4],\"uv\":[0,48],\"inflate\":0.25}]},"
        "{\"name\":\"rightLeg\",\"parent\":\"root\",\"pivot\":[-1.9,12,0],"
        "\"cubes\":[{\"origin\":[-3.9,0,-2],\"size\":[4,12,4],\"uv\":[0,16]}]},"
        "{\"name\":\"rightPants\",\"parent\":\"rightLeg\",\"pivot\":[-1.9,12,0],"
        "\"cubes\":[{\"origin\":[-3.9,0,-2],\"size\":[4,12,4],\"uv\":[0,32],\"inflate\":0.25}]},"
        "{\"name\":\"jacket\",\"parent\":\"body\",\"pivot\":[0,24,0],"
        "\"cubes\":[{\"origin\":[-4,12,-2],\"size\":[8,12,4],\"uv\":[16,32],\"inflate\":0.25}]},"
        "{\"name\":\"cape\",\"parent\":\"body\",\"pivot\":[0,24,-3]}"
        "]}]}");

    const QString &json = armSize == QStringLiteral("wide") ? kWideGeometry : kSlimGeometry;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(json.toUtf8());
    file.close();
    return true;
}

void Backend::setSkinPngPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (m_skinPngPath == trimmed) {
        return;
    }
    if (!trimmed.isEmpty()) {
        QImage png(trimmed);
        if (png.isNull()) {
            m_logModel->append(QStringLiteral("[MODEL] 皮肤文件无法读取或不是 PNG：%1").arg(trimmed));
            return;
        }
        const QSize size = png.size();
        if (!(size == QSize(64, 64) || size == QSize(128, 128) || size == QSize(256, 256))) {
            m_logModel->append(QStringLiteral("[MODEL] 皮肤 PNG 必须是 64/128/256 像素，当前 %1x%2")
                                   .arg(size.width()).arg(size.height()));
            return;
        }
        m_skinPngWidth = size.width();
        m_skinPngHeight = size.height();
    } else {
        m_skinPngWidth = 0;
        m_skinPngHeight = 0;
    }
    m_skinPngPath = trimmed;
    QSettings settings(QStringLiteral("NoteBot"), QStringLiteral("Injector"));
    settings.setValue(QStringLiteral("skinPngPath"), trimmed);
    emit skinPngPathChanged();
    emit skinPngInfoChanged();
    m_logModel->append(trimmed.isEmpty()
        ? QStringLiteral("[MODEL] 已清除皮肤 PNG 路径")
        : QStringLiteral("[MODEL] 皮肤 PNG 已选择：%1 (%2x%3)")
              .arg(trimmed).arg(m_skinPngWidth).arg(m_skinPngHeight));
    if (modelRuntimeRequested() && m_authSessionVerified) {
        requestModelReplacementRestart();
    }
}

void Backend::setSkinPngPathFromQml(const QString &path)
{
    const QString trimmed = path.trimmed();
    // QML FileDialog passes file:// URL on some platforms; strip prefix
    QString clean = trimmed;
    if (clean.startsWith(QStringLiteral("file:///"))) {
        clean = clean.mid(8);
    } else if (clean.startsWith(QStringLiteral("file://"))) {
        clean = clean.mid(7);
    }
    // Decode percent-encoded characters
    clean = QUrl::fromPercentEncoding(clean.toUtf8());
    clean = QDir::toNativeSeparators(clean);
    setSkinPngPath(clean);
}

bool Backend::isValidSkinPng(const QString &path)
{
    if (path.isEmpty()) return false;
    QImage png(path);
    if (png.isNull()) return false;
    const QSize size = png.size();
    return size == QSize(64, 64) || size == QSize(128, 128) || size == QSize(256, 256);
}

void Backend::handleModelDllState(const QString &key, const QString &value)
{
    std::lock_guard<std::recursive_mutex> lock(m_modelDllMutex);
    if (!m_modelFuncs || !m_hModelDll || !m_modelRuntimeAvailable) {
        return;
    }
    if (key == QStringLiteral("model_running")) {
        setModelReplacementRunning(value == QStringLiteral("true"));
        if (value != QStringLiteral("true") && m_modelReplacementRestartPending &&
            modelRuntimeRequested() && m_authSessionVerified) {
            m_modelReplacementRestartPending = false;
            QTimer::singleShot(0, this, [this]() {
                startModelReplacementWait();
            });
        } else if (value != QStringLiteral("true") && modelRuntimeRequested() &&
            m_modelReplacementStatus == QStringLiteral("已停止")) {
            setModelReplacementStatus(QStringLiteral("等待密钥验证"));
        }
        return;
    }
    if (key == QStringLiteral("model_state")) {
        setModelReplacementStatus(value);
        if (m_modelReplacementRestartPending && m_modelReplacementRunning &&
            m_modelFuncs && m_hModelDll && m_modelFuncs->fn_is_waiting_for_process &&
            m_modelFuncs->fn_is_waiting_for_process() == 1) {
            if (m_modelFuncs->fn_stop_wait) {
                m_modelFuncs->fn_stop_wait();
            }
            m_logModel->append(QStringLiteral("[MODEL] 本轮完成，正在切换到新模型等待下一次启动"));
        }
        return;
    }
}

void Backend::syncStatusFromDll()
{
    if (!m_funcs) return;

    bool snapshotApplied = false;
    if (m_funcs->fn_call) {
        const QJsonObject resp =
            parseDllJsonObject(callDll(QStringLiteral("get_status_snapshot")));
        const QJsonObject snap = extractStatusSnapshot(resp);
        if (!snap.isEmpty()) {
            m_isActivated = snap.value(QStringLiteral("active")).toBool(false);
            m_licenseTier = snap.value(QStringLiteral("tier_name"))
                                .toString(QStringLiteral("None"))
                                .trimmed();
            if (m_licenseTier.isEmpty()) {
                m_licenseTier = QStringLiteral("None");
            }

            m_licenseStatus =
                snap.value(QStringLiteral("license_status")).toString(QStringLiteral("未授权"));
            snapshotApplied = true;
        }
    }

    if (!snapshotApplied) {
        m_isActivated = (m_funcs->fn_is_active() != 0);

        int tier = m_funcs->fn_get_tier();
        m_licenseTier = (tier == NB_TIER_DEV) ? "Dev"
                      : (tier == NB_TIER_PREMIUM) ? "Premium"
                      : (tier == NB_TIER_TRIAL) ? "Trial"
                      : "None";

        char buf[256];
        buf[0] = '\0';
        m_funcs->fn_get_status(buf, sizeof(buf));
        m_licenseStatus = QString::fromUtf8(buf);
    }

    if (!m_isActivated) {
        setAuthSessionVerified(false);
    }
}

void Backend::syncHostUpdateSnapshot(const QString &state, bool authDllPendingReplace)
{
    const QString rawState = state.trimmed().isEmpty() ? QStringLiteral("idle") : state.trimmed();
    if (rawState == QStringLiteral("checking") ||
        rawState == QStringLiteral("downloading") ||
        rawState == QStringLiteral("ready") ||
        rawState == QStringLiteral("error") ||
        rawState == QStringLiteral("idle")) {
        m_hostUpdateState = rawState;
    } else {
        m_hostUpdateState = QStringLiteral("ready");
    }
    if (!m_funcs || !m_funcs->fn_call) {
        return;
    }

    QJsonObject data;
    data[QStringLiteral("main_exe")] = QJsonObject{
        {QStringLiteral("version"), QString::fromLatin1(kCurrentMainVersion)},
        {QStringLiteral("version_code"), kCurrentMainVersionCode},
        {QStringLiteral("required"), false},
        {QStringLiteral("pending_replace"), false},
        {QStringLiteral("status"), m_hostUpdateState},
    };
    data[QStringLiteral("auth_dll")] = QJsonObject{
        {QStringLiteral("version"), QString::fromLatin1(kCurrentAuthDllVersion)},
        {QStringLiteral("version_code"), kCurrentAuthDllVersionCode},
        {QStringLiteral("required"), m_authUpdateRequired},
        {QStringLiteral("pending_replace"), authDllPendingReplace},
        {QStringLiteral("status"), authDllPendingReplace ? QStringLiteral("ready")
                                                        : m_hostUpdateState},
    };
    data[QStringLiteral("updater_exe")] = QJsonObject{
        {QStringLiteral("version"), QString::fromLatin1(kCurrentUpdaterVersion)},
        {QStringLiteral("version_code"), kCurrentUpdaterVersionCode},
        {QStringLiteral("required"), false},
        {QStringLiteral("pending_replace"), false},
        {QStringLiteral("status"), QStringLiteral("idle")},
    };
    data[QStringLiteral("update_state")] = m_hostUpdateState;

    QJsonObject req;
    req[QStringLiteral("data")] = data;
    callDll(QStringLiteral("set_host_update_snapshot"),
            QString::fromUtf8(QJsonDocument(req).toJson(QJsonDocument::Compact)));
}

/* ============================================================
 *  异步初始化辅助
 * ============================================================ */
void Backend::setInitStatus(const QString &status, int step)
{
    m_initStatus = status;
    emit initStatusChanged();
    if (step >= 0) {
        m_initStep = step;
        emit initStepChanged();
    }
}

void Backend::logFromThread(const QString &msg)
{
    QMetaObject::invokeMethod(m_logModel, [this, msg]() {
        m_logModel->append(sanitizeUiLogMessage(msg));
    }, Qt::QueuedConnection);
}

void Backend::updateInjectUiState(const QString &state,
                                  const QString &stageText,
                                  int progress,
                                  bool keepDownloadProgress)
{
    QMetaObject::invokeMethod(this, [this, state, stageText, progress, keepDownloadProgress]() {
        m_injectState = state;
        emit injectStateChanged();
        if (m_injectStageText != stageText) {
            m_injectStageText = stageText;
            emit injectStageTextChanged();
        }
        if (m_injectProgress != progress) {
            m_injectProgress = progress;
            emit injectProgressChanged();
        }
        if (!keepDownloadProgress && m_downloadProgress != -1) {
            m_downloadProgress = -1;
            emit downloadProgressChanged();
        }
    }, Qt::QueuedConnection);
}

void Backend::finishInjectUiState(const QString &state,
                                  const QString &stageText,
                                  int progress)
{
    QMetaObject::invokeMethod(this, [this, state, stageText, progress]() {
        m_injectState = state;
        emit injectStateChanged();
        if (m_injectStageText != stageText) {
            m_injectStageText = stageText;
            emit injectStageTextChanged();
        }
        if (m_injectProgress != progress) {
            m_injectProgress = progress;
            emit injectProgressChanged();
        }
        if (m_downloadProgress != -1) {
            m_downloadProgress = -1;
            emit downloadProgressChanged();
        }
        m_injectRunning = false;
    }, Qt::QueuedConnection);
}

bool Backend::prepareUpdaterExecutable(const QString &downloadedUpdaterPath,
                                       QString *readyPath,
                                       QString *errorMessage)
{
    const QString installedPath = installedUpdaterPath();

    auto ensureCopied = [&](const QString &src, const QString &dst) -> bool {
        if (src.isEmpty() || !QFile::exists(src)) {
            return false;
        }
        QDir().mkpath(QFileInfo(dst).absolutePath());
        if (QFile::exists(dst)) {
            const QString srcSha = computeFileSha256(src);
            const QString dstSha = computeFileSha256(dst);
            if (!srcSha.isEmpty() && srcSha == dstSha) {
                return true;
            }
            QFile::remove(dst);
        }
        return QFile::copy(src, dst);
    };

    auto ensureReleasedFromEmbedded = [&](const QString &dst) -> bool {
        const QByteArray embeddedBytes = embeddedUpdaterBytes();
        if (embeddedBytes.isEmpty()) {
            return false;
        }
        QDir().mkpath(QFileInfo(dst).absolutePath());
        const QString embeddedSha = QString::fromLatin1(
            QCryptographicHash::hash(embeddedBytes, QCryptographicHash::Sha256).toHex()).toLower();
        if (QFile::exists(dst)) {
            const QString dstSha = computeFileSha256(dst);
            if (!dstSha.isEmpty() && dstSha == embeddedSha) {
                return true;
            }
            QFile::remove(dst);
        }

        QFile file(dst);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        const qint64 written = file.write(embeddedBytes);
        file.close();
        return written == embeddedBytes.size();
    };

    if (!downloadedUpdaterPath.isEmpty() && QFile::exists(downloadedUpdaterPath)) {
        if (!ensureCopied(downloadedUpdaterPath, installedPath)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("无法同步 updater.exe 到本地安装路径");
            }
            return false;
        }
        if (readyPath) {
            *readyPath = installedPath;
        }
        return true;
    }

    if (!ensureReleasedFromEmbedded(installedPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("无法从 EXE 内置资源释放 updater.exe");
        }
        return false;
    }

    if (readyPath) {
        *readyPath = installedPath;
    }
    return true;
}

bool Backend::launchMainExeReplacement(const QString &updaterPath,
                                       const QString &downloadedMainPath,
                                       QString *errorMessage)
{
    if (updaterPath.isEmpty() || !QFile::exists(updaterPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("updater.exe 不存在");
        }
        return false;
    }
    if (downloadedMainPath.isEmpty() || !QFile::exists(downloadedMainPath)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("新主程序文件不存在");
        }
        return false;
    }

    const QString currentExePath = QCoreApplication::applicationFilePath();
    const QString backupPath =
        QDir(QFileInfo(currentExePath).absolutePath()).absoluteFilePath(
            QStringLiteral("NoteBotInjector.backup.exe"));

    QStringList args;
    args << QStringLiteral("--replace-main")
         << QStringLiteral("--pid") << QString::number(QCoreApplication::applicationPid())
         << QStringLiteral("--src") << downloadedMainPath
         << QStringLiteral("--dst") << currentExePath
         << QStringLiteral("--restart") << currentExePath
         << QStringLiteral("--backup") << backupPath;

    if (!QProcess::startDetached(updaterPath, args)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("拉起 updater.exe 失败");
        }
        return false;
    }
    return true;
}

void Backend::checkDllUpdateAsync()
{
    std::thread([this]() {
        checkDllUpdateAsyncInternal();
    }).detach();
}

void Backend::checkDllUpdateAsyncInternal()
{
    logFromThread("[UPD] 正在检查 NoteBotAuth.dll 更新...");

    Updater updater;
    updater.setLogCallback([this](const QString &msg) { logFromThread(msg); });
    updater.setProgressCallback([this](int pct) {
        QMetaObject::invokeMethod(this, [this, pct]() {
            m_downloadProgress = pct;
            emit downloadProgressChanged();
        }, Qt::QueuedConnection);
    });

    QString localDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QString mainPath = QDir(localDir).absoluteFilePath("NoteBotAuth.dll");
    QString updaterMainPath = installedUpdaterPath();
    QString mainMd5 = QFile::exists(mainPath) ? computeFileMd5(mainPath) : QString();
    QString mainSha256 = QFile::exists(mainPath) ? computeFileSha256(mainPath) : QString();
    Q_UNUSED(mainMd5);
    Q_UNUSED(mainSha256);

    QString downloadedUpdaterPath;
    QString readyUpdaterPath;

    auto info = updater.checkManifest(QString::fromLatin1(kCurrentMainVersion),
                                      kCurrentMainVersionCode,
                                      QString::fromLatin1(kCurrentAuthDllVersion),
                                      kCurrentAuthDllVersionCode,
                                      QString::fromLatin1(kCurrentUpdaterVersion),
                                      kCurrentUpdaterVersionCode);

    auto keepArtifactForUpdate = [](const Updater::ArtifactInfo &artifact,
                                    int currentVersionCode,
                                    bool allowSameVersionWhenMissing,
                                    const QString &localPath) -> bool {
        if (artifact.fileName.isEmpty()) {
            return false;
        }
        if (artifact.versionCode <= 0 || artifact.versionCode < currentVersionCode) {
            return false;
        }
        if (artifact.versionCode == currentVersionCode) {
            return allowSameVersionWhenMissing && !localPath.isEmpty() && !QFile::exists(localPath);
        }
        return true;
    };

    auto normalizeUpdateInfo = [&](Updater::UpdateInfo &updateInfo) {
        if (!keepArtifactForUpdate(updateInfo.mainExe,
                                   kCurrentMainVersionCode,
                                   false,
                                   QString())) {
            updateInfo.mainExe = Updater::ArtifactInfo();
        }
        if (!keepArtifactForUpdate(updateInfo.authDll,
                                   kCurrentAuthDllVersionCode,
                                   true,
                                   mainPath)) {
            updateInfo.authDll = Updater::ArtifactInfo();
        }
        if (!keepArtifactForUpdate(updateInfo.updaterExe,
                                   kCurrentUpdaterVersionCode,
                                   true,
                                   updaterMainPath)) {
            updateInfo.updaterExe = Updater::ArtifactInfo();
        }
        updateInfo.hasAnyArtifact = !updateInfo.mainExe.fileName.isEmpty() ||
                                    !updateInfo.authDll.fileName.isEmpty() ||
                                    !updateInfo.updaterExe.fileName.isEmpty();
    };

    auto pullFreshManifest = [&](const QString &artifactType,
                                 Updater::ArtifactInfo *artifactOut,
                                 QString *errorOut) -> bool {
        auto refreshed = updater.checkManifest(QString::fromLatin1(kCurrentMainVersion),
                                               kCurrentMainVersionCode,
                                               QString::fromLatin1(kCurrentAuthDllVersion),
                                               kCurrentAuthDllVersionCode,
                                               QString::fromLatin1(kCurrentUpdaterVersion),
                                               kCurrentUpdaterVersionCode);
        if (!refreshed.error.isEmpty()) {
            if (errorOut) {
                *errorOut = refreshed.error;
            }
            return false;
        }
        normalizeUpdateInfo(refreshed);
        info = refreshed;
        if (!artifactOut) {
            return true;
        }
        if (artifactType == QStringLiteral("main_exe")) {
            *artifactOut = info.mainExe;
        } else if (artifactType == QStringLiteral("auth_dll")) {
            *artifactOut = info.authDll;
        } else if (artifactType == QStringLiteral("updater_exe")) {
            *artifactOut = info.updaterExe;
        } else {
            *artifactOut = Updater::ArtifactInfo();
        }
        if (artifactOut->fileName.isEmpty()) {
            if (errorOut) {
                *errorOut = QStringLiteral("刷新后的更新清单里缺少目标产物");
            }
            return false;
        }
        return true;
    };

    auto makeRefreshCallback = [&](const QString &artifactType) -> Updater::ArtifactRefreshCallback {
        return [&, artifactType](Updater::ArtifactInfo &artifact,
                                 int &expiresIn,
                                 qint64 &receivedAtUtcSec,
                                 QString &error) -> bool {
            if (!pullFreshManifest(artifactType, &artifact, &error)) {
                return false;
            }
            expiresIn = info.expiresIn;
            receivedAtUtcSec = info.receivedAt;
            return true;
        };
    };

    normalizeUpdateInfo(info);

    m_authUpdateRequired = false;
    if (!info.updaterExe.fileName.isEmpty()) {
        logFromThread("[UPD] 检测到更新器更新项");
        const QString updaterCachePath = QDir(localDir).absoluteFilePath(
            QStringLiteral("updates/updater/%1-%2-%3.bin")
                .arg(info.updaterExe.artifactType,
                     info.updaterExe.version,
                     info.updaterExe.fileSha256.isEmpty() ? QStringLiteral("nosha")
                                                          : info.updaterExe.fileSha256.left(12)));
        if (fileMatchesArtifact(updaterMainPath, info.updaterExe)) {
            logFromThread("[UPD] updater.exe 已匹配清单版本");
            downloadedUpdaterPath = updaterMainPath;
        } else if (fileMatchesArtifact(updaterCachePath, info.updaterExe)) {
            logFromThread("[UPD] 本地缓存已有相同 updater.exe，无需重复下载");
            downloadedUpdaterPath = updaterCachePath;
        } else {
            downloadedUpdaterPath = updater.downloadArtifact(info.updaterExe,
                                                             QStringLiteral("updater"),
                                                             info.expiresIn,
                                                             info.receivedAt,
                                                             makeRefreshCallback(QStringLiteral("updater_exe")));
        }
        if (downloadedUpdaterPath.isEmpty() && info.updaterExe.required) {
            const QString errMsg = QStringLiteral("updater.exe 下载失败，无法继续执行主程序更新");
            QMetaObject::invokeMethod(this, [this, errMsg]() {
                syncHostUpdateSnapshot(QStringLiteral("error"), false);
                setInitStatus(QStringLiteral("updater 更新失败"), 0);
                m_initializing = false;
                emit initializingChanged();
                emit initializationFailed(errMsg);
            }, Qt::QueuedConnection);
            return;
        }
    }

    {
        QString updaterError;
        if (!prepareUpdaterExecutable(downloadedUpdaterPath, &readyUpdaterPath, &updaterError)) {
            logFromThread(QString("[UPD] [ERR] 准备 updater.exe 失败: %1").arg(updaterError));
            if (!info.mainExe.fileName.isEmpty() && info.mainExe.required) {
                QMetaObject::invokeMethod(this, [this, updaterError]() {
                    syncHostUpdateSnapshot(QStringLiteral("error"), false);
                    setInitStatus(QStringLiteral("updater 不可用"), 0);
                    m_initializing = false;
                    emit initializingChanged();
                    emit initializationFailed(updaterError);
                }, Qt::QueuedConnection);
                return;
            }
        }
    }

    if (!info.mainExe.fileName.isEmpty()) {
        logFromThread("[UPD] 检测到主程序更新项");
        const QString currentExePath = QCoreApplication::applicationFilePath();
        if (!fileMatchesArtifact(currentExePath, info.mainExe)) {
            QMetaObject::invokeMethod(this, [this]() {
                syncHostUpdateSnapshot(QStringLiteral("downloading"), false);
                setInitStatus(QStringLiteral("正在下载主程序更新..."), 40);
            }, Qt::QueuedConnection);
            const QString downloadedMainPath =
                updater.downloadArtifact(info.mainExe,
                                         QStringLiteral("main"),
                                         info.expiresIn,
                                         info.receivedAt,
                                         makeRefreshCallback(QStringLiteral("main_exe")));
            if (downloadedMainPath.isEmpty()) {
                const QString errMsg = QStringLiteral("主程序更新下载失败");
                if (info.mainExe.required) {
                    QMetaObject::invokeMethod(this, [this, errMsg]() {
                        syncHostUpdateSnapshot(QStringLiteral("error"), false);
                        setInitStatus(QStringLiteral("主程序更新失败"), 0);
                        m_initializing = false;
                        emit initializingChanged();
                        emit initializationFailed(errMsg);
                    }, Qt::QueuedConnection);
                    return;
                }
                logFromThread(QString("[UPD] [ERR] %1").arg(errMsg));
            } else {
                QString launchError;
                if (!launchMainExeReplacement(readyUpdaterPath, downloadedMainPath, &launchError)) {
                    if (info.mainExe.required) {
                        QMetaObject::invokeMethod(this, [this, launchError]() {
                            syncHostUpdateSnapshot(QStringLiteral("error"), false);
                            setInitStatus(QStringLiteral("主程序切换失败"), 0);
                            m_initializing = false;
                            emit initializingChanged();
                            emit initializationFailed(launchError);
                        }, Qt::QueuedConnection);
                        return;
                    }
                    logFromThread(QString("[UPD] [ERR] %1").arg(launchError));
                } else {
                    logFromThread("[UPD] 已拉起 updater.exe，准备替换主程序并重启");
                    QMetaObject::invokeMethod(this, [this]() {
                        syncHostUpdateSnapshot(QStringLiteral("ready"), false);
                        setInitStatus(QStringLiteral("主程序更新中，准备重启..."), 100);
                        m_initializing = false;
                        emit initializingChanged();
                        QCoreApplication::quit();
                    }, Qt::QueuedConnection);
                    return;
                }
            }
        } else {
            logFromThread("[UPD] 主程序已匹配清单版本");
        }
    }

    Updater::ArtifactInfo authArtifact = info.authDll;
    m_authUpdateRequired = authDllStillRequiresMandatoryUpdate(mainPath, authArtifact);
    if (authArtifact.fileName.isEmpty() || authArtifact.fileSize <= 0 ||
        (authArtifact.fileMd5.isEmpty() && authArtifact.fileSha256.isEmpty())) {
        logFromThread("[UPD] 当前已是最新版本");
        QMetaObject::invokeMethod(this, [this]() {
            syncHostUpdateSnapshot(QStringLiteral("idle"), false);
            setInitStatus("当前已是最新版本", 70);
            finishInitialization();
        }, Qt::QueuedConnection);
        return;
    }

    QString tempPath = QDir(localDir).absoluteFilePath(
        QStringLiteral("updates/auth/%1-%2-%3.bin")
            .arg(authArtifact.artifactType,
                 authArtifact.version,
                 authArtifact.fileSha256.isEmpty() ? QStringLiteral("nosha")
                                                   : authArtifact.fileSha256.left(12)));

    bool needDownload = false;
    QString targetPath;

    if (fileMatchesArtifact(mainPath, authArtifact)) {
        m_authUpdateRequired = false;
        logFromThread("[UPD] 主路径 Auth DLL 已是最新版本");
        QMetaObject::invokeMethod(this, [this]() {
            syncHostUpdateSnapshot(QStringLiteral("idle"), false);
            setInitStatus("当前已是最新版本", 90);
            finishInitialization();
        }, Qt::QueuedConnection);
        return;
    }

    if (fileMatchesArtifact(tempPath, authArtifact)) {
        logFromThread("[UPD] 本地缓存已有相同 Auth DLL，无需重复下载");
        targetPath = tempPath;
    } else {
        needDownload = true;
    }

    if (needDownload) {
        logFromThread("[UPD] 正在下载授权模块更新...");
        QMetaObject::invokeMethod(this, [this]() {
            m_downloadProgress = 0;
            emit downloadProgressChanged();
            syncHostUpdateSnapshot(QStringLiteral("downloading"), true);
            setInitStatus("正在下载更新...", 70);
        }, Qt::QueuedConnection);

        QString downloadedPath = updater.downloadArtifact(authArtifact,
                                                          QStringLiteral("auth"),
                                                          info.expiresIn,
                                                          info.receivedAt,
                                                          makeRefreshCallback(QStringLiteral("auth_dll")));
        if (downloadedPath.isEmpty()) {
            logFromThread("[UPD] [ERR] DLL 下载失败");
            QString errMsg = QString("NoteBotAuth.dll 更新下载失败。\n"
                                     "可能原因：\n"
                                     "1. 网络连接问题\n"
                                     "2. Token 验证失败\n"
                                     "3. 磁盘空间不足\n"
                                     "\n请检查日志窗口的详细错误信息。");
            QMetaObject::invokeMethod(this, [this, errMsg]() {
                syncHostUpdateSnapshot(QStringLiteral("error"), true);
                MessageBoxW(nullptr,
                    reinterpret_cast<const wchar_t*>(errMsg.utf16()),
                    L"NoteBot - DLL 更新失败",
                    MB_OK | MB_ICONWARNING);
                setInitStatus("授权模块更新失败", 0);
                m_downloadProgress = -1;
                emit downloadProgressChanged();
                m_initializing = false;
                emit initializingChanged();
                emit initializationFailed(errMsg);
            }, Qt::QueuedConnection);
            return;
        }

        if (!info.authDll.fileName.isEmpty()) {
            authArtifact = info.authDll;
        }
        targetPath = downloadedPath;
        logFromThread("[UPD] 授权模块下载完成");
        QMetaObject::invokeMethod(this, [this]() {
            m_downloadProgress = 100;
            emit downloadProgressChanged();
            syncHostUpdateSnapshot(QStringLiteral("ready"), true);
            setInitStatus("下载完成，正在同步...", 90);
        }, Qt::QueuedConnection);
    }

    // 更新检查发生在 LoadLibrary 之前，主路径此时不应被本进程锁住。
    if (!targetPath.isEmpty() && targetPath != mainPath) {
        if (fileMatchesArtifact(mainPath, authArtifact)) {
            logFromThread("[UPD] 主路径已是相同版本，无需同步");
        } else {
            bool removed = !QFile::exists(mainPath) || QFile::remove(mainPath);
            if (!removed) {
                logFromThread("[UPD] [ERR] 主路径授权模块正在被占用，无法替换");
                QString errMsg = QString("NoteBotAuth.dll 已下载成功，但主路径 DLL 正在被占用，无法替换。\n\n"
                                         "请关闭所有 NoteBot Injector 窗口后重新启动。\n\n"
                                         "当前不会继续加载旧 DLL。");
                QMetaObject::invokeMethod(this, [this, errMsg]() {
                    syncHostUpdateSnapshot(QStringLiteral("error"), true);
                    MessageBoxW(nullptr,
                        reinterpret_cast<const wchar_t*>(errMsg.utf16()),
                        L"NoteBot - DLL 被占用",
                        MB_OK | MB_ICONWARNING);
                    setInitStatus("授权模块更新失败", 0);
                    m_downloadProgress = -1;
                    emit downloadProgressChanged();
                    m_initializing = false;
                    emit initializingChanged();
                    emit initializationFailed(errMsg);
                }, Qt::QueuedConnection);
                return;
            }

            if (QFile::copy(targetPath, mainPath)) {
                m_authUpdateRequired = authDllStillRequiresMandatoryUpdate(mainPath, authArtifact);
                logFromThread("[UPD] 新版本已准备就绪");
            } else {
                logFromThread("[UPD] [ERR] 无法同步授权模块到主路径");
                QString errMsg = QString("NoteBotAuth.dll 已下载成功，但无法同步到主路径。\n\n"
                                         "请关闭所有 NoteBot Injector 窗口后重新启动。\n\n"
                                         "当前不会继续加载旧 DLL。");
                QMetaObject::invokeMethod(this, [this, errMsg]() {
                    syncHostUpdateSnapshot(QStringLiteral("error"), true);
                    MessageBoxW(nullptr,
                        reinterpret_cast<const wchar_t*>(errMsg.utf16()),
                        L"NoteBot - DLL 同步失败",
                        MB_OK | MB_ICONWARNING);
                    setInitStatus("授权模块更新失败", 0);
                    m_downloadProgress = -1;
                    emit downloadProgressChanged();
                    m_initializing = false;
                    emit initializingChanged();
                    emit initializationFailed(errMsg);
                }, Qt::QueuedConnection);
                return;
            }
        }
    }

    QMetaObject::invokeMethod(this, [this, mainPath, authArtifact]() {
        m_authUpdateRequired = authDllStillRequiresMandatoryUpdate(mainPath, authArtifact);
        syncHostUpdateSnapshot(QStringLiteral("idle"), false);
        finishInitialization();
    }, Qt::QueuedConnection);
}

/* ============================================================
 *  授权槽函数
 * ============================================================ */
void Backend::doActivate()
{
    if (!loadAuthDll()) return;
    setAuthSessionVerified(false);
    QString blockedReason;
    if (authActionsBlocked(&blockedReason)) {
        m_logModel->append(QString("[AUTH] %1").arg(blockedReason));
        return;
    }
    if (m_licenseKey.trimmed().isEmpty()) {
        m_logModel->append(QStringLiteral("[AUTH] 请先输入密钥"));
        return;
    }

    m_logModel->append("[AUTH] 正在执行 V3 设备激活...");

    const QJsonObject resp = parseDllJsonObject(
        callDll(QStringLiteral("activate"),
                QJsonDocument(QJsonObject{
                    {QStringLiteral("key"), m_licenseKey}
                }).toJson(QJsonDocument::Compact)));
    const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
    const QString message = resp.value(QStringLiteral("message")).toString();

    syncStatusFromDll();
    emit licenseStatusChanged();

    if (rc == NB_OK) {
        setAuthSessionVerified(true);
        m_logModel->append(QString("[AUTH] 激活成功：%1").arg(m_licenseTier));
    } else {
        setAuthSessionVerified(false);
        m_logModel->append(QString("[AUTH] V3 激活未完成: %1 (code=%2)")
                               .arg(describeV3Rc(rc, message))
                               .arg(rc));
    }
}

void Backend::doLocalVerify()
{
    if (!loadAuthDll()) return;

    setAuthSessionVerified(false);
    syncStatusFromDll();
    emit licenseStatusChanged();
    emit licenseKeyChanged();
}

void Backend::doDiagnose()
{
    if (!loadAuthDll()) return;
    QString blockedReason;
    if (authActionsBlocked(&blockedReason)) {
        m_logModel->append(QString("[AUTH] %1").arg(blockedReason));
        return;
    }

    m_logModel->append("[AUTH] 正在执行注入前检查...");

    const QJsonObject resp = parseDllJsonObject(callDll(QStringLiteral("diagnose")));
    const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
    const QString message = resp.value(QStringLiteral("message")).toString();
    const QJsonObject data = resp.value(QStringLiteral("data")).toObject();
    const QString dllName = data.value(QStringLiteral("dll_name")).toString();
    const QString dllSha256 = data.value(QStringLiteral("dll_sha256")).toString();

    if (rc == NB_OK) {
        m_logModel->append("[AUTH] 注入前检查通过");
        if (!dllName.isEmpty()) {
            m_logModel->append(QString("[AUTH] 目标 DLL: %1").arg(dllName));
        }
        if (!dllSha256.isEmpty()) {
            m_logModel->append(QString("[AUTH] DLL SHA256: %1").arg(dllSha256));
        }
    } else {
        m_logModel->append(QString("[AUTH] [ERR] 注入前检查未通过: %1")
                               .arg(describeV3Rc(rc, message)));
    }
    emit licenseStatusChanged();
}

void Backend::resetActivation()
{
    if (!loadAuthDll()) return;

    m_logModel->append("[AUTH] 正在重置授权核心状态...");
    callDll(QStringLiteral("reset"));
    setAuthSessionVerified(false);
    syncStatusFromDll();
    emit licenseStatusChanged();
    m_logModel->append("[AUTH] 授权核心状态已重置");
}

void Backend::setLicenseKeyProperty(const QString &v)
{
    if (v != m_licenseKey) {
        m_licenseKey = v.trimmed();
        setAuthSessionVerified(false);
        clearModelCatalog();
        emit licenseKeyChanged();
        QSettings settings("NoteBot", "Injector");
        settings.setValue("licenseKey", m_licenseKey);
        if (m_funcs) {
            m_funcs->fn_set_key(m_licenseKey.toUtf8().constData());
        }
    }
}

void Backend::setLicenseKey(const QString &key)
{
    setLicenseKeyProperty(key);
}

QString Backend::callDll(const QString &action, const QString &json)
{
    std::lock_guard<std::mutex> lock(m_authCallMutex);
    if (!loadAuthDll()) {
        return QStringLiteral("{\"ok\":false,\"message\":\"dll_not_loaded\"}");
    }
    if (!m_funcs || !m_funcs->fn_call) {
        return QStringLiteral("{\"ok\":false,\"message\":\"nb_call_unavailable\"}");
    }

    QByteArray actionUtf8 = action.toUtf8();
    QByteArray jsonUtf8 = json.isEmpty() ? QByteArray("{}") : json.toUtf8();
    QByteArray out;
    out.resize(1024 * 1024);
    int actual = m_funcs->fn_call(actionUtf8.constData(),
                                  jsonUtf8.constData(),
                                  out.data(),
                                  out.size());
    if (actual < 0) {
        return QStringLiteral("{\"ok\":false,\"message\":\"call_failed\"}");
    }
    out.resize(actual);
    return QString::fromUtf8(out);
}

void Backend::callDllAsync(const QString &action, const QString &json)
{
    std::thread([this, action, json]() {
        QString result = callDll(action, json);
        logFromThread(QString("[DLLCALL] %1 -> %2").arg(action, result));
    }).detach();
}

void Backend::appendLog(const QString &msg)
{
    m_logModel->append(sanitizeUiLogMessage(msg));
}

void Backend::verifyLicense()
{
    if (!loadAuthDll()) return;
    setAuthSessionVerified(false);
    QString blockedReason;
    if (authActionsBlocked(&blockedReason)) {
        m_logModel->append(QString("[AUTH] %1").arg(blockedReason));
        syncStatusFromDll();
        emit licenseStatusChanged();
        return;
    }
    if (m_licenseKey.trimmed().isEmpty()) {
        m_logModel->append(QStringLiteral("[AUTH] 请先输入密钥"));
        return;
    }

    const QJsonObject layoutResp =
        parseDllJsonObject(callDll(QStringLiteral("get_local_layout")));
    const QJsonObject exists =
        layoutResp.value(QStringLiteral("data")).toObject()
            .value(QStringLiteral("exists")).toObject();

    if (exists.value(QStringLiteral("license_v3")).toBool(false)) {
        const QJsonObject snapshotResp =
            parseDllJsonObject(callDll(QStringLiteral("get_status_snapshot")));
        const QJsonObject cachedSnapshot = extractStatusSnapshot(snapshotResp);
        const QString cachedKeyId =
            cachedSnapshot.value(QStringLiteral("key_id")).toString().trimmed();
        const QString typedKeyId = NBVmp_Injector_MakeLocalKeyId(m_licenseKey);
        const bool cachedActive = cachedSnapshot.value(QStringLiteral("active")).toBool(false);
        if (cachedKeyId.isEmpty() || typedKeyId.isEmpty()) {
            syncStatusFromDll();
            emit licenseStatusChanged();
            emit licenseKeyChanged();
            m_logModel->append(QStringLiteral("[AUTH] 本地授权身份不完整，请重新激活"));
            return;
        }
        if (cachedKeyId != typedKeyId) {
            syncStatusFromDll();
            emit licenseStatusChanged();
            emit licenseKeyChanged();
            m_logModel->append(QStringLiteral("[AUTH] 检测到新密钥，正在向服务器重新激活..."));
            doActivate();
            return;
        }
        if (!cachedActive) {
            syncStatusFromDll();
            emit licenseStatusChanged();
            emit licenseKeyChanged();
            m_logModel->append(QStringLiteral("[AUTH] 当前设备尚未完成联网激活，正在向服务器重新激活..."));
            doActivate();
            return;
        }

        const QJsonObject resp =
            parseDllJsonObject(callDll(QStringLiteral("device_check_v3")));
        const int rc = resp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString message = resp.value(QStringLiteral("message")).toString();
        syncStatusFromDll();
        emit licenseStatusChanged();
        emit licenseKeyChanged();
        if (rc == 0) {
            setAuthSessionVerified(true);
            m_logModel->append(QString("[AUTH] 授权状态已同步：%1 / %2")
                                   .arg(m_licenseStatus, m_licenseTier));
        } else {
            setAuthSessionVerified(false);
            if (rc == 1007 &&
                message.contains(QStringLiteral("尚未完成联网激活"))) {
                m_logModel->append(QStringLiteral("[AUTH] 服务端记录显示当前设备尚未完成联网激活，正在自动重新激活..."));
                doActivate();
                return;
            }
            m_logModel->append(QString("[AUTH] 设备检查未完成: %1 (code=%2)")
                                   .arg(describeV3Rc(rc, message))
                                   .arg(rc));
        }
        return;
    }

    doActivate();
}

/* ============================================================
 *  注入 — 当前由 DLL 维护注入壳接口，EXE 只负责触发
 * ============================================================ */
void Backend::doInject()
{
    if (m_selectedPid == 0) {
        m_logModel->append("[ERR] 请先等待目标进程");
        return;
    }
    const QList<ProcessInfo> liveTargets = Win32Process::findAllProcesses(m_scanTarget);
    bool selectedStillAlive = false;
    for (const auto &proc : liveTargets) {
        if (proc.pid == m_selectedPid) {
            selectedStillAlive = true;
            break;
        }
    }
    if (!selectedStillAlive) {
        m_selectedPid = 0;
        m_procModel->setSelected(0);
        emit selectedPidChanged();
        m_processCount = liveTargets.size();
        emit processCountChanged();
        m_injectState = QStringLiteral("idle");
        emit injectStateChanged();
        m_logModel->append("[ERR] 未检测到可注入的游戏进程，请先启动游戏");
        return;
    }
    if (!loadAuthDll()) {
        m_logModel->append("[ERR] 授权模块未加载");
        return;
    }
    QString blockedReason;
    if (injectBlocked(&blockedReason)) {
        m_injectState = QStringLiteral("error");
        emit injectStateChanged();
        m_logModel->append(QString("[AUTH] %1").arg(blockedReason));
        return;
    }
    if (m_injectRunning) {
        m_logModel->append("[AUTH] 当前已有注入任务在执行");
        return;
    }

    m_injectRunning = true;
    updateInjectUiState(QStringLiteral("injecting"),
                        QStringLiteral("准备注入"),
                        0);

    const uint targetPid = m_selectedPid;
    std::thread([this, targetPid]() {
        auto fail = [this](const QString &stage, int progress, const QString &message) {
            logFromThread(message);
            finishInjectUiState(QStringLiteral("error"), stage, progress);
        };

        logFromThread("[INJ] 开始注入");
        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("注入前设备检查"),
                            6);
        logFromThread("[INJ] 正在检查设备状态...");
        const QJsonObject checkResp =
            parseDllJsonObject(callDll(QStringLiteral("device_check_v3")));
        const int checkRc = checkResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString checkMessage = checkResp.value(QStringLiteral("message")).toString();
        QMetaObject::invokeMethod(this, [this]() {
            syncStatusFromDll();
            emit licenseStatusChanged();
        }, Qt::QueuedConnection);
        if (checkRc != 0) {
            QMetaObject::invokeMethod(this, [this]() { setAuthSessionVerified(false); }, Qt::QueuedConnection);
            fail(QStringLiteral("设备检查失败"),
                 6,
                 QString("[AUTH] 注入前设备检查失败: %1 (code=%2)")
                     .arg(describeV3Rc(checkRc, checkMessage))
                     .arg(checkRc));
            return;
        }
        QMetaObject::invokeMethod(this, [this]() { setAuthSessionVerified(true); }, Qt::QueuedConnection);
        logFromThread("[INJ] 设备状态正常");

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("获取业务 DLL 策略"),
                            12);
        logFromThread("[INJ] 正在获取业务 DLL 信息...");
        const QJsonObject policyResp =
            parseDllJsonObject(callDll(QStringLiteral("dll_policy_v3")));
        const int policyRc = policyResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString policyMessage = policyResp.value(QStringLiteral("message")).toString();
        if (policyRc != 0) {
            fail(QStringLiteral("策略获取失败"),
                 12,
                 QString("[AUTH] DLL 策略获取失败: %1 (code=%2)")
                     .arg(describeV3Rc(policyRc, policyMessage))
                     .arg(policyRc));
            return;
        }

        const QJsonObject policyData = policyResp.value(QStringLiteral("data")).toObject();
        const QString dllName = policyData.value(QStringLiteral("dll_name")).toString().trimmed();
        const QString dllSha256 = policyData.value(QStringLiteral("dll_sha256")).toString().trimmed().toLower();
        const QString dllMd5 = policyData.value(QStringLiteral("dll_md5")).toString().trimmed().toLower();
        const qint64 dllSize = policyData.value(QStringLiteral("dll_size")).toVariant().toLongLong();
        const QString downloadDir = policyData.value(QStringLiteral("download_dir")).toString().trimmed();
        const QUrl downloadUrl(policyData.value(QStringLiteral("download_url")).toString().trimmed());
        const QString ticketFilePath = injectorTicketPath();
        const QString resultFilePath = injectorResultPath();
        const QString gateLogFilePath = injectorGateLogPath();
        const QString uiLogFilePath = injectorUiLogPath();
        if (dllName.isEmpty() || dllSha256.isEmpty() || downloadDir.isEmpty() ||
            dllSize <= 0 || !downloadUrl.isValid()) {
            fail(QStringLiteral("策略无效"),
                 12,
                 QStringLiteral("[AUTH] 当前 DLL 策略缓存不完整，无法继续注入"));
            return;
        }
        logFromThread(QString("[INJ] 业务 DLL：%1").arg(dllName));

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("下载业务 DLL"),
                            18,
                            true);
        Updater::ArtifactInfo overlayArtifact;
        overlayArtifact.artifactType = QStringLiteral("overlay_dll");
        overlayArtifact.channel = policyData.value(QStringLiteral("channel")).toString().trimmed();
        overlayArtifact.version = QStringLiteral("bound");
        overlayArtifact.fileName = dllName;
        overlayArtifact.fileMd5 = dllMd5;
        overlayArtifact.fileSha256 = dllSha256;
        overlayArtifact.fileSize = dllSize;
        overlayArtifact.downloadUrl = downloadUrl;
        Updater updater;
        const QString dllPath = QDir(downloadDir).absoluteFilePath(dllName);
        logFromThread("[INJ] 正在确认本地缓存");
        if (fileMatchesArtifact(dllPath, overlayArtifact)) {
            QMetaObject::invokeMethod(this, [this]() {
                m_downloadProgress = 100;
                emit downloadProgressChanged();
                if (m_injectProgress != 50) {
                    m_injectProgress = 50;
                    emit injectProgressChanged();
                }
            }, Qt::QueuedConnection);
            logFromThread(QString("[INJ] 本地缓存命中，校验通过：%1").arg(dllName));
        } else {
            logFromThread(QString("[INJ] 正在下载业务 DLL：%1").arg(dllName));
            updater.setLogCallback([this](const QString &msg) {
                logFromThread(msg);
            });
            updater.setProgressCallback([this](int pct) {
                QMetaObject::invokeMethod(this, [this, pct]() {
                    m_downloadProgress = pct;
                    emit downloadProgressChanged();
                    const int mapped = qBound(20, 20 + (pct * 30) / 100, 50);
                    if (m_injectProgress != mapped) {
                        m_injectProgress = mapped;
                        emit injectProgressChanged();
                    }
                }, Qt::QueuedConnection);
            });
            const QString downloadedCachePath = updater.downloadArtifact(overlayArtifact, QStringLiteral("overlay"));
            if (downloadedCachePath.isEmpty()) {
                fail(QStringLiteral("下载失败"),
                     24,
                     QStringLiteral("[AUTH] 业务 DLL 下载失败"));
                return;
            }
            logFromThread(QString("[INJ] 下载完成：%1").arg(dllName));
            if (!fileMatchesArtifact(dllPath, overlayArtifact)) {
                QFile::remove(dllPath);
                if (!QFile::copy(downloadedCachePath, dllPath)) {
                    fail(QStringLiteral("落盘失败"),
                         32,
                         QString("[AUTH] 业务 DLL 无法同步到本地目录: %1").arg(dllName));
                    return;
                }
            }
            const QString localSha256 = computeFileSha256(dllPath);
            if (localSha256.isEmpty() || localSha256 != dllSha256) {
                fail(QStringLiteral("校验失败"),
                     38,
                     QString("[AUTH] 业务 DLL 校验失败: %1").arg(dllName));
                return;
            }
            logFromThread(QString("[INJ] 业务 DLL 已就绪：%1").arg(dllName));
        }

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("同步本地业务 DLL 状态"),
                            46,
                            true);
        logFromThread("[INJ] 正在准备注入状态...");
        const QJsonObject downloadResp =
            parseDllJsonObject(callDll(QStringLiteral("download_overlay_dll_v3")));
        const int downloadRc = downloadResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString downloadMessage = downloadResp.value(QStringLiteral("message")).toString();
        if (downloadRc != 0) {
            fail(QStringLiteral("本地状态同步失败"),
                 46,
                 QString("[AUTH] 业务 DLL 准备失败: %1 (code=%2)")
                     .arg(describeV3Rc(downloadRc, downloadMessage))
                     .arg(downloadRc));
            return;
        }
        logFromThread("[INJ] 注入状态已准备");

        if (QFile::exists(resultFilePath)) {
            if (QFile::remove(resultFilePath)) {
            } else {
                logFromThread("[INJ] [WRN] 旧注入结果清理失败");
            }
        }
        if (QFile::exists(ticketFilePath)) {
            if (QFile::remove(ticketFilePath)) {
            } else {
                logFromThread("[INJ] [WRN] 旧注入票据清理失败");
            }
        }

        QJsonObject ticketReq;
        ticketReq[QStringLiteral("target_pid")] = static_cast<qint64>(targetPid);
        ticketReq[QStringLiteral("dll_name")] = dllName;
        ticketReq[QStringLiteral("dll_sha256")] = dllSha256;
        ticketReq[QStringLiteral("exe_version")] = QString::fromLatin1(kCurrentMainVersion);

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("签发注入票据"),
                            58,
                            true);
        logFromThread("[INJ] 正在申请注入票据...");
        const QJsonObject issueResp = parseDllJsonObject(
            callDll(QStringLiteral("issue_inject_ticket_v3"),
                    QString::fromUtf8(QJsonDocument(ticketReq).toJson(QJsonDocument::Compact))));
        const int issueRc = issueResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
        const QString issueMessage = issueResp.value(QStringLiteral("message")).toString();
        if (issueRc != 0) {
            fail(QStringLiteral("票据生成失败"),
                 58,
                 QString("[AUTH] 票据生成失败: %1 (code=%2)")
                     .arg(describeV3Rc(issueRc, issueMessage))
                     .arg(issueRc));
            return;
        }
        logFromThread("[INJ] 注入票据已准备");

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("注入业务 DLL"),
                            72,
                            true);
        logFromThread("[INJ] 正在载入业务 DLL...");
        const bool injected = Win32Injector::injectDll(
            targetPid,
            dllPath,
            [this](const QString &msg) {
                if (msg.startsWith(QStringLiteral("[ERR]"))) {
                    logFromThread(msg);
                }
            });
        if (!injected) {
            fail(QStringLiteral("注入失败"),
                 72,
                 QStringLiteral("[AUTH] 业务 DLL 注入失败"));
            return;
        }
        logFromThread("[INJ] 业务 DLL 已载入，等待验证结果...");

        updateInjectUiState(QStringLiteral("injecting"),
                            QStringLiteral("等待验证结果"),
                            84,
                            true);

        QElapsedTimer timer;
        timer.start();
        constexpr qint64 kInjectResultTimeoutMs = 9000;
        bool resultFileSeen = false;
        while (timer.elapsed() < kInjectResultTimeoutMs) {
            if (!resultFileSeen && QFile::exists(resultFilePath)) {
                resultFileSeen = true;
                logFromThread("[INJ] 已收到业务 DLL 回写");
            }
            const QJsonObject consumeResp =
                parseDllJsonObject(callDll(QStringLiteral("consume_inject_result_v3")));
            const int consumeRc = consumeResp.value(QStringLiteral("rc")).toInt(NB_ERR_OTHER);
            const QString consumeMessage = consumeResp.value(QStringLiteral("message")).toString();
            if (consumeRc == 0) {
                updateInjectUiState(QStringLiteral("injecting"),
                                    QStringLiteral("上报注入结果"),
                                    95,
                                    true);
                const QJsonObject consumeData = consumeResp.value(QStringLiteral("data")).toObject();
                logFromThread(QString("[INJ] 验证结果：%1 / %2")
                                  .arg(consumeData.value(QStringLiteral("status")).toString())
                                  .arg(consumeData.value(QStringLiteral("reason")).toString()));
                const QJsonObject reportResp =
                    parseDllJsonObject(callDll(QStringLiteral("report_inject_result_v3")));
                Q_UNUSED(reportResp);
                Q_UNUSED(consumeMessage);
                QMetaObject::invokeMethod(this, [this]() {
                    syncStatusFromDll();
                    emit licenseStatusChanged();
                }, Qt::QueuedConnection);
                finishInjectUiState(QStringLiteral("success"),
                                    QStringLiteral("注入完成"),
                                    100);
                return;
            }
            if (consumeRc != 1017) {
                fail(QStringLiteral("结果校验失败"),
                     92,
                     QString("[AUTH] 注入结果无效: %1 (code=%2)")
                         .arg(describeV3Rc(consumeRc, consumeMessage))
                         .arg(consumeRc));
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }

        const QJsonObject finalizeReq = QJsonObject{
            {QStringLiteral("status"), QStringLiteral("inject_failed")},
            {QStringLiteral("reason"), QStringLiteral("inject_failed:no_result_file")}
        };
        const QJsonObject finalizeResp = parseDllJsonObject(
            callDll(QStringLiteral("finalize_inject_failure_v3"),
                    QString::fromUtf8(QJsonDocument(finalizeReq).toJson(QJsonDocument::Compact))));
        const QJsonObject reportResp =
            parseDllJsonObject(callDll(QStringLiteral("report_inject_result_v3")));
        logFromThread("[AUTH] 等待注入结果超时");
        logFromThread("[AUTH] 未收到业务 DLL 的结果回写");
        logFromThread(QString("[AUTH] %1").arg(
            finalizeResp.value(QStringLiteral("message")).toString(
                QStringLiteral("待处理票据已作废"))));
        logFromThread(QString("[AUTH] %1").arg(
            reportResp.value(QStringLiteral("message")).toString(
                QStringLiteral("结果上报失败"))));
        finishInjectUiState(QStringLiteral("error"),
                            QStringLiteral("结果回写超时"),
                            100);
    }).detach();
}

/* ============================================================
 *  进程扫描与 UI
 * ============================================================ */
void Backend::startScanning()
{
    m_scanTimer->start(1000);
    autoScan();
    m_logModel->append("[SYS] 已就绪，开始扫描进程");
}

void Backend::selectProcess(int pid)
{
    m_selectedPid = static_cast<uint>(pid);
    m_procModel->setSelected(m_selectedPid);
    emit selectedPidChanged();
    m_logModel->append(QStringLiteral("[SEL] 已选中游戏进程"));
}

void Backend::bringToFront(int pid)
{
    Win32Process::bringToFront(static_cast<uint>(pid));
    m_logModel->append(QStringLiteral("[WIN] 已尝试唤起游戏窗口"));
}

void Backend::setScanTarget(const QString &name)
{
    m_scanTarget = name.trimmed();
}

void Backend::onLogMsg(const QString &msg)
{
    m_logModel->append(sanitizeUiLogMessage(msg));
}

void Backend::onDllStateChanged(const QString &key, const QString &value)
{
    if (key == "inject_state") {
        m_injectState = value;
        emit injectStateChanged();
    } else if (key == "inject_stage") {
        m_injectStageText = value;
        emit injectStageTextChanged();
    } else if (key == "inject_cooldown") {
        m_injectCooldown = (value == "true");
        emit injectCooldownChanged();
    } else if (key == "download_progress") {
        m_downloadProgress = value.toInt();
        emit downloadProgressChanged();
        if (m_injectRunning) {
            const int mapped = qBound(20, 20 + (m_downloadProgress * 30) / 100, 50);
            if (m_injectProgress != mapped) {
                m_injectProgress = mapped;
                emit injectProgressChanged();
            }
        }
    }
}

void Backend::autoScan()
{
    QString name = m_scanTarget;
    if (name.isEmpty()) return;
    QList<ProcessInfo> procs = Win32Process::findAllProcesses(name);
    QSet<uint> newPids;
    for (const auto &p : procs) newPids.insert(p.pid);

    QSet<uint> added = newPids - m_prevPids;
    QSet<uint> removed = m_prevPids - newPids;
    if (!added.isEmpty()) {
        m_logModel->append(QStringLiteral("[SCN] 发现游戏进程"));
    }
    if (!removed.isEmpty()) {
        m_logModel->append(QStringLiteral("[SCN] 游戏进程已关闭"));
    }
    m_prevPids = newPids;

    if (procs.isEmpty()) {
        if (m_selectedPid != 0) {
            m_selectedPid = 0;
            emit selectedPidChanged();
        }
    } else if (!newPids.contains(m_selectedPid)) {
        if (m_selectedPid != procs.first().pid) {
            m_selectedPid = procs.first().pid;
            emit selectedPidChanged();
        }
    }

    QList<ProcessItem> items;
    for (const auto &p : procs) {
        WindowInfo winfo = Win32Process::getWindowForPid(p.pid);
        ProcessItem it;
        it.pid = p.pid;
        it.exe = p.exe;
        it.path = p.path;
        it.title = winfo.title.isEmpty() ? "Running (no visible window)" : winfo.title;
        it.hasWindow = (winfo.hwnd != nullptr);
        it.selected = false;
        items.append(it);
    }
    m_procModel->updateList(items, m_selectedPid);
    m_processCount = procs.size();
    emit processCountChanged();

    if (!procs.isEmpty()) {
        m_scanStatus = QString("Found %1 %2")
                           .arg(procs.size())
                           .arg(procs.size() == 1 ? "process" : "processes");
    } else {
        m_scanStatus = "Waiting for process...";
    }
    emit scanStatusChanged();
}
