#ifndef TIFFREADER_H
#define TIFFREADER_H

#include <QString>
#include <QVector>
#include <QPointF>

class TiffReader
{
public:
    TiffReader();
    ~TiffReader();

    // 加载 TIFF 文件和对应的 TFW 文件
    bool load(const QString &tifPath, const QString &tfwPath);

    // 获取高程图的宽高（像素）
    int width() const { return m_width; }
    int height() const { return m_height; }

    // 获取指定像素坐标 (col, row) 的高程值（米）
    int elevationAtPixel(int col, int row) const;

    // 获取指定经纬度 (lat, lon) 处的高程值（米）
    int elevationAtLatLon(double lat, double lon) const;

    // 经纬度转像素坐标
    QPointF latLonToPixel(double lat, double lon) const;

    // 像素坐标转经纬度
    QPointF pixelToLatLon(double col, double row) const;

    // 是否加载成功
    bool isValid() const { return m_isValid; }

private:
    bool readTfw(const QString &tfwPath);
    bool readTif(const QString &tifPath);

    int m_width;
    int m_height;
    QVector<qint16> m_elevationData; // 存储 16 位有符号高程数据
    bool m_isValid;

    // TFW 仿射变换参数
    double m_pixelSizeX; // A
    double m_rotationY;  // D
    double m_rotationX;  // B
    double m_pixelSizeY; // E
    double m_originX;    // C (左上角像素中心经度)
    double m_originY;    // F (左上角像素中心纬度)
};

#endif // TIFFREADER_H
