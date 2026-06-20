#include "modelpreviewitem.h"

#include <QFile>
#include <QHoverEvent>
#include <QImage>
#include <QJsonDocument>
#include <QUrl>
#include <QMouseEvent>
#include <QWheelEvent>
#include <rhi/qrhi.h>
#include <rhi/qshader.h>
#include <QtMath>
#include <algorithm>
#include <limits>
#include <memory>

namespace {
constexpr int FACE_EAST = 0;
constexpr int FACE_WEST = 1;
constexpr int FACE_UP = 2;
constexpr int FACE_DOWN = 3;
constexpr int FACE_SOUTH = 4;
constexpr int FACE_NORTH = 5;

QJsonArray arrayAt(const QJsonObject &obj, const QString &key)
{
    return obj.value(key).toArray();
}

qreal numAt(const QJsonArray &arr, int index, qreal fallback = 0.0)
{
    return index >= 0 && index < arr.size() ? arr.at(index).toDouble(fallback) : fallback;
}

void setFace(ModelPreviewItem::Face *face, qreal x1, qreal y1, qreal x2, qreal y2)
{
    face->enabled = true;
    face->uv[0] = x1;
    face->uv[1] = y1;
    face->uv[2] = x2;
    face->uv[3] = y2;
}

void mapUvCorners(const ModelPreviewItem::Face &face,
                  int textureWidth,
                  int textureHeight,
                  QPointF out[4])
{
    const qreal insetX = qAbs(face.uv[2] - face.uv[0]) > 0.0001
        ? (face.uv[2] >= face.uv[0] ? 0.5 : -0.5)
        : 0.0;
    const qreal insetY = qAbs(face.uv[3] - face.uv[1]) > 0.0001
        ? (face.uv[3] >= face.uv[1] ? 0.5 : -0.5)
        : 0.0;

    QPointF corners[4] = {
        QPointF((face.uv[0] + insetX) / textureWidth, (face.uv[1] + insetY) / textureHeight),
        QPointF((face.uv[2] - insetX) / textureWidth, (face.uv[1] + insetY) / textureHeight),
        QPointF((face.uv[0] + insetX) / textureWidth, (face.uv[3] - insetY) / textureHeight),
        QPointF((face.uv[2] - insetX) / textureWidth, (face.uv[3] - insetY) / textureHeight)
    };

    int rotation = face.rotation;
    while (rotation < 0) rotation += 360;
    while (rotation >= 360) rotation -= 360;
    while (rotation > 0) {
        const QPointF first = corners[0];
        corners[0] = corners[2];
        corners[2] = corners[3];
        corners[3] = corners[1];
        corners[1] = first;
        rotation -= 90;
    }

    out[0] = corners[0];
    out[1] = corners[1];
    out[2] = corners[2];
    out[3] = corners[3];
}

qreal faceUvArea(const ModelPreviewItem::Face &face)
{
    return qAbs(face.uv[2] - face.uv[0]) * qAbs(face.uv[3] - face.uv[1]);
}

bool faceLooksPlaceholder(const ModelPreviewItem::Face &face)
{
    return faceUvArea(face) <= 1.05;
}

bool facesUseSameUv(const ModelPreviewItem::Face &left, const ModelPreviewItem::Face &right)
{
    if (left.rotation != right.rotation) {
        return false;
    }
    for (int i = 0; i < 4; ++i) {
        if (qAbs(left.uv[i] - right.uv[i]) > 0.01) {
            return false;
        }
    }
    return true;
}

struct GpuVertex
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    float depthBias = 0.0f;
};

static QShader loadShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
}

static QVector<GpuVertex> trianglesToGpuVertices(const QVector<ModelPreviewItem::Triangle> &triangles,
                                                 const QSize &targetSize)
{
    QVector<GpuVertex> vertices;
    if (triangles.isEmpty() || targetSize.width() <= 0 || targetSize.height() <= 0) {
        return vertices;
    }

    vertices.reserve(triangles.size() * 3);
    for (const ModelPreviewItem::Triangle &tri : triangles) {
        for (const ModelPreviewItem::ProjectedVertex &v : tri.v) {
            GpuVertex out;
            out.x = v.pos.x();
            out.y = v.pos.y();
            out.z = v.pos.z();
            out.u = static_cast<float>(v.uv.x());
            out.v = static_cast<float>(v.uv.y());
            out.depthBias = static_cast<float>(v.depthBias);
            vertices.push_back(out);
        }
    }
    return vertices;
}

class ModelPreviewRhiRenderer final : public QQuickRhiItemRenderer
{
protected:
    void initialize(QRhiCommandBuffer *cb) override;
    void synchronize(QQuickRhiItem *item) override;
    void render(QRhiCommandBuffer *cb) override;

private:
    void resetScene();
    void initScene();
    void uploadTexture();
    void uploadVertices();
    void uploadMatrix();

    QRhi *m_rhi = nullptr;
    int m_sampleCount = 1;
    QSize m_outputSize;
    QVector<GpuVertex> m_vertices;
    QImage m_textureImage;
    QMatrix4x4 m_mvp;
    bool m_verticesDirty = true;
    bool m_textureDirty = true;
    bool m_matrixDirty = true;
    bool m_sceneReady = false;
    QRhiResourceUpdateBatch *m_resourceUpdates = nullptr;
    std::unique_ptr<QRhiBuffer> m_vertexBuffer;
    std::unique_ptr<QRhiBuffer> m_uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiTexture> m_texture;
};
}

void ModelPreviewRhiRenderer::resetScene()
{
    m_pipeline.reset();
    m_srb.reset();
    m_sampler.reset();
    m_texture.reset();
    m_vertexBuffer.reset();
    m_uniformBuffer.reset();
    m_resourceUpdates = nullptr;
    m_sceneReady = false;
    m_verticesDirty = true;
    m_textureDirty = true;
    m_matrixDirty = true;
}

void ModelPreviewRhiRenderer::initialize(QRhiCommandBuffer *)
{
    if (m_rhi != rhi()) {
        m_rhi = rhi();
        resetScene();
    }

    if (!renderTarget()) {
        return;
    }

    const int sampleCount = renderTarget()->sampleCount();
    const QSize outputSize = renderTarget()->pixelSize();
    if (m_sampleCount != sampleCount || m_outputSize != outputSize) {
        m_sampleCount = sampleCount;
        m_outputSize = outputSize;
        resetScene();
    }

    if (!m_sceneReady && m_rhi) {
        initScene();
    }
}

void ModelPreviewRhiRenderer::synchronize(QQuickRhiItem *item)
{
    auto *preview = static_cast<ModelPreviewItem *>(item);
    const QSize outputSize = preview->effectiveColorBufferSize();
    const bool geometryDirty = preview->consumeGeometryDirty();
    if (geometryDirty || m_outputSize != outputSize || m_vertices.isEmpty()) {
        const QVector<ModelPreviewItem::Triangle> triangles = preview->gpuTriangles(outputSize);
        const QVector<GpuVertex> vertices = trianglesToGpuVertices(triangles, outputSize);
        if (vertices.size() != m_vertices.size() || geometryDirty || m_outputSize != outputSize) {
            m_vertices = vertices;
            m_verticesDirty = true;
        }
    }

    m_mvp = preview->gpuMvpMatrix(outputSize);
    m_matrixDirty = true;

    const QImage texture = preview->gpuTextureImage();
    if (preview->consumeTextureDirty() || texture.cacheKey() != m_textureImage.cacheKey()) {
        m_textureImage = texture;
        m_textureDirty = true;
    }

}

void ModelPreviewRhiRenderer::initScene()
{
    const qsizetype vertexBytes = std::max(qsizetype(sizeof(GpuVertex)), m_vertices.size() * qsizetype(sizeof(GpuVertex)));
    m_vertexBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, int(vertexBytes)));
    m_vertexBuffer->create();

    m_uniformBuffer.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    m_uniformBuffer->create();

    const QSize textureSize = m_textureImage.isNull() ? QSize(1, 1) : m_textureImage.size();
    m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, textureSize));
    m_texture->create();

    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Nearest,
                                      QRhiSampler::Nearest,
                                      QRhiSampler::None,
                                      QRhiSampler::ClampToEdge,
                                      QRhiSampler::ClampToEdge));
    m_sampler->create();

    m_srb.reset(m_rhi->newShaderResourceBindings());
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, m_uniformBuffer.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_texture.get(), m_sampler.get())
    });
    m_srb->create();

    m_pipeline.reset(m_rhi->newGraphicsPipeline());
    m_pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    m_pipeline->setDepthTest(true);
    m_pipeline->setDepthWrite(true);
    m_pipeline->setDepthOp(QRhiGraphicsPipeline::Less);
    m_pipeline->setCullMode(QRhiGraphicsPipeline::None);
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = false;
    m_pipeline->setTargetBlends({ blend });
    const QShader vs = loadShader(QStringLiteral(":/shaders/modelpreview.vert.qsb"));
    const QShader fs = loadShader(QStringLiteral(":/shaders/modelpreview.frag.qsb"));
    m_pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 6 * int(sizeof(float)) } });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * int(sizeof(float)) },
        { 0, 2, QRhiVertexInputAttribute::Float, 5 * int(sizeof(float)) }
    });
    m_pipeline->setVertexInputLayout(inputLayout);
    m_pipeline->setSampleCount(m_sampleCount);
    m_pipeline->setShaderResourceBindings(m_srb.get());
    m_pipeline->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    m_pipeline->create();

    m_sceneReady = true;
    m_verticesDirty = true;
    m_textureDirty = true;
    m_matrixDirty = true;
}

void ModelPreviewRhiRenderer::uploadTexture()
{
    if (!m_rhi || !m_texture || !m_textureDirty) {
        return;
    }

    QImage image = m_textureImage;
    if (image.isNull()) {
        image = QImage(1, 1, QImage::Format_RGBA8888);
        image.fill(Qt::transparent);
    } else {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    if (m_texture->pixelSize() != image.size()) {
        resetScene();
        initScene();
    }

    if (!m_resourceUpdates) {
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }
    m_resourceUpdates->uploadTexture(m_texture.get(), image);
    m_textureDirty = false;
}

void ModelPreviewRhiRenderer::uploadVertices()
{
    if (!m_rhi || !m_vertexBuffer || !m_verticesDirty) {
        return;
    }

    const qsizetype bytes = m_vertices.size() * qsizetype(sizeof(GpuVertex));
    if (bytes > m_vertexBuffer->size()) {
        resetScene();
        initScene();
    }

    if (!m_resourceUpdates) {
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }
    if (bytes > 0) {
        m_resourceUpdates->updateDynamicBuffer(m_vertexBuffer.get(), 0, int(bytes), m_vertices.constData());
    }
    m_verticesDirty = false;
}

void ModelPreviewRhiRenderer::uploadMatrix()
{
    if (!m_rhi || !m_uniformBuffer || !m_matrixDirty) {
        return;
    }

    if (!m_resourceUpdates) {
        m_resourceUpdates = m_rhi->nextResourceUpdateBatch();
    }
    m_resourceUpdates->updateDynamicBuffer(m_uniformBuffer.get(), 0, 64, m_mvp.constData());
    m_matrixDirty = false;
}

void ModelPreviewRhiRenderer::render(QRhiCommandBuffer *cb)
{
    if (!m_sceneReady || !m_pipeline || !m_vertexBuffer || !renderTarget()) {
        return;
    }

    uploadTexture();
    uploadVertices();
    uploadMatrix();

    QRhiResourceUpdateBatch *rub = m_resourceUpdates;
    m_resourceUpdates = nullptr;

    cb->beginPass(renderTarget(), Qt::transparent, { 1.0f, 0 }, rub);
    if (!m_vertices.isEmpty()) {
        cb->setGraphicsPipeline(m_pipeline.get());
        const QSize outputSize = renderTarget()->pixelSize();
        cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(m_vertexBuffer.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(quint32(m_vertices.size()));
    }
    cb->endPass();
}

ModelPreviewItem::ModelPreviewItem(QQuickItem *parent)
    : QQuickRhiItem(parent)
{
    setSampleCount(1);
    setAlphaBlending(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);

    m_rotateTimer.setInterval(13);
    m_rotateTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_rotateTimer, &QTimer::timeout, this, [this]() {
        if (m_dragging) {
            return;
        }
        if (m_hoverTracking) {
            setYaw(m_yaw + (m_hoverTargetYaw - m_yaw) * 0.34);
            setPitch(m_pitch + (m_hoverTargetPitch - m_pitch) * 0.34);
            return;
        }
        if (qAbs(m_pitch - m_defaultPitch) > 0.01) {
            setPitch(m_pitch + (m_defaultPitch - m_pitch) * 0.20);
        }
        if (!m_autoRotate) {
            return;
        }
        setYaw(m_yaw + 0.50);
    });
    m_rotateTimer.start();

    m_resumeAutoRotateTimer.setInterval(280);
    m_resumeAutoRotateTimer.setSingleShot(true);
    connect(&m_resumeAutoRotateTimer, &QTimer::timeout, this, [this]() {
        if (m_autoRotate && !m_dragging && !m_hoverTracking) {
            m_rotateTimer.start();
        }
    });
}

QString ModelPreviewItem::localSourcePath(const QString &source)
{
    if (source.startsWith(QStringLiteral("qrc:/"))) {
        return QStringLiteral(":") + source.mid(4);
    }
    if (source.startsWith(QStringLiteral("file:/"))) {
        return QUrl(source).toLocalFile();
    }
    return source;
}

QVector3D ModelPreviewItem::jsonVec3(const QJsonValue &value, const QVector3D &fallback)
{
    const QJsonArray arr = value.toArray();
    if (arr.size() < 3) return fallback;
    return QVector3D(arr.at(0).toDouble(), arr.at(1).toDouble(), arr.at(2).toDouble());
}

QVector3D ModelPreviewItem::convertPivot(const QJsonArray &arr)
{
    return QVector3D(-numAt(arr, 0), numAt(arr, 1), numAt(arr, 2));
}

QVector3D ModelPreviewItem::convertRotation(const QJsonArray &arr)
{
    return QVector3D(-numAt(arr, 0), -numAt(arr, 1), numAt(arr, 2));
}

QMatrix4x4 ModelPreviewItem::rotationMatrixZyx(const QVector3D &degrees)
{
    QMatrix4x4 m;
    m.rotate(degrees.z(), 0, 0, 1);
    m.rotate(degrees.y(), 0, 1, 0);
    m.rotate(degrees.x(), 1, 0, 0);
    return m;
}

QVector3D ModelPreviewItem::applyBlockbenchRescale(const QVector3D &scaleIn,
                                                   const QVector3D &rotationDegrees)
{
    auto factor = [](qreal angle) {
        qreal value = qAbs(angle);
        if (value > 45.0) value = 90.0 - value;
        return 1.0 / qCos(qDegreesToRadians(value));
    };

    const QVector3D rescale(factor(rotationDegrees.x()),
                            factor(rotationDegrees.y()),
                            factor(rotationDegrees.z()));
    return QVector3D(scaleIn.x() * rescale.y() * rescale.z(),
                     scaleIn.y() * rescale.x() * rescale.z(),
                     scaleIn.z() * rescale.x() * rescale.y());
}

void ModelPreviewItem::setStatus(const QString &status)
{
    if (m_status == status) return;
    m_status = status;
    emit statusChanged();
}

void ModelPreviewItem::setModelSource(const QString &source)
{
    if (m_modelSource == source) return;
    m_modelSource = source;
    emit modelSourceChanged();
    reloadAssets();
}

void ModelPreviewItem::setTextureSource(const QString &source)
{
    if (m_textureSource == source) return;
    m_textureSource = source;
    emit textureSourceChanged();
    reloadAssets();
}

void ModelPreviewItem::setAutoRotate(bool enabled)
{
    if (m_autoRotate == enabled) return;
    m_autoRotate = enabled;
    emit autoRotateChanged();
    if (m_autoRotate && !m_dragging && !m_hoverTracking) m_rotateTimer.start();
    if (!m_autoRotate) {
        m_hoverTracking = false;
        m_rotateTimer.stop();
        m_resumeAutoRotateTimer.stop();
    }
}

void ModelPreviewItem::setYaw(qreal value)
{
    while (value >= 360.0) value -= 360.0;
    while (value < -360.0) value += 360.0;
    if (qFuzzyCompare(m_yaw, value)) return;
    m_yaw = value;
    emit yawChanged();
    update();
}

void ModelPreviewItem::setPitch(qreal value)
{
    value = std::clamp<qreal>(value, -88.0, 88.0);
    if (qFuzzyCompare(m_pitch, value)) return;
    m_pitch = value;
    emit pitchChanged();
    update();
}

void ModelPreviewItem::setZoom(qreal value)
{
    value = std::clamp<qreal>(value, 0.35, 3.25);
    if (qFuzzyCompare(m_zoom, value)) return;
    m_zoom = value;
    emit zoomChanged();
    update();
}

void ModelPreviewItem::resetView()
{
    setYaw(m_defaultYaw);
    setPitch(m_defaultPitch);
    setZoom(m_defaultZoom);
}

void ModelPreviewItem::captureDefaultView()
{
    m_defaultYaw = m_yaw;
    m_defaultPitch = m_pitch;
    m_defaultZoom = m_zoom;
}

void ModelPreviewItem::scheduleAutoRotateResume()
{
    if (!m_autoRotate) return;
    m_resumeAutoRotateTimer.start();
}

void ModelPreviewItem::reloadAssets()
{
    if (m_modelSource.isEmpty() || m_textureSource.isEmpty()) {
        setStatus(QStringLiteral("等待模型"));
        return;
    }

    const bool modelOk = loadModel(m_modelSource);
    const bool textureOk = loadTexture(m_textureSource);
    if (modelOk && textureOk) {
        buildBoneMatrices();
        setStatus(QStringLiteral("模型已就绪"));
    } else {
        setStatus(QStringLiteral("资源加载失败"));
    }
    m_geometryDirty = true;
    m_cachedTrianglesDirty = true;
    m_cachedBoundsValid = false;
    m_textureDirty = true;
    update();
}

bool ModelPreviewItem::loadTexture(const QString &source)
{
    QImage image(localSourcePath(source));
    if (image.isNull()) return false;
    m_textureImage = image.convertToFormat(QImage::Format_RGBA8888);
    return !m_textureImage.isNull();
}

bool ModelPreviewItem::loadModel(const QString &source)
{
    QFile file(localSourcePath(source));
    if (!file.open(QIODevice::ReadOnly)) return false;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) return false;

    const QJsonArray geometries = doc.object().value(QStringLiteral("minecraft:geometry")).toArray();
    if (geometries.isEmpty()) return false;

    const QJsonObject geometry = geometries.at(0).toObject();
    const QJsonObject description = geometry.value(QStringLiteral("description")).toObject();
    m_modelTextureWidth = description.value(QStringLiteral("texture_width")).toInt(64);
    m_modelTextureHeight = description.value(QStringLiteral("texture_height")).toInt(64);
    if (m_modelTextureWidth <= 0) m_modelTextureWidth = 64;
    if (m_modelTextureHeight <= 0) m_modelTextureHeight = 64;
    const QJsonArray bones = geometry.value(QStringLiteral("bones")).toArray();
    if (bones.isEmpty()) return false;

    m_bones.clear();
    m_bones.reserve(bones.size());
    for (const QJsonValue &value : bones) {
        const QJsonObject boneObj = value.toObject();
        Bone bone;
        bone.name = boneObj.value(QStringLiteral("name")).toString();
        bone.parent = boneObj.value(QStringLiteral("parent")).toString();
        bone.origin = convertPivot(arrayAt(boneObj, QStringLiteral("pivot")));
        bone.rotation = convertRotation(arrayAt(boneObj, QStringLiteral("rotation")));

        const QJsonArray cubes = boneObj.value(QStringLiteral("cubes")).toArray();
        bone.cubes.reserve(cubes.size());
        for (const QJsonValue &cubeValue : cubes) {
            Cube cube;
            parseCube(cubeValue.toObject(), boneObj, &cube);
            bone.cubes.push_back(cube);
        }
        m_bones.push_back(bone);
    }
    return !m_bones.isEmpty();
}

void ModelPreviewItem::parseCube(const QJsonObject &obj,
                                 const QJsonObject &boneObj,
                                 Cube *cube) const
{
    const QVector3D size = jsonVec3(obj.value(QStringLiteral("size")), QVector3D());
    cube->pivot = convertPivot(arrayAt(obj, QStringLiteral("pivot")));
    if (cube->pivot.isNull()) cube->pivot = convertPivot(arrayAt(boneObj, QStringLiteral("pivot")));

    QVector3D from = jsonVec3(obj.value(QStringLiteral("origin")), QVector3D());
    from.setX(-(from.x() + size.x()));
    cube->size = size;
    cube->from = from;
    cube->to = QVector3D(from.x() + size.x(), from.y() + size.y(), from.z() + size.z());
    cube->inflate = obj.value(QStringLiteral("inflate")).toDouble(
        boneObj.value(QStringLiteral("inflate")).toDouble(0.0));
    cube->mirrorUv = obj.contains(QStringLiteral("mirror"))
        ? obj.value(QStringLiteral("mirror")).toBool(false)
        : boneObj.value(QStringLiteral("mirror")).toBool(false);
    cube->rotation = convertRotation(arrayAt(obj, QStringLiteral("rotation")));
    cube->rescale = obj.value(QStringLiteral("rescale")).toBool(false);

    for (Face &face : cube->faces) {
        face.enabled = true;
        face.rotation = 0;
        setFace(&face, 0, 0, 16, 16);
    }

    const QJsonValue uvValue = obj.value(QStringLiteral("uv"));
    if (uvValue.isArray()) {
        cube->boxUv = true;
        const QJsonArray uv = uvValue.toArray();
        cube->uvOffset[0] = numAt(uv, 0);
        cube->uvOffset[1] = numAt(uv, 1);
        updateBoxUv(cube);
        return;
    }

    if (uvValue.isObject()) {
        static const QString names[6] = {
            QStringLiteral("east"), QStringLiteral("west"), QStringLiteral("up"),
            QStringLiteral("down"), QStringLiteral("south"), QStringLiteral("north")
        };
        for (int i = 0; i < 6; ++i) {
            const QJsonObject faceObj = uvValue.toObject().value(names[i]).toObject();
            if (faceObj.isEmpty()) {
                cube->faces[i].enabled = false;
                continue;
            }
            const QJsonArray uv = faceObj.value(QStringLiteral("uv")).toArray();
            const QJsonArray uvSize = faceObj.value(QStringLiteral("uv_size")).toArray();
            const qreal x = numAt(uv, 0);
            const qreal y = numAt(uv, 1);
            const qreal w = uvSize.isEmpty() ? qAbs(cube->to.x() - cube->from.x()) : numAt(uvSize, 0);
            const qreal h = uvSize.isEmpty() ? qAbs(cube->to.y() - cube->from.y()) : numAt(uvSize, 1);
            setFace(&cube->faces[i], x, y, x + w, y + h);
            cube->faces[i].rotation = faceObj.value(QStringLiteral("uv_rotation")).toInt(0);
            if (names[i] == QLatin1String("up") || names[i] == QLatin1String("down")) {
                setFace(&cube->faces[i], x + w, y + h, x, y);
            }
        }
    }
}

void ModelPreviewItem::updateBoxUv(Cube *cube) const
{
    if (!cube->boxUv) return;

    const qreal widthScale = qMax<qreal>(1.0, qreal(m_modelTextureWidth) / 64.0);
    const qreal heightScale = qMax<qreal>(1.0, qreal(m_modelTextureHeight) / 64.0);
    const qreal sx = qFloor((cube->to.x() - cube->from.x()) * widthScale + 0.0000001);
    const qreal sy = qFloor((cube->to.y() - cube->from.y()) * heightScale + 0.0000001);
    const qreal szWidth = qFloor((cube->to.z() - cube->from.z()) * widthScale + 0.0000001);
    const qreal szHeight = qFloor((cube->to.z() - cube->from.z()) * heightScale + 0.0000001);

    struct FaceUv {
        int face;
        qreal fromX;
        qreal fromY;
        qreal sizeX;
        qreal sizeY;
    };

    FaceUv list[6] = {
        {FACE_EAST, 0, szHeight, szWidth, sy},
        {FACE_WEST, szWidth + sx, szHeight, szWidth, sy},
        {FACE_UP, szWidth + sx, szHeight, -sx, -szHeight},
        {FACE_DOWN, szWidth + sx * 2, 0, -sx, szHeight},
        {FACE_SOUTH, szWidth * 2 + sx, szHeight, sx, sy},
        {FACE_NORTH, szWidth, szHeight, sx, sy},
    };

    if (cube->mirrorUv) {
        for (FaceUv &item : list) {
            item.fromX += item.sizeX;
            item.sizeX *= -1;
        }
        std::swap(list[0].fromX, list[1].fromX);
        std::swap(list[0].fromY, list[1].fromY);
        std::swap(list[0].sizeX, list[1].sizeX);
        std::swap(list[0].sizeY, list[1].sizeY);
    }

    for (const FaceUv &item : list) {
        setFace(&cube->faces[item.face],
                item.fromX + cube->uvOffset[0],
                item.fromY + cube->uvOffset[1],
                item.fromX + item.sizeX + cube->uvOffset[0],
                item.fromY + item.sizeY + cube->uvOffset[1]);
    }
}

void ModelPreviewItem::buildBoneMatrices()
{
    QHash<QString, int> indices;
    for (int i = 0; i < m_bones.size(); ++i) indices.insert(m_bones[i].name, i);

    for (int i = 0; i < m_bones.size(); ++i) {
        Bone &bone = m_bones[i];
        QMatrix4x4 local;
        QVector3D pos = bone.origin;
        if (!bone.parent.isEmpty() && indices.contains(bone.parent)) {
            pos -= m_bones[indices.value(bone.parent)].origin;
        }
        local.translate(pos);
        local *= rotationMatrixZyx(bone.rotation);
        if (!bone.parent.isEmpty() && indices.contains(bone.parent)) {
            bone.world = m_bones[indices.value(bone.parent)].world * local;
        } else {
            bone.world = local;
        }
    }
}

QVector<ModelPreviewItem::Triangle> ModelPreviewItem::buildTriangles(const QSize &targetSize,
                                                                     qreal padding) const
{
    struct RawTriangle {
        QVector3D p[3];
        QPointF uv[3];
        qreal avgDepth = 0.0;
        qreal depthBias = 0.0;
        int order = 0;
    };

    QVector<RawTriangle> raw;
    qreal minX = 0.0;
    qreal minY = 0.0;
    qreal minZ = 0.0;
    qreal maxX = 0.0;
    qreal maxY = 0.0;
    qreal maxZ = 0.0;
    bool boundsInitialized = false;

    int triangleOrder = 0;

    auto addRawTriangle = [&](const QVector3D points[3], const QPointF uvs[3], bool doubleSided = false) {
        RawTriangle tri;
        tri.order = triangleOrder++;
        qreal depthSum = 0.0;
        for (int i = 0; i < 3; ++i) {
            tri.p[i] = points[i];
            tri.uv[i] = uvs[i];
            depthSum += tri.p[i].z();
            if (!boundsInitialized) {
                minX = maxX = tri.p[i].x();
                minY = maxY = tri.p[i].y();
                minZ = maxZ = tri.p[i].z();
                boundsInitialized = true;
            } else {
                minX = std::min(minX, static_cast<qreal>(tri.p[i].x()));
                maxX = std::max(maxX, static_cast<qreal>(tri.p[i].x()));
                minY = std::min(minY, static_cast<qreal>(tri.p[i].y()));
                maxY = std::max(maxY, static_cast<qreal>(tri.p[i].y()));
                minZ = std::min(minZ, static_cast<qreal>(tri.p[i].z()));
                maxZ = std::max(maxZ, static_cast<qreal>(tri.p[i].z()));
            }
        }
        tri.avgDepth = depthSum / 3.0;
        raw.push_back(tri);

        if (doubleSided) {
            RawTriangle backTri = tri;
            backTri.order = triangleOrder++;
            std::swap(backTri.p[1], backTri.p[2]);
            std::swap(backTri.uv[1], backTri.uv[2]);
            raw.push_back(backTri);
        }
    };

    auto addFace = [&](const QVector3D verts[8], int a, int b, int c, int d,
                       const Face &face, bool boxUv, const QMatrix4x4 &matrix,
                       bool doubleSided = false) {
        if (!face.enabled) return;

        const int ids[4] = {a, b, c, d};
        QVector3D faceVerts[4];
        for (int i = 0; i < 4; ++i) {
            faceVerts[i] = matrix.map(verts[ids[i]]);
        }

        Face adjustedFace = face;
        if (boxUv) {
            for (int i = 0; i < 2; ++i) {
                const qreal margin = adjustedFace.uv[i] > adjustedFace.uv[i + 2]
                    ? -1.0 / 64.0
                    : 1.0 / 64.0;
                adjustedFace.uv[i] += margin;
                adjustedFace.uv[i + 2] -= margin;
            }
        }

        QPointF faceUv[4];
        mapUvCorners(adjustedFace, m_textureImage.width(), m_textureImage.height(), faceUv);

        QVector3D triPointsA[3] = { faceVerts[0], faceVerts[2], faceVerts[1] };
        QPointF triUvA[3] = { faceUv[0], faceUv[2], faceUv[1] };
        addRawTriangle(triPointsA, triUvA, doubleSided);

        QVector3D triPointsB[3] = { faceVerts[2], faceVerts[3], faceVerts[1] };
        QPointF triUvB[3] = { faceUv[2], faceUv[3], faceUv[1] };
        addRawTriangle(triPointsB, triUvB, doubleSided);
    };

    for (const Bone &bone : m_bones) {
        for (const Cube &cube : bone.cubes) {
            QVector3D from = cube.from;
            QVector3D to = cube.to;
            const bool thinX = cube.size.x() <= 0.02;
            const bool thinY = cube.size.y() <= 0.02;
            const bool thinZ = cube.size.z() <= 0.02;
            if (cube.inflate != 0.0) {
                const QVector3D center = (from + to) * 0.5f;
                QVector3D half = (to - from) * 0.5f + QVector3D(cube.inflate, cube.inflate, cube.inflate);
                from = center - half;
                to = center + half;
            }

            for (int i = 0; i < 3; ++i) {
                from[i] -= cube.pivot[i];
                to[i] -= cube.pivot[i];
                if (qFuzzyCompare(from[i], to[i])) to[i] += 0.001f;
            }

            QVector3D verts[8] = {
                {to.x(), to.y(), to.z()}, {to.x(), to.y(), from.z()},
                {to.x(), from.y(), to.z()}, {to.x(), from.y(), from.z()},
                {from.x(), to.y(), from.z()}, {from.x(), to.y(), to.z()},
                {from.x(), from.y(), from.z()}, {from.x(), from.y(), to.z()}
            };

            QMatrix4x4 local;
            local.translate(cube.pivot - bone.origin);
            local *= rotationMatrixZyx(cube.rotation);
            if (cube.rescale) {
                const QVector3D scale = applyBlockbenchRescale(QVector3D(1, 1, 1), cube.rotation);
                local.scale(scale);
            }
            const QMatrix4x4 matrix = bone.world * local;

            if (thinX || thinY || thinZ) {
                Face planeA;
                Face planeB;
                int a1 = 0;
                int b1 = 0;
                int c1 = 0;
                int d1 = 0;
                int a2 = 0;
                int b2 = 0;
                int c2 = 0;
                int d2 = 0;

                if (thinX) {
                    planeA = cube.faces[FACE_EAST];
                    planeB = cube.faces[FACE_WEST];
                    a1 = 0; b1 = 1; c1 = 2; d1 = 3;
                    a2 = 4; b2 = 5; c2 = 6; d2 = 7;
                } else if (thinY) {
                    planeA = cube.faces[FACE_UP];
                    planeB = cube.faces[FACE_DOWN];
                    a1 = 4; b1 = 1; c1 = 5; d1 = 0;
                    a2 = 7; b2 = 2; c2 = 6; d2 = 3;
                } else {
                    planeA = cube.faces[FACE_SOUTH];
                    planeB = cube.faces[FACE_NORTH];
                    a1 = 5; b1 = 0; c1 = 7; d1 = 2;
                    a2 = 1; b2 = 4; c2 = 3; d2 = 6;
                }

                const bool renderA = planeA.enabled && !faceLooksPlaceholder(planeA);
                const bool renderB = planeB.enabled && !faceLooksPlaceholder(planeB);

                if (renderA && renderB) {
                    if (facesUseSameUv(planeA, planeB)) {
                        addFace(verts, a1, b1, c1, d1, planeA, cube.boxUv, matrix, true);
                    } else {
                        addFace(verts, a1, b1, c1, d1, planeA, cube.boxUv, matrix, false);
                        addFace(verts, a2, b2, c2, d2, planeB, cube.boxUv, matrix, false);
                    }
                } else if (renderA) {
                    addFace(verts, a1, b1, c1, d1, planeA, cube.boxUv, matrix, true);
                } else if (renderB) {
                    addFace(verts, a2, b2, c2, d2, planeB, cube.boxUv, matrix, true);
                }
                continue;
            }

            addFace(verts, 0, 1, 2, 3, cube.faces[FACE_EAST], cube.boxUv, matrix);
            addFace(verts, 4, 5, 6, 7, cube.faces[FACE_WEST], cube.boxUv, matrix);
            addFace(verts, 4, 1, 5, 0, cube.faces[FACE_UP], cube.boxUv, matrix);
            addFace(verts, 7, 2, 6, 3, cube.faces[FACE_DOWN], cube.boxUv, matrix);
            addFace(verts, 5, 0, 7, 2, cube.faces[FACE_SOUTH], cube.boxUv, matrix);
            addFace(verts, 1, 4, 3, 6, cube.faces[FACE_NORTH], cube.boxUv, matrix);
        }
    }

    QVector<Triangle> out;
    if (raw.isEmpty() || !boundsInitialized || maxX <= minX || maxY <= minY || targetSize.isEmpty()) {
        m_cachedBoundsValid = false;
        return out;
    }

    m_cachedBoundsMin = QVector3D(float(minX), float(minY), float(minZ));
    m_cachedBoundsMax = QVector3D(float(maxX), float(maxY), float(maxZ));
    m_cachedBoundsValid = true;
    const QVector3D boundsCenter = (m_cachedBoundsMin + m_cachedBoundsMax) * 0.5f;
    const float boundsRadius = std::max(1.0f, (m_cachedBoundsMax - m_cachedBoundsMin).length() * 0.5f);

    std::stable_sort(raw.begin(), raw.end(), [](const RawTriangle &left, const RawTriangle &right) {
        if (qAbs(left.avgDepth - right.avgDepth) <= 0.00001) {
            return left.order < right.order;
        }
        return left.avgDepth < right.avgDepth;
    });

    out.reserve(raw.size());
    for (const RawTriangle &rt : raw) {
        Triangle tri;
        const QVector3D centroid = (rt.p[0] + rt.p[1] + rt.p[2]) / 3.0f;
        const float outerness = std::clamp((centroid - boundsCenter).length() / boundsRadius, 0.0f, 1.0f);
        const qreal layeredDepthBias = qreal(outerness) * 0.0012;
        for (int i = 0; i < 3; ++i) {
            tri.v[i] = {QPointF(), rt.p[i], rt.uv[i], rt.p[i].z(), layeredDepthBias};
        }
        out.push_back(tri);
    }

    return out;
}

QMatrix4x4 ModelPreviewItem::gpuMvpMatrix(const QSize &targetSize) const
{
    if (!m_cachedBoundsValid) {
        const_cast<ModelPreviewItem *>(this)->gpuTriangles(targetSize);
    }

    QMatrix4x4 mvp;
    if (targetSize.isEmpty() || !m_cachedBoundsValid) {
        return mvp;
    }

    const QVector3D boundsMin = m_cachedBoundsMin;
    const QVector3D boundsMax = m_cachedBoundsMax;
    const QVector3D center = (boundsMin + boundsMax) * 0.5f;
    const QVector3D span = boundsMax - boundsMin;
    const float radius = std::max(1.0f, span.length() * 0.5f);
    const float scale = float(1.62 * m_zoom) / radius;
    const float aspectX = targetSize.width() > 0
        ? float(targetSize.height()) / float(targetSize.width())
        : 1.0f;
    const float zScale = 0.38f / radius;

    QMatrix4x4 view;
    view.rotate(float(m_pitch), 1.0f, 0.0f, 0.0f);
    view.rotate(float(m_yaw), 0.0f, 1.0f, 0.0f);
    view.translate(-center);

    QMatrix4x4 projection;
    projection.translate(0.0f, 0.0f, 0.5f);
    projection.scale(scale * aspectX, scale, -zScale);

    return projection * view;
}

QVector<ModelPreviewItem::Triangle> ModelPreviewItem::gpuTriangles(const QSize &targetSize) const
{
    if (targetSize.isEmpty() || m_textureImage.isNull() || m_bones.isEmpty()) {
        return {};
    }
    if (m_cachedTrianglesDirty || m_cachedTriangleSize != targetSize) {
        m_cachedTriangles = buildTriangles(targetSize, 8.0);
        m_cachedTriangleSize = targetSize;
        m_cachedTrianglesDirty = false;
    }
    return m_cachedTriangles;
}

QImage ModelPreviewItem::gpuTextureImage() const
{
    return m_textureImage;
}

bool ModelPreviewItem::consumeGeometryDirty()
{
    const bool dirty = m_geometryDirty;
    m_geometryDirty = false;
    return dirty;
}

bool ModelPreviewItem::consumeTextureDirty()
{
    const bool dirty = m_textureDirty;
    m_textureDirty = false;
    return dirty;
}

QQuickRhiItemRenderer *ModelPreviewItem::createRenderer()
{
    return new ModelPreviewRhiRenderer();
}

void ModelPreviewItem::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;
    m_hoverTracking = false;
    m_lastMousePos = event->position();
    m_rotateTimer.stop();
    m_resumeAutoRotateTimer.stop();
    event->accept();
}

void ModelPreviewItem::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging) {
        QQuickItem::mouseMoveEvent(event);
        return;
    }

    const QPointF delta = event->position() - m_lastMousePos;
    m_lastMousePos = event->position();
    setYaw(m_yaw + delta.x() * 0.42);
    setPitch(m_pitch - delta.y() * 0.30);
    event->accept();
}

void ModelPreviewItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    scheduleAutoRotateResume();
    event->accept();
}

void ModelPreviewItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    resetView();
    event->accept();
}

void ModelPreviewItem::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    m_hoverTracking = true;
    m_hoverBaseYaw = m_defaultYaw;
    m_hoverBasePitch = m_defaultPitch;
    m_hoverTargetYaw = m_defaultYaw;
    m_hoverTargetPitch = m_defaultPitch;
    m_resumeAutoRotateTimer.stop();
    m_rotateTimer.start();
}

void ModelPreviewItem::hoverMoveEvent(QHoverEvent *event)
{
    if (!m_hoverTracking || m_dragging || width() <= 1.0 || height() <= 1.0) {
        return;
    }

    const QPointF pos = event->position();
    const qreal nx = std::clamp((pos.x() / width()) * 2.0 - 1.0, -1.0, 1.0);
    const qreal ny = std::clamp((pos.y() / height()) * 2.0 - 1.0, -1.0, 1.0);
    m_hoverTargetYaw = m_hoverBaseYaw + nx * 36.0;
    m_hoverTargetPitch = m_hoverBasePitch - ny * 28.0;
}

void ModelPreviewItem::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);
    m_hoverTracking = false;
    scheduleAutoRotateResume();
}

void ModelPreviewItem::wheelEvent(QWheelEvent *event)
{
    event->ignore();
    QQuickRhiItem::wheelEvent(event);
}
