#pragma once

#include <QString>
#include <QList>
#include <QPair>

struct SysData {
    QString user;
    QString host;
    QList<QPair<QString, QString>> fields;

    // Raw numeric values for progress bars in the GUI
    long memUsedMiB  = 0;
    long memTotalMiB = 0;
    long diskUsedGiB = 0;
    long diskTotalGiB = 0;
    int  batteryPct  = -1; // -1 = no battery
};

class SysInfo {
public:
    static SysData fetch();
};
