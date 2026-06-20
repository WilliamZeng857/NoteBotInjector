#include <windows.h>
#include <shellapi.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QStringConverter>
#include <QStringList>
#include <QTextStream>

namespace {

QString localAppDataPath()
{
    wchar_t buffer[MAX_PATH] = {0};
    const DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        return QStringLiteral("C:/Users/William/AppData/Local");
    }
    return QString::fromWCharArray(buffer, static_cast<int>(len));
}

QString updaterLogPath()
{
    return QDir(localAppDataPath())
        .absoluteFilePath(QStringLiteral("NoteBotInjector/updater/logs/updater.log"));
}

QString nowUtcIso()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

void appendLog(const QString &action,
               const QString &src,
               const QString &dst,
               const QString &backup,
               quint32 oldPid,
               const QString &status,
               quint32 errorCode,
               const QString &errorMessage)
{
    const QString path = updaterLogPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << "ts_utc=" << nowUtcIso()
        << " action=" << action
        << " src=\"" << src << "\""
        << " dst=\"" << dst << "\""
        << " backup=\"" << backup << "\""
        << " old_pid=" << oldPid
        << " status=" << status
        << " error_code=" << errorCode
        << " error_message=\"" << errorMessage << "\"\n";
}

QString argValue(const QStringList &args, const QString &name)
{
    const int index = args.indexOf(name);
    if (index < 0 || index + 1 >= args.size()) {
        return QString();
    }
    return args.at(index + 1).trimmed();
}

quint32 argUInt(const QStringList &args, const QString &name)
{
    bool ok = false;
    const quint32 value = argValue(args, name).toUInt(&ok);
    return ok ? value : 0;
}

bool waitForExit(quint32 pid, QString *errorMessage)
{
    if (pid == 0) {
        return true;
    }
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!process) {
        return true;
    }
    const DWORD rc = WaitForSingleObject(process, 60000);
    CloseHandle(process);
    if (rc == WAIT_OBJECT_0) {
        return true;
    }
    if (errorMessage) {
        *errorMessage = (rc == WAIT_TIMEOUT)
            ? QStringLiteral("等待目标进程退出超时")
            : QStringLiteral("等待目标进程退出失败");
    }
    return false;
}

QString sha256Hex(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(64 * 1024);
        if (chunk.isEmpty() && file.error() != QFile::NoError) {
            return QString();
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex()).toLower();
}

bool replaceFile(const QString &src,
                 const QString &dst,
                 const QString &backup,
                 quint32 waitPid,
                 const QString &restartTarget,
                 QString *errorMessage)
{
    if (src.isEmpty() || dst.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("缺少 src 或 dst");
        }
        return false;
    }
    if (!QFile::exists(src)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("源文件不存在");
        }
        return false;
    }
    if (!waitForExit(waitPid, errorMessage)) {
        return false;
    }

    QDir().mkpath(QFileInfo(dst).absolutePath());
    if (!backup.isEmpty()) {
        QDir().mkpath(QFileInfo(backup).absolutePath());
        QFile::remove(backup);
        if (QFile::exists(dst) && !QFile::copy(dst, backup)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("创建 backup 失败");
            }
            return false;
        }
    }

    if (QFile::exists(dst) && !QFile::remove(dst)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("删除旧文件失败");
        }
        return false;
    }

    if (!QFile::copy(src, dst)) {
        if (!backup.isEmpty() && QFile::exists(backup)) {
            QFile::remove(dst);
            QFile::copy(backup, dst);
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("复制新文件失败");
        }
        return false;
    }

    const QString srcSha = sha256Hex(src);
    const QString dstSha = sha256Hex(dst);
    if (srcSha.isEmpty() || dstSha.isEmpty() || srcSha != dstSha) {
        QFile::remove(dst);
        if (!backup.isEmpty() && QFile::exists(backup)) {
            QFile::copy(backup, dst);
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("复制后校验失败");
        }
        return false;
    }

    if (!restartTarget.isEmpty() && !QProcess::startDetached(restartTarget, {})) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("重启主程序失败");
        }
        return false;
    }
    return true;
}

int fail(const QString &action,
         const QString &src,
         const QString &dst,
         const QString &backup,
         quint32 pid,
         const QString &message,
         quint32 errorCode = 1)
{
    appendLog(action, src, dst, backup, pid, QStringLiteral("failed"), errorCode, message);
    return static_cast<int>(errorCode);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    QStringList args;
    for (int i = 0; i < argc; ++i) {
        args.append(QString::fromWCharArray(argv[i]));
    }
    if (argv) {
        LocalFree(argv);
    }

    const QString action = args.size() > 1 ? args.at(1).trimmed() : QString();
    const QString src = argValue(args, QStringLiteral("--src"));
    const QString dst = argValue(args, QStringLiteral("--dst"));
    const QString backup = argValue(args, QStringLiteral("--backup"));
    const QString restart = argValue(args, QStringLiteral("--restart"));
    const quint32 pid = argUInt(args, QStringLiteral("--pid")) != 0
        ? argUInt(args, QStringLiteral("--pid"))
        : argUInt(args, QStringLiteral("--wait-pid"));

    if (action != QStringLiteral("--replace-main") &&
        action != QStringLiteral("--replace-auth-dll") &&
        action != QStringLiteral("--replace-file")) {
        return fail(action, src, dst, backup, pid, QStringLiteral("不支持的命令"));
    }

    if (action == QStringLiteral("--replace-auth-dll") && !restart.isEmpty()) {
        return fail(action, src, dst, backup, pid, QStringLiteral("replace-auth-dll 不允许重启参数"));
    }

    QString errorMessage;
    if (!replaceFile(src, dst, backup, pid, restart, &errorMessage)) {
        return fail(action, src, dst, backup, pid, errorMessage, 2);
    }

    appendLog(action, src, dst, backup, pid, QStringLiteral("ok"), 0, QStringLiteral("success"));
    return 0;
}
