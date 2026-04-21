#include "mapentity.h"
#include "CoordinateConverter.h"

#include <Qt3DRender/QTexture>
#include <Qt3DRender/QTextureImage>
#include <Qt3DRender/QParameter>
#include <Qt3DExtras/QDiffuseMapMaterial>
#include <Qt3DExtras/QTextureMaterial>

#include <QNetworkRequest>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QDebug>
#include <cmath>
#include <QCoreApplication>

// ─── 地面分辨率表（赤道，米/像素）────────────────────────────────────────────

static const QMap<int, double> s_groundResTable = {
    {0,  156412.0}, {1,  78206.0},  {2,  39103.0},  {3,  19551.0},
    {4,   9776.0},  {5,   4888.0},  {6,   2444.0},  {7,   1222.0},
    {8,    610.984},{9,    305.492},{10,   152.746}, {11,   76.373},
    {12,   38.187}, {13,   19.093},{14,     9.547},  {15,    4.773},
    {16,    2.387}, {17,    1.193}, {18,    0.596},  {19,    0.298},
    {20,    0.149}
};

// ─── 静态工具 ─────────────────────────────────────────────────────────────────

double MapEntity::equatorResolution(int zoom)
{
    return s_groundResTable.value(zoom, 9.547);
}

double MapEntity::metersPerPixelAt(int zoom, double lat)
{
    return equatorResolution(zoom) * std::cos(lat * M_PI / 180.0);
}

// 连续瓦片坐标（带小数）
QPointF MapEntity::latLonToTileF(double lat, double lon, int zoom)
{
    double n = std::pow(2.0, zoom);
    double latRad = lat * M_PI / 180.0;
    double tx = (lon + 180.0) / 360.0 * n;
    double ty = (1.0 - std::log(std::tan(latRad) + 1.0 / std::cos(latRad)) / M_PI) / 2.0 * n;
    return QPointF(tx, ty);
}

// 整数瓦片坐标
QPair<int,int> MapEntity::latLonToTileXY(double lat, double lon, int zoom)
{
    QPointF f = latLonToTileF(lat, lon, zoom);
    return qMakePair(static_cast<int>(std::floor(f.x())),
                     static_cast<int>(std::floor(f.y())));
}

// ─── 构造 / 析构 ──────────────────────────────────────────────────────────────

MapEntity::MapEntity(Qt3DCore::QEntity *parent)
    : Qt3DCore::QEntity(parent)
    , m_centerLat(0)
    , m_centerLon(0)
    , m_zoom(14)
    , m_tileRadius(10)        // 7×7 瓦片
    , m_totalTiles(0)
    , m_pendingTiles(0)
    , m_sceneUnitsPerMeter(1.0 / DISBASE)
    , m_nam(new QNetworkAccessManager(this))
    , m_planeMesh(nullptr)
    , m_material(nullptr)
    , m_transform(nullptr)
    , m_texImage(nullptr)
{
    connect(m_nam, &QNetworkAccessManager::finished,
            this, &MapEntity::onReplyFinished);
}

MapEntity::~MapEntity() {}

// ─── loadMap ─────────────────────────────────────────────────────────────────

void MapEntity::loadMap(double centerLat, double centerLon, int zoom)
{
    m_centerLat  = centerLat;
    m_centerLon  = centerLon;
    m_zoom       = zoom;
    m_tileImages.clear();

    int side      = 2 * m_tileRadius + 1;   // 7
    m_totalTiles  = side * side;             // 49
    m_pendingTiles = 0;

    auto centerTile = latLonToTileXY(centerLat, centerLon, zoom);
    int cx = centerTile.first;
    int cy = centerTile.second;

    qDebug() << "MapEntity: 开始加载地图 lat=" << centerLat
             << " lon=" << centerLon << " zoom=" << zoom
             << " 中心瓦片=(" << cx << "," << cy << ")";

    for (int dr = -m_tileRadius; dr <= m_tileRadius; dr++) {
        for (int dc = -m_tileRadius; dc <= m_tileRadius; dc++) {
            int tx  = cx + dc;
            int ty  = cy + dr;
            int col = dc + m_tileRadius;   // 0 ~ 2*radius
            int row = dr + m_tileRadius;
            requestTile(tx, ty, col, row);
        }
    }
}

// ─── 瓦片请求（带本地缓存）────────────────────────────────────────────────────

void MapEntity::requestTile(int tx, int ty, int col, int row)
{
    // 缓存路径使用可执行文件同级目录，避免相对路径找不到
    static QString s_cacheRoot;
    if (s_cacheRoot.isEmpty()) {
        s_cacheRoot = QCoreApplication::applicationDirPath() + "/map_cache";
    }

    QString cacheDir  = QString("%1/%2/%3").arg(s_cacheRoot).arg(m_zoom).arg(tx);
    QDir().mkpath(cacheDir);
    QString cachePath = QString("%1/%2.png").arg(cacheDir).arg(ty);

    if (QFile::exists(cachePath)) {
        QImage img(cachePath);
        if (!img.isNull()) {
            qDebug() << "MapEntity: 命中缓存" << cachePath;
            onTileReady(col, row, img);
            return;
        }
    }

    // 缓存未命中，从高德下载（使用 http:// 避免 SSL 依赖）
    m_pendingTiles++;
    // style=7: 路网地图；style=6: 卫星图；style=8: 路网（高德二版）
    QString urlStr = QString(
        "http://wprd04.is.autonavi.com/appmaptile"
        "?style=6&lang=zh_cn&size=1&scl=1&x=%1&y=%2&z=%3"
    ).arg(tx).arg(ty).arg(m_zoom);

    qDebug() << "MapEntity: 下载瓦片 col=" << col << " row=" << row
             << " url=" << urlStr;

    QUrl tileUrl(urlStr);
    QNetworkRequest req(tileUrl);
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    // 允许 HTTP 重定向
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);

    QNetworkReply *reply = m_nam->get(req);

    reply->setProperty("col",       col);
    reply->setProperty("row",       row);
    reply->setProperty("cachePath", cachePath);
}

void MapEntity::onReplyFinished(QNetworkReply *reply)
{
    int    col       = reply->property("col").toInt();
    int    row       = reply->property("row").toInt();
    QString cachePath = reply->property("cachePath").toString();

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "MapEntity: 瓦片下载失败"
                   << "col=" << col << "row=" << row
                   << "error=" << reply->errorString()
                   << "http=" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_pendingTiles--;
        QImage placeholder(256, 256, QImage::Format_RGB888);
        placeholder.fill(QColor(80, 80, 80));
        onTileReady(col, row, placeholder);
        return;
    }

    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray data = reply->readAll();
    qDebug() << "MapEntity: 瓦片响应 col=" << col << "row=" << row
             << "http=" << httpStatus << "bytes=" << data.size();

    QImage img;
    img.loadFromData(data);

    if (img.isNull()) {
        qWarning() << "MapEntity: 瓦片图像解析失败 col=" << col << "row=" << row
                   << "bytes=" << data.size();
        img = QImage(256, 256, QImage::Format_RGB888);
        img.fill(QColor(100, 60, 60));
    } else {
        // 写入本地缓存
        if (!cachePath.isEmpty()) {
            bool saved = img.save(cachePath, "PNG");
            qDebug() << "MapEntity: 缓存写入" << (saved ? "成功" : "失败") << cachePath;
        }
    }

    m_pendingTiles--;
    onTileReady(col, row, img);
}

// ─── 瓦片收集完成后拼图 ───────────────────────────────────────────────────────

void MapEntity::onTileReady(int col, int row, const QImage &img)
{
    m_tileImages.insert(qMakePair(col, row), img);

    int received = m_tileImages.size();
    emit loadProgress(received, m_totalTiles);

    qDebug() << "MapEntity: 瓦片(" << col << "," << row << ") 已就绪"
             << received << "/" << m_totalTiles;

    if (received == m_totalTiles) {
        assembleAndApply();
    }
}

// ─── 拼图并应用到 Qt3D 纹理平面 ─────────────────────────────────────────────

void MapEntity::assembleAndApply()
{
    int side       = 2 * m_tileRadius + 1;
    int totalPix   = side * 256;

    // 计算中心点在中心瓦片内的小数偏移，整体平移绘制使真实中心点对齐 mosaic 中心像素
    QPointF cf    = latLonToTileF(m_centerLat, m_centerLon, m_zoom);
    double frac_x = cf.x() - std::floor(cf.x());
    double frac_y = cf.y() - std::floor(cf.y());
    int dx_shift  = static_cast<int>(std::round(frac_x * 256.0 - 128.0 ));
    int dy_shift  = static_cast<int>(std::round(frac_y * 256.0 - 128.0 ));

    qDebug() << "MapEntity: 中心点对齐 frac=(" << frac_x << "," << frac_y << ")"
             << " shift=(" << dx_shift << "," << dy_shift << ")px";

    QImage mosaic(totalPix, totalPix, QImage::Format_RGBA8888);
    mosaic.fill(Qt::black);

    QPainter painter(&mosaic);
    for (int row = 0; row < side; row++) {
        for (int col = 0; col < side; col++) {
            auto key = qMakePair(col, row);
            if (m_tileImages.contains(key)) {
                painter.drawImage(col * 256 - dx_shift, row * 256 - dy_shift, m_tileImages[key]);
            }
            // 每张瓦片叠加红色边框便于辨识
            // painter.setPen(QPen(Qt::red, 2));
            // painter.setBrush(Qt::NoBrush);
            // painter.drawRect(col * 256 + dx_shift, row * 256 + dy_shift, 255, 255);
        }
    }
    painter.end();

    // 计算场景坐标缩放比
    double mpp      = metersPerPixelAt(m_zoom, m_centerLat);  // 米/像素 @当前纬度
    double realWidth = totalPix * mpp;                         // 真实宽度（米）
    // 地图平面在场景中占 20 个场景单元（-10 ~ +10）
    const double sceneSize = 200.0;
    m_sceneUnitsPerMeter   = sceneSize / realWidth;

    qDebug() << "MapEntity: 拼图完成"
             << "mpp=" << mpp << "m/px"
             << "realWidth=" << realWidth << "m"
             << "sceneUnitsPerMeter=" << m_sceneUnitsPerMeter;

    buildOrUpdatePlane(mosaic, sceneSize, sceneSize);

    emit mapLoaded(m_sceneUnitsPerMeter);
}

// ─── Qt3D 平面构建 / 更新 ─────────────────────────────────────────────────────

void MapEntity::buildOrUpdatePlane(const QImage &mosaic, double sceneW, double sceneH)
{
    if (!m_planeMesh) {
        // 首次：创建所有 Qt3D 组件
        m_planeMesh = new Qt3DExtras::QPlaneMesh(this);
        m_planeMesh->setWidth(static_cast<float>(sceneW));
        m_planeMesh->setHeight(static_cast<float>(sceneH));
        m_planeMesh->setMeshResolution(QSize(2, 2));

        // 创建纹理
        Qt3DRender::QTexture2D *texture = new Qt3DRender::QTexture2D(this);
        texture->setMinificationFilter(Qt3DRender::QTexture2D::Linear);
        texture->setMagnificationFilter(Qt3DRender::QTexture2D::Linear);
        texture->setWrapMode(Qt3DRender::QTextureWrapMode(Qt3DRender::QTextureWrapMode::ClampToEdge));
        texture->setGenerateMipMaps(false);
        texture->setFormat(Qt3DRender::QAbstractTexture::RGBA8_UNorm);

        m_texImage = new InMemoryTextureImage(mosaic, this);
        texture->addTextureImage(m_texImage);

        // 使用 QTextureMaterial（仅漫射纹理，不受光照影响，地图颜色保真）
        Qt3DExtras::QTextureMaterial *mat = new Qt3DExtras::QTextureMaterial(this);
        mat->setTexture(texture);
        mat->setAlphaBlendingEnabled(false);
        m_material = mat;

        // 变换：地图放在 y = -0.01 轻微下移，避免与 y=0 的网格 z-fighting
        m_transform = new Qt3DCore::QTransform(this);
        m_transform->setTranslation(QVector3D(0.0f, -0.01f, 0.0f));

        addComponent(m_planeMesh);
        addComponent(m_material);
        addComponent(m_transform);

        qDebug() << "MapEntity: Qt3D 地图平面已创建，尺寸=" << sceneW << "×" << sceneH;
    } else {
        // 后续重新加载：只更新尺寸和纹理
        m_planeMesh->setWidth(static_cast<float>(sceneW));
        m_planeMesh->setHeight(static_cast<float>(sceneH));
        if (m_texImage) {
            m_texImage->updateImage(mosaic);
        }
        qDebug() << "MapEntity: Qt3D 地图纹理已更新";
    }
}
