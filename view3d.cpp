#include "View3D.h"
#include "CoordinateConverter.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QSurfaceFormat>
#include <Qt3DExtras/QForwardRenderer>
#include <QThread>
#include <QApplication>

// View3D.cpp 实现
#include <Qt3DCore/QAspectEngine>
#include <Qt3DRender/QRenderAspect>
#include <QMetaObject>
#include <QMetaMethod>

#include "math.h"

// 核心：反射获取 Qt3DWindow 的 aspectEngine
Qt3DRender::QRenderAspect* View3D::getRenderAspect()
{
    if (!m_view) return nullptr;

    // 步骤1：通过元对象获取 aspectEngine（兼容 Qt 5.x）
    const QMetaObject* metaObj = m_view->metaObject();
    QMetaMethod aspectEngineMethod = metaObj->method(metaObj->indexOfMethod("aspectEngine()"));

    Qt3DCore::QAspectEngine* aspectEngine = nullptr;
    if (aspectEngineMethod.invoke(m_view, Qt::DirectConnection, Q_RETURN_ARG(Qt3DCore::QAspectEngine*, aspectEngine))) {
        // 步骤2：获取渲染切面
        QVector<Qt3DCore::QAbstractAspect*> aspects = aspectEngine->aspects();
        for (Qt3DCore::QAbstractAspect* aspect : aspects) {
            if (auto renderAspect = qobject_cast<Qt3DRender::QRenderAspect*>(aspect)) {
                return renderAspect;
            }
        }
    }
    return nullptr;
}

// 单独注册单个实体到渲染管线
void View3D::registerEntityToPipeline(Qt3DCore::QEntity* entity)
{
    if (!entity) return;

    Qt3DRender::QRenderAspect* renderAspect = getRenderAspect();
    if (!renderAspect) {
        qWarning() << "获取渲染切面失败，无法注册实体";
        return;
    }

    // 核心：调用 RenderAspect 的 registerEntity 方法（Qt 5.x 私有接口，但稳定可用）
    const QMetaObject* renderMeta = renderAspect->metaObject();
    QMetaMethod registerMethod = renderMeta->method(renderMeta->indexOfMethod("registerEntity(Qt3DCore::QEntity*)"));

    if (registerMethod.invoke(renderAspect, Qt::DirectConnection, Q_ARG(Qt3DCore::QEntity*, entity))) {
        qDebug() << "实体" << entity << "已单独注册到渲染管线";
    } else {
        qWarning() << "注册实体失败，降级使用刷新根实体方案";
        refreshRootEntity(); // 兜底方案
    }
}

void View3D::refreshRootEntity()
{
    if (!m_rootEntity || !m_view) return;

        // 1. 临时保存当前根实体
        Qt3DCore::QEntity* tempRoot = m_rootEntity;

        // 2. 先设置空根实体（触发渲染管线卸载）
        m_view->setRootEntity(nullptr);

        // 3. 延迟 10ms（确保管线卸载完成，Qt 事件循环兼容）
       {
            // 4. 重新设置根实体（触发管线重新扫描所有子实体）
            m_view->setRootEntity(tempRoot);
            qDebug() << "根实体刷新完成，所有子实体已重新注册到渲染管线";
        }
}


View3D::View3D(QWidget *parent) :
    QWidget(parent),
    m_gridVisible(true),
    m_maxTargetId(0),
    m_mapEntity(nullptr)
{
    setupUI();
    setupScene();
}

View3D::~View3D()
{
    if (m_rootEntity) {
        m_rootEntity->deleteLater();
    }

    // 清理所有目标
    clearAllTargets();
}

void View3D::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 1. 确保在主线程创建 Qt3DWindow
    //    Q_ASSERT(QThread::currentThread() == qApp->mainThread());

    m_view = new Qt3DExtras::Qt3DWindow();

    // 2. 配置 OpenGL 格式（保留你的配置）
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(4);
    m_view->setFormat(format);

    m_container = QWidget::createWindowContainer(m_view, this);
    m_container->setMinimumSize(600, 400);
    m_container->setFocusPolicy(Qt::StrongFocus);

    layout->addWidget(m_container);
}
void View3D::setupScene()
{
    m_rootEntity = new Qt3DCore::QEntity();

    // 创建坐标系统
    createCoordinateSystem();

    // 设置Qt3D窗口的根实体
    m_view->setRootEntity(m_rootEntity);

    if (m_view->defaultFrameGraph()) {
        m_view->defaultFrameGraph()->setClearColor(QColor(25, 25, 35));
    }

    // 相机设置...
    m_camera = m_view->camera();
    m_camera->setPosition(QVector3D(-2, 6, -15));
    m_camera->setViewCenter(QVector3D(0, 0, 0));
    m_camera->setUpVector(QVector3D(0, 1, 0));
    m_camera->lens()->setPerspectiveProjection(60.0f, 16.0f/9.0f, 0.1f, 200.0f);

    m_cameraController = new Qt3DExtras::QOrbitCameraController(m_rootEntity);
    m_cameraController->setCamera(m_camera);
    m_cameraController->setLinearSpeed(50.0f);
    m_cameraController->setLookSpeed(180.0f);

    // 连接相机变化信号
    connect(m_camera, &Qt3DRender::QCamera::positionChanged, this, &View3D::updateCameraInfo);
    connect(m_camera, &Qt3DRender::QCamera::viewCenterChanged, this, &View3D::updateCameraInfo);

    // 光照设置
    setupLighting();

    // 地图实体（不立即加载，等待用户输入经纬度）
    m_mapEntity = new MapEntity(m_rootEntity);

    updateCameraInfo();

    qDebug() << "3D场景初始化完成，支持多目标跟踪";
}

void View3D::createCoordinateSystem()
{
    qDebug()<<__func__<<QThread::currentThread();

    // X轴 - 红色（东方向）
    QVector<QVector3D> xAxis = {QVector3D(-8, 0, 0), QVector3D(8, 0, 0)};
    CustomLineEntity* xAxisLine = new CustomLineEntity(100, xAxis, QColor(255, 100, 100), 3.0f, m_rootEntity);

    // Y轴 - 绿色（高度）
    QVector<QVector3D> yAxis = {QVector3D(0, 0, 0), QVector3D(0, 6, 0)};
    CustomLineEntity* yAxisLine = new CustomLineEntity(101, yAxis, QColor(100, 255, 100), 3.0f, m_rootEntity);

    // Z轴 - 蓝色（北方向）
    QVector<QVector3D> zAxis = {QVector3D(0, 0, -6), QVector3D(0, 0, 6)};
    CustomLineEntity* zAxisLine = new CustomLineEntity(102, zAxis, QColor(100, 100, 255), 3.0f, m_rootEntity);

    // 创建网格平面 - RGB(50, 80, 50)
    for (int i = -10; i <= 10; i++) {
        if (i == 0) continue;

        // XY平面网格
//        QVector<QVector3D> gridLineX = {QVector3D(-5, i, 0), QVector3D(5, i, 0)};
//        QVector<QVector3D> gridLineY = {QVector3D(i, -5, 0), QVector3D(i, 5, 0)};

//        CustomLineEntity* gridX = new CustomLineEntity(200 + i, gridLineX, QColor(100, 180, 10), 0.5f, m_rootEntity);
//        CustomLineEntity* gridY = new CustomLineEntity(210 + i, gridLineY, QColor(100, 180, 10), 0.5f, m_rootEntity);

//        m_gridEntities.append(gridX);
//        m_gridEntities.append(gridY);

        // XZ平面网格
        QVector<QVector3D> gridLineXZ1 = {QVector3D(-10, 0, i), QVector3D(10, 0, i)};
        QVector<QVector3D> gridLineXZ2 = {QVector3D(i, 0, -10), QVector3D(i, 0, 10)};

        CustomLineEntity* gridXZ1 = new CustomLineEntity(220 + i, gridLineXZ1, QColor(100, 180, 10), 0.3f, m_rootEntity);
        CustomLineEntity* gridXZ2 = new CustomLineEntity(230 + i, gridLineXZ2, QColor(100, 180, 10), 0.3f, m_rootEntity);

        m_gridEntities.append(gridXZ1);
        m_gridEntities.append(gridXZ2);
    }

    //创建一个水平圆环（XZ平面）
    for(int i=0;i<10;i++){
        QVector3D center1(0, 0, 0);  // 中心点
        float radius1 = i;        // 半径
        int segments1 = 120;          // 分段数
        QVector3D normal1(0, 1, 0);  // 法向量向上

        QVector<QVector3D> circlePoints1 = generateCirclePoints(center1, radius1, segments1, normal1);

        // 创建水平圆环实体
        CustomLineEntity* horizontalCircle = new CustomLineEntity(
            3001,  // 唯一ID
            circlePoints1,
            QColor(100, 180, 10),  // 红色
            1.0f,               // 线宽
            m_rootEntity
        );
    }


}

void View3D::setupLighting()
{
    // 主光源
    Qt3DCore::QEntity *mainLight = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QDirectionalLight *directionalLight = new Qt3DRender::QDirectionalLight();
    directionalLight->setColor(QColor(255, 255, 255));
    directionalLight->setIntensity(0.9f);
    directionalLight->setWorldDirection(QVector3D(-0.7f, -0.5f, -0.5f));
    mainLight->addComponent(directionalLight);

    // 补光
    Qt3DCore::QEntity *fillLight = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QDirectionalLight *fillLightComp = new Qt3DRender::QDirectionalLight();
    fillLightComp->setColor(QColor(200, 220, 255));
    fillLightComp->setIntensity(0.3f);
    fillLightComp->setWorldDirection(QVector3D(0.3f, 0.2f, 0.5f));
    fillLight->addComponent(fillLightComp);

    // 环境光
    Qt3DCore::QEntity *ambientLight = new Qt3DCore::QEntity(m_rootEntity);
    Qt3DRender::QDirectionalLight *ambientLightComp = new Qt3DRender::QDirectionalLight();
    ambientLightComp->setColor(QColor(150, 150, 150));
    ambientLightComp->setIntensity(0.2f);
    ambientLightComp->setWorldDirection(QVector3D(0, -1, 0));
    ambientLight->addComponent(ambientLightComp);
}

QColor View3D::getTargetColor(int id) const
{
    // 预设颜色表，不同ID显示不同颜色
    static QList<QColor> colorTable = {
        QColor(255, 0, 0),      // ID 1: 红色
        QColor(0, 255, 0),      // ID 2: 绿色
        QColor(0, 0, 255),      // ID 3: 蓝色
        QColor(255, 255, 0),    // ID 4: 黄色
        QColor(255, 0, 255),    // ID 5: 紫色
        QColor(0, 255, 255),    // ID 6: 青色
        QColor(255, 128, 0),    // ID 7: 橙色
        QColor(128, 0, 255),    // ID 8: 紫红
        QColor(0, 255, 128),    // ID 9: 青绿
        QColor(255, 128, 128),  // ID 10: 粉红
        QColor(128, 255, 0),    // ID 11: 黄绿
        QColor(0, 128, 255),    // ID 12: 天蓝
        QColor(255, 0, 128),    // ID 13: 玫红
        QColor(128, 128, 255),  // ID 14: 淡紫
        QColor(255, 255, 128)   // ID 15: 浅黄
    };

    int index = (id - 1) % colorTable.size();
    return colorTable[index];
}


void View3D::createNewTarget(int id, const QVector3D &position)
{
    if (m_targets.contains(id)) {
        qWarning() << "目标" << id << "已存在，使用更新函数";
        updateExistingTarget(id, position);
        return;
    }

    qDebug() << "创建新目标:" << id << "位置:" << position;

    // 更新最大ID
    if (id > m_maxTargetId) {
        m_maxTargetId = id;
    }

    // 获取目标颜色
    QColor targetColor = getTargetColor(id);

    // 创建轨迹线（初始只有一个点）
    // 创建轨迹线 - 初始至少需要2个点才能显示
    QVector<QVector3D> initialPath = {position, position};  // 两个相同的点，形成零长度线

    CustomLineEntity* line = new CustomLineEntity(
                1000+id,  // 线ID = 1000 + 目标ID
                initialPath,
                targetColor,  // 使用目标颜色
                3.0f,         // 线宽
                m_rootEntity
                );
    line->setEnabled(true);

    this->registerEntityToPipeline(line);

    // 创建目标实体
    TargetEntity* target = new TargetEntity(m_rootEntity);
    target->setId(id);
    target->setPosition(position);
    target->setHeading(QVector3D(1, 0, 0));  // 默认航向 X+，高度针在球后与航线重合
    target->setVisible(true);

    initialPath = {position};
    // 存储目标数据
    TargetData data;
    data.target = target;
    data.line = line;
    data.path = initialPath;
    data.visible = true;

    m_targets.insert(id, data);

    qDebug() << "已创建目标" << id << "，当前目标数量:" << m_targets.size();

}

void View3D::updateExistingTarget(int id, const QVector3D &position)
{
    if (!m_targets.contains(id)) {
        qWarning() << "目标" << id << "不存在，使用创建函数";
        createNewTarget(id, position);
        return;
    }

    TargetData& data = m_targets[id];

    // 添加新的路径点
    data.path.append(position);

    // 更新轨迹线
    if (data.line) {
        data.line->setPoints(data.path);
    }

    // 更新目标位置与航向（航向由最后两点计算，高度针在 XZ 平面、球后、与航线重合）
    if (data.target) {
        data.target->setPosition(position);
        if (data.path.size() >= 2) {
            QVector3D from = data.path[data.path.size() - 2];
            QVector3D to = data.path[data.path.size() - 1];
            QVector3D dir = to - from;
            if (dir.lengthSquared() > 1e-10f) {
                data.target->setHeading(dir.normalized());
            }
        }
    }

    qDebug() << "更新目标" << id << "位置:" << position
             << "，轨迹点数:" << data.path.size();

    // 发出目标更新信号
    emit targetUpdated(id, position);
}

void View3D::setTarget(int id, const QVector3D &position)
{
    qDebug()<<__func__<<QThread::currentThread();
    if (id <= 0) {
        qWarning() << "无效的目标ID:" << id;
        return;
    }

    if (m_targets.contains(id)) {
        updateExistingTarget(id, position);
    } else {
        createNewTarget(id, position);
    }
}

void View3D::setTargetVisible(int id, bool visible)
{
    if (m_targets.contains(id)) {
        TargetData& data = m_targets[id];
        data.visible = visible;

        if (data.target) {
            data.target->setVisible(visible);
        }
        if (data.line) {
            data.line->setEnabled(visible);
        }

        qDebug() << "设置目标" << id << "可见性:" << (visible ? "显示" : "隐藏");
    }
}

void View3D::setAllTargetsVisible(bool visible)
{
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
        int id = it.key();
        setTargetVisible(id, visible);
    }
    qDebug() << "设置所有目标可见性:" << (visible ? "显示" : "隐藏");
}

QString View3D::getCameraInfo() const
{
    if (!m_camera) return "相机未初始化";

    QVector3D pos = m_camera->position();
    QVector3D center = m_camera->viewCenter();

    return QString("位置: (%1, %2, %3)\n观察点: (%4, %5, %6)")
            .arg(pos.x(), 0, 'f', 2)
            .arg(pos.y(), 0, 'f', 2)
            .arg(pos.z(), 0, 'f', 2)
            .arg(center.x(), 0, 'f', 2)
            .arg(center.y(), 0, 'f', 2)
            .arg(center.z(), 0, 'f', 2);
}

void View3D::resetCamera()
{
    if (m_camera) {
        m_camera->setPosition(QVector3D(12, 8, 10));
        m_camera->setViewCenter(QVector3D(0, 0, 0));
        updateCameraInfo();
    }
}

void View3D::toggleGrid()
{
    m_gridVisible = !m_gridVisible;
    for (CustomLineEntity* grid : m_gridEntities) {
        grid->setEnabled(m_gridVisible);
    }
    qDebug() << "网格可见性:" << (m_gridVisible ? "显示" : "隐藏");
}

QVector3D View3D::getTargetPosition(int id) const
{
    if (m_targets.contains(id) && m_targets[id].target) {
        return m_targets[id].target->position();
    }
    return QVector3D();
}

bool View3D::hasTarget(int id) const
{
    return m_targets.contains(id);
}

QList<int> View3D::getAllTargetIds() const
{
    return m_targets.keys();
}

void View3D::clearAllTargets()
{
    qDebug() << "清理所有目标，数量:" << m_targets.size();

    for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
        TargetData& data = it.value();
        if (data.target) {
            data.target->deleteLater();
        }
        if (data.line) {
            data.line->deleteLater();
        }
    }

    m_targets.clear();
    m_maxTargetId = 0;
}

void View3D::updateCameraInfo()
{
    emit cameraChanged(getCameraInfo());
}

void View3D::loadMap(double lat, double lon, int zoom)
{
    if (!m_mapEntity) {
        qWarning() << "View3D::loadMap: mapEntity 未初始化";
        return;
    }

    // 地图加载完成后更新 CoordinateConverter 缩放比
    connect(m_mapEntity, &MapEntity::mapLoaded, this, [this](double scale) {
        CoordinateConverter::setSceneUnitsPerMeter(scale);
        qDebug() << "View3D: 地图加载完成，CoordinateConverter 缩放比已更新为" << scale;
    }, Qt::UniqueConnection);

    m_mapEntity->loadMap(lat, lon, zoom);
    qDebug() << "View3D::loadMap 已触发 lat=" << lat << " lon=" << lon << " zoom=" << zoom;
}


QVector<QVector3D> View3D::generateCirclePoints(const QVector3D& center, float radius,
                                               int segments, const QVector3D& normal)
{
    QVector<QVector3D> points;

    // 标准化法向量
    QVector3D n = normal.normalized();

    // 检查是哪个主平面
    if (qAbs(n.x()) > 0.99f) {
        // 法向量沿X轴 -> YZ平面
        qDebug() << "生成YZ平面圆环";
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x();  // X坐标固定
            float y = center.y() + radius * cos(angle);
            float z = center.z() + radius * sin(angle);
            points.append(QVector3D(x, y, z));
        }
    }
    else if (qAbs(n.y()) > 0.99f) {
        // 法向量沿Y轴 -> XZ平面（水平圆）
        qDebug() << "生成XZ平面圆环（水平）";
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x() + radius * cos(angle);
            float y = center.y();  // Y坐标固定
            float z = center.z() + radius * sin(angle);
            points.append(QVector3D(x, y, z));
        }
    }
    else if (qAbs(n.z()) > 0.99f) {
        // 法向量沿Z轴 -> XY平面（垂直圆）
        qDebug() << "生成XY平面圆环（垂直）";
        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            float x = center.x() + radius * cos(angle);
            float y = center.y() + radius * sin(angle);
            float z = center.z();  // Z坐标固定
            points.append(QVector3D(x, y, z));
        }
    }
    else {
        // 任意法向量的通用情况
        qDebug() << "生成任意方向圆环";

        // 构建局部坐标系
        QVector3D up(0, 1, 0);
        if (qAbs(QVector3D::dotProduct(n, up)) > 0.99f) {
            up = QVector3D(0, 0, 1);
        }

        QVector3D u = QVector3D::crossProduct(up, n).normalized();
        QVector3D v = QVector3D::crossProduct(n, u).normalized();

        for (int i = 0; i <= segments; i++) {
            float angle = 2.0f * M_PI * i / segments;
            QVector3D localPoint = radius * (u * cos(angle) + v * sin(angle));
            points.append(center + localPoint);
        }
    }

    return points;
}
