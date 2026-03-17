#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QVector3D>
#include <QTimer>

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel();  // 添加析构函数

    // 获取目标输入值
    double getTargetDistance() const;    // 米
    double getTargetAzimuth() const;     // 度
    double getTargetHeight() const;      // 米
    int getTargetId() const;             // 编号
    QVector3D getTargetPosition() const;

signals:
    void resetCameraRequested();
    void toggleGridRequested();
    void targetChanged(int id, const QVector3D &position);  // 发送编号和位置
    void targetVisibilityChanged(bool visible);              // 所有目标可见性
    void loadMapRequested(double lat, double lon, int zoom); // 请求加载地图

public slots:
    void setInfo(const QString &text);
    void setCameraPosition(const QString &position);
    void setTarget(int id, const QVector3D &position);
    void setMapLoadProgress(int done, int total);        // 更新地图加载进度

private slots:
    void onTargetInputChanged();
    void onSetTargetClicked();
    void onClearTargetClicked();
    void onStartSimulationClicked();
    void onStopSimulationClicked();
    void onSimulationTimeout();
    void onLoadMapClicked();             // 加载地图按钮

private:
    void setupUI();
    QVector3D generateSimulationPoint(); // 新增：生成模拟点

    // UI控件 - 基本信息
    QLabel *m_infoLabel;
    QLabel *m_cameraLabel;
    QPushButton *m_resetCameraBtn;
    QPushButton *m_toggleGridBtn;

    // UI控件 - 目标输入
    QSpinBox *m_idSpinBox;
    QDoubleSpinBox *m_distanceSpinBox;
    QDoubleSpinBox *m_azimuthSpinBox;
    QDoubleSpinBox *m_heightSpinBox;

    // UI控件 - 显示坐标
    QLineEdit *m_xCoordEdit;
    QLineEdit *m_yCoordEdit;
    QLineEdit *m_zCoordEdit;

    // UI控件 - 目标控制按钮
    QPushButton *m_setTargetBtn;
    QPushButton *m_clearTargetBtn;
    QPushButton *m_showHideTargetBtn;
    QPushButton *m_startSimBtn;          // 新增：开始模拟按钮
    QPushButton *m_stopSimBtn;           // 新增：停止模拟按钮

    // UI控件 - 模拟参数
    QDoubleSpinBox *m_simRadiusSpinBox;
    QSpinBox *m_simSpeedSpinBox;

    // UI控件 - 地图加载
    QDoubleSpinBox *m_latSpinBox;
    QDoubleSpinBox *m_lonSpinBox;
    QSpinBox       *m_zoomSpinBox;
    QPushButton    *m_loadMapBtn;
    QLabel         *m_mapProgressLabel;

    bool m_targetVisible;
    QTimer *m_simulationTimer;
    int m_simulationCounter;
};

#endif // CONTROLPANEL_H
