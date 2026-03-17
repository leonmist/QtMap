#include "MainWindow.h"
#include "MapEntity.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    setupUI();
}

void MainWindow::setupUI()
{
    setWindowTitle("三维坐标系显示 - 目标定位系统");
    resize(1200, 700);

    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 创建3D视图（占3/4宽度）
    m_view3D = new View3D(this);
    m_view3D->setMinimumSize(600, 400);

    // 创建控制面板（占1/4宽度）
    m_controlPanel = new ControlPanel(this);
    m_controlPanel->setMinimumWidth(250);

    // 添加到布局
    mainLayout->addWidget(m_view3D, 3);  // 3份宽度
    mainLayout->addWidget(m_controlPanel, 1); // 1份宽度

    // 连接信号和槽
    connect(m_controlPanel, &ControlPanel::resetCameraRequested, this, &MainWindow::handleResetCamera);
    connect(m_controlPanel, &ControlPanel::toggleGridRequested, this, &MainWindow::handleToggleGrid);
    connect(m_view3D, &View3D::cameraChanged, this, &MainWindow::updateCameraInfo);

    // 连接目标相关信号
    connect(m_controlPanel, &ControlPanel::targetChanged,
            m_view3D, &View3D::setTarget, Qt::QueuedConnection);
    connect(m_controlPanel, &ControlPanel::targetVisibilityChanged,
            m_view3D, &View3D::setAllTargetsVisible, Qt::QueuedConnection);

    // 连接地图加载信号
    connect(m_controlPanel, &ControlPanel::loadMapRequested,
            this, &MainWindow::handleLoadMap);
    if (m_view3D->mapEntity()) {
        connect(m_view3D->mapEntity(), &MapEntity::loadProgress,
                m_controlPanel, &ControlPanel::setMapLoadProgress,
                Qt::QueuedConnection);
    }
    // 初始信息
    m_controlPanel->setInfo("X轴: 东方向 (红色)\nY轴: 高度 (绿色)\nZ轴: 北方向 (蓝色)");

    // 初始更新相机信息
    updateCameraInfo(m_view3D->getCameraInfo());

    qDebug() << "=== 三维坐标系显示Demo ===";
    qDebug() << "目标定位系统已启动";
}

void MainWindow::handleResetCamera()
{
    m_view3D->resetCamera();
}

void MainWindow::handleToggleGrid()
{
    m_view3D->toggleGrid();
}

void MainWindow::updateCameraInfo(const QString &info)
{
    m_controlPanel->setCameraPosition(info);
}

void MainWindow::handleLoadMap(double lat, double lon, int zoom)
{
    m_view3D->loadMap(lat, lon, zoom);
}
