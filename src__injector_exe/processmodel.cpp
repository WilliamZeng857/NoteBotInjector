#include "processmodel.h"

ProcessModel::ProcessModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProcessModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_items.size();
}

QVariant ProcessModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const ProcessItem &item = m_items.at(index.row());
    switch (role) {
    case PidRole: return item.pid;
    case ExeRole: return item.exe;
    case TitleRole: return item.title;
    case HasWindowRole: return item.hasWindow;
    case SelectedRole: return item.selected;
    case PathRole: return item.path;
    default: return QVariant();
    }
}

QHash<int, QByteArray> ProcessModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[PidRole] = "pid";
    names[ExeRole] = "exe";
    names[TitleRole] = "winTitle";
    names[HasWindowRole] = "hasWindow";
    names[SelectedRole] = "isSelected";
    names[PathRole] = "path";
    return names;
}

void ProcessModel::updateList(const QList<ProcessItem> &items, uint selectedPid)
{
    // Check if the list structure changed (different pids or order)
    bool structureChanged = (m_items.size() != items.size());
    if (!structureChanged) {
        for (int i = 0; i < m_items.size(); ++i) {
            if (m_items[i].pid != items[i].pid) {
                structureChanged = true;
                break;
            }
        }
    }

    if (structureChanged) {
        // Full reset only when processes are added/removed/reordered
        beginResetModel();
        m_items = items;
        for (auto &it : m_items) {
            it.selected = (it.pid == selectedPid);
        }
        endResetModel();
    } else {
        // Structure unchanged — only emit dataChanged for modified fields
        for (int i = 0; i < m_items.size(); ++i) {
            const auto &ni = items[i];
            auto &cur = m_items[i];
            bool changed = false;
            if (cur.exe != ni.exe) { cur.exe = ni.exe; changed = true; }
            if (cur.title != ni.title) { cur.title = ni.title; changed = true; }
            if (cur.hasWindow != ni.hasWindow) { cur.hasWindow = ni.hasWindow; changed = true; }
            if (cur.path != ni.path) { cur.path = ni.path; changed = true; }
            bool selChanged = (cur.selected != (ni.pid == selectedPid));
            cur.selected = (ni.pid == selectedPid);
            if (changed || selChanged) {
                QModelIndex idx = index(i);
                emit dataChanged(idx, idx);
            }
        }
    }
}

void ProcessModel::setSelected(uint pid)
{
    for (int i = 0; i < m_items.size(); ++i) {
        bool old = m_items[i].selected;
        m_items[i].selected = (m_items[i].pid == pid);
        if (old != m_items[i].selected) {
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {SelectedRole});
        }
    }
}
