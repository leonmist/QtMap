#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "View3D.h"
#include "ControlPanel.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void handleResetCamera();
    void handleToggleGrid();
    void updateCameraInfo(const QString &info);
    void handleLoadMap(double lat, double lon, int zoom);

private:
    void setupUI();

    View3D *m_view3D;
    ControlPanel *m_controlPanel;
};

#endif // MAINWINDOW_H
