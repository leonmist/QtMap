#ifndef SKYDOMEENTITY_H
#define SKYDOMEENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DExtras/QPerVertexColorMaterial>
#include <QColor>
#include <QVector3D>

// 半球天空穹顶实体
// 以场景原点为中心生成上半球（y >= 0），内表面可见，顶点色从地平线渐变至天顶
class SkyDomeEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

public:
    explicit SkyDomeEntity(Qt3DCore::QEntity *parent = nullptr);

    float radius() const { return m_radius; }
    QColor zenithColor()  const { return m_zenithColor;  }
    QColor horizonColor() const { return m_horizonColor; }

    void setRadius(float radius);
    void setZenithColor(const QColor &color);
    void setHorizonColor(const QColor &color);

    // 更新穹顶中心位置（让穹顶跟随相机平移，避免穿出远裁面）
    void setCenterPosition(const QVector3D &pos);
    QVector3D centerPosition() const;

private:
    void buildGeometry();

    float  m_radius;
    QColor m_zenithColor;
    QColor m_horizonColor;

    Qt3DCore::QTransform                *m_transform;
    Qt3DRender::QGeometryRenderer       *m_renderer;
    Qt3DExtras::QPerVertexColorMaterial *m_material;
};

#endif // SKYDOMEENTITY_H
