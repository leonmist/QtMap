#include "tiffreader.h"
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QDebug>
#include <QtEndian>
#include <cmath>

TiffReader::TiffReader()
    : m_width(0)
    , m_height(0)
    , m_isValid(false)
    , m_pixelSizeX(0.0)
    , m_rotationY(0.0)
    , m_rotationX(0.0)
    , m_pixelSizeY(0.0)
    , m_originX(0.0)
    , m_originY(0.0)
{
}

TiffReader::~TiffReader()
{
}

bool TiffReader::load(const QString &tifPath, const QString &tfwPath)
{
    m_isValid = false;
    m_elevationData.clear();

    if (!readTfw(tfwPath)) {
        qWarning() << "TiffReader: 读取 TFW 伴随文件失败:" << tfwPath;
        return false;
    }

    if (!readTif(tifPath)) {
        qWarning() << "TiffReader: 读取 TIFF 高程文件失败:" << tifPath;
        return false;
    }

    m_isValid = true;
    qDebug() << "TiffReader: 成功加载高程地图，尺寸:" << m_width << "x" << m_height
             << "数据点数:" << m_elevationData.size();
    return true;
}

bool TiffReader::readTfw(const QString &tfwPath)
{
    QFile file(tfwPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    m_pixelSizeX = in.readLine().trimmed().toDouble(); // A
    m_rotationY  = in.readLine().trimmed().toDouble(); // D
    m_rotationX  = in.readLine().trimmed().toDouble(); // B
    m_pixelSizeY = in.readLine().trimmed().toDouble(); // E
    m_originX    = in.readLine().trimmed().toDouble(); // C
    m_originY    = in.readLine().trimmed().toDouble(); // F

    file.close();
    return true;
}

// 极其轻量级的 TIFF 解析器，专门用于解析 16位 (Grayscale 16) 未压缩的 DEM 高程数据。
// 避免引入 GDAL/libtiff 等庞大依赖。
bool TiffReader::readTif(const QString &tifPath)
{
    QFile file(tifPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QDataStream in(&file);
    
    // 1. 读取 TIFF 头部 (8 字节)
    char byteOrder[2];
    in.readRawData(byteOrder, 2);
    
    bool isLittleEndian = true;
    if (byteOrder[0] == 'I' && byteOrder[1] == 'I') {
        isLittleEndian = true;
        in.setByteOrder(QDataStream::LittleEndian);
    } else if (byteOrder[0] == 'M' && byteOrder[1] == 'M') {
        isLittleEndian = false;
        in.setByteOrder(QDataStream::BigEndian);
    } else {
        qWarning() << "TiffReader: 无效的 TIFF 字节序标识";
        return false;
    }

    quint16 magic;
    in >> magic;
    if (magic != 42) {
        qWarning() << "TiffReader: 无效的 TIFF 魔数";
        return false;
    }

    quint32 ifdOffset;
    in >> ifdOffset;

    // 2. 跳转到第一个图像文件目录 (IFD)
    if (!file.seek(ifdOffset)) {
        qWarning() << "TiffReader: 无法跳转到 IFD 偏移处";
        return false;
    }

    quint16 numDirectoryEntries;
    in >> numDirectoryEntries;

    quint32 stripOffset = 0;
    quint32 stripByteCount = 0;
    quint16 bitsPerSample = 0;
    quint16 sampleFormat = 1; // 默认 1 = 无符号整型

    quint32 tileWidth = 0;
    quint32 tileHeight = 0;
    quint32 tileOffsetsOffset = 0;
    quint32 tileOffsetsCount = 0;
    quint32 tileByteCountsOffset = 0;
    quint32 tileByteCountsCount = 0;
    bool isTiled = false;

    // 3. 遍历 IFD 标签
    for (int i = 0; i < numDirectoryEntries; ++i) {
        quint16 tag;
        quint16 type;
        quint32 count;
        quint32 valueOffset;

        in >> tag >> type >> count >> valueOffset;

        // 字节序处理：如果值能直接放在 4 字节的 valueOffset 中
        if (tag == 256) { // ImageWidth
            m_width = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
        } else if (tag == 257) { // ImageLength (Height)
            m_height = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
        } else if (tag == 258) { // BitsPerSample
            bitsPerSample = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
        } else if (tag == 273) { // StripOffsets
            stripOffset = valueOffset;
        } else if (tag == 279) { // StripByteCounts
            stripByteCount = valueOffset;
        } else if (tag == 322) { // TileWidth
            tileWidth = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
            isTiled = true;
        } else if (tag == 323) { // TileLength
            tileHeight = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
            isTiled = true;
        } else if (tag == 324) { // TileOffsets
            tileOffsetsOffset = valueOffset;
            tileOffsetsCount = count;
            isTiled = true;
        } else if (tag == 325) { // TileByteCounts
            tileByteCountsOffset = valueOffset;
            tileByteCountsCount = count;
            isTiled = true;
        } else if (tag == 339) { // SampleFormat (1=unsigned, 2=signed int, 3=float)
            sampleFormat = (type == 3) ? (valueOffset & 0xFFFF) : valueOffset;
        }
    }

    qDebug() << "TiffReader: 解析 TIFF 属性: 宽度=" << m_width << "高度=" << m_height
             << "BitsPerSample=" << bitsPerSample << "SampleFormat=" << sampleFormat
             << "是否瓦片式=" << isTiled;

    // 4. 验证格式是否为 16位 高程数据
    if (bitsPerSample != 16) {
        qWarning() << "TiffReader: 目前仅支持 16 位高程 TIFF 文件！当前文件为:" << bitsPerSample << "位";
        return false;
    }

    // 读取瓦片偏移和大小数组（如果是瓦片式 TIFF）
    QVector<quint32> tileOffsets;
    QVector<quint32> tileByteCounts;

    if (isTiled) {
        // 读取 TileOffsets
        if (!file.seek(tileOffsetsOffset)) {
            qWarning() << "TiffReader: 无法跳转到 TileOffsets 偏移处";
            return false;
        }
        tileOffsets.resize(tileOffsetsCount);
        for (quint32 i = 0; i < tileOffsetsCount; ++i) {
            quint32 val;
            in >> val;
            tileOffsets[i] = val;
        }

        // 读取 TileByteCounts
        if (!file.seek(tileByteCountsOffset)) {
            qWarning() << "TiffReader: 无法跳转到 TileByteCounts 偏移处";
            return false;
        }
        tileByteCounts.resize(tileByteCountsCount);
        for (quint32 i = 0; i < tileByteCountsCount; ++i) {
            quint32 val;
            in >> val;
            tileByteCounts[i] = val;
        }
    }

    // 5. 读取原始高程像素数据
    int numPixels = m_width * m_height;
    m_elevationData.resize(numPixels);
    m_elevationData.fill(0);

    if (isTiled) {
        quint32 tilesAcross = (m_width + tileWidth - 1) / tileWidth;
        quint32 tilesDown = (m_height + tileHeight - 1) / tileHeight;

        qDebug() << "TiffReader: 瓦片式 TIFF, tilesAcross=" << tilesAcross << "tilesDown=" << tilesDown;

        for (quint32 tileIndex = 0; tileIndex < tileOffsetsCount; ++tileIndex) {
            quint32 tileCol = tileIndex % tilesAcross;
            quint32 tileRow = tileIndex / tilesAcross;

            quint32 offset = tileOffsets[tileIndex];
            quint32 byteCount = tileByteCounts[tileIndex];

            if (!file.seek(offset)) {
                qWarning() << "TiffReader: 无法跳转到瓦片偏移:" << offset;
                return false;
            }

            QByteArray tileBytes = file.read(byteCount);
            if (tileBytes.size() < static_cast<int>(byteCount)) {
                qWarning() << "TiffReader: 瓦片数据不完整, 预期:" << byteCount << "实际:" << tileBytes.size();
                return false;
            }

            const quint16 *rawPtr = reinterpret_cast<const quint16*>(tileBytes.constData());
            int numTilePixels = byteCount / 2;

            for (int i = 0; i < numTilePixels; ++i) {
                quint16 val = rawPtr[i];
                if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && !isLittleEndian) ||
                    (QSysInfo::ByteOrder == QSysInfo::BigEndian && isLittleEndian)) {
                    val = qbswap(val);
                }

                int localCol = i % tileWidth;
                int localRow = i / tileWidth;

                int globalCol = tileCol * tileWidth + localCol;
                int globalRow = tileRow * tileHeight + localRow;

                if (globalCol < m_width && globalRow < m_height) {
                    int destIdx = globalRow * m_width + globalCol;

                    if (sampleFormat == 2) {
                        qint16 sVal = static_cast<qint16>(val);
                        if (sVal < -1000 || sVal > 9000) {
                            m_elevationData[destIdx] = 0;
                        } else {
                            m_elevationData[destIdx] = sVal;
                        }
                    } else {
                        if (val > 20000) {
                            m_elevationData[destIdx] = 0;
                        } else {
                            m_elevationData[destIdx] = static_cast<qint16>(val);
                        }
                    }
                }
            }
        }
    } else {
        // 传统的条带式 (Stripped) TIFF
        if (!file.seek(stripOffset)) {
            qWarning() << "TiffReader: 无法跳转到数据偏移处";
            return false;
        }

        QByteArray rawBytes = file.read(numPixels * 2);
        if (rawBytes.size() < numPixels * 2) {
            qWarning() << "TiffReader: 读取的数据不完整，预期:" << numPixels * 2 << "实际:" << rawBytes.size();
            return false;
        }

        const quint16 *rawPtr = reinterpret_cast<const quint16*>(rawBytes.constData());
        for (int i = 0; i < numPixels; ++i) {
            quint16 val = rawPtr[i];
            if ((QSysInfo::ByteOrder == QSysInfo::LittleEndian && !isLittleEndian) ||
                (QSysInfo::ByteOrder == QSysInfo::BigEndian && isLittleEndian)) {
                val = qbswap(val);
            }

            if (sampleFormat == 2) {
                qint16 sVal = static_cast<qint16>(val);
                if (sVal < -1000 || sVal > 9000) {
                    m_elevationData[i] = 0;
                } else {
                    m_elevationData[i] = sVal;
                }
            } else {
                if (val > 20000) {
                    m_elevationData[i] = 0;
                } else {
                    m_elevationData[i] = static_cast<qint16>(val);
                }
            }
        }
    }

    file.close();
    return true;
}

int TiffReader::elevationAtPixel(int col, int row) const
{
    if (!m_isValid || col < 0 || col >= m_width || row < 0 || row >= m_height) {
        return 0;
    }
    
    return m_elevationData[row * m_width + col];
}

int TiffReader::elevationAtLatLon(double lat, double lon) const
{
    QPointF pixel = latLonToPixel(lat, lon);
    return elevationAtPixel(static_cast<int>(std::round(pixel.x())),
                            static_cast<int>(std::round(pixel.y())));
}

QPointF TiffReader::latLonToPixel(double lat, double lon) const
{
    if (m_pixelSizeX == 0.0 || m_pixelSizeY == 0.0) return QPointF(0, 0);

    // 仿射变换逆矩阵计算
    // lon = m_pixelSizeX * col + m_rotationX * row + m_originX
    // lat = m_rotationY * col + m_pixelSizeY * row + m_originY
    // 假设无旋转 (m_rotationX = m_rotationY = 0)
    double col = (lon - m_originX) / m_pixelSizeX;
    double row = (lat - m_originY) / m_pixelSizeY;

    return QPointF(col, row);
}

QPointF TiffReader::pixelToLatLon(double col, double row) const
{
    double lon = m_pixelSizeX * col + m_rotationX * row + m_originX;
    double lat = m_rotationY * col + m_pixelSizeY * row + m_originY;
    return QPointF(lat, lon);
}
