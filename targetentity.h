#ifndef TARGETENTITY_H
#define TARGETENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QExtrudedTextMesh>
#include <QVector3D>
#include <QString>

class TargetEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(QVector3D position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString batchNumber READ batchNumber WRITE setBatchNumber NOTIFY batchNumberChanged)

public:
    explicit TargetEntity(Qt3DCore::QEntity *parent = nullptr);

    QVector3D position() const;
    bool isVisible() const;
    int id() const;
    QString batchNumber() const;

public slots:
    void setPosition(const QVector3D &position);
    void setVisible(bool visible);
    void setId(int id);
    void setBatchNumber(const QString &batchNumber);
    /** 设置航向（单位方向），高度针在 XZ 平面与航线重合、在球后 */
    void setHeading(const QVector3D &direction);

signals:
    void positionChanged(const QVector3D &position);
    void visibleChanged(bool visible);
    void idChanged(int id);
    void batchNumberChanged(const QString &batchNumber);

private:
    void createTarget();
    void updateBatchNumberText();
    void updatePinTransform();  // 根据 m_heading 更新高度针在 XZ 平面、球后、与航线重合

    Qt3DCore::QTransform *m_transform;
    QVector3D m_position;
    QVector3D m_heading;       // 航向（单位向量），默认 X+
    bool m_visible;
    int m_id;
    QString m_batchNumber;

    Qt3DCore::QTransform *m_pinTransform;  // 高度针变换
    Qt3DCore::QEntity *m_textEntity;
    Qt3DExtras::QExtrudedTextMesh *m_textMesh;
};

#endif // TARGETENTITY_H
