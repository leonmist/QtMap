#ifndef MAPENTITY_H
#define MAPENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QAbstractTextureImage>
#include <Qt3DRender/QTextureImageData>
#include <Qt3DRender/QTextureImageDataGenerator>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QMap>
#include <QPair>
#include <QDir>
#include "tiffreader.h"
#include "terraingeometry.h"
#include <Qt3DRender/QGeometryRenderer>

// ─── 内存纹理图像（持有 QImage，不需要临时文件）────────────────────────────────

class InMemoryTextureDataGen : public Qt3DRender::QTextureImageDataGenerator
{
public:
    explicit InMemoryTextureDataGen(const QImage &img) : m_image(img) {}

        Qt3DRender::QTextureImageDataPtr operator()() override
        {
            auto data = Qt3DRender::QTextureImageDataPtr::create();
            // OpenGL 纹理 Y=0 在底部，需垂直翻转(vertical=true)；
            // 水平方向不翻转(horizontal=false)，左右方向由网格自身的 U 纹理坐标决定。
            QImage converted = m_image.convertToFormat(QImage::Format_RGBA8888)
                                      .mirrored(false, true);
            data->setImage(converted);
            return data;
        }

    bool operator==(const Qt3DRender::QTextureImageDataGenerator &other) const override
    {
        const auto *otherGen = dynamic_cast<const InMemoryTextureDataGen*>(&other);
        return otherGen && otherGen->m_image.cacheKey() == m_image.cacheKey();
    }

    QT3D_FUNCTOR(InMemoryTextureDataGen)

private:
    QImage m_image;
};

class InMemoryTextureImage : public Qt3DRender::QAbstractTextureImage
{
    Q_OBJECT
public:
    explicit InMemoryTextureImage(const QImage &img, Qt3DCore::QNode *parent = nullptr)
        : Qt3DRender::QAbstractTextureImage(parent), m_image(img) {}

    void updateImage(const QImage &img) {
        m_image = img;
        notifyDataGeneratorChanged();
    }

protected:
    Qt3DRender::QTextureImageDataGeneratorPtr dataGenerator() const override {
        return Qt3DRender::QTextureImageDataGeneratorPtr(new InMemoryTextureDataGen(m_image));
    }

private:
    QImage m_image;
};

// ─── MapEntity ───────────────────────────────────────────────────────────────

class MapEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

public:
    explicit MapEntity(Qt3DCore::QEntity *parent = nullptr);
    ~MapEntity();

    // 开始加载地图，zoom 默认 14
    void loadMap(double centerLat, double centerLon, int zoom = 14);

    double sceneUnitsPerMeter() const { return m_sceneUnitsPerMeter; }

    // 将真实米（相对中心点的东向、北向偏移）映射到场景坐标
    // 供外部使用：sceneX = eastMeters  * sceneUnitsPerMeter
    //             sceneZ = northMeters * sceneUnitsPerMeter

    // Web Mercator 工具
    static QPointF latLonToTileF(double lat, double lon, int zoom);
    static QPair<int,int> latLonToTileXY(double lat, double lon, int zoom);

signals:
    void mapLoaded(double sceneUnitsPerMeter);
    void loadProgress(int done, int total);
    void loadError(const QString &msg);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    // 地面分辨率表（赤道处，米/像素），来自用户实测数据
    static double equatorResolution(int zoom);

    // 当前纬度实际地面分辨率（米/像素）
    static double metersPerPixelAt(int zoom, double lat);

    // 瓦片下载与缓存
    void requestTile(int tx, int ty, int col, int row);
    void onTileReady(int col, int row, const QImage &img);
    void assembleAndApply();

    // Qt3D 纹理平面构建（首次调用时创建，后续只更新纹理）
    void buildOrUpdatePlane(const QImage &mosaic, double sceneW, double sceneH);

    // ── 状态 ──
    double m_centerLat;
    double m_centerLon;
    int    m_zoom;
    int    m_tileRadius;          // ±N 瓦片（默认 3，共 7×7）
    int    m_totalTiles;
    int    m_pendingTiles;
    double m_sceneUnitsPerMeter;

    // col/row → 瓦片图像（col/row 是相对中心的偏移，范围 0~2*radius）
    QMap<QPair<int,int>, QImage> m_tileImages;

    QNetworkAccessManager *m_nam;

    // Qt3D 组件（首次 buildOrUpdatePlane 后持有）
    Qt3DExtras::QPlaneMesh    *m_planeMesh;
    Qt3DRender::QMaterial     *m_material;
    Qt3DCore::QTransform      *m_transform;
    InMemoryTextureImage      *m_texImage;    // 可重复 updateImage()

    // ── 3D 高程地形 ──
    TiffReader                m_tiffReader;
    Qt3DRender::QGeometryRenderer *m_terrainRenderer;
    TerrainGeometry           *m_terrainGeometry;
    bool                      m_hasTerrain;
};

#endif // MAPENTITY_H
