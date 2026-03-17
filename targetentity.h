#ifndef TARGETENTITY_H
#define TARGETENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <QVector3D>

class TargetEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)  // 增加编号属性

public:
    explicit TargetEntity(Qt3DCore::QEntity *parent = nullptr);

    QVector3D position() const;
    bool isVisible() const;
    int id() const;  // 获取编号

public slots:
    void setPosition(const QVector3D &position);
    void setVisible(bool visible);
    void setId(int id);  // 设置编号

signals:
    void positionChanged(const QVector3D &position);
    void visibleChanged(bool visible);
    void idChanged(int id);  // 编号变化信号

private:
    void createTarget();

    Qt3DCore::QTransform *m_transform;
    QVector3D m_position;
    bool m_visible;
    int m_id;  // 编号成员变量
};

#endif // TARGETENTITY_H
