#include "terraingeometry.h"
#include "MapEntity.h"
#include "CoordinateConverter.h"
#include <QByteArray>
#include <QDebug>
#include <cmath>

static QPointF tileToLatLon(double tx, double ty, int zoom)
{
    double n = std::pow(2.0, zoom);
    double lon = tx * 360.0 / n - 180.0;
    double y_val = M_PI * (1.0 - 2.0 * ty / n);
    double A = std::exp(y_val);
    double latRad = 2.0 * std::atan(A) - M_PI / 2.0;
    double lat = latRad * 180.0 / M_PI;
    return QPointF(lat, lon);
}

TerrainGeometry::TerrainGeometry(Qt3DCore::QNode *parent)
    : Qt3DRender::QGeometry(parent)
    , m_vertexBuffer(new Qt3DRender::QBuffer(this))
    , m_indexBuffer(new Qt3DRender::QBuffer(this))
    , m_positionAttribute(new Qt3DRender::QAttribute(this))
    , m_texCoordAttribute(new Qt3DRender::QAttribute(this))
    , m_normalAttribute(new Qt3DRender::QAttribute(this))
    , m_indexAttribute(new Qt3DRender::QAttribute(this))
    , m_vertexCount(0)
    , m_indexCount(0)
{
    // 1. 设置顶点位置属性
    m_positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    m_positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_positionAttribute->setVertexSize(3);
    m_positionAttribute->setByteOffset(0);
    m_positionAttribute->setByteStride(8 * sizeof(float)); // X, Y, Z, U, V, Nx, Ny, Nz
    m_positionAttribute->setBuffer(m_vertexBuffer);

    // 2. 设置纹理坐标属性
    m_texCoordAttribute->setName(Qt3DRender::QAttribute::defaultTextureCoordinateAttributeName());
    m_texCoordAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_texCoordAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_texCoordAttribute->setVertexSize(2);
    m_texCoordAttribute->setByteOffset(3 * sizeof(float));
    m_texCoordAttribute->setByteStride(8 * sizeof(float));
    m_texCoordAttribute->setBuffer(m_vertexBuffer);

    // 3. 设置法向量属性（用于光照）
    m_normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());
    m_normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_normalAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_normalAttribute->setVertexSize(3);
    m_normalAttribute->setByteOffset(5 * sizeof(float));
    m_normalAttribute->setByteStride(8 * sizeof(float));
    m_normalAttribute->setBuffer(m_vertexBuffer);

    // 4. 设置索引属性
    m_indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    m_indexAttribute->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    m_indexAttribute->setVertexSize(1);
    m_indexAttribute->setByteOffset(0);
    m_indexAttribute->setByteStride(0);
    m_indexAttribute->setBuffer(m_indexBuffer);

    // 添加属性到几何体
    addAttribute(m_positionAttribute);
    addAttribute(m_texCoordAttribute);
    addAttribute(m_normalAttribute);
    addAttribute(m_indexAttribute);
}

TerrainGeometry::~TerrainGeometry()
{
}

bool TerrainGeometry::generate(const TiffReader &reader, double centerLat, double centerLon, double scale, int zoom, float heightScale)
{
    if (!reader.isValid()) return false;

    // 方案B：在整个场景范围 [-100, 100] 内均匀生成 3D 地形网格
    const int gridW = 128;
    const int gridH = 128;

    m_vertexCount = gridW * gridH;
    m_indexCount = (gridW - 1) * (gridH - 1) * 6;

    qDebug() << "TerrainGeometry: 生成起伏网格尺寸:" << gridW << "x" << gridH
             << "顶点数:" << m_vertexCount << "索引数:" << m_indexCount;

    // 1. 填充顶点缓冲区数据
    QByteArray vertexData;
    vertexData.resize(m_vertexCount * 8 * sizeof(float));
    float *rawVertex = reinterpret_cast<float*>(vertexData.data());

    // 临时存储高度值，用于计算法向量
    QVector<QVector3D> positions(m_vertexCount);

    const double sceneSize = 200.0;
    const double side = 21.0; // 2 * 10 + 1

    QPointF centerTileF = MapEntity::latLonToTileF(centerLat, centerLon, zoom);

    // 遍历网格点，计算 3D 坐标 and 纹理坐标
    int vIdx = 0;
    for (int r = 0; r < gridH; ++r) {
        for (int c = 0; c < gridW; ++c) {
            // 在 [-100, 100] 的场景空间中均匀计算 X 和 Z 坐标
            double sceneX = -100.0 + (static_cast<double>(c) / (gridW - 1)) * 200.0;
            double sceneZ = -100.0 + (static_cast<double>(r) / (gridH - 1)) * 200.0;

            // 纹理坐标 U, V 分布于 [0, 1] 之间。
            // 共享纹理仅做了垂直翻转 mirrored(false, true)，所以这里：
            //   U 直接采用自然顺序（c 增大向东，与顶点 sceneX 一致）；
            //   V 取反，配合纹理的垂直翻转使南北方向与高程对齐（避免雪山落在低处）。
            double u = static_cast<double>(c) / (gridW - 1);
            double v = 1.0 - static_cast<double>(r) / (gridH - 1);

            // 逆向计算该场景坐标对应的地图瓦片坐标 tx, ty
            double dx = sceneX * side / sceneSize;
            double dy = sceneZ * side / sceneSize;
            double tx = dx + centerTileF.x();
            double ty = dy + centerTileF.y();

            // 瓦片坐标 tx, ty 转换为经纬度。
            // 注意：瓦片中心来自高德地图，处于 GCJ02 坐标系，故此处得到的是 GCJ02 经纬度。
            QPointF latLon = tileToLatLon(tx, ty, zoom);
            double gcjLat = latLon.x();
            double gcjLon = latLon.y();

            // DEM 高程数据是 WGS84 坐标系，必须先把 GCJ02 转回 WGS84 再查高程，
            // 否则地形起伏会相对卫星纹理整体错位（中国境内偏移约 100~700m）。
            double lat = gcjLat, lon = gcjLon;
            CoordinateConverter::gcj02ToWgs84(gcjLat, gcjLon, lat, lon);

            // 获取该经纬度对应的 TIFF 高程值
            double elevation = 0.0;
            QPointF pixel = reader.latLonToPixel(lat, lon);
            int pCol = static_cast<int>(std::round(pixel.x()));
            int pRow = static_cast<int>(std::round(pixel.y()));

            if (pCol >= 0 && pCol < reader.width() && pRow >= 0 && pRow < reader.height()) {
                elevation = reader.elevationAtPixel(pCol, pRow);
            }

            // 高度 Y 轴：高程米数 * 场景缩放比 * 夸大系数
            double sceneY = elevation * scale * heightScale;

            // 存储 3D 坐标
            positions[vIdx] = QVector3D(sceneX, sceneY, sceneZ);

            // 写入顶点数组
            rawVertex[vIdx * 8 + 0] = static_cast<float>(sceneX);
            rawVertex[vIdx * 8 + 1] = static_cast<float>(sceneY);
            rawVertex[vIdx * 8 + 2] = static_cast<float>(sceneZ);
            rawVertex[vIdx * 8 + 3] = static_cast<float>(u);
            rawVertex[vIdx * 8 + 4] = static_cast<float>(v);

            // 默认法向量朝上，稍后计算精确法向量
            rawVertex[vIdx * 8 + 5] = 0.0f;
            rawVertex[vIdx * 8 + 6] = 1.0f;
            rawVertex[vIdx * 8 + 7] = 0.0f;

            vIdx++;
        }
    }

    // 2. 计算精确的法向量（用于光照立体感）
    for (int r = 0; r < gridH; ++r) {
        for (int c = 0; c < gridW; ++c) {
            int idx = r * gridW + c;

            // 获取周围四个点
            QVector3D current = positions[idx];
            QVector3D right  = (c < gridW - 1) ? positions[idx + 1] : current;
            QVector3D bottom = (r < gridH - 1) ? positions[idx + gridW] : current;

            QVector3D v1 = right - current;
            QVector3D v2 = bottom - current;
            QVector3D normal = QVector3D::crossProduct(v2, v1).normalized(); // 叉乘计算法向量

            rawVertex[idx * 8 + 5] = normal.x();
            rawVertex[idx * 8 + 6] = normal.y();
            rawVertex[idx * 8 + 7] = normal.z();
        }
    }

    m_vertexBuffer->setData(vertexData);
    m_positionAttribute->setCount(m_vertexCount);
    m_texCoordAttribute->setCount(m_vertexCount);
    m_normalAttribute->setCount(m_vertexCount);

    // 3. 填充索引缓冲区数据 (构建三角形网格)
    QByteArray indexData;
    indexData.resize(m_indexCount * sizeof(unsigned int));
    unsigned int *rawIndex = reinterpret_cast<unsigned int*>(indexData.data());

    int iIdx = 0;
    for (int r = 0; r < gridH - 1; ++r) {
        for (int c = 0; c < gridW - 1; ++c) {
            unsigned int v0 = r * gridW + c;
            unsigned int v1 = v0 + 1;
            unsigned int v2 = (r + 1) * gridW + c;
            unsigned int v3 = v2 + 1;

            // 三角形 1
            rawIndex[iIdx++] = v0;
            rawIndex[iIdx++] = v2;
            rawIndex[iIdx++] = v1;

            // 三角形 2
            rawIndex[iIdx++] = v1;
            rawIndex[iIdx++] = v2;
            rawIndex[iIdx++] = v3;
        }
    }

    m_indexBuffer->setData(indexData);
    m_indexAttribute->setCount(m_indexCount);

    return true;
}
