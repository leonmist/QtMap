#include "CustomLineEntity.h"
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>

CustomLineEntity::CustomLineEntity(Qt3DCore::QNode *parent)
    : Qt3DCore::QEntity(parent)
    , m_lineId(-1)
    , m_lineWidth(2.0f)
    , m_color(Qt::white)
    , m_lineWidthState(nullptr)
{
    initializeGeometry();
    initializeMaterial();
}

CustomLineEntity::CustomLineEntity(int id, const QVector<QVector3D> &points,
                                   const QColor &color, float lineWidth,
                                   Qt3DCore::QNode *parent)
    : Qt3DCore::QEntity(parent)
    , m_lineId(id)
    , m_points(points)
    , m_color(color)
    , m_lineWidth(lineWidth)
    , m_lineWidthState(nullptr)
{
    initializeGeometry();
    initializeMaterial();
    updateGeometry();
}

void CustomLineEntity::initializeGeometry()
{
    // 创建几何渲染器
    m_lineRenderer = new Qt3DRender::QGeometryRenderer(this);
    m_lineRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::LineStrip);

    // 创建几何体
    m_geometry = new Qt3DRender::QGeometry(this);

    // 创建缓冲区
    m_vertexBuffer = new Qt3DRender::QBuffer(m_geometry);
    m_indexBuffer = new Qt3DRender::QBuffer(m_geometry);


    // 创建位置属性
    m_positionAttribute = new Qt3DRender::QAttribute();
    m_positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    m_positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_positionAttribute->setVertexSize(3);
    m_positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_positionAttribute->setBuffer(m_vertexBuffer);
    m_positionAttribute->setByteStride(3 * sizeof(float));
    m_geometry->addAttribute(m_positionAttribute);

    // 创建索引属性
    m_indexAttribute = new Qt3DRender::QAttribute();
    m_indexAttribute->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    m_indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    m_indexAttribute->setBuffer(m_indexBuffer);
    m_geometry->addAttribute(m_indexAttribute);

    m_lineRenderer->setGeometry(m_geometry);



    // 创建变换组件
    m_transform = new Qt3DCore::QTransform();

    // 添加对象拾取器（用于点击检测）
    Qt3DRender::QObjectPicker *picker = new Qt3DRender::QObjectPicker(this);
    connect(picker, &Qt3DRender::QObjectPicker::clicked, this, [this](Qt3DRender::QPickEvent *event) {
        if (event->button() == Qt3DRender::QPickEvent::LeftButton) {
            emit lineClicked(m_lineId);
        }
    });

    // 添加到实体
    this->addComponent(m_lineRenderer);
    this->addComponent(m_transform);
    this->addComponent(picker);
}

void CustomLineEntity::initializeMaterial()
{
    m_material = new Qt3DExtras::QPhongMaterial(this);
    m_material->setDiffuse(m_color);
    m_material->setAmbient(m_color.darker(150));
    m_material->setShininess(10.0f);

    // 创建线宽渲染状态，并添加到所有渲染通道
    m_lineWidthState = new Qt3DRender::QLineWidth();
    m_lineWidthState->setValue(m_lineWidth);
    m_lineWidthState->setSmooth(true);

    for (auto *technique : m_material->effect()->techniques()) {
        for (auto *renderPass : technique->renderPasses()) {
            renderPass->addRenderState(m_lineWidthState);
        }
    }

    this->addComponent(m_material);
}

void CustomLineEntity::setLineId(int id)
{
    if (m_lineId != id) {
        m_lineId = id;
        emit lineIdChanged(id);
    }
}

void CustomLineEntity::setPoints(const QVector<QVector3D> &points)
{
    qDebug() << "CustomLineEntity::setPoints 被调用，ID:" << m_lineId;
    qDebug() << "接收到的点数:" << points.size();

    if (points.size() > 0) {
        qDebug() << "第一个点:" << points.first();
        qDebug() << "最后一个点:" << points.last();
    }

    if (m_points != points) {
        m_points = points;
        qDebug() << "开始更新几何数据...";
        updateGeometry();
        qDebug() << "几何数据更新完成";
        emit pointsChanged(points);
    } else {
        qDebug() << "点集未变化，跳过更新";
    }


}

void CustomLineEntity::setColor(const QColor &color)
{
    if (m_color != color) {
        m_color = color;
        if (m_material) {
            m_material->setDiffuse(color);
            m_material->setAmbient(color.darker(150));
        }
        emit colorChanged(color);
    }
}

void CustomLineEntity::setLineWidth(float width)
{
    if (!qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        if (m_lineWidthState) {
            m_lineWidthState->setValue(width);
        }
        emit lineWidthChanged(width);
    }
}

void CustomLineEntity::updateGeometry()
{

    if (m_points.size() < 2) {
        // 至少需要2个点来形成线段
        m_positionAttribute->setCount(0);
        m_indexAttribute->setCount(0);
        m_lineRenderer->setVertexCount(0);
        return;
    }

    int vertexCount = m_points.size();
    int indexCount = (vertexCount - 1) * 2;

    // 更新顶点数据
    QByteArray vertexData;
    vertexData.resize(vertexCount * 3 * sizeof(float));
    float *rawVertexData = reinterpret_cast<float*>(vertexData.data());

    for (int i = 0; i < vertexCount; ++i) {
        *rawVertexData++ = m_points[i].x();
        *rawVertexData++ = m_points[i].y();
        *rawVertexData++ = m_points[i].z();
    }

    // 更新索引数据
    QByteArray indexData;
    indexData.resize(indexCount * sizeof(unsigned int));
    unsigned int *rawIndexData = reinterpret_cast<unsigned int*>(indexData.data());

    for (int i = 0; i < vertexCount - 1; ++i){
        *rawIndexData++ = i;
        *rawIndexData++ = i + 1;
    }

    m_vertexBuffer->setData(vertexData);
    m_indexBuffer->setData(indexData);
    // 更新属性计数
    m_positionAttribute->setCount(vertexCount);
    m_indexAttribute->setCount(indexCount);
    m_lineRenderer->setVertexCount(indexCount);


//    // 1. 解绑-重绑几何（核心：触发渲染器重新识别几何数据）
//    m_lineRenderer->setGeometry(nullptr);
//    m_lineRenderer->setGeometry(m_geometry);

//    // 2. 临时修改 primitiveType（触发渲染器属性通知，纯公有接口）
//    Qt3DRender::QGeometryRenderer::PrimitiveType oldType = m_lineRenderer->primitiveType();
//    // 临时切换为 LineLoop（任意其他类型，不影响最终渲染）
//    m_lineRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::LineLoop);
//    // 立即切回原类型（仅触发通知，无视觉变化）
//    m_lineRenderer->setPrimitiveType(oldType);
}

void CustomLineEntity::addPoint(const QVector3D &point)
{
    m_points.append(point);
    updateGeometry();
    emit pointsChanged(m_points);
}

void CustomLineEntity::clearPoints()
{
    m_points.clear();
    updateGeometry();
    emit pointsChanged(m_points);
}
