#include "logmodel.h"
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDir>

namespace {
constexpr int kMaxVisibleLogItems = 10000;

void appendPersistentLogLine(const QString &line)
{
    const QString appLocalDir =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QStringList logPaths{
        QDir(appLocalDir).absoluteFilePath("notebot_injector_debug.log"),
        QDir(appLocalDir).absoluteFilePath("state_v3/injector_ui.log"),
    };

    for (const QString &logPath : logPaths) {
        QDir().mkpath(QFileInfo(logPath).absolutePath());
        QFile f(logPath);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            continue;
        }
        QTextStream stream(&f);
        stream << line << "\n";
    }
}
}

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const LogItem &item = m_items.at(index.row());
    switch (role) {
    case TimeRole: return item.ts;
    case TagRole: return item.tag;
    case TextRole: return item.text;
    default: return QVariant();
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[TimeRole] = "ts";
    names[TagRole] = "tag";
    names[TextRole] = "text";
    return names;
}

void LogModel::append(const QString &msg)
{
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    const QString tsFull = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString tag = "sys";
    if (msg.startsWith("[OK!]") || msg.startsWith("[MEM]") || msg.startsWith("[THR]")) tag = "green";
    else if (msg.startsWith("[ERR]")) tag = "red";
    else if (msg.startsWith("[WRN]")) tag = "orange";
    else if (msg.startsWith("[SYS]") || msg.startsWith("[API]") || msg.startsWith("[SCN]")) tag = "cyan";
    else if (msg.startsWith("[INJ]") || msg.startsWith("[SEL]") || msg.startsWith("[WIN]") || msg.startsWith("[DLL]")) tag = "accent";

    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append({ts, tag, msg});
    endInsertRows();

    while (m_items.size() > kMaxVisibleLogItems) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_items.removeFirst();
        endRemoveRows();
    }

    appendPersistentLogLine(QString("[%1] %2").arg(tsFull, msg));
    emit lineAppended(ts + "  " + msg);
}

void LogModel::appendDebug(const QString &msg)
{
    // 只写文件日志，不显示在 UI 上
    appendPersistentLogLine(QString("[%1] %2").arg(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"),
        msg));
}
