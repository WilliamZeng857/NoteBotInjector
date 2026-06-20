#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QList>

struct LogItem {
    QString ts;
    QString tag;
    QString text;
};

class LogModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TimeRole = Qt::UserRole + 1,
        TagRole,
        TextRole
    };
    Q_ENUM(Roles)

    explicit LogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString &msg);
    void appendDebug(const QString &msg);

signals:
    void lineAppended(const QString &line);

private:
    QList<LogItem> m_items;
};

#endif // LOGMODEL_H
