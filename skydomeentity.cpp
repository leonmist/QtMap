#include "SkyDomeEntity.h"

#include <cmath>
#include <QByteArray>
#include <QDebug>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 纬度分段数（0°→90°，共 24 段，每段 3.75°）
static const int LAT_BANDS    = 24;
// 经度分段数（0°→360°，共 48 段，每段 7.5°）
static const int LON_SEGMENTS = 48;

SkyDomeEntity::SkyDomeEntity(Qt3DCore::QEntity *parent)
    : Qt3DCore::QEntity(parent)
    , m_radius(100.0f)
    , m_zenithColor(10, 35, 120)       // 天顶：深蓝
    , m_horizonColor(80, 145, 215)     // 地平线：天蓝
    , m_transform(new Qt3DCore::QTransform(this))
    , m_renderer(new Qt3DRender::QGeometryRenderer(this))
    , m_material(new Qt3DExtras::QPerVertexColorMaterial(this))
{
    buildGeometry();

    addComponent(m_transform);
    addComponent(m_renderer);
    addComponent(m_material);

    qDebug() << "SkyDomeEntity 创建完成，半径=" << m_radius;
}

void SkyDomeEntity::setRadius(float radius)
{
    if (qFuzzyCompare(m_radius, radius)) return;
    m_radius = radius;
    buildGeometry();
}

void SkyDomeEntity::setZenithColor(const QColor &color)
{
    m_zenithColor = color;
    buildGeometry();
}

void SkyDomeEntity::setHorizonColor(const QColor &color)
{
    m_horizonColor = color;
    buildGeometry();
}

void SkyDomeEntity::setCenterPosition(const QVector3D &pos)
{
    m_transform->setTranslation(pos);
}

QVector3D SkyDomeEntity::centerPosition() const
{
    return m_transform->translation();
}

void SkyDomeEntity::buildGeometry()
{
    // 销毁旧几何体（若有），重新构建
    if (m_renderer->geometry()) {
        delete m_renderer->geometry();
    }

    Qt3DRender::QGeometry *geo = new Qt3DRender::QGeometry(m_renderer);

    const int vertexCount = (LAT_BANDS + 1) * (LON_SEGMENTS + 1);
    const int indexCount  = LAT_BANDS * LON_SEGMENTS * 6;

    QByteArray posData, colData, idxData;
    posData.resize(vertexCount * 3 * sizeof(float));
    colData.resize(vertexCount * 4 * sizeof(float));
    idxData.resize(indexCount  * sizeof(unsigned int));

    float        *pos = reinterpret_cast<float*>(posData.data());
    float        *col = reinterpret_cast<float*>(colData.data());
    unsigned int *idx = reinterpret_cast<unsigned int*>(idxData.data());

    // ── 顶点数据 ───────────────────────────────────────────────────────────
    // 纬度 lat：0（地平线）→ π/2（天顶）
    // 经度 lon：0 → 2π
    // 颜色：lat=0 取 horizonColor，lat=π/2 取 zenithColor，线性插值

    int vIdx = 0;
    for (int i = 0; i <= LAT_BANDS; ++i) {
        float lat    = static_cast<float>(i) / LAT_BANDS * (static_cast<float>(M_PI) * 0.5f);
        float sinLat = std::sin(lat);
        float cosLat = std::cos(lat);

        // 渐变因子：0=地平线色，1=天顶色
        float t = static_cast<float>(i) / LAT_BANDS;
        float r = m_horizonColor.redF()   * (1.0f - t) + m_zenithColor.redF()   * t;
        float g = m_horizonColor.greenF() * (1.0f - t) + m_zenithColor.greenF() * t;
        float b = m_horizonColor.blueF()  * (1.0f - t) + m_zenithColor.blueF()  * t;

        for (int j = 0; j <= LON_SEGMENTS; ++j) {
            float lon = static_cast<float>(j) / LON_SEGMENTS * 2.0f * static_cast<float>(M_PI);

            pos[vIdx * 3 + 0] = cosLat * std::cos(lon) * m_radius;
            pos[vIdx * 3 + 1] = sinLat * m_radius;
            pos[vIdx * 3 + 2] = cosLat * std::sin(lon) * m_radius;

            col[vIdx * 4 + 0] = r;
            col[vIdx * 4 + 1] = g;
            col[vIdx * 4 + 2] = b;
            col[vIdx * 4 + 3] = 1.0f;

            ++vIdx;
        }
    }

    // ── 索引数据 ───────────────────────────────────────────────────────────
    // 翻转绕序（顺时针），使内表面成为正面，从穹顶内部可见

    int iIdx = 0;
    for (int i = 0; i < LAT_BANDS; ++i) {
        for (int j = 0; j < LON_SEGMENTS; ++j) {
            unsigned int v0 = static_cast<unsigned int>(i * (LON_SEGMENTS + 1) + j);
            unsigned int v1 = v0 + 1;
            unsigned int v2 = static_cast<unsigned int>((i + 1) * (LON_SEGMENTS + 1) + j);
            unsigned int v3 = v2 + 1;

            // 第一个三角形（顺时针 = 内部正面）
            idx[iIdx++] = v0;
            idx[iIdx++] = v1;
            idx[iIdx++] = v2;

            // 第二个三角形
            idx[iIdx++] = v1;
            idx[iIdx++] = v3;
            idx[iIdx++] = v2;
        }
    }

    // ── 构建 Qt3D 缓冲区与属性 ──────────────────────────────────────────────

    auto *posBuffer = new Qt3DRender::QBuffer(geo);
    posBuffer->setData(posData);

    auto *colBuffer = new Qt3DRender::QBuffer(geo);
    colBuffer->setData(colData);

    auto *idxBuffer = new Qt3DRender::QBuffer(geo);
    idxBuffer->setData(idxData);

    // 顶点位置属性
    auto *posAttr = new Qt3DRender::QAttribute(geo);
    posAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    posAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    posAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    posAttr->setVertexSize(3);
    posAttr->setByteOffset(0);
    posAttr->setByteStride(3 * sizeof(float));
    posAttr->setCount(static_cast<uint>(vertexCount));
    posAttr->setBuffer(posBuffer);

    // 顶点颜色属性
    auto *colAttr = new Qt3DRender::QAttribute(geo);
    colAttr->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
    colAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    colAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
    colAttr->setVertexSize(4);
    colAttr->setByteOffset(0);
    colAttr->setByteStride(4 * sizeof(float));
    colAttr->setCount(static_cast<uint>(vertexCount));
    colAttr->setBuffer(colBuffer);

    // 索引属性
    auto *idxAttr = new Qt3DRender::QAttribute(geo);
    idxAttr->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    idxAttr->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    idxAttr->setVertexSize(1);
    idxAttr->setByteOffset(0);
    idxAttr->setByteStride(0);
    idxAttr->setCount(static_cast<uint>(indexCount));
    idxAttr->setBuffer(idxBuffer);

    geo->addAttribute(posAttr);
    geo->addAttribute(colAttr);
    geo->addAttribute(idxAttr);

    m_renderer->setGeometry(geo);
    m_renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    m_renderer->setVertexCount(indexCount);
}
