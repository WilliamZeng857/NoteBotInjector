#include "modelcatalogmodel.h"

ModelCatalogModel::ModelCatalogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ModelCatalogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant ModelCatalogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const ModelCatalogEntry &entry = m_entries.at(index.row());
    switch (role) {
    case ModelIdRole:
        return entry.modelId;
    case NameRole:
        return entry.name;
    case SubtitleRole:
        return entry.subtitle;
    case StateCodeRole:
        return entry.stateCode;
    case StateLabelRole:
        return entry.stateLabel;
    case ModelFileRole:
        return entry.modelFile;
    case TextureFileRole:
        return entry.textureFile;
    case CoverFileRole:
        return entry.coverFile;
    case OwnedRole:
        return entry.owned;
    case ActiveRole:
        return entry.active;
    case StartYawRole:
        return entry.startYaw;
    case FooterLeftRole:
        return entry.footerLeft;
    case FooterRightRole:
        return entry.footerRight;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModelCatalogModel::roleNames() const
{
    return {
        {ModelIdRole, "modelId"},
        {NameRole, "name"},
        {SubtitleRole, "subtitle"},
        {StateCodeRole, "stateCode"},
        {StateLabelRole, "stateLabel"},
        {ModelFileRole, "modelFile"},
        {TextureFileRole, "textureFile"},
        {CoverFileRole, "coverFile"},
        {OwnedRole, "owned"},
        {ActiveRole, "active"},
        {StartYawRole, "startYaw"},
        {FooterLeftRole, "footerLeft"},
        {FooterRightRole, "footerRight"},
    };
}

void ModelCatalogModel::setEntries(const QVector<ModelCatalogEntry> &entries)
{
    const int oldCount = m_entries.size();
    beginResetModel();
    m_entries = entries;
    endResetModel();
    if (oldCount != m_entries.size()) {
        emit countChanged();
    }
}

void ModelCatalogModel::setActiveModelId(const QString &modelId)
{
    bool changed = false;
    for (ModelCatalogEntry &entry : m_entries) {
        const bool nextActive = entry.owned && !modelId.isEmpty() && entry.modelId == modelId;
        const QString nextStateCode = nextActive
            ? QStringLiteral("active")
            : entry.owned
              ? QStringLiteral("owned")
              : QStringLiteral("unowned");
        const QString nextStateLabel = nextActive
            ? QStringLiteral("已激活")
            : entry.owned
              ? QStringLiteral("已拥有")
              : QStringLiteral("未拥有");
        if (entry.active != nextActive ||
            entry.stateCode != nextStateCode ||
            entry.stateLabel != nextStateLabel) {
            entry.active = nextActive;
            entry.stateCode = nextStateCode;
            entry.stateLabel = nextStateLabel;
            changed = true;
        }
    }
    if (!changed || m_entries.isEmpty()) {
        return;
    }
    emit dataChanged(index(0, 0), index(m_entries.size() - 1, 0),
                     {StateCodeRole, StateLabelRole, ActiveRole});
}

void ModelCatalogModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    const int oldCount = m_entries.size();
    beginResetModel();
    m_entries.clear();
    endResetModel();
    if (oldCount != 0) {
        emit countChanged();
    }
}
