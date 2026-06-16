#ifndef TERRAINGEOMETRY_H
#define TERRAINGEOMETRY_H

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include <QVector3D>
#include "tiffreader.h"

class TerrainGeometry : public Qt3DRender::QGeometry
{
    Q_OBJECT

public:
    explicit TerrainGeometry(Qt3DCore::QNode *parent = nullptr);
    ~TerrainGeometry();

    // 根据高程图和高德地图中心点、缩放比，生成 3D 起伏地形网格
    // centerLat/centerLon: 高德地图中心点
    // scale: 高度方向场景缩放比（场景单元/米，sceneUnitsPerMeterHeight）。
    //        仅用于 Y 轴高程，水平 X/Z 由场景范围与瓦片数固定映射，不使用此值。
    // zoom: 当前高德瓦片的 zoom 级别
    // heightScale: 额外高度夸大系数（默认 1.0，想让起伏更明显可设为 2.0~5.0）
    bool generate(const TiffReader &reader, double centerLat, double centerLon, double scale, int zoom, float heightScale = 1.5f);

    int vertexCount() const { return m_vertexCount; }
    int indexCount() const { return m_indexCount; }

private:
    Qt3DRender::QBuffer *m_vertexBuffer;
    Qt3DRender::QBuffer *m_indexBuffer;

    Qt3DRender::QAttribute *m_positionAttribute;
    Qt3DRender::QAttribute *m_texCoordAttribute;
    Qt3DRender::QAttribute *m_normalAttribute;
    Qt3DRender::QAttribute *m_indexAttribute;

    int m_vertexCount;
    int m_indexCount;
};

#endif // TERRAINGEOMETRY_H
