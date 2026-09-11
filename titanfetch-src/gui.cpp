#include "gui.h"
#include "sysinfo.h"

#include <QApplication>
#include <QClipboard>
#include <QProcess>
#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QTimer>
#include <QPushButton>
#include <QProgressBar>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <algorithm>

// ── Color palette ─────────────────────────────────────────────────────────────
namespace C {
    static const QColor bg          {0x0b, 0x0d, 0x14, 0xf6};
    static const QColor card        {0x12, 0x14, 0x22};
    static const QColor border      {0x24, 0x27, 0x3d};
    static const QColor text        {0xcd, 0xd6, 0xf4};
    static const QColor textSub     {0xa6, 0xad, 0xc8};
    static const QColor muted       {0x58, 0x5b, 0x70};
    static const QColor cyan        {0x89, 0xdc, 0xeb};
    static const QColor blue        {0x89, 0xb4, 0xfa};
    static const QColor green       {0xa6, 0xe3, 0xa1};
    static const QColor yellow      {0xf9, 0xe2, 0xaf};
    static const QColor red         {0xf3, 0x8b, 0xa8};
    static const QColor purple      {0xcb, 0xa6, 0xf7};
    static const QColor peach       {0xfa, 0xb3, 0x87};
    static const QColor sapphire    {0x74, 0xc7, 0xec};
    static const QColor teal        {0x94, 0xe2, 0xd5};
}

// ── Low-level sysfs/procfs reader ─────────────────────────────────────────────
static QString readSysLine(const char *path)
{
    FILE *f = std::fopen(path, "r");
    if (!f) return {};
    char buf[256] = {};
    if (std::fgets(buf, sizeof(buf), f)) {
        std::size_t len = std::strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
            buf[--len] = '\0';
    }
    std::fclose(f);
    return QString::fromUtf8(buf);
}

static double readCpuTemp()
{
    for (int i = 0; i < 5; ++i) {
        char path[128];
        std::snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", i);
        const QString val = readSysLine(path);
        if (!val.isEmpty()) {
            bool ok = false;
            double t = val.toDouble(&ok);
            if (ok && t > 0) return (t > 1000.0) ? (t / 1000.0) : t;
        }
    }
    return -1.0;
}

static bool parseCpuStat(CpuData &totalData, std::vector<CpuData> &coreData)
{
    FILE *f = std::fopen("/proc/stat", "r");
    if (!f) return false;

    char line[512];
    coreData.clear();

    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "cpu ", 4) == 0) {
            CpuData d {};
            std::sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                        &d.user, &d.nice, &d.system, &d.idle,
                        &d.iowait, &d.irq, &d.softirq, &d.steal);
            totalData = d;
        } else if (std::strncmp(line, "cpu", 3) == 0) {
            CpuData d {};
            int id = 0;
            std::sscanf(line, "cpu%d %llu %llu %llu %llu %llu %llu %llu %llu",
                        &id, &d.user, &d.nice, &d.system, &d.idle,
                        &d.iowait, &d.irq, &d.softirq, &d.steal);
            coreData.push_back(d);
        }
    }
    std::fclose(f);
    return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// Gui Constructor — Glassmorphic Technical HUD
// ═════════════════════════════════════════════════════════════════════════════
Gui::Gui(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(QStringLiteral("TitanFetch HUD"));
    setFixedSize(940, 630);

    const SysData d = SysInfo::fetch();

    const QString monoFont =
        QStringLiteral("'JetBrains Mono', 'Noto Mono', 'Fira Code', 'DejaVu Sans Mono', monospace");

    // Root Container
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    root->setGeometry(0, 0, 940, 630);
    root->setStyleSheet(
        QStringLiteral(
            "QWidget#root {"
            " background: rgba(12, 14, 22, 0.95);"
            " border-radius: 18px;"
            " border: 1px solid rgba(137, 180, 250, 0.3);"
            "}"
            "QLabel { font-family: %1; color: #cdd6f4; }"
        ).arg(monoFont)
    );

    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ═════════════════════════════════════════════════════════════════════════
    // HEADER BAR
    // ═════════════════════════════════════════════════════════════════════════
    auto *header = new QWidget(root);
    header->setFixedHeight(76);
    header->setObjectName(QStringLiteral("header"));
    header->setStyleSheet(
        QStringLiteral(
            "QWidget#header {"
            " background: rgba(18, 20, 34, 0.85);"
            " border-top-left-radius: 18px; border-top-right-radius: 18px;"
            " border-bottom: 1px solid #24273d;"
            "}"
        )
    );

    auto *hdrLayout = new QHBoxLayout(header);
    hdrLayout->setContentsMargins(22, 0, 22, 0);
    hdrLayout->setSpacing(14);

    // Logo Badge
    auto *logoBadge = new QLabel(header);
    logoBadge->setFixedSize(48, 48);
    {
        QPixmap px(QStringLiteral("/home/msfvenom/custom-os-build/assets/logo.png"));
        if (!px.isNull())
            logoBadge->setPixmap(px.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        else
            logoBadge->setText(QStringLiteral("AT"));
    }
    logoBadge->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    hdrLayout->addWidget(logoBadge, 0, Qt::AlignVCenter);

    // Title Block
    auto *titleW = new QWidget(header);
    titleW->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *titleVL = new QVBoxLayout(titleW);
    titleVL->setContentsMargins(0, 0, 0, 0);
    titleVL->setSpacing(2);

    auto *titleLbl = new QLabel(titleW);
    titleLbl->setTextFormat(Qt::RichText);
    titleLbl->setText(
        QStringLiteral(
            "<span style='color:#cdd6f4; font-size:21px; font-weight:bold;'>TITAN</span>"
            "<span style='color:#89dceb; font-size:21px; font-weight:bold;'>FETCH</span>"
            "<span style='color:#585b70; font-size:11px; margin-left:8px; font-weight:bold;'> SYSTEM HUD</span>"
        )
    );
    titleVL->addWidget(titleLbl);

    auto *userHostLbl = new QLabel(QStringLiteral("%1@%2").arg(d.user, d.host), titleW);
    userHostLbl->setStyleSheet(QStringLiteral("color: #a6adc8; font-size: 12px; font-weight: 500;"));
    titleVL->addWidget(userHostLbl);

    hdrLayout->addWidget(titleW, 1, Qt::AlignVCenter);

    // THM Profile Badge
    m_thmBadge = new QLabel(header);
    m_thmBadge->setFixedHeight(32);
    m_thmBadge->setStyleSheet(
        QStringLiteral(
            "background: rgba(137, 220, 235, 0.12);"
            "color: #89dceb;"
            "border: 1px solid rgba(137, 220, 235, 0.35);"
            "border-radius: 16px;"
            "padding: 0 16px;"
            "font-size: 11px;"
            "font-weight: bold;"
        )
    );
    m_thmBadge->setText(QStringLiteral("● THM: ACTIVE"));
    hdrLayout->addWidget(m_thmBadge, 0, Qt::AlignVCenter);

    // Window Controls
    auto *btnMin = new QPushButton(QStringLiteral("−"), header);
    btnMin->setFixedSize(32, 32);
    btnMin->setCursor(Qt::PointingHandCursor);
    btnMin->setStyleSheet(
        QStringLiteral(
            "QPushButton { background: #1a1c2e; color: #a6adc8; border-radius: 16px; border: 1px solid #292c45; font-size: 16px; }"
            "QPushButton:hover { background: #292c45; color: #cdd6f4; }"
        )
    );
    connect(btnMin, &QPushButton::clicked, this, &QWidget::showMinimized);
    hdrLayout->addWidget(btnMin, 0, Qt::AlignVCenter);

    auto *btnClose = new QPushButton(QStringLiteral("×"), header);
    btnClose->setFixedSize(32, 32);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setStyleSheet(
        QStringLiteral(
            "QPushButton { background: rgba(243, 139, 168, 0.15); color: #f38ba8; border-radius: 16px; border: 1px solid rgba(243, 139, 168, 0.35); font-size: 18px; font-weight: bold; }"
            "QPushButton:hover { background: #f38ba8; color: #11111b; }"
        )
    );
    connect(btnClose, &QPushButton::clicked, this, &QWidget::close);
    hdrLayout->addWidget(btnClose, 0, Qt::AlignVCenter);

    rootLayout->addWidget(header);

    // ═════════════════════════════════════════════════════════════════════════
    // BODY AREA (2 Columns)
    // ═════════════════════════════════════════════════════════════════════════
    auto *bodyW = new QWidget(root);
    bodyW->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *bodyLayout = new QHBoxLayout(bodyW);
    bodyLayout->setContentsMargins(18, 16, 18, 14);
    bodyLayout->setSpacing(16);

    // ─────────────────────────────────────────────────────────────────────────
    // LEFT COLUMN: Overview & Toolchain
    // ─────────────────────────────────────────────────────────────────────────
    auto *leftCol = new QWidget(bodyW);
    leftCol->setFixedWidth(410);
    leftCol->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *leftVL = new QVBoxLayout(leftCol);
    leftVL->setContentsMargins(0, 0, 0, 0);
    leftVL->setSpacing(12);

    // System Overview Card
    auto *sysCard = new QWidget(leftCol);
    sysCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *sysVL = new QVBoxLayout(sysCard);
    sysVL->setContentsMargins(18, 16, 18, 16);
    sysVL->setSpacing(9);

    auto *sysHdr = new QLabel(QStringLiteral("SYSTEM SPECIFICATIONS"), sysCard);
    sysHdr->setStyleSheet(
        QStringLiteral("color: #89b4fa; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; border: none;")
    );
    sysVL->addWidget(sysHdr);

    auto addSysRow = [&](const QString &icon, const QString &label, const QString &val) {
        auto *row = new QWidget(sysCard);
        row->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0, 0, 0, 0);
        hl->setSpacing(8);

        auto *lbl = new QLabel(QStringLiteral("%1 %2").arg(icon, label), row);
        lbl->setFixedWidth(120);
        lbl->setStyleSheet(QStringLiteral("color: #585b70; font-size: 12px; border: none;"));
        hl->addWidget(lbl);

        auto *valLbl = new QLabel(val, row);
        valLbl->setStyleSheet(QStringLiteral("color: #cdd6f4; font-size: 12px; font-weight: 500; border: none;"));
        valLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        hl->addWidget(valLbl, 1);

        sysVL->addWidget(row);
    };

    QString kernelVal, uptimeVal, pkgVal, shellVal, deVal, termVal;
    for (const auto &[k, v] : d.fields) {
        if (k == QLatin1String("Kernel"))   kernelVal = v;
        if (k == QLatin1String("Uptime"))   uptimeVal = v;
        if (k == QLatin1String("Packages")) pkgVal = v;
        if (k == QLatin1String("Shell"))    shellVal = v;
        if (k == QLatin1String("DE"))       deVal = v;
        if (k == QLatin1String("Terminal")) termVal = v;
    }

    addSysRow(QStringLiteral("󰣇"), QStringLiteral("OS"), QStringLiteral("ArchTitan Linux x86_64"));
    addSysRow(QStringLiteral("󰌽"), QStringLiteral("Kernel"), kernelVal.isEmpty() ? QStringLiteral("Linux") : kernelVal);
    
    // Live Uptime Row
    auto *uptRow = new QWidget(sysCard);
    uptRow->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *uptHL = new QHBoxLayout(uptRow);
    uptHL->setContentsMargins(0, 0, 0, 0);
    uptHL->setSpacing(8);
    auto *uptName = new QLabel(QStringLiteral("󱑂 Uptime"), uptRow);
    uptName->setFixedWidth(120);
    uptName->setStyleSheet(QStringLiteral("color: #585b70; font-size: 12px; border: none;"));
    uptHL->addWidget(uptName);
    m_uptimeLbl = new QLabel(uptimeVal, uptRow);
    m_uptimeLbl->setStyleSheet(QStringLiteral("color: #cdd6f4; font-size: 12px; font-weight: 500; border: none;"));
    uptHL->addWidget(m_uptimeLbl, 1);
    sysVL->addWidget(uptRow);

    addSysRow(QStringLiteral("󰏖"), QStringLiteral("Packages"), pkgVal.isEmpty() ? QStringLiteral("Pacman") : pkgVal);
    addSysRow(QStringLiteral("󰞷"), QStringLiteral("Shell"), shellVal.isEmpty() ? QStringLiteral("fish") : shellVal);
    addSysRow(QStringLiteral("󰍹"), QStringLiteral("Desktop"), deVal.isEmpty() ? QStringLiteral("Hyprland (Wayland)") : deVal);
    addSysRow(QStringLiteral("󰆍"), QStringLiteral("Terminal"), termVal.isEmpty() ? QStringLiteral("Kitty") : termVal);

    leftVL->addWidget(sysCard);

    // Toolchain Status Card
    auto *devCard = new QWidget(leftCol);
    devCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *devVL = new QVBoxLayout(devCard);
    devVL->setContentsMargins(18, 16, 18, 16);
    devVL->setSpacing(10);

    auto *devHdr = new QLabel(QStringLiteral("DEVELOPER TOOLCHAIN"), devCard);
    devHdr->setStyleSheet(
        QStringLiteral("color: #cba6f7; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; border: none;")
    );
    devVL->addWidget(devHdr);

    auto *toolGrid = new QWidget(devCard);
    toolGrid->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *gridL = new QGridLayout(toolGrid);
    gridL->setContentsMargins(0, 0, 0, 0);
    gridL->setHorizontalSpacing(10);
    gridL->setVerticalSpacing(8);

    auto checkCmd = [](const char *cmd) -> bool {
        char checkStr[64];
        std::snprintf(checkStr, sizeof(checkStr), "which %s >/dev/null 2>&1", cmd);
        return (::system(checkStr) == 0);
    };

    struct ToolInfo { QString name; bool installed; };
    std::vector<ToolInfo> tools = {
        {QStringLiteral("GCC / Clang"), checkCmd("gcc") || checkCmd("clang")},
        {QStringLiteral("Rust / Cargo"), checkCmd("cargo")},
        {QStringLiteral("Go Toolchain"), checkCmd("go")},
        {QStringLiteral("Node.js / Vite"), checkCmd("node")},
        {QStringLiteral("Python 3"), checkCmd("python3")},
        {QStringLiteral("Docker Engine"), checkCmd("docker")}
    };

    int rowIdx = 0, colIdx = 0;
    for (const auto &t : tools) {
        auto *badge = new QLabel(
            QStringLiteral("%1 %2").arg(t.installed ? QStringLiteral("●") : QStringLiteral("○"), t.name),
            toolGrid
        );
        badge->setStyleSheet(
            t.installed
                ? QStringLiteral("color: #a6e3a1; font-size: 11px; background: rgba(166, 227, 161, 0.12); border: 1px solid rgba(166, 227, 161, 0.25); border-radius: 6px; padding: 5px 8px; font-weight: 500;")
                : QStringLiteral("color: #585b70; font-size: 11px; background: rgba(88, 91, 112, 0.1); border: 1px solid rgba(88, 91, 112, 0.2); border-radius: 6px; padding: 5px 8px;")
        );
        gridL->addWidget(badge, rowIdx, colIdx);
        colIdx++;
        if (colIdx >= 2) { colIdx = 0; rowIdx++; }
    }
    devVL->addWidget(toolGrid);
    leftVL->addWidget(devCard);

    // Palette Card
    auto *palCard = new QWidget(leftCol);
    palCard->setFixedHeight(50);
    palCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *palHL = new QHBoxLayout(palCard);
    palHL->setContentsMargins(18, 0, 18, 0);
    palHL->setSpacing(8);

    auto *palLbl = new QLabel(QStringLiteral("THEME ACCENTS"), palCard);
    palLbl->setStyleSheet(QStringLiteral("color: #585b70; font-size: 10px; font-weight: bold; border: none;"));
    palHL->addWidget(palLbl);
    palHL->addStretch();

    const std::vector<QColor> colors = { C::blue, C::cyan, C::green, C::yellow, C::peach, C::purple };
    for (const auto &c : colors) {
        auto *dot = new QLabel(palCard);
        dot->setFixedSize(16, 16);
        dot->setStyleSheet(QStringLiteral("background: %1; border-radius: 8px; border: none;").arg(c.name()));
        palHL->addWidget(dot);
    }
    leftVL->addWidget(palCard);

    bodyLayout->addWidget(leftCol);

    // ─────────────────────────────────────────────────────────────────────────
    // RIGHT COLUMN: Live Telemetry HUD
    // ─────────────────────────────────────────────────────────────────────────
    auto *rightCol = new QWidget(bodyW);
    rightCol->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *rightVL = new QVBoxLayout(rightCol);
    rightVL->setContentsMargins(0, 0, 0, 0);
    rightVL->setSpacing(12);

    // Live CPU Telemetry & Core Matrix
    auto *cpuCard = new QWidget(rightCol);
    cpuCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *cpuVL = new QVBoxLayout(cpuCard);
    cpuVL->setContentsMargins(18, 16, 18, 16);
    cpuVL->setSpacing(9);

    auto *cpuTopRow = new QWidget(cpuCard);
    cpuTopRow->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *cpuTopHL = new QHBoxLayout(cpuTopRow);
    cpuTopHL->setContentsMargins(0, 0, 0, 0);

    auto *cpuHdr = new QLabel(QStringLiteral("CPU CORE MATRIX & LOAD"), cpuTopRow);
    cpuHdr->setStyleSheet(
        QStringLiteral("color: #89dceb; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; border: none;")
    );
    cpuTopHL->addWidget(cpuHdr);
    cpuTopHL->addStretch();

    m_cpuTempLbl = new QLabel(QStringLiteral("TEMP: --°C"), cpuTopRow);
    m_cpuTempLbl->setStyleSheet(QStringLiteral("color: #fab387; font-size: 11px; font-weight: bold; border: none;"));
    cpuTopHL->addWidget(m_cpuTempLbl);
    cpuVL->addWidget(cpuTopRow);

    QString cpuModelStr;
    for (const auto &[k, v] : d.fields) {
        if (k == QLatin1String("CPU")) { cpuModelStr = v; break; }
    }
    m_cpuModelLbl = new QLabel(cpuModelStr.isEmpty() ? QStringLiteral("Processor") : cpuModelStr, cpuCard);
    m_cpuModelLbl->setStyleSheet(QStringLiteral("color: #a6adc8; font-size: 12px; border: none;"));
    cpuVL->addWidget(m_cpuModelLbl);

    // Overall CPU Load
    auto *overallW = new QWidget(cpuCard);
    overallW->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *overallHL = new QHBoxLayout(overallW);
    overallHL->setContentsMargins(0, 4, 0, 0);

    m_cpuUsageLbl = new QLabel(QStringLiteral("0%"), overallW);
    m_cpuUsageLbl->setFixedWidth(42);
    m_cpuUsageLbl->setStyleSheet(QStringLiteral("color: #89dceb; font-weight: bold; font-size: 12px; border: none;"));
    overallHL->addWidget(m_cpuUsageLbl);

    m_cpuOverallBar = new QProgressBar(overallW);
    m_cpuOverallBar->setFixedHeight(8);
    m_cpuOverallBar->setTextVisible(false);
    m_cpuOverallBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar { background: #1a1c2e; border-radius: 4px; border: none; }"
            "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #89b4fa, stop:1 #89dceb); border-radius: 4px; }"
        )
    );
    m_cpuOverallBar->setRange(0, 100);
    m_cpuOverallBar->setValue(0);
    overallHL->addWidget(m_cpuOverallBar, 1);
    cpuVL->addWidget(overallW);

    // Per-Core Grid
    auto *coreGridW = new QWidget(cpuCard);
    coreGridW->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *coreGrid = new QGridLayout(coreGridW);
    coreGrid->setContentsMargins(0, 6, 0, 0);
    coreGrid->setHorizontalSpacing(14);
    coreGrid->setVerticalSpacing(6);

    initLiveCpu();
    int cRow = 0, cCol = 0;
    for (std::size_t i = 0; i < m_coreBars.size() && i < 16; ++i) {
        auto *cW = new QWidget(coreGridW);
        cW->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
        auto *chl = new QHBoxLayout(cW);
        chl->setContentsMargins(0, 0, 0, 0);
        chl->setSpacing(6);

        auto *cLbl = new QLabel(QStringLiteral("C%1").arg(i), cW);
        cLbl->setFixedWidth(22);
        cLbl->setStyleSheet(QStringLiteral("color: #585b70; font-size: 10px; font-weight: bold; border: none;"));
        chl->addWidget(cLbl);

        chl->addWidget(m_coreBars[i], 1);

        if (i < m_corePctLabels.size()) {
            chl->addWidget(m_corePctLabels[i]);
        }

        coreGrid->addWidget(cW, cRow, cCol);
        cCol++;
        if (cCol >= 2) { cCol = 0; cRow++; }
    }
    cpuVL->addWidget(coreGridW);
    rightVL->addWidget(cpuCard);

    // Memory & Swap Card
    auto *memCard = new QWidget(rightCol);
    memCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *memVL = new QVBoxLayout(memCard);
    memVL->setContentsMargins(18, 16, 18, 16);
    memVL->setSpacing(8);

    auto *memHdr = new QLabel(QStringLiteral("MEMORY & SWAP GAUGES"), memCard);
    memHdr->setStyleSheet(
        QStringLiteral("color: #a6e3a1; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; border: none;")
    );
    memVL->addWidget(memHdr);

    // RAM Row
    auto *memRow = new QWidget(memCard);
    memRow->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *memHL = new QHBoxLayout(memRow);
    memHL->setContentsMargins(0, 0, 0, 0);

    auto *memTitle = new QLabel(QStringLiteral("RAM"), memRow);
    memTitle->setFixedWidth(50);
    memTitle->setStyleSheet(QStringLiteral("color: #a6adc8; font-size: 12px; font-weight: bold; border: none;"));
    memHL->addWidget(memTitle);

    m_memUsageLbl = new QLabel(QStringLiteral("%1 MiB / %2 MiB").arg(d.memUsedMiB).arg(d.memTotalMiB), memRow);
    m_memUsageLbl->setStyleSheet(QStringLiteral("color: #cdd6f4; font-size: 11px; border: none;"));
    memHL->addWidget(m_memUsageLbl, 1, Qt::AlignRight);
    memVL->addWidget(memRow);

    m_memBar = new QProgressBar(memCard);
    m_memBar->setFixedHeight(8);
    m_memBar->setTextVisible(false);
    m_memBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar { background: #1a1c2e; border-radius: 4px; border: none; }"
            "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a6e3a1, stop:1 #89dceb); border-radius: 4px; }"
        )
    );
    m_memBar->setRange(0, 100);
    int memPct = (d.memTotalMiB > 0) ? static_cast<int>((d.memUsedMiB * 100) / d.memTotalMiB) : 0;
    m_memBar->setValue(memPct);
    memVL->addWidget(m_memBar);

    // Swap Row
    auto *swapRow = new QWidget(memCard);
    swapRow->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *swapHL = new QHBoxLayout(swapRow);
    swapHL->setContentsMargins(0, 4, 0, 0);

    auto *swapTitle = new QLabel(QStringLiteral("SWAP"), swapRow);
    swapTitle->setFixedWidth(50);
    swapTitle->setStyleSheet(QStringLiteral("color: #a6adc8; font-size: 12px; font-weight: bold; border: none;"));
    swapHL->addWidget(swapTitle);

    m_swapUsageLbl = new QLabel(QStringLiteral("0 MiB / 0 MiB"), swapRow);
    m_swapUsageLbl->setStyleSheet(QStringLiteral("color: #cdd6f4; font-size: 11px; border: none;"));
    swapHL->addWidget(m_swapUsageLbl, 1, Qt::AlignRight);
    memVL->addWidget(swapRow);

    m_swapBar = new QProgressBar(memCard);
    m_swapBar->setFixedHeight(8);
    m_swapBar->setTextVisible(false);
    m_swapBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar { background: #1a1c2e; border-radius: 4px; border: none; }"
            "QProgressBar::chunk { background: #cba6f7; border-radius: 4px; }"
        )
    );
    m_swapBar->setRange(0, 100);
    m_swapBar->setValue(0);
    memVL->addWidget(m_swapBar);

    rightVL->addWidget(memCard);

    // Storage & Network
    auto *diskCard = new QWidget(rightCol);
    diskCard->setStyleSheet(
        QStringLiteral("QWidget { background: #121422; border-radius: 14px; border: 1px solid #24273d; }")
    );
    auto *diskVL = new QVBoxLayout(diskCard);
    diskVL->setContentsMargins(18, 12, 18, 12);
    diskVL->setSpacing(6);

    auto *diskHdrRow = new QWidget(diskCard);
    diskHdrRow->setStyleSheet(QStringLiteral("border: none; background: transparent;"));
    auto *diskHL = new QHBoxLayout(diskHdrRow);
    diskHL->setContentsMargins(0, 0, 0, 0);

    auto *diskLbl = new QLabel(QStringLiteral("STORAGE (/) & NETWORK"), diskCard);
    diskLbl->setStyleSheet(
        QStringLiteral("color: #fab387; font-size: 11px; font-weight: bold; letter-spacing: 1.5px; border: none;")
    );
    diskHL->addWidget(diskLbl);
    diskHL->addStretch();

    m_diskUsageLbl = new QLabel(QStringLiteral("%1 GiB / %2 GiB").arg(d.diskUsedGiB).arg(d.diskTotalGiB), diskCard);
    m_diskUsageLbl->setStyleSheet(QStringLiteral("color: #cdd6f4; font-size: 11px; border: none;"));
    diskHL->addWidget(m_diskUsageLbl);
    diskVL->addWidget(diskHdrRow);

    m_diskBar = new QProgressBar(diskCard);
    m_diskBar->setFixedHeight(6);
    m_diskBar->setTextVisible(false);
    m_diskBar->setStyleSheet(
        QStringLiteral(
            "QProgressBar { background: #1a1c2e; border-radius: 3px; border: none; }"
            "QProgressBar::chunk { background: #fab387; border-radius: 3px; }"
        )
    );
    m_diskBar->setRange(0, 100);
    int diskPct = (d.diskTotalGiB > 0) ? static_cast<int>((d.diskUsedGiB * 100) / d.diskTotalGiB) : 0;
    m_diskBar->setValue(diskPct);
    diskVL->addWidget(m_diskBar);

    rightVL->addWidget(diskCard);

    // Action Footer Buttons
    auto *actRow = new QWidget(rightCol);
    actRow->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *actHL = new QHBoxLayout(actRow);
    actHL->setContentsMargins(0, 0, 0, 0);
    actHL->setSpacing(10);

    auto *btnSettings = new QPushButton(QStringLiteral("⚙ Settings"), actRow);
    btnSettings->setCursor(Qt::PointingHandCursor);
    btnSettings->setFixedHeight(38);
    btnSettings->setStyleSheet(
        QStringLiteral(
            "QPushButton { background: #181a2e; color: #89b4fa; border: 1px solid rgba(137, 180, 250, 0.35); border-radius: 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #89b4fa; color: #11111b; }"
        )
    );
    connect(btnSettings, &QPushButton::clicked, this, []() {
        QProcess::startDetached(QStringLiteral("archtitan-settings"), QStringList());
    });
    actHL->addWidget(btnSettings);

    auto *btnThm = new QPushButton(QStringLiteral("📊 THM Metrics"), actRow);
    btnThm->setCursor(Qt::PointingHandCursor);
    btnThm->setFixedHeight(38);
    btnThm->setStyleSheet(
        QStringLiteral(
            "QPushButton { background: #181a2e; color: #89dceb; border: 1px solid rgba(137, 220, 235, 0.35); border-radius: 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #89dceb; color: #11111b; }"
        )
    );
    connect(btnThm, &QPushButton::clicked, this, []() {
        QProcess::startDetached(QStringLiteral("kitty"), QStringList() << QStringLiteral("titan-hwm") << QStringLiteral("metrics"));
    });
    actHL->addWidget(btnThm);

    m_btnCopy = new QPushButton(QStringLiteral("📋 Copy Specs"), actRow);
    m_btnCopy->setCursor(Qt::PointingHandCursor);
    m_btnCopy->setFixedHeight(38);
    m_btnCopy->setStyleSheet(
        QStringLiteral(
            "QPushButton { background: #181a2e; color: #a6e3a1; border: 1px solid rgba(166, 227, 161, 0.35); border-radius: 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background: #a6e3a1; color: #11111b; }"
        )
    );
    connect(m_btnCopy, &QPushButton::clicked, this, &Gui::copySysInfoToClipboard);
    actHL->addWidget(m_btnCopy);

    rightVL->addWidget(actRow);

    bodyLayout->addWidget(rightCol, 1);
    rootLayout->addWidget(bodyW, 1);

    // Live Telemetry Polling Timer (1s)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &Gui::updateLiveTelemetry);
    m_timer->start(1000);

    updateLiveTelemetry();
}

// ═════════════════════════════════════════════════════════════════════════════
// Init Core Grid
// ═════════════════════════════════════════════════════════════════════════════
void Gui::initLiveCpu()
{
    CpuData total {};
    std::vector<CpuData> cores;
    if (!parseCpuStat(total, cores)) return;

    m_prevCpuTotal = total;
    m_prevCpuCores = cores;

    for (std::size_t i = 0; i < cores.size() && i < 16; ++i) {
        auto *bar = new QProgressBar(this);
        bar->setFixedHeight(6);
        bar->setTextVisible(false);
        bar->setStyleSheet(
            QStringLiteral(
                "QProgressBar { background: #1a1c2e; border-radius: 3px; border: none; }"
                "QProgressBar::chunk { background: #89dceb; border-radius: 3px; }"
            )
        );
        bar->setRange(0, 100);
        bar->setValue(0);
        m_coreBars.push_back(bar);

        auto *pctLbl = new QLabel(QStringLiteral("0%"), this);
        pctLbl->setFixedWidth(28);
        pctLbl->setStyleSheet(QStringLiteral("color: #a6adc8; font-size: 10px; border: none;"));
        m_corePctLabels.push_back(pctLbl);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Live Telemetry Slot
// ═════════════════════════════════════════════════════════════════════════════
void Gui::updateLiveTelemetry()
{
    updateCpuUsage();
    updateMemoryUsage();
    updateThmStatus();
}

void Gui::updateCpuUsage()
{
    CpuData currTotal {};
    std::vector<CpuData> currCores;
    if (!parseCpuStat(currTotal, currCores)) return;

    const unsigned long long totalDiff = currTotal.getTotal() - m_prevCpuTotal.getTotal();
    const unsigned long long idleDiff  = currTotal.getIdle() - m_prevCpuTotal.getIdle();
    if (totalDiff > 0) {
        const double overallPct = (1.0 - (static_cast<double>(idleDiff) / totalDiff)) * 100.0;
        int pctVal = std::clamp(static_cast<int>(overallPct), 0, 100);
        if (m_cpuUsageLbl) m_cpuUsageLbl->setText(QStringLiteral("%1%").arg(pctVal));
        if (m_cpuOverallBar) m_cpuOverallBar->setValue(pctVal);
    }
    m_prevCpuTotal = currTotal;

    for (std::size_t i = 0; i < currCores.size() && i < m_coreBars.size(); ++i) {
        if (i < m_prevCpuCores.size()) {
            const unsigned long long cTotDiff = currCores[i].getTotal() - m_prevCpuCores[i].getTotal();
            const unsigned long long cIdleDiff = currCores[i].getIdle() - m_prevCpuCores[i].getIdle();
            if (cTotDiff > 0) {
                const double corePct = (1.0 - (static_cast<double>(cIdleDiff) / cTotDiff)) * 100.0;
                int cVal = std::clamp(static_cast<int>(corePct), 0, 100);
                m_coreBars[i]->setValue(cVal);
                if (i < m_corePctLabels.size()) {
                    m_corePctLabels[i]->setText(QStringLiteral("%1%").arg(cVal));
                }
            }
        }
    }
    m_prevCpuCores = currCores;

    const double temp = readCpuTemp();
    if (m_cpuTempLbl) {
        if (temp > 0)
            m_cpuTempLbl->setText(QStringLiteral("TEMP: %1°C").arg(static_cast<int>(temp)));
        else
            m_cpuTempLbl->setText(QStringLiteral("TEMP: OK"));
    }
}

void Gui::updateMemoryUsage()
{
    FILE *f = std::fopen("/proc/meminfo", "r");
    if (!f) return;

    long memTotal = 0, memAvailable = 0, swapTotal = 0, swapFree = 0;
    char line[256];
    while (std::fgets(line, sizeof(line), f)) {
        long val = 0;
        if (std::sscanf(line, "MemTotal: %ld kB", &val) == 1) memTotal = val;
        else if (std::sscanf(line, "MemAvailable: %ld kB", &val) == 1) memAvailable = val;
        else if (std::sscanf(line, "SwapTotal: %ld kB", &val) == 1) swapTotal = val;
        else if (std::sscanf(line, "SwapFree: %ld kB", &val) == 1) swapFree = val;
    }
    std::fclose(f);

    if (memTotal > 0) {
        long usedMiB  = (memTotal - memAvailable) / 1024;
        long totalMiB = memTotal / 1024;
        int pct = static_cast<int>((usedMiB * 100) / totalMiB);
        if (m_memUsageLbl) m_memUsageLbl->setText(QStringLiteral("%1 MiB / %2 MiB").arg(usedMiB).arg(totalMiB));
        if (m_memBar) m_memBar->setValue(pct);
    }

    if (swapTotal > 0) {
        long swapUsedMiB = (swapTotal - swapFree) / 1024;
        long swapTotalMiB = swapTotal / 1024;
        int swapPct = static_cast<int>((swapUsedMiB * 100) / swapTotalMiB);
        if (m_swapUsageLbl) m_swapUsageLbl->setText(QStringLiteral("%1 MiB / %2 MiB").arg(swapUsedMiB).arg(swapTotalMiB));
        if (m_swapBar) m_swapBar->setValue(swapPct);
    } else {
        if (m_swapUsageLbl) m_swapUsageLbl->setText(QStringLiteral("0 MiB (No Swap)"));
        if (m_swapBar) m_swapBar->setValue(0);
    }
}

void Gui::updateThmStatus()
{
    if (!m_thmBadge) return;
    const QString st = readSysLine("/tmp/titan_hwm_state");
    if (!st.isEmpty()) {
        const QString prof = st.toUpper();
        m_thmBadge->setText(QStringLiteral("● THM: %1").arg(prof));

        if (prof.contains(QLatin1String("SYSTEM"))) {
            m_thmBadge->setStyleSheet(
                QStringLiteral("background: rgba(203, 166, 247, 0.15); color: #cba6f7; border: 1px solid rgba(203, 166, 247, 0.35); border-radius: 16px; padding: 0 16px; font-size: 11px; font-weight: bold;")
            );
        } else if (prof.contains(QLatin1String("WEB"))) {
            m_thmBadge->setStyleSheet(
                QStringLiteral("background: rgba(137, 220, 235, 0.15); color: #89dceb; border: 1px solid rgba(137, 220, 235, 0.35); border-radius: 16px; padding: 0 16px; font-size: 11px; font-weight: bold;")
            );
        } else if (prof.contains(QLatin1String("ANDROID"))) {
            m_thmBadge->setStyleSheet(
                QStringLiteral("background: rgba(166, 227, 161, 0.15); color: #a6e3a1; border: 1px solid rgba(166, 227, 161, 0.35); border-radius: 16px; padding: 0 16px; font-size: 11px; font-weight: bold;")
            );
        } else {
            m_thmBadge->setStyleSheet(
                QStringLiteral("background: rgba(137, 180, 250, 0.15); color: #89b4fa; border: 1px solid rgba(137, 180, 250, 0.35); border-radius: 16px; padding: 0 16px; font-size: 11px; font-weight: bold;")
            );
        }
    } else {
        m_thmBadge->setText(QStringLiteral("● THM: CASUAL"));
    }
}

void Gui::copySysInfoToClipboard()
{
    const SysData d = SysInfo::fetch();
    QString out = QStringLiteral("ArchTitan OS Specifications:\n");
    out += QStringLiteral("User: %1@%2\n").arg(d.user, d.host);
    for (const auto &[k, v] : d.fields) {
        out += QStringLiteral("%1: %2\n").arg(k, v);
    }
    QClipboard *cb = QGuiApplication::clipboard();
    if (cb) cb->setText(out);

    if (m_btnCopy) {
        m_btnCopy->setText(QStringLiteral("✓ Copied!"));
        QTimer::singleShot(2000, this, [this]() {
            if (m_btnCopy) m_btnCopy->setText(QStringLiteral("📋 Copy Specs"));
        });
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// Event Handlers
// ═════════════════════════════════════════════════════════════════════════════
void Gui::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), 18, 18);
    p.setPen(QPen(QColor(0x89, 0xb4, 0xfa, 70), 1));
    p.setBrush(QColor(0x0c, 0x0e, 0x16, 246));
    p.drawPath(path);
}

void Gui::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton && ev->position().y() <= 76) {
        m_dragPos = ev->globalPosition().toPoint() - frameGeometry().topLeft();
        ev->accept();
    }
}

void Gui::mouseMoveEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton && ev->position().y() <= 76 && !m_dragPos.isNull()) {
        move(ev->globalPosition().toPoint() - m_dragPos);
        ev->accept();
    }
}
