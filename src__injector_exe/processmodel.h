#ifndef PROCESSMODEL_H
#define PROCESSMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <QList>

struct ProcessItem {
    uint pid = 0;
    QString exe;
    QString title;
    bool hasWindow = false;
    bool selected = false;
    QString path;
};

class ProcessModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        PidRole = Qt::UserRole + 1,
        ExeRole,
        TitleRole,
        HasWindowRole,
        SelectedRole,
        PathRole
    };
    Q_ENUM(Roles)

    explicit ProcessModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateList(const QList<ProcessItem> &items, uint selectedPid);
    void setSelected(uint pid);

private:
    QList<ProcessItem> m_items;
};

#endif // PROCESSMODEL_H
