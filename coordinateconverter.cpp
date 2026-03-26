#include "CoordinateConverter.h"
#include <QDebug>

// 默认值对应原始 DISBASE=1000, HGTBASE=100
double CoordinateConverter::s_sceneUnitsPerMeter       = 1.0 / DISBASE;
double CoordinateConverter::s_sceneUnitsPerMeterHeight = 1.0 / HGTBASE;

CoordinateConverter::CoordinateConverter(QObject *parent) : QObject(parent)
{
}

void CoordinateConverter::setSceneUnitsPerMeter(double s)
{
    if (s <= 0.0) {
        qWarning() << "CoordinateConverter: 无效的缩放比" << s << "，保持原值";
        return;
    }
    s_sceneUnitsPerMeter = s;
    // 高度方向按原比例关系等比放大（原来高度是水平的 DISBASE/HGTBASE = 10 倍夸大）
    //s_sceneUnitsPerMeterHeight = s * (static_cast<double>(DISBASE) / HGTBASE);
    qDebug() << "CoordinateConverter: 新缩放比 sceneUnitsPerMeter =" << s
             << " sceneUnitsPerMeterHeight =" << s_sceneUnitsPerMeterHeight;
}

double CoordinateConverter::sceneUnitsPerMeter()
{
    return s_sceneUnitsPerMeter;
}

QVector3D CoordinateConverter::distanceAzimuthHeightToCartesian(double distance, double azimuth, double height)
{
    // 计算水平距离
    double horizontalDistance = calculateHorizontalDistance(distance, height);

    // 将方位角转换为弧度
    double azimuthRad = degreesToRadians(azimuth);

    // 在XZ平面上计算位置（X指向东，Z指向北）
    double x = -horizontalDistance * sin(azimuthRad);  // 东方向
    double z = horizontalDistance * cos(azimuthRad);  // 北方向
    double y = height;                                  // 高度（Y轴向上）

    // 应用动态缩放（地图加载前使用 1/DISBASE，加载后使用瓦片像素分辨率推导的比例）
    x = x * s_sceneUnitsPerMeter;
    z = z * s_sceneUnitsPerMeter;
    y = y * s_sceneUnitsPerMeterHeight;

    return QVector3D(x, y, z);
}

void CoordinateConverter::cartesianToDistanceAzimuthHeight(const QVector3D &point,
                                                          double &distance,
                                                          double &azimuth,
                                                          double &height)
{
    // 将显示坐标转换为实际坐标（反缩放）
    double realX = (s_sceneUnitsPerMeter > 0) ? point.x() / s_sceneUnitsPerMeter : point.x() * DISBASE;
    double realZ = (s_sceneUnitsPerMeter > 0) ? point.z() / s_sceneUnitsPerMeter : point.z() * DISBASE;
    double realY = (s_sceneUnitsPerMeterHeight > 0) ? point.y() / s_sceneUnitsPerMeterHeight : point.y() * HGTBASE;

    // 计算斜距（使用实际坐标）
    distance = sqrt(realX * realX + realZ * realZ + realY * realY);

    // 计算方位角（0-360度）- 使用实际坐标
    // 注意：atan2(y, x)中的y是东方向，x是北方向，所以参数是(realX, realZ)
    if (realZ != 0.0 || realX != 0.0) {
        azimuth = radiansToDegrees(atan2(-realZ, -realX));
    } else {
        azimuth = 0.0;
    }

    if (azimuth < 0) {
        azimuth += 360.0;
    }

    // 计算高度（实际坐标）
    height = realY;
}

double CoordinateConverter::calculateElevationAngle(double distance, double height)
{
    if (distance > 0.001) {
        double elevationRad = asin(height / distance);
        return radiansToDegrees(elevationRad);
    }
    return 0.0;
}

double CoordinateConverter::calculateHorizontalDistance(double distance, double height)
{
    // 计算水平距离：√(斜距² - 高度²)
    double diff = distance * distance - height * height;
    if (diff >= 0) {
        return sqrt(diff);
    } else if (diff > -0.001) {  // 处理浮点误差
        return 0.0;
    } else {
        qWarning() << "CoordinateConverter: height exceeds distance! height ="
                  << height << ", distance =" << distance;
        return 0.0;
    }
}

double CoordinateConverter::degreesToRadians(double degrees)
{
    return degrees * M_PI / 180.0;
}

double CoordinateConverter::radiansToDegrees(double radians)
{
    return radians * 180.0 / M_PI;
}

double CoordinateConverter::calculateDistance(const QVector3D &p1, const QVector3D &p2)
{
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    double dz = p2.z() - p1.z();
    return sqrt(dx * dx + dy * dy + dz * dz);
}
