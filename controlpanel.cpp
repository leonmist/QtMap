#include "ControlPanel.h"
#include "CoordinateConverter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDebug>

#include <QTime>

ControlPanel::ControlPanel(QWidget *parent) :
    QWidget(parent),
    m_targetVisible(false),
    m_simulationCounter(1)
{
    setupUI();

    // 初始化随机种子
    QTime time = QTime::currentTime();
    qsrand(static_cast<uint>(time.msec() + time.second() * 1000));

    // 初始化模拟定时器
    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(1000); // 1秒间隔

    // 连接定时器信号
    connect(m_simulationTimer, &QTimer::timeout, this, &ControlPanel::onSimulationTimeout);
}

ControlPanel::~ControlPanel()
{
    // 确保定时器停止
    if (m_simulationTimer->isActive()) {
        m_simulationTimer->stop();
    }
}

void ControlPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ==== 坐标系信息 ====
    QGroupBox *infoGroup = new QGroupBox("坐标系信息");
    QVBoxLayout *infoLayout = new QVBoxLayout();

    m_infoLabel = new QLabel("三维坐标系 \nX轴: 东方向 (红色)\nY轴: 高度 (绿色)\nZ轴: 北方向 (蓝色)");
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setAlignment(Qt::AlignLeft);
    infoLayout->addWidget(m_infoLabel);

    m_cameraLabel = new QLabel("相机位置: 未初始化");
    m_cameraLabel->setWordWrap(true);
    infoLayout->addWidget(m_cameraLabel);

    infoGroup->setLayout(infoLayout);
    mainLayout->addWidget(infoGroup);

    // ==== 目标输入 ====
    QGroupBox *targetGroup = new QGroupBox("目标位置输入");
    QVBoxLayout *targetLayout = new QVBoxLayout();

    // 目标参数输入
    QGroupBox *paramGroup = new QGroupBox("目标参数");
    QFormLayout *paramLayout = new QFormLayout();

    // 编号输入
    m_idSpinBox = new QSpinBox();
    m_idSpinBox->setRange(1, 999);
    m_idSpinBox->setValue(1);
    m_idSpinBox->setSuffix(" 号");
    paramLayout->addRow("目标编号:", m_idSpinBox);

    // 距离输入
    m_distanceSpinBox = new QDoubleSpinBox();
    m_distanceSpinBox->setRange(0, 10000);
    m_distanceSpinBox->setValue(100);
    m_distanceSpinBox->setSingleStep(10);
    m_distanceSpinBox->setSuffix(" m");
    paramLayout->addRow("距离:", m_distanceSpinBox);

    // 方位角输入
    m_azimuthSpinBox = new QDoubleSpinBox();
    m_azimuthSpinBox->setRange(0, 360);
    m_azimuthSpinBox->setValue(45);
    m_azimuthSpinBox->setSingleStep(5);
    m_azimuthSpinBox->setSuffix(" °");
    paramLayout->addRow("方位角:", m_azimuthSpinBox);

    // 高度输入
    m_heightSpinBox = new QDoubleSpinBox();
    m_heightSpinBox->setRange(-1000, 10000);
    m_heightSpinBox->setValue(0);
    m_heightSpinBox->setSingleStep(10);
    m_heightSpinBox->setSuffix(" m");
    paramLayout->addRow("高度:", m_heightSpinBox);

    paramGroup->setLayout(paramLayout);
    targetLayout->addWidget(paramGroup);

    // 直角坐标显示
    QGroupBox *cartesianGroup = new QGroupBox("直角坐标");
    QFormLayout *cartesianLayout = new QFormLayout();

    m_xCoordEdit = new QLineEdit();
    m_xCoordEdit->setReadOnly(true);
    cartesianLayout->addRow("X (东):", m_xCoordEdit);

    m_yCoordEdit = new QLineEdit();
    m_yCoordEdit->setReadOnly(true);
    cartesianLayout->addRow("Y (高):", m_yCoordEdit);

    m_zCoordEdit = new QLineEdit();
    m_zCoordEdit->setReadOnly(true);
    cartesianLayout->addRow("Z (北):", m_zCoordEdit);

    cartesianGroup->setLayout(cartesianLayout);
    targetLayout->addWidget(cartesianGroup);

    // 目标控制按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_setTargetBtn = new QPushButton("设置目标");
    m_clearTargetBtn = new QPushButton("清除目标");
    m_showHideTargetBtn = new QPushButton("显示目标");

    buttonLayout->addWidget(m_setTargetBtn);
    buttonLayout->addWidget(m_clearTargetBtn);
    buttonLayout->addWidget(m_showHideTargetBtn);
    targetLayout->addLayout(buttonLayout);

    targetGroup->setLayout(targetLayout);
    mainLayout->addWidget(targetGroup);

    // ==== 模拟控制 ====
    QGroupBox *simulationGroup = new QGroupBox("目标模拟");
    QVBoxLayout *simulationLayout = new QVBoxLayout();

    // 模拟参数
    QGroupBox *simParamGroup = new QGroupBox("模拟参数");
    QFormLayout *simParamLayout = new QFormLayout();

    m_simRadiusSpinBox = new QDoubleSpinBox();
    m_simRadiusSpinBox->setRange(100, 5000);
    m_simRadiusSpinBox->setValue(2000);
    m_simRadiusSpinBox->setSingleStep(100);
    m_simRadiusSpinBox->setSuffix(" m");
    simParamLayout->addRow("模拟半径:", m_simRadiusSpinBox);

    m_simSpeedSpinBox = new QSpinBox();
    m_simSpeedSpinBox->setRange(1, 10);
    m_simSpeedSpinBox->setValue(1);
    m_simSpeedSpinBox->setSuffix(" 点/秒");
    simParamLayout->addRow("模拟速度:", m_simSpeedSpinBox);

    simParamGroup->setLayout(simParamLayout);
    simulationLayout->addWidget(simParamGroup);

    // 模拟控制按钮
    QHBoxLayout *simButtonLayout = new QHBoxLayout();
    m_startSimBtn = new QPushButton("开始模拟");
    m_stopSimBtn = new QPushButton("停止模拟");
    m_stopSimBtn->setEnabled(false);  // 初始不可用

    simButtonLayout->addWidget(m_startSimBtn);
    simButtonLayout->addWidget(m_stopSimBtn);
    simulationLayout->addLayout(simButtonLayout);

    simulationGroup->setLayout(simulationLayout);
    mainLayout->addWidget(simulationGroup);

    // ==== 地图加载 ====
    QGroupBox *mapGroup = new QGroupBox("高德地图加载");
    QVBoxLayout *mapLayout = new QVBoxLayout();

    QFormLayout *mapFormLayout = new QFormLayout();

    m_latSpinBox = new QDoubleSpinBox();
    m_latSpinBox->setRange(-90.0, 90.0);
    m_latSpinBox->setValue(31.2304);   // 默认上海
    m_latSpinBox->setDecimals(6);
    m_latSpinBox->setSingleStep(0.01);
    m_latSpinBox->setSuffix(" °N");
    mapFormLayout->addRow("中心纬度:", m_latSpinBox);

    m_lonSpinBox = new QDoubleSpinBox();
    m_lonSpinBox->setRange(-180.0, 180.0);
    m_lonSpinBox->setValue(121.4737);  // 默认上海
    m_lonSpinBox->setDecimals(6);
    m_lonSpinBox->setSingleStep(0.01);
    m_lonSpinBox->setSuffix(" °E");
    mapFormLayout->addRow("中心经度:", m_lonSpinBox);

    m_zoomSpinBox = new QSpinBox();
    m_zoomSpinBox->setRange(8, 18);
    m_zoomSpinBox->setValue(14);
    mapFormLayout->addRow("缩放级别:", m_zoomSpinBox);

    mapLayout->addLayout(mapFormLayout);

    m_loadMapBtn = new QPushButton("加载地图");
    mapLayout->addWidget(m_loadMapBtn);

    m_mapProgressLabel = new QLabel("未加载");
    m_mapProgressLabel->setAlignment(Qt::AlignCenter);
    m_mapProgressLabel->setStyleSheet("QLabel { color: gray; }");
    mapLayout->addWidget(m_mapProgressLabel);

    mapGroup->setLayout(mapLayout);
    mainLayout->addWidget(mapGroup);

    connect(m_loadMapBtn, &QPushButton::clicked, this, &ControlPanel::onLoadMapClicked);

    // ==== 相机控制 ====
    QGroupBox *cameraGroup = new QGroupBox("相机控制");
    QVBoxLayout *cameraLayout = new QVBoxLayout();

    m_resetCameraBtn = new QPushButton("重置相机视角");
    m_toggleGridBtn = new QPushButton("显示/隐藏网格");

    cameraLayout->addWidget(m_resetCameraBtn);
    cameraLayout->addWidget(m_toggleGridBtn);

    cameraGroup->setLayout(cameraLayout);
    mainLayout->addWidget(cameraGroup);

    mainLayout->addStretch();

    // 连接信号
    connect(m_resetCameraBtn, &QPushButton::clicked, this, &ControlPanel::resetCameraRequested);
    connect(m_toggleGridBtn, &QPushButton::clicked, this, &ControlPanel::toggleGridRequested);

    // 连接参数变化信号
    // (原有的参数变化连接，根据您的注释是否启用)
    // connect(m_idSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    //         this, &ControlPanel::onTargetInputChanged);
    // connect(m_distanceSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    //         this, &ControlPanel::onTargetInputChanged);
    // connect(m_azimuthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    //         this, &ControlPanel::onTargetInputChanged);
    // connect(m_heightSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    //         this, &ControlPanel::onTargetInputChanged);

    // 连接按钮信号
    connect(m_setTargetBtn, &QPushButton::clicked, this, &ControlPanel::onSetTargetClicked);
    connect(m_clearTargetBtn, &QPushButton::clicked, this, &ControlPanel::onClearTargetClicked);

    connect(m_showHideTargetBtn, &QPushButton::clicked, [this]() {
        m_targetVisible = !m_targetVisible;
        m_showHideTargetBtn->setText(m_targetVisible ? "隐藏目标" : "显示目标");
        emit targetVisibilityChanged(m_targetVisible);
    });

    // 连接模拟按钮信号
    connect(m_startSimBtn, &QPushButton::clicked, this, &ControlPanel::onStartSimulationClicked);
    connect(m_stopSimBtn, &QPushButton::clicked, this, &ControlPanel::onStopSimulationClicked);

    // 初始更新
    onTargetInputChanged();
}

// ... 原有的其他函数保持不变 ...

// 新增模拟相关函数

void ControlPanel::onStartSimulationClicked()
{
    m_simulationCounter = 1;
    m_simulationTimer->start(1000 / m_simSpeedSpinBox->value());

    m_startSimBtn->setEnabled(false);
    m_stopSimBtn->setEnabled(true);

    qDebug() << "开始目标模拟，速度:" << m_simSpeedSpinBox->value() << "点/秒";
}

void ControlPanel::onStopSimulationClicked()
{
    m_simulationTimer->stop();

    m_startSimBtn->setEnabled(true);
    m_stopSimBtn->setEnabled(false);

    qDebug() << "停止目标模拟";
}

void ControlPanel::onSimulationTimeout()
{
    // 生成模拟点
    QVector3D position = generateSimulationPoint();

    // 获取当前编号（或使用模拟计数器）
    int id = m_idSpinBox->value();

    // 更新UI显示
    m_distanceSpinBox->setValue(position.x());
    m_azimuthSpinBox->setValue(position.y());
    m_heightSpinBox->setValue(position.z());

    this->onSetTargetClicked();

    qDebug() << "模拟目标点" << m_simulationCounter++
             << " - 编号:" << id
             << "位置:" << position;
}

QVector3D ControlPanel::generateSimulationPoint()
{
    static double angle = 0.0;  // 静态角度变量

    // 模拟参数
    const double radius = m_simRadiusSpinBox ? m_simRadiusSpinBox->value() : 5000.0; // 圆周半径（来自UI）
    const double height = 200.0;       // 高度（沿用当前输入）
    const double speed = 5.0;                                                         // 角度增加速度（度/次）

    // 角度增加（0-360度循环）
    angle += speed;
    if (angle >= 360.0) {
        angle -= 360.0;
    }

    // 固定距离，角度变化
    double distance = radius;  // 固定距离

    // 转换为直角坐标
    QVector3D position(
        distance, angle, height);

    qDebug() << "圆周模拟 - 角度:" << angle
             << "距离:" << distance
             << "高度:" << height;

    return position;
}
// ... 原有的其他函数保持不变 ...
double ControlPanel::getTargetDistance() const
{
    return m_distanceSpinBox->value();
}

double ControlPanel::getTargetAzimuth() const
{
    return m_azimuthSpinBox->value();
}

double ControlPanel::getTargetHeight() const
{
    return m_heightSpinBox->value();
}

int ControlPanel::getTargetId() const
{
    return m_idSpinBox->value();
}

QVector3D ControlPanel::getTargetPosition() const
{
    return CoordinateConverter::distanceAzimuthHeightToCartesian(
                getTargetDistance(),
                getTargetAzimuth(),
                getTargetHeight()
                );
}

void ControlPanel::setInfo(const QString &text)
{
    m_infoLabel->setText(text);
}

void ControlPanel::setCameraPosition(const QString &position)
{
    m_cameraLabel->setText("相机位置:\n" + position);
}

void ControlPanel::setTarget(int id, const QVector3D &position)
{
    // 设置编号
    m_idSpinBox->setValue(id);

    // 转换坐标并设置
    double distance, azimuth, height;
    CoordinateConverter::cartesianToDistanceAzimuthHeight(position, distance, azimuth, height);

    m_distanceSpinBox->setValue(distance);
    m_azimuthSpinBox->setValue(azimuth);
    m_heightSpinBox->setValue(height);

    onTargetInputChanged();
}

void ControlPanel::onTargetInputChanged()
{
    QVector3D position = getTargetPosition();

    // 更新直角坐标显示
    m_xCoordEdit->setText(QString::number(position.x(), 'f', 3));
    m_yCoordEdit->setText(QString::number(position.y(), 'f', 3));
    m_zCoordEdit->setText(QString::number(position.z(), 'f', 3));

    // 更新信息标签
    QString info = QString("目标编号: %1\n").arg(getTargetId())
            .append(QString("距离: %1 m\n").arg(getTargetDistance(), 0, 'f', 1))
            .append(QString("方位角: %1 °\n").arg(getTargetAzimuth(), 0, 'f', 1))
            .append(QString("高度: %1 m").arg(getTargetHeight(), 0, 'f', 1));

    // 高度验证
    if (fabs(getTargetHeight()) > getTargetDistance() + 0.1) {
        info += "\n⚠️ 警告: 高度超过距离!";
        m_heightSpinBox->setStyleSheet("QDoubleSpinBox { background-color: #FFCDD2; }");
    } else {
        m_heightSpinBox->setStyleSheet("");
    }

    m_infoLabel->setText(info);
}

void ControlPanel::onSetTargetClicked()
{
    int id = getTargetId();
    QVector3D position = getTargetPosition();

    // 发出目标变化信号（包含编号和位置）
    emit targetChanged(id, position);

    // 如果目标当前不可见，自动显示
    if (!m_targetVisible) {
        m_targetVisible = true;
        m_showHideTargetBtn->setText("隐藏目标");
        emit targetVisibilityChanged(true);
    }

    qDebug() << "设置/更新目标 - 编号:" << id << "位置:" << position;
}
void ControlPanel::onClearTargetClicked()
{
    // 重置位置输入（保留编号）
    m_distanceSpinBox->setValue(0);
    m_azimuthSpinBox->setValue(0);
    m_heightSpinBox->setValue(0);

    // 更新显示
    onTargetInputChanged();

    // 隐藏目标
    m_targetVisible = false;
    m_showHideTargetBtn->setText("显示目标");
    emit targetVisibilityChanged(false);

    qDebug() << "清除目标位置，编号保持:" << getTargetId();
}

void ControlPanel::onLoadMapClicked()
{
    double lat  = m_latSpinBox->value();
    double lon  = m_lonSpinBox->value();
    int    zoom = m_zoomSpinBox->value();

    m_loadMapBtn->setEnabled(false);
    m_mapProgressLabel->setText("加载中 0/49...");
    m_mapProgressLabel->setStyleSheet("QLabel { color: orange; }");

    qDebug() << "ControlPanel: 请求加载地图 lat=" << lat
             << " lon=" << lon << " zoom=" << zoom;

    emit loadMapRequested(lat, lon, zoom);
}

void ControlPanel::setMapLoadProgress(int done, int total)
{
    if (done < total) {
        m_mapProgressLabel->setText(QString("加载中 %1/%2...").arg(done).arg(total));
        m_mapProgressLabel->setStyleSheet("QLabel { color: orange; }");
    } else {
        m_mapProgressLabel->setText(QString("加载完成 %1 片").arg(total));
        m_mapProgressLabel->setStyleSheet("QLabel { color: green; }");
        m_loadMapBtn->setEnabled(true);
    }
}
