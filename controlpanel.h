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
    void targetVisibilityChanged(bool visible);  // 所有目标可见性

public slots:
    void setInfo(const QString &text);
    void setCameraPosition(const QString &position);
    void setTarget(int id, const QVector3D &position);  // 修改：合并槽函数

private slots:
    void onTargetInputChanged();
    void onSetTargetClicked();
    void onClearTargetClicked();
    void onStartSimulationClicked();     // 新增：开始模拟
    void onStopSimulationClicked();      // 新增：停止模拟
    void onSimulationTimeout();          // 新增：模拟定时器超时

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
    QDoubleSpinBox *m_simRadiusSpinBox;  // 新增：模拟半径
    QSpinBox *m_simSpeedSpinBox;         // 新增：模拟速度

    bool m_targetVisible;
    QTimer *m_simulationTimer;           // 新增：模拟定时器
    int m_simulationCounter;             // 新增：模拟计数器
};

#endif // CONTROLPANEL_H
