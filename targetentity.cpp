#include "TargetEntity.h"
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QExtrudedTextMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <QFont>
#include <QDebug>
#include <QtMath>

// 雷达风格颜色：主色(青绿)、环、高度针、标签
static const QColor s_radarBlipColor(0, 255, 200);
static const QColor s_radarRingColor(100, 255, 220);
static const QColor s_radarPinColor(80, 220, 180);
static const QColor s_radarLabelColor(255, 255, 255);

static const QVector3D s_defaultHeading(1.0f, 0.0f, 0.0f);  // 默认航向 X+
static const float s_pinLength = 0.32f;

TargetEntity::TargetEntity(Qt3DCore::QEntity *parent) :
    Qt3DCore::QEntity(parent),
    m_position(0, 0, 0),
    m_heading(s_defaultHeading),
    m_visible(false),
    m_id(0),
    m_transform(nullptr),
    m_pinTransform(nullptr),
    m_textEntity(nullptr),
    m_textMesh(nullptr)
{
    m_batchNumber = QStringLiteral("TGT-0");
    createTarget();
}

void TargetEntity::createTarget()
{
    m_transform = new Qt3DCore::QTransform();
    this->addComponent(m_transform);

    // ---- 1. 高度针：XZ 平面、球后、与航线重合（圆柱轴沿航向反方向）----
    Qt3DCore::QEntity *pinEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QCylinderMesh *pinMesh = new Qt3DExtras::QCylinderMesh();
    pinMesh->setRadius(0.018f);
    pinMesh->setLength(s_pinLength);

    Qt3DExtras::QPhongMaterial *pinMaterial = new Qt3DExtras::QPhongMaterial();
    pinMaterial->setDiffuse(s_radarPinColor);
    pinMaterial->setSpecular(QColor(180, 255, 220));
    pinMaterial->setShininess(6.0f);
    pinMaterial->setAmbient(QColor(20, 80, 60));

    m_pinTransform = new Qt3DCore::QTransform();
    updatePinTransform();

    pinEntity->addComponent(pinMesh);
    pinEntity->addComponent(pinMaterial);
    pinEntity->addComponent(m_pinTransform);

    // ---- 2. 目标核心（雷达回波球体）----
    Qt3DCore::QEntity *blipEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QSphereMesh *blipMesh = new Qt3DExtras::QSphereMesh();
    blipMesh->setRadius(0.14f);
    blipMesh->setSlices(20);
    blipMesh->setRings(20);

    Qt3DExtras::QPhongMaterial *blipMaterial = new Qt3DExtras::QPhongMaterial();
    blipMaterial->setDiffuse(s_radarBlipColor);
    blipMaterial->setSpecular(QColor(200, 255, 240));
    blipMaterial->setShininess(12.0f);
    blipMaterial->setAmbient(QColor(30, 100, 80));

    Qt3DCore::QTransform *blipTransform = new Qt3DCore::QTransform();
    blipTransform->setTranslation(QVector3D(0, 0, 0));

    blipEntity->addComponent(blipMesh);
    blipEntity->addComponent(blipMaterial);
    blipEntity->addComponent(blipTransform);

    // ---- 3. 跟踪环（水平环，增强“在跟踪”的视觉效果）----
    Qt3DCore::QEntity *ringEntity = new Qt3DCore::QEntity(this);
    Qt3DExtras::QTorusMesh *ringMesh = new Qt3DExtras::QTorusMesh();
    ringMesh->setRadius(0.22f);
    ringMesh->setMinorRadius(0.02f);
    ringMesh->setRings(24);
    ringMesh->setSlices(24);

    Qt3DExtras::QPhongMaterial *ringMaterial = new Qt3DExtras::QPhongMaterial();
    ringMaterial->setDiffuse(s_radarRingColor);
    ringMaterial->setSpecular(QColor(180, 255, 240));
    ringMaterial->setShininess(8.0f);
    ringMaterial->setAmbient(QColor(20, 90, 70));

    Qt3DCore::QTransform *ringTransform = new Qt3DCore::QTransform();
    // 不旋转，Torus 默认在 XY 平面（XY 水平）

    ringEntity->addComponent(ringMesh);
    ringEntity->addComponent(ringMaterial);
    ringEntity->addComponent(ringTransform);

    // ---- 4. 目标编号标签（TGT-N，战术风格）----
    m_textEntity = new Qt3DCore::QEntity(this);
    m_textMesh = new Qt3DExtras::QExtrudedTextMesh(m_textEntity);
    m_textMesh->setText(m_batchNumber);
    QFont font(QStringLiteral("Consolas"), 52);
    font.setBold(true);
    m_textMesh->setFont(font);
    m_textMesh->setDepth(0.05f);

    Qt3DExtras::QPhongMaterial *textMaterial = new Qt3DExtras::QPhongMaterial();
    textMaterial->setDiffuse(s_radarLabelColor);
    textMaterial->setSpecular(QColor(220, 220, 220));
    textMaterial->setShininess(6.0f);
    textMaterial->setAmbient(QColor(100, 100, 100));

    Qt3DCore::QTransform *textTransform = new Qt3DCore::QTransform();
    textTransform->setTranslation(QVector3D(0.22f, 0.5f, 0.1f));
    textTransform->setScale(2.0f);
    textTransform->setRotation(QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 180.0f));

    m_textEntity->addComponent(m_textMesh);
    m_textEntity->addComponent(textMaterial);
    m_textEntity->addComponent(textTransform);
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

QString TargetEntity::batchNumber() const
{
    return m_batchNumber;
}

void TargetEntity::updateBatchNumberText()
{
    if (m_textMesh) {
        m_textMesh->setText(m_batchNumber.isEmpty() ? QStringLiteral("TGT-0") : m_batchNumber);
    }
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
        setBatchNumber(QStringLiteral("TGT-%1").arg(id));
        qDebug() << "目标编号设置为:" << id;
        emit idChanged(id);
    }
}

void TargetEntity::setBatchNumber(const QString &batchNumber)
{
    if (m_batchNumber != batchNumber) {
        m_batchNumber = batchNumber.isEmpty() ? QStringLiteral("TGT-0") : batchNumber;
        updateBatchNumberText();
        emit batchNumberChanged(m_batchNumber);
    }
}

void TargetEntity::updatePinTransform()
{
    if (!m_pinTransform) return;
    // 水平航向（XZ 平面），单位化
    QVector3D h(m_heading.x(), 0.0f, m_heading.z());
    if (h.lengthSquared() < 1e-10f) {
        h = QVector3D(-1.0f, 0.0f, 0.0f);  // 默认“后”为 -X
    } else {
        h.normalize();
    }
    // 球后 = 航向反方向
    QVector3D back = -h;
    // 圆柱默认沿 Y，旋转使 Y -> back（高度针在 XZ 平面，指向后方）
    float dot = qBound(-1.0f, QVector3D::dotProduct(QVector3D(0, 1, 0), back), 1.0f);
    float angleRad = qAcos(dot);
    QVector3D axis = QVector3D::crossProduct(QVector3D(0, 1, 0), back);
    if (axis.lengthSquared() < 1e-12f) {
        axis = QVector3D(1, 0, 0);
        if (dot < 0) angleRad = float(M_PI);
    } else {
        axis.normalize();
    }
    m_pinTransform->setRotation(QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angleRad)));
    // 平移：圆柱中心在 (0,0,0)，一端在 0（球心），另一端在 back * length
    m_pinTransform->setTranslation(back * (s_pinLength * 0.5f));
}

void TargetEntity::setHeading(const QVector3D &direction)
{
    QVector3D dir = direction;
    if (dir.lengthSquared() < 1e-10f) {
        dir = s_defaultHeading;
    } else {
        dir.normalize();
    }
    if (qAbs(dir.x() - m_heading.x()) < 1e-6f && qAbs(dir.y() - m_heading.y()) < 1e-6f && qAbs(dir.z() - m_heading.z()) < 1e-6f) {
        return;
    }
    m_heading = dir;
    updatePinTransform();
}
