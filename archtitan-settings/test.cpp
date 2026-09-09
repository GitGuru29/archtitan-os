#include <QString>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <iostream>

int main() {
    QString m_cpuModel;
    QFile f("/proc/cpuinfo");
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream ts(&f);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("model name")) {
                m_cpuModel = line.section(':', 1).trimmed();
                m_cpuModel.replace(QRegularExpression("\\s{2,}"), " ");
                std::cout << "CPU found: " << m_cpuModel.toStdString() << std::endl;
                break;
            }
        }
    }

    QFile f2("/proc/meminfo");
    if (f2.open(QIODevice::ReadOnly)) {
        QTextStream ts(&f2);
        long long total = 0, avail = 0;
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (line.startsWith("MemTotal:")) {
                total = line.split(QRegularExpression("\\s+"))[1].toLongLong();
            } else if (line.startsWith("MemAvailable:")) {
                avail = line.split(QRegularExpression("\\s+"))[1].toLongLong();
            }
        }
        std::cout << "Mem: total=" << total << ", avail=" << avail << std::endl;
    }
    return 0;
}
