#pragma once

#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <vector>

struct CpuData {
    unsigned long long user = 0;
    unsigned long long nice = 0;
    unsigned long long system = 0;
    unsigned long long idle = 0;
    unsigned long long iowait = 0;
    unsigned long long irq = 0;
    unsigned long long softirq = 0;
    unsigned long long steal = 0;

    unsigned long long getTotal() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    unsigned long long getIdle() const {
        return idle + iowait;
    }
};

class Gui final : public QWidget {
    Q_OBJECT
public:
    explicit Gui(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;

private slots:
    void updateLiveTelemetry();
    void copySysInfoToClipboard();

private:
    QPoint m_dragPos;
    QTimer *m_timer = nullptr;

    // UI elements for live updates
    QLabel *m_thmBadge = nullptr;
    QLabel *m_cpuModelLbl = nullptr;
    QLabel *m_cpuTempLbl = nullptr;
    QLabel *m_cpuUsageLbl = nullptr;
    QProgressBar *m_cpuOverallBar = nullptr;
    QLabel *m_memUsageLbl = nullptr;
    QProgressBar *m_memBar = nullptr;
    QLabel *m_swapUsageLbl = nullptr;
    QProgressBar *m_swapBar = nullptr;
    QLabel *m_diskUsageLbl = nullptr;
    QProgressBar *m_diskBar = nullptr;
    QLabel *m_uptimeLbl = nullptr;
    QPushButton *m_btnCopy = nullptr;

    std::vector<QProgressBar*> m_coreBars;
    std::vector<QLabel*> m_corePctLabels;
    std::vector<CpuData> m_prevCpuCores;
    CpuData m_prevCpuTotal;

    void initLiveCpu();
    void updateCpuUsage();
    void updateMemoryUsage();
    void updateThmStatus();
};
