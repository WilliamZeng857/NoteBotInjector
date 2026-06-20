#ifndef MODELPREVIEWITEM_H
#define MODELPREVIEWITEM_H

#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QMatrix4x4>
#include <QPointF>
#include <QQuickRhiItem>
#include <QTimer>
#include <QVector>
#include <QVector3D>
#include <limits>

class QMouseEvent;
class QHoverEvent;
class QWheelEvent;
class QQuickRhiItemRenderer;

class ModelPreviewItem : public QQuickRhiItem
{
    Q_OBJECT
    Q_PROPERTY(QString modelSource READ modelSource WRITE setModelSource NOTIFY modelSourceChanged)
    Q_PROPERTY(QString textureSource READ textureSource WRITE setTextureSource NOTIFY textureSourceChanged)
    Q_PROPERTY(bool autoRotate READ autoRotate WRITE setAutoRotate NOTIFY autoRotateChanged)
    Q_PROPERTY(qreal yaw READ yaw WRITE setYaw NOTIFY yawChanged)
    Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)

public:
    explicit ModelPreviewItem(QQuickItem *parent = nullptr);

    QString modelSource() const { return m_modelSource; }
    QString textureSource() const { return m_textureSource; }
    bool autoRotate() const { return m_autoRotate; }
    qreal yaw() const { return m_yaw; }
    qreal pitch() const { return m_pitch; }
    qreal zoom() const { return m_zoom; }
    QString status() const { return m_status; }

    void setModelSource(const QString &source);
    void setTextureSource(const QString &source);
    void setAutoRotate(bool enabled);
    void setYaw(qreal value);
    void setPitch(qreal value);
    void setZoom(qreal value);

    Q_INVOKABLE void resetView();
    Q_INVOKABLE void captureDefaultView();

signals:
    void modelSourceChanged();
    void textureSourceChanged();
    void autoRotateChanged();
    void yawChanged();
    void pitchChanged();
    void zoomChanged();
    void statusChanged();

public:
    struct Face {
        bool enabled = true;
        qreal uv[4] = {0.0, 0.0, 16.0, 16.0};
        int rotation = 0;
    };

    struct Cube {
        QVector3D from;
        QVector3D to;
        QVector3D size;
        QVector3D pivot;
        QVector3D rotation;
        qreal inflate = 0.0;
        bool boxUv = false;
        bool mirrorUv = false;
        bool rescale = false;
        qreal uvOffset[2] = {0.0, 0.0};
        Face faces[6];
    };

    struct Bone {
        QString name;
        QString parent;
        QVector3D origin;
        QVector3D rotation;
        QVector<Cube> cubes;
        QMatrix4x4 world;
    };

    struct ProjectedVertex {
        QPointF pt;
        QVector3D pos;
        QPointF uv;
        qreal depth = 0.0;
        qreal depthBias = 0.0;
    };

    struct Triangle {
        ProjectedVertex v[3];
    };

    QVector<Triangle> gpuTriangles(const QSize &targetSize) const;
    QImage gpuTextureImage() const;
    QMatrix4x4 gpuMvpMatrix(const QSize &targetSize) const;
    bool consumeGeometryDirty();
    bool consumeTextureDirty();

protected:
    QQuickRhiItemRenderer *createRenderer() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:

    void reloadAssets();
    bool loadModel(const QString &source);
    bool loadTexture(const QString &source);
    void parseCube(const QJsonObject &obj, const QJsonObject &boneObj, Cube *cube) const;
    void updateBoxUv(Cube *cube) const;
    void buildBoneMatrices();
    QVector<Triangle> buildTriangles(const QSize &targetSize, qreal padding) const;
    void setStatus(const QString &status);
    void scheduleAutoRotateResume();

    static QString localSourcePath(const QString &source);
    static QVector3D jsonVec3(const QJsonValue &value, const QVector3D &fallback);
    static QVector3D convertPivot(const QJsonArray &arr);
    static QVector3D convertRotation(const QJsonArray &arr);
    static QMatrix4x4 rotationMatrixZyx(const QVector3D &degrees);
    static QVector3D applyBlockbenchRescale(const QVector3D &scaleIn, const QVector3D &rotationDegrees);

    QString m_modelSource;
    QString m_textureSource;
    QString m_status = QStringLiteral("等待模型");
    QImage m_textureImage;
    QVector<Bone> m_bones;
    int m_modelTextureWidth = 64;
    int m_modelTextureHeight = 64;
    bool m_autoRotate = true;
    qreal m_yaw = 180.0;
    qreal m_pitch = -8.0;
    qreal m_zoom = 1.0;
    qreal m_defaultYaw = 180.0;
    qreal m_defaultPitch = -8.0;
    qreal m_defaultZoom = 1.0;
    bool m_dragging = false;
    QPointF m_lastMousePos;
    QTimer m_rotateTimer;
    QTimer m_resumeAutoRotateTimer;
    bool m_geometryDirty = true;
    bool m_textureDirty = true;
    mutable QSize m_cachedTriangleSize;
    mutable QVector<Triangle> m_cachedTriangles;
    mutable bool m_cachedTrianglesDirty = true;
    mutable QVector3D m_cachedBoundsMin;
    mutable QVector3D m_cachedBoundsMax;
    mutable bool m_cachedBoundsValid = false;
    bool m_hoverTracking = false;
    qreal m_hoverBaseYaw = 180.0;
    qreal m_hoverBasePitch = -8.0;
    qreal m_hoverTargetYaw = 180.0;
    qreal m_hoverTargetPitch = -8.0;
};

#endif
