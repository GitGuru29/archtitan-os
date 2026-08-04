#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include "systeminfo.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    SystemInfo info;
    
    qDebug() << "CPU Model:" << info.cpuModel();
    qDebug() << "Total RAM:" << info.totalRam();
    qDebug() << "Used RAM:" << info.usedRam();
    
    QTimer::singleShot(2500, [&]() {
        qDebug() << "Used RAM after 2.5s:" << info.usedRam();
        qDebug() << "CPU Usage after 2.5s:" << info.cpuUsage();
        app.quit();
    });
    
    return app.exec();
}
