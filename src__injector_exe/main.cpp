#include <QGuiApplication>
#include <QCoreApplication>
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
#include <QTimer>
#include <windows.h>
#include <shellapi.h>
#include <cmath>
#include "processmodel.h"
#include "logmodel.h"
#include "backend.h"
#include "modelcatalogmodel.h"
#include "modelpreviewitem.h"

static QFile *g_logFile = nullptr;

class AspectRatioWindowGuard final : public QObject
{
public:
    explicit AspectRatioWindowGuard(QWindow *window)
        : QObject(window)
        , m_window(window)
    {
        if (m_window) {
            m_window->setMinimumSize(QSize(kMinWidth, kMinHeight));
            m_hwnd = reinterpret_cast<HWND>(m_window->winId());
            installWindowProc();
            connect(m_window, &QWindow::widthChanged, this, [this]() {
                scheduleAdjust();
            });
            connect(m_window, &QWindow::heightChanged, this, [this]() {
                scheduleAdjust();
            });
        }
    }

    ~AspectRatioWindowGuard() override
    {
        if (m_hwnd && IsWindow(m_hwnd)) {
            if (m_subclassed && reinterpret_cast<WNDPROC>(GetWindowLongPtrW(m_hwnd, GWLP_WNDPROC)) == &AspectRatioWindowGuard::windowProc) {
                SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
            }
            RemovePropW(m_hwnd, guardPropertyName());
        }
    }

private:
    static constexpr int kMinWidth = 840;
    static constexpr int kMinHeight = 560;
    static constexpr double kAspect = 1.5;

    static const wchar_t *guardPropertyName()
    {
        return L"NoteBotAspectRatioWindowGuard";
    }

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto *guard = reinterpret_cast<AspectRatioWindowGuard *>(GetPropW(hwnd, guardPropertyName()));
        if (!guard) {
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
        return guard->handleWindowMessage(hwnd, message, wParam, lParam);
    }

    void installWindowProc()
    {
        if (!m_hwnd || !IsWindow(m_hwnd)) {
            return;
        }

        SetPropW(m_hwnd, guardPropertyName(), reinterpret_cast<HANDLE>(this));
        SetLastError(0);
        const LONG_PTR previous = SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&AspectRatioWindowGuard::windowProc));
        if (previous) {
            m_originalWndProc = reinterpret_cast<WNDPROC>(previous);
            m_subclassed = true;
        } else {
            RemovePropW(m_hwnd, guardPropertyName());
        }
    }

    LRESULT handleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (hwnd != m_hwnd || m_adjusting) {
            return callOriginal(hwnd, message, wParam, lParam);
        }

        if (message == WM_SIZING) {
            adjustSizingRect(static_cast<UINT>(wParam), reinterpret_cast<RECT *>(lParam));
            return TRUE;
        }

        const LRESULT result = callOriginal(hwnd, message, wParam, lParam);
        if (message == WM_SIZE || message == WM_WINDOWPOSCHANGED) {
            scheduleAdjust();
        }
        return result;
    }

    LRESULT callOriginal(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) const
    {
        if (m_originalWndProc) {
            return CallWindowProcW(m_originalWndProc, hwnd, message, wParam, lParam);
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    static int maxInt(int a, int b)
    {
        return a > b ? a : b;
    }

    void scheduleAdjust()
    {
        if (!m_window || m_adjusting || m_pendingAdjust) {
            return;
        }

        m_pendingAdjust = true;
        QTimer::singleShot(0, this, [this]() {
            m_pendingAdjust = false;
            adjustToAspect(m_window ? m_window->size() : QSize());
        });
    }

    QSize nativeClientSize() const
    {
        RECT clientRect {};
        if (!m_hwnd || !GetClientRect(m_hwnd, &clientRect)) {
            return {};
        }
        return QSize(clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
    }

    QSize frameDelta() const
    {
        RECT windowRect {};
        RECT clientRect {};
        if (!m_hwnd || !GetWindowRect(m_hwnd, &windowRect) || !GetClientRect(m_hwnd, &clientRect)) {
            return {};
        }

        const int outerWidth = windowRect.right - windowRect.left;
        const int outerHeight = windowRect.bottom - windowRect.top;
        const int clientWidth = clientRect.right - clientRect.left;
        const int clientHeight = clientRect.bottom - clientRect.top;
        return QSize(maxInt(0, outerWidth - clientWidth), maxInt(0, outerHeight - clientHeight));
    }

    QSize targetClientSize(const QSize &sourceSize) const
    {
        if (sourceSize.isEmpty()) {
            return QSize(kMinWidth, kMinHeight);
        }

        int targetWidth = maxInt(kMinWidth, sourceSize.width());
        int targetHeight = maxInt(kMinHeight, sourceSize.height());
        const double currentAspect = static_cast<double>(targetWidth) / targetHeight;

        if (currentAspect >= kAspect) {
            targetHeight = static_cast<int>(std::round(targetWidth / kAspect));
        } else {
            targetWidth = static_cast<int>(std::round(targetHeight * kAspect));
        }

        if (targetWidth < kMinWidth || targetHeight < kMinHeight) {
            return QSize(kMinWidth, kMinHeight);
        }
        return QSize(targetWidth, targetHeight);
    }

    QSize targetClientSizeForSizingEdge(UINT edge, const QSize &sourceSize) const
    {
        int targetWidth = maxInt(kMinWidth, sourceSize.width());
        int targetHeight = maxInt(kMinHeight, sourceSize.height());

        const auto fromWidth = [this](int width) {
            const int clampedWidth = maxInt(kMinWidth, width);
            return QSize(clampedWidth, maxInt(kMinHeight, static_cast<int>(std::round(clampedWidth / kAspect))));
        };
        const auto fromHeight = [this](int height) {
            const int clampedHeight = maxInt(kMinHeight, height);
            return QSize(maxInt(kMinWidth, static_cast<int>(std::round(clampedHeight * kAspect))), clampedHeight);
        };

        if (edge == WMSZ_LEFT || edge == WMSZ_RIGHT) {
            return fromWidth(targetWidth);
        }

        if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM) {
            return fromHeight(targetHeight);
        }

        const QSize currentSize = nativeClientSize();
        if (!currentSize.isEmpty()) {
            const int widthDelta = std::abs(targetWidth - currentSize.width());
            const int heightDelta = static_cast<int>(std::round(std::abs(targetHeight - currentSize.height()) * kAspect));
            return widthDelta >= heightDelta ? fromWidth(targetWidth) : fromHeight(targetHeight);
        }

        return targetClientSize(sourceSize);
    }

    void adjustSizingRect(UINT edge, RECT *rect) const
    {
        if (!rect) {
            return;
        }

        const QSize frame = frameDelta();
        const int outerWidth = rect->right - rect->left;
        const int outerHeight = rect->bottom - rect->top;
        const QSize clientTarget = targetClientSizeForSizingEdge(edge, QSize(outerWidth - frame.width(), outerHeight - frame.height()));
        const int targetOuterWidth = clientTarget.width() + frame.width();
        const int targetOuterHeight = clientTarget.height() + frame.height();

        const bool leftAnchored = edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT;
        const bool topAnchored = edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT;

        if (leftAnchored) {
            rect->left = rect->right - targetOuterWidth;
        } else {
            rect->right = rect->left + targetOuterWidth;
        }

        if (topAnchored) {
            rect->top = rect->bottom - targetOuterHeight;
        } else {
            rect->bottom = rect->top + targetOuterHeight;
        }
    }

    void adjustToAspect(const QSize &newSize)
    {
        if (!m_window || m_adjusting || newSize.isEmpty()) {
            return;
        }

        const QSize currentClientSize = nativeClientSize();
        const QSize clientTarget = targetClientSize(currentClientSize.isEmpty() ? newSize : currentClientSize);

        if (currentClientSize == clientTarget) {
            return;
        }

        m_adjusting = true;
        const QSize frame = frameDelta();
        SetWindowPos(m_hwnd,
                     nullptr,
                     0,
                     0,
                     clientTarget.width() + frame.width(),
                     clientTarget.height() + frame.height(),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        m_adjusting = false;
    }

    QWindow *m_window = nullptr;
    HWND m_hwnd = nullptr;
    WNDPROC m_originalWndProc = nullptr;
    bool m_adjusting = false;
    bool m_pendingAdjust = false;
    bool m_subclassed = false;
};

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
    if (auto *rootWindow = qobject_cast<QWindow *>(engine.rootObjects().constFirst())) {
        new AspectRatioWindowGuard(rootWindow);
    } else {
        qWarning() << "QML root object is not a QWindow; aspect ratio guard not installed";
    }

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
