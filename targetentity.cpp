#include "TargetEntity.h"
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <QDebug>

TargetEntity::TargetEntity(Qt3DCore::QEntity *parent) :
    Qt3DCore::QEntity(parent),
    m_position(0, 0, 0),
    m_visible(false),
    m_id(0),  // 默认编号为0
    m_transform(nullptr)
{
    createTarget();
//    setVisible(false);  // 初始隐藏
}

void TargetEntity::createTarget()
{
    // ==== 创建整体变换 ====
    m_transform = new Qt3DCore::QTransform();
    this->addComponent(m_transform);

    // ==== 创建目标球体 ====
    Qt3DCore::QEntity *sphereEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QSphereMesh *sphereMesh = new Qt3DExtras::QSphereMesh();
    sphereMesh->setRadius(0.1f);
    sphereMesh->setSlices(16);
    sphereMesh->setRings(16);

    Qt3DExtras::QPhongMaterial *sphereMaterial = new Qt3DExtras::QPhongMaterial();
    sphereMaterial->setDiffuse(QColor(255, 255, 0));  // 黄色
    sphereMaterial->setSpecular(QColor(255, 255, 200));
    sphereMaterial->setShininess(10.0f);

    Qt3DCore::QTransform *sphereTransform = new Qt3DCore::QTransform();
    sphereTransform->setTranslation(QVector3D(0, 0, 0));

    sphereEntity->addComponent(sphereMesh);
    sphereEntity->addComponent(sphereMaterial);
    sphereEntity->addComponent(sphereTransform);

    // ==== 创建目标十字 ====
    // X方向线（红色）
    Qt3DCore::QEntity *xLineEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QCylinderMesh *xLineMesh = new Qt3DExtras::QCylinderMesh();
    xLineMesh->setRadius(0.03f);
    xLineMesh->setLength(0.5f);

    Qt3DExtras::QPhongMaterial *xLineMaterial = new Qt3DExtras::QPhongMaterial();
    xLineMaterial->setDiffuse(QColor(125, 12, 33));  // 红色
    xLineMaterial->setSpecular(QColor(200, 200, 200));

    Qt3DCore::QTransform *xLineTransform = new Qt3DCore::QTransform();
    xLineTransform->setRotation(QQuaternion::fromAxisAndAngle(0, 1, 0, 90));  // 绕Z轴旋转90度

    xLineEntity->addComponent(xLineMesh);
    xLineEntity->addComponent(xLineMaterial);
    xLineEntity->addComponent(xLineTransform);

    // Z方向线（蓝色）
    Qt3DCore::QEntity *zLineEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QCylinderMesh *zLineMesh = new Qt3DExtras::QCylinderMesh();
    zLineMesh->setRadius(0.03f);
    zLineMesh->setLength(0.5f);

    Qt3DExtras::QPhongMaterial *zLineMaterial = new Qt3DExtras::QPhongMaterial();
    zLineMaterial->setDiffuse(QColor(0, 0, 255));  // 蓝色
    zLineMaterial->setSpecular(QColor(200, 200, 200));

    Qt3DCore::QTransform *zLineTransform = new Qt3DCore::QTransform();
    zLineTransform->setRotation(QQuaternion::fromAxisAndAngle(1, 0, 0, 90));  // 绕X轴旋转90度

    zLineEntity->addComponent(zLineMesh);
    zLineEntity->addComponent(zLineMaterial);
    zLineEntity->addComponent(zLineTransform);
}

QVector3D TargetEntity::position() const
{
    return m_position;
}

bool TargetEntity::isVisible() const
{
    return m_visible;
}

int TargetEntity::id() const
{
    return m_id;
}

void TargetEntity::setPosition(const QVector3D &position)
{
    if (m_position != position) {
        m_position = position;
        if (m_transform) {
            m_transform->setTranslation(position);
        }
        emit positionChanged(position);
    }
}

void TargetEntity::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        this->setEnabled(visible);
        emit visibleChanged(visible);
    }
}

void TargetEntity::setId(int id)
{
    if (m_id != id) {
        m_id = id;
        qDebug() << "目标编号设置为:" << id;
        emit idChanged(id);
    }
}
