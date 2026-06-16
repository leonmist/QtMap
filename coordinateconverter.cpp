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

double CoordinateConverter::sceneUnitsPerMeterHeight()
{
    return s_sceneUnitsPerMeterHeight;
}

QVector3D CoordinateConverter::distanceAzimuthHeightToCartesian(double distance, double azimuth, double height)
{
    // 计算水平距离
    double horizontalDistance = calculateHorizontalDistance(distance, height);

    // 将方位角转换为弧度
    double azimuthRad = degreesToRadians(azimuth);

    // 在XZ平面上计算位置，朝向与三维地形地图保持一致：+X 指向东，-Z 指向北
    // （地形网格中 sceneZ 越小纬度越高，即 -Z 为北；东为 +X）
    double x = horizontalDistance * sin(azimuthRad);   // 东方向(+X)
    double z = -horizontalDistance * cos(azimuthRad);  // 北方向(-Z)
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
    // 与正向一致：x = h*sin(az)（东=+X），z = -h*cos(az)（北=-Z）
    // 故 az = atan2(realX, -realZ)
    if (realZ != 0.0 || realX != 0.0) {
        azimuth = radiansToDegrees(atan2(realX, -realZ));
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

bool CoordinateConverter::isOutOfChina(double lat, double lon)
{
    return !(lon > 73.66 && lon < 135.05 && lat > 3.86 && lat < 53.55);
}

double CoordinateConverter::transformLatOffset(double x, double y)
{
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y
                 + 0.1 * x * y + 0.2 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * M_PI) + 20.0 * sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * M_PI) + 40.0 * sin(y / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * M_PI) + 320.0 * sin(y * M_PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

double CoordinateConverter::transformLonOffset(double x, double y)
{
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x
                 + 0.1 * x * y + 0.1 * sqrt(fabs(x));
    ret += (20.0 * sin(6.0 * x * M_PI) + 20.0 * sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * M_PI) + 40.0 * sin(x / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * M_PI) + 300.0 * sin(x / 30.0 * M_PI)) * 2.0 / 3.0;
    return ret;
}

void CoordinateConverter::wgs84ToGcj02(double wgsLat, double wgsLon,
                                       double &gcjLat, double &gcjLon)
{
    // 中国范围外（含港澳台习惯上不偏移）直接返回原始坐标
    if (isOutOfChina(wgsLat, wgsLon)) {
        gcjLat = wgsLat;
        gcjLon = wgsLon;
        return;
    }

    // 克拉索夫斯基椭球参数
    const double a  = 6378245.0;              // 长半轴
    const double ee = 0.00669342162296594323; // 第一偏心率平方

    double dLat = transformLatOffset(wgsLon - 105.0, wgsLat - 35.0);
    double dLon = transformLonOffset(wgsLon - 105.0, wgsLat - 35.0);

    double radLat = wgsLat / 180.0 * M_PI;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);

    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * M_PI);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * M_PI);

    gcjLat = wgsLat + dLat;
    gcjLon = wgsLon + dLon;
}

void CoordinateConverter::gcj02ToWgs84(double gcjLat, double gcjLon,
                                       double &wgsLat, double &wgsLon)
{
    // 中国范围外不做偏移
    if (isOutOfChina(gcjLat, gcjLon)) {
        wgsLat = gcjLat;
        wgsLon = gcjLon;
        return;
    }

    // GCJ02 加密没有解析逆变换，使用迭代逼近：
    // 不断用当前估计的 WGS84 正向加密，再用残差修正，几次即可收敛到亚米级。
    double curLat = gcjLat;
    double curLon = gcjLon;
    for (int i = 0; i < 3; ++i) {
        double testGcjLat = 0.0, testGcjLon = 0.0;
        wgs84ToGcj02(curLat, curLon, testGcjLat, testGcjLon);
        curLat += gcjLat - testGcjLat;
        curLon += gcjLon - testGcjLon;
    }

    wgsLat = curLat;
    wgsLon = curLon;
}
