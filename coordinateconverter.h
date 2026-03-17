#ifndef COORDINATECONVERTER_H
#define COORDINATECONVERTER_H

#include <QObject>
#include <QVector3D>
#include <cmath>

#define DISBASE 1000
#define HGTBASE 100

class CoordinateConverter : public QObject
{
    Q_OBJECT

public:
    explicit CoordinateConverter(QObject *parent = nullptr);

    // 从距离、方位角、高度转换为三维坐标（相对于观察者）
    // 距离：斜距，单位米
    // 方位角：0-360度，正北为0度，顺时针增加
    // 高度：单位米（相对于观察者）
    static QVector3D distanceAzimuthHeightToCartesian(double distance, double azimuth, double height);

    // 从三维坐标转换为距离、方位角、高度
    static void cartesianToDistanceAzimuthHeight(const QVector3D &point,
                                                double &distance,
                                                double &azimuth,
                                                double &height);

    // 计算俯仰角（仰角/俯角）
    static double calculateElevationAngle(double distance, double height);

    // 计算水平距离
    static double calculateHorizontalDistance(double distance, double height);

    // 度数转弧度
    static double degreesToRadians(double degrees);

    // 弧度转度数
    static double radiansToDegrees(double radians);

    // 计算两点之间的距离
    static double calculateDistance(const QVector3D &p1, const QVector3D &p2);
};

#endif // COORDINATECONVERTER_H
