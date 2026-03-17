#ifndef VIEW3D_H
#define VIEW3D_H

#include <QWidget>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QDirectionalLight>
#include <QMap>
#include <QVector3D>

#include "CustomLineEntity.h"
#include "TargetEntity.h"

// 目标数据，包含目标实体和轨迹线
struct TargetData {
    TargetEntity* target;        // 目标实体
    CustomLineEntity* line;      // 轨迹线
    QVector<QVector3D> path;     // 历史路径点
    bool visible;                // 是否可见
};

class View3D : public QWidget
{
    Q_OBJECT

public:
    explicit View3D(QWidget *parent = nullptr);
    ~View3D();

    QString getCameraInfo() const;
    void resetCamera();

    // 获取目标信息
    QVector3D getTargetPosition(int id) const;
    bool hasTarget(int id) const;
    QList<int> getAllTargetIds() const;

    // 清理所有目标
    void clearAllTargets();

public slots:
    void toggleGrid();
    void setTarget(int id, const QVector3D &position);
    void setTargetVisible(int id, bool visible);
    void setAllTargetsVisible(bool visible);

signals:
    void cameraChanged(const QString &info);
    void targetUpdated(int id, const QVector3D &position);  // 目标更新信号

private:
    void setupUI();
    void setupScene();
    void setupLighting();
    void createCoordinateSystem();
    void updateCameraInfo();

    // 目标管理
    void createNewTarget(int id, const QVector3D &position);
    void updateExistingTarget(int id, const QVector3D &position);
    QColor getTargetColor(int id) const;

    // 生成圆形路径点
    QVector<QVector3D> generateCirclePoints(const QVector3D& center, float radius,
                                            int segments = 36, const QVector3D& normal = QVector3D(0, 1, 0));

    Qt3DExtras::Qt3DWindow *m_view;
    QWidget *m_container;
    Qt3DCore::QEntity *m_rootEntity;
    Qt3DRender::QCamera *m_camera;
    Qt3DExtras::QOrbitCameraController *m_cameraController;

    QList<CustomLineEntity*> m_gridEntities;
    bool m_gridVisible;

    CustomLineEntity* testLine;
    CustomLineEntity* testLine2;

    // 目标管理
    QMap<int, TargetData> m_targets;  // 按ID存储目标数据
    int m_maxTargetId;                // 最大目标ID（用于颜色分配）

    void refreshRootEntity(); // 强制刷新根实体渲染上下文
    Qt3DRender::QRenderAspect* getRenderAspect(); // 获取渲染切面
    void registerEntityToPipeline(Qt3DCore::QEntity* entity); // 单独注册实体到管线
};

#endif // VIEW3D_H
