#include <QApplication>
#include "mainwindow.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.setWindowTitle("三维显示V2.0");
    window.resize(1200, 700);
    window.show();
    qDebug() << "编译时Qt版本:" << QT_VERSION_STR;

    qDebug() << "=== 三维显示Demo ===";
    qDebug() << "程序已启动";

    return app.exec();
}
