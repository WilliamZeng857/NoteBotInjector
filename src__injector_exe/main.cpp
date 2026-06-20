#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFont>
#include <QIcon>
#include <QQuickWindow>
#include <QWindow>
#include <QSGRendererInterface>
#include <QQuickStyle>
#include <qqml.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QtMessageHandler>
#include <windows.h>
#include <shellapi.h>
#include "processmodel.h"
#include "logmodel.h"
#include "backend.h"
#include "modelcatalogmodel.h"
#include "modelpreviewitem.h"

static QFile *g_logFile = nullptr;

void logHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    QString level;
    switch (type) {
        case QtDebugMsg: level = "DEBUG"; break;
        case QtInfoMsg: level = "INFO"; break;
        case QtWarningMsg: level = "WARN"; break;
        case QtCriticalMsg: level = "CRIT"; break;
        case QtFatalMsg: level = "FATAL"; break;
    }
    QString line = QString("[%1] %2: %3")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"))
        .arg(level)
        .arg(msg);
    if (g_logFile && g_logFile->isOpen()) {
        QTextStream(g_logFile) << line << "\n";
        g_logFile->flush();
    }
    OutputDebugStringW(reinterpret_cast<const wchar_t*>(line.utf16()));
}

int main(int argc, char *argv[])
{
    HANDLE singleInstance = CreateMutexW(nullptr, TRUE, L"Local\\NoteBotInjector.SingleInstance");
    if (!singleInstance || GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr,
            L"NoteBot Injector 已经在运行。\n\n请先关闭现有窗口后再重新启动。",
            L"NoteBot Injector",
            MB_OK | MB_ICONINFORMATION);
        if (singleInstance) CloseHandle(singleInstance);
        return 0;
    }
    // 隐藏控制台窗口
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) ShowWindow(hwndConsole, SW_HIDE);

    // Setup file logging for debugging
    g_logFile = new QFile(QDir::tempPath() + "/notebot_injector_debug.log");
    if (!g_logFile->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Failed to open debug log file";
    }
    qInstallMessageHandler(logHandler);
    qInfo() << "=== NoteBotInjector starting ===";

    qInfo() << "Setting env vars...";
    qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");
    qputenv("QML_DISABLE_DISK_CACHE", "1");
    qputenv("QT_DISABLE_SHADER_DISK_CACHE", "1");

    qInfo() << "Setting DPI policy...";
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    qInfo() << "Setting graphics API to Direct3D11...";
    qputenv("QSG_FIXED_ANIMATION_STEP", "13");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);

    qInfo() << "Creating QGuiApplication...";
    QGuiApplication app(argc, argv);
    qInfo() << "QGuiApplication created";
    qInfo() << "Setting font...";
    app.setFont(QFont("Microsoft YaHei", 10));

    qInfo() << "Setting app icon...";
    // 设置应用图标
    QStringList iconPaths = {
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("avatar.ico"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("resources/avatar.ico"),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../resources/avatar.ico"),
        QStringLiteral("D:/Downloads/cheatengine-mcp-bridge-main/cheatengine-mcp-bridge-main/NoteBot/NoteBot_injector/avatar.ico")
    };
    for (const QString &ip : iconPaths) {
        if (QFile::exists(ip)) {
            app.setWindowIcon(QIcon(ip));
            qInfo() << "Icon set from:" << ip;
            break;
        }
    }

    qInfo() << "Setting Quick style...";
    QQuickStyle::setStyle("Basic");

    qInfo() << "Creating models and backend...";
    ProcessModel procModel;
    qInfo() << "ProcessModel created";
    LogModel logModel;
    qInfo() << "LogModel created";
    ModelCatalogModel modelCatalogModel;
    qInfo() << "ModelCatalogModel created";
    Backend backend(&procModel, &logModel, &modelCatalogModel);
    qInfo() << "Backend created";

    qInfo() << "Creating QML engine...";
    qmlRegisterType<ModelPreviewItem>("NoteBot", 1, 0, "ModelPreview");
    QQmlApplicationEngine engine;
    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty("processModel", &procModel);
    ctx->setContextProperty("logModel", &logModel);
    ctx->setContextProperty("modelCatalogModel", &modelCatalogModel);
    ctx->setContextProperty("backend", &backend);

    qInfo() << "Loading EXE bootstrap QML ...";
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "QML rootObjects isEmpty - load failed";
        QString errMsg = "QML 界面加载失败。\n"
                         "可能原因：\n"
                         "1. EXE 内置 QML 资源缺失或损坏\n"
                         "2. Qt 静态链接资源表异常\n"
                         "\n请检查：\n"
                         "- %LOCALAPPDATA%\\NoteBotInjector\\notebot_injector_debug.log";
        MessageBoxW(nullptr,
            reinterpret_cast<const wchar_t*>(errMsg.utf16()),
            L"NoteBot Injector - 界面加载失败",
            MB_OK | MB_ICONERROR);
        return 1;
    }
    qInfo() << "QML loaded successfully, rootObjects count:" << engine.rootObjects().size();

    // 窗口显示后，异步执行初始化（本地验证 + 更新检查）
    // singleShot 让 QML 先渲染一次窗口，实际延迟在 initializeAsync() 内部控制
    QTimer::singleShot(0, &backend, [&]() {
        backend.initializeAsync();
    });

    int rc = app.exec();
    ReleaseMutex(singleInstance);
    CloseHandle(singleInstance);
    return rc;
}
