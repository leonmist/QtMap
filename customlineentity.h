#ifndef CUSTOMLINEENTITY_H
#define CUSTOMLINEENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QLineWidth>
#include <QVector3D>
#include <QColor>
#include <QMutex>


class CustomLineEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(int lineId READ lineId WRITE setLineId NOTIFY lineIdChanged)
    Q_PROPERTY(QVector<QVector3D> points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)

public:
    explicit CustomLineEntity(Qt3DCore::QNode *parent = nullptr);
    CustomLineEntity(int id, const QVector<QVector3D> &points,
                    const QColor &color = Qt::white,
                    float lineWidth = 2.0f,
                    Qt3DCore::QNode *parent = nullptr);

    // Getters
    int lineId() const { return m_lineId; }
    QVector<QVector3D> points() const { return m_points; }
    QColor color() const { return m_color; }
    float lineWidth() const { return m_lineWidth; }

    // Setters
    void setLineId(int id);
    void setPoints(const QVector<QVector3D> &points);
    void setColor(const QColor &color);
    void setLineWidth(float width);

    // 工具方法
    void updateGeometry();
    void addPoint(const QVector3D &point);
    void clearPoints();
    int pointCount() const { return m_points.size(); }

signals:
    void lineIdChanged(int id);
    void pointsChanged(const QVector<QVector3D> &points);
    void colorChanged(const QColor &color);
    void lineWidthChanged(float width);
    void lineClicked(int lineId);  // 点击事件

private:
    void initializeGeometry();
    void initializeMaterial();

private:
    int m_lineId;
    QVector<QVector3D> m_points;
    QColor m_color;
    float m_lineWidth;

    // Qt3D组件
    Qt3DRender::QGeometryRenderer *m_lineRenderer;
    Qt3DRender::QGeometry *m_geometry;
    Qt3DRender::QBuffer *m_vertexBuffer;
    Qt3DRender::QBuffer *m_indexBuffer;
    Qt3DRender::QAttribute *m_positionAttribute;
    Qt3DRender::QAttribute *m_indexAttribute;
    Qt3DCore::QTransform *m_transform;
    Qt3DExtras::QPhongMaterial *m_material;
    Qt3DRender::QLineWidth *m_lineWidthState;
};

#endif // CUSTOMLINEENTITY_H
