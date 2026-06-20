#ifndef MODELCATALOGMODEL_H
#define MODELCATALOGMODEL_H

#include <QAbstractListModel>
#include <QVector>

struct ModelCatalogEntry {
    QString modelId;
    QString name;
    QString subtitle;
    QString stateCode;
    QString stateLabel;
    QString modelFile;
    QString textureFile;
    QString coverFile;
    bool owned = false;
    bool active = false;
    qreal startYaw = 0.0;
    QString footerLeft;
    QString footerRight;
};

class ModelCatalogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role {
        ModelIdRole = Qt::UserRole + 1,
        NameRole,
        SubtitleRole,
        StateCodeRole,
        StateLabelRole,
        ModelFileRole,
        TextureFileRole,
        CoverFileRole,
        OwnedRole,
        ActiveRole,
        StartYawRole,
        FooterLeftRole,
        FooterRightRole,
    };

    explicit ModelCatalogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int count() const { return m_entries.size(); }
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(const QVector<ModelCatalogEntry> &entries);
    void setActiveModelId(const QString &modelId);
    bool activeEntry(ModelCatalogEntry *out) const;
    void clear();

signals:
    void countChanged();

private:
    QVector<ModelCatalogEntry> m_entries;
};

#endif // MODELCATALOGMODEL_H
