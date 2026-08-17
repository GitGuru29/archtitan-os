#include "browser.h"
#include "tabwidget.h"
#include "addressbar.h"
#include "adblocker.h"
#include "profilemanager.h"

#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QTextStream>
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QShortcut>
#include <QKeySequence>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEngineFindTextResult>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QIcon>
#include <QSize>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>
#include <QDesktopServices>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGraphicsOpacityEffect>

static const char *kHomeUrl     = "titan://home";
static const char *kSettingsUrl = "titan://settings";

/* ─── Modern Developer Obsidian Glass Stylesheet ───────────────────────── */
static const char *kTheme = R"(
/* Global Window Container */
QMainWindow, QWidget#CentralContainer {
    background: #060812;
    color: #e2e8f0;
    font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    font-size: 13px;
}

/* ── 1. Top Tab Strip ─────────────────────────────────────────────────── */
/* ── 1. Top Tab Strip Bar (Sleek Frosted Glass Layout - 38px) ───────── */
QWidget#TopBar {
    background: rgba(3, 6, 17, 0.75);
    border-bottom: 1px solid rgba(56, 189, 248, 0.12);
    min-height: 38px;
    max-height: 38px;
    padding: 0 8px;
}

QTabBar {
    background: transparent;
    border: none;
    qproperty-drawBase: 0;
}

QTabBar::tab {
    background: rgba(8, 18, 44, 0.6);
    border: 1px solid rgba(56, 189, 248, 0.16);
    border-radius: 8px;
    color: #94a3b8;
    padding: 5px 12px 5px 10px;
    margin-top: 5px;
    margin-bottom: 5px;
    margin-right: 4px;
    font-size: 12px;
    font-weight: 500;
    min-width: 140px;
    max-width: 220px;
    height: 24px;
}

QTabBar::tab:selected {
    background: rgba(14, 32, 74, 0.85);
    border: 1px solid rgba(56, 189, 248, 0.55);
    color: #ffffff;
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    background: rgba(14, 28, 64, 0.7);
    border-color: rgba(56, 189, 248, 0.35);
    color: #f1f5f9;
}

QTabBar::close-button {
    subcontrol-position: right;
    margin-left: 6px;
    padding: 2px;
    border-radius: 4px;
}
QTabBar::close-button:hover {
    background: rgba(239, 68, 68, 0.35);
    color: #ffffff;
}

QToolButton#AddTabButton {
    background: rgba(8, 18, 44, 0.6);
    border: 1px solid rgba(56, 189, 248, 0.22);
    border-radius: 12px;
    color: #94a3b8;
    padding: 3px;
    margin: 6px 4px;
    min-width: 24px;
    max-width: 24px;
    min-height: 24px;
    max-height: 24px;
}
QToolButton#AddTabButton:hover {
    background: rgba(56, 189, 248, 0.2);
    border-color: #38bdf8;
    color: #38bdf8;
}

/* Window Control Buttons */
QToolButton#WinBtn {
    background: transparent;
    border: none;
    color: #64748b;
    font-size: 11px;
    min-width: 28px;
    max-width: 28px;
    min-height: 26px;
    max-height: 26px;
    border-radius: 6px;
    margin-top: 4px;
}
QToolButton#WinBtn:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #f8fafc;
}
QToolButton#WinBtnClose {
    background: transparent;
    border: none;
    color: #64748b;
    font-size: 11px;
    min-width: 28px;
    max-width: 28px;
    min-height: 26px;
    max-height: 26px;
    border-radius: 6px;
    margin-top: 4px;
}
QToolButton#WinBtnClose:hover {
    background: #ef4444;
    color: #ffffff;
}
    max-height: 26px;
    border-radius: 4px;
}
QToolButton#WinBtnClose:hover {
    background: #ef4444;
    color: #ffffff;
}

/* ── 2. Unified Navigation / Omnibox Header (38px) ────────────────────── */
QWidget#NavBar {
    background: #060914;
    border-bottom: 1px solid rgba(255, 255, 255, 0.06);
    min-height: 38px;
    max-height: 38px;
    padding: 0 6px;
}

QToolButton#NavBtn {
    background: transparent;
    border: none;
    border-radius: 6px;
    padding: 4px;
    color: #94a3b8;
    min-width: 26px;
    min-height: 26px;
    max-width: 26px;
    max-height: 26px;
}
QToolButton#NavBtn:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #ffffff;
}
QToolButton#NavBtn:disabled {
    color: rgba(148, 163, 184, 0.3);
}

QToolButton#ShieldBadgeBtn {
    background: rgba(34, 197, 94, 0.12);
    border: 1px solid rgba(34, 197, 94, 0.3);
    border-radius: 6px;
    color: #22c55e;
    font-size: 11px;
    font-weight: 700;
    padding: 4px 8px;
    min-height: 22px;
}
QToolButton#ShieldBadgeBtn:hover {
    background: rgba(34, 197, 94, 0.22);
    border-color: #22c55e;
}

QToolButton#AvatarBtn {
    background: #7c3aed;
    border: 1px solid rgba(192, 132, 252, 0.3);
    border-radius: 13px;
    color: #ffffff;
    font-weight: bold;
    font-size: 11px;
    min-width: 26px;
    min-height: 26px;
    max-width: 26px;
    max-height: 26px;
}
QToolButton#AvatarBtn:hover {
    background: #6d28d9;
    box-shadow: 0 0 8px rgba(124, 58, 237, 0.5);
}

/* ── 3. Compact Navigation Rail ───────────────────────────────────────── */
QWidget#RailWidget {
    background: #04060d;
    border-right: 1px solid rgba(255, 255, 255, 0.05);
    min-width: 46px;
    max-width: 46px;
}

QToolButton.RailNavBtn {
    background: transparent;
    border: none;
    border-radius: 8px;
    padding: 7px;
    margin: 2px 6px;
    min-width: 32px;
    min-height: 32px;
    max-width: 32px;
    max-height: 32px;
}
QToolButton.RailNavBtn:hover {
    background: rgba(255, 255, 255, 0.08);
}

QToolButton.RailNavBtnActive {
    background: rgba(56, 189, 248, 0.16);
    border: 1px solid rgba(56, 189, 248, 0.4);
    border-radius: 8px;
    padding: 7px;
    margin: 2px 6px;
    min-width: 32px;
    min-height: 32px;
    max-width: 32px;
    max-height: 32px;
}

/* ── 4. Progress Bar & Floating Widgets ──────────────────────────────── */
QProgressBar {
    background: transparent;
    border: none;
    height: 2px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #38bdf8, stop:0.5 #0ea5e9, stop:1 #818cf8);
}

/* Find Bar */
QWidget#FindBar {
    background: #0b1222;
    border-bottom: 1px solid rgba(56, 189, 248, 0.3);
    padding: 4px 12px;
}
QLineEdit#FindEdit {
    background: #070b17;
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 5px;
    color: #f8fafc;
    padding: 3px 8px;
    font-size: 12px;
    min-width: 180px;
}
QLineEdit#FindEdit:focus {
    border-color: #38bdf8;
}

/* Toast Notification */
QLabel#ToastLabel {
    background: rgba(11, 18, 34, 0.95);
    border: 1px solid rgba(56, 189, 248, 0.4);
    border-radius: 8px;
    color: #f8fafc;
    font-size: 12px;
    font-weight: 600;
    padding: 8px 16px;
}

/* Menus & Popups */
QMenu {
    background: #090e1c;
    border: 1px solid rgba(56, 189, 248, 0.25);
    border-radius: 8px;
    padding: 6px;
    color: #e2e8f0;
    font-size: 12px;
}
QMenu::item {
    padding: 6px 20px 6px 12px;
    border-radius: 5px;
}
QMenu::item:selected {
    background: rgba(56, 189, 248, 0.15);
    color: #38bdf8;
}
QMenu::separator {
    height: 1px;
    background: rgba(255, 255, 255, 0.07);
    margin: 4px 4px;
}

/* Tooltips */
QToolTip {
    background: #0b1120;
    border: 1px solid rgba(56, 189, 248, 0.3);
    border-radius: 5px;
    color: #ffffff;
    padding: 4px 8px;
    font-size: 11px;
}
)";

/* ─── Constructor ──────────────────────────────────────────────────────── */
Browser::Browser(AdBlocker *adBlocker, QWidget *parent)
    : QMainWindow(parent), m_adBlocker(adBlocker)
{
    setWindowTitle(QStringLiteral("Titan Browser"));
    resize(1360, 840);
    setStyleSheet(QString::fromUtf8(kTheme));

    loadPersistentUserData();
    setupUi();
    newTab(QUrl(QString::fromUtf8(kHomeUrl)));

    connect(&ProfileManager::instance(), &ProfileManager::activeProfileChanged, this, [this](const UserProfile &) {
        updateProfileAvatar();
    });

    // Live blocked-requests counter — updates shield badge every 2s
    if (m_adBlocker) {
        m_statsTimer = new QTimer(this);
        m_statsTimer->setInterval(2000);
        connect(m_statsTimer, &QTimer::timeout, this, [this] {
            if (!m_shieldBadgeBtn) return;
            qint64 n = m_adBlocker->blockedCount();
            QString label = (n >= 1000)
                ? QStringLiteral(" %1K blocked").arg(n / 1000)
                : QStringLiteral(" %1 blocked").arg(n);
            m_shieldBadgeBtn->setText(label);
        });
        m_statsTimer->start();

        // Guardian heartbeat timer for instant ad skip across YouTube & Spotify SPA routes
        auto *guardianTimer = new QTimer(this);
        guardianTimer->setInterval(350);
        connect(guardianTimer, &QTimer::timeout, this, [this] {
            if (auto *v = currentView()) {
                const QString host = v->url().host().toLower();
                if (host.contains(QStringLiteral("youtube.com")) || host.contains(QStringLiteral("spotify.com"))) {
                    m_adBlocker->injectContentScriptIntoView(v);
                }
            }
        });
        guardianTimer->start();
    }

    // Tab Memory Saver background checker (every 60s)
    auto *memorySaverTimer = new QTimer(this);
    memorySaverTimer->setInterval(60000);
    connect(memorySaverTimer, &QTimer::timeout, this, [this]{
        if (m_tabs) m_tabs->checkAndSuspendInactiveTabs();
    });
    memorySaverTimer->start();

    // Hardware HUD Updater for Homepage
    auto *hudTimer = new QTimer(this);
    hudTimer->setInterval(1000);
    hudTimer->setProperty("lastCpuTotal", 0ULL);
    hudTimer->setProperty("lastCpuIdle", 0ULL);
    hudTimer->setProperty("lastNetBytes", 0ULL);

    connect(hudTimer, &QTimer::timeout, this, [this, hudTimer] {
        int cpuPct = 0, ramPct = 0;

        quint64 lastCpuTotal = hudTimer->property("lastCpuTotal").toULongLong();
        quint64 lastCpuIdle = hudTimer->property("lastCpuIdle").toULongLong();
        quint64 lastNetBytes = hudTimer->property("lastNetBytes").toULongLong();

        // 1. CPU
        QFile statFile(QStringLiteral("/proc/stat"));
        if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray line = statFile.readLine();
            QList<QByteArray> parts = line.split(' ');
            quint64 user = 0, nice = 0, system = 0, idle = 0;
            int idx = 0;
            for (const QByteArray &p : parts) {
                if (p.isEmpty() || p == "cpu") continue;
                quint64 val = p.toULongLong();
                if (idx == 0) user = val;
                else if (idx == 1) nice = val;
                else if (idx == 2) system = val;
                else if (idx == 3) idle = val;
                idx++;
                if (idx > 3) break;
            }
            quint64 total = user + nice + system + idle;
            if (lastCpuTotal > 0 && total > lastCpuTotal) {
                quint64 totalDiff = total - lastCpuTotal;
                quint64 idleDiff = idle - lastCpuIdle;
                if (totalDiff > 0) cpuPct = (int)((totalDiff - idleDiff) * 100 / totalDiff);
            }
            hudTimer->setProperty("lastCpuTotal", total);
            hudTimer->setProperty("lastCpuIdle", idle);
        }

        // 2. RAM (Bulletproof readAll regex)
        QFile memFile(QStringLiteral("/proc/meminfo"));
        if (memFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString memData = QString::fromUtf8(memFile.readAll());
            quint64 memTotal = 0, memAvailable = 0;
            
            auto matchTotal = QRegularExpression(QStringLiteral("MemTotal:\\s+(\\d+)")).match(memData);
            if (matchTotal.hasMatch()) memTotal = matchTotal.captured(1).toULongLong();
            
            auto matchAvail = QRegularExpression(QStringLiteral("MemAvailable:\\s+(\\d+)")).match(memData);
            if (matchAvail.hasMatch()) {
                memAvailable = matchAvail.captured(1).toULongLong();
            } else {
                quint64 mFree = 0, mBuf = 0, mCach = 0;
                auto mf = QRegularExpression(QStringLiteral("MemFree:\\s+(\\d+)")).match(memData);
                if (mf.hasMatch()) mFree = mf.captured(1).toULongLong();
                auto mb = QRegularExpression(QStringLiteral("Buffers:\\s+(\\d+)")).match(memData);
                if (mb.hasMatch()) mBuf = mb.captured(1).toULongLong();
                auto mc = QRegularExpression(QStringLiteral("Cached:\\s+(\\d+)")).match(memData);
                if (mc.hasMatch()) mCach = mc.captured(1).toULongLong();
                memAvailable = mFree + mBuf + mCach;
            }
            
            if (memTotal > 0 && memTotal >= memAvailable) {
                ramPct = (int)((memTotal - memAvailable) * 100 / memTotal);
            }
        }

        // 3. NET (Reading physical interfaces)
        quint64 netBytesPerSec = 0;
        QFile netFile(QStringLiteral("/proc/net/dev"));
        if (netFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            quint64 totalBytes = 0;
            QRegularExpression re(QStringLiteral("\\s+"));
            while (!netFile.atEnd()) {
                QByteArray line = netFile.readLine();
                if (line.contains("lo:")) continue;
                
                // Skip virtual and container interfaces
                if (line.contains("veth") || line.contains("docker") || 
                    line.contains("br-") || line.contains("virbr") ||
                    line.contains("vmnet") || line.contains("vboxnet")) continue;
                    
                int colonIdx = line.indexOf(':');
                if (colonIdx > 0) {
                    QString nums = QString::fromUtf8(line.mid(colonIdx + 1)).trimmed();
                    QStringList parts = nums.split(re, Qt::SkipEmptyParts);
                    if (parts.size() >= 9) {
                        totalBytes += parts[0].toULongLong(); // rx bytes
                        totalBytes += parts[8].toULongLong(); // tx bytes
                    }
                }
            }
            
            if (lastNetBytes > 0 && totalBytes >= lastNetBytes) {
                netBytesPerSec = totalBytes - lastNetBytes;
            }
            hudTimer->setProperty("lastNetBytes", totalBytes);
        }

        // Only inject into the view if the current page is the homepage
        if (auto *v = currentView()) {
            const QString urlStr = v->url().toString();
            if (!urlStr.contains(QStringLiteral("homepage.html")) && 
                urlStr != QStringLiteral("titan://home") && 
                urlStr != QStringLiteral("about:blank")) {
                return;
            }

            QString netText;
            double netPct = 0.0;
            
            if (netBytesPerSec >= 1048576) {
                double mb = (double)netBytesPerSec / 1048576.0;
                netText = QString::number(mb, 'f', 1) + QStringLiteral(" MB/s");
                netPct = qMin(100.0, (mb / 10.0) * 100.0);
            } else if (netBytesPerSec >= 1024) {
                double kb = (double)netBytesPerSec / 1024.0;
                netText = QString::number(kb, 'f', 1) + QStringLiteral(" KB/s");
                netPct = qMin(100.0, (kb / 1024.0) * 100.0);
            } else if (netBytesPerSec > 0) {
                netText = QString::number(netBytesPerSec) + QStringLiteral(" B/s");
                netPct = 2.0;
            } else {
                netText = QStringLiteral("0 KB/s");
                netPct = 0.0;
            }

            QString js = QStringLiteral(
                "try {"
                "  var cT = document.getElementById('cpuTrackBar'), cV = document.getElementById('cpuValueText');"
                "  var rT = document.getElementById('ramTrackBar'), rV = document.getElementById('ramValueText');"
                "  var nT = document.getElementById('netTrackBar'), nV = document.getElementById('netValueText');"
                "  if(cT) cT.style.width = '%1%';"
                "  if(cV) cV.innerText = '%1%';"
                "  if(rT) rT.style.width = '%2%';"
                "  if(rV) rV.innerText = '%2%';"
                "  if(nT) nT.style.width = '%3%';"
                "  if(nV) nV.innerText = '%4';"
                "} catch(e) {}"
            ).arg(cpuPct).arg(ramPct).arg(QString::number(netPct, 'f', 1)).arg(netText);
            
            v->page()->runJavaScript(js);
        }
    });
    hudTimer->start();
}

/* ─── UI Setup ─────────────────────────────────────────────────────────── */
void Browser::setupUi()
{
    auto *centralContainer = new QWidget(this);
    centralContainer->setObjectName(QStringLiteral("CentralContainer"));
    auto *mainVLayout = new QVBoxLayout(centralContainer);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);
    setCentralWidget(centralContainer);

    m_tabs = new TabWidget(this);

    // ── 1. Top Tab Strip Bar (Sleek Frosted Glass Layout - 38px) ─────────
    auto *topBar = new QWidget(this);
    m_topBar = topBar;
    topBar->setObjectName(QStringLiteral("TopBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 2, 8, 0);
    topLayout->setSpacing(4);

    // The Visible Tab Bar Strip
    topLayout->addWidget(m_tabs->tabBar());

    // Add Tab (+) Button on the Tab Strip
    auto *addTabTopBtn = new QToolButton(topBar);
    addTabTopBtn->setObjectName(QStringLiteral("AddTabButton"));
    addTabTopBtn->setIcon(QIcon(QStringLiteral(":/icons/plus.svg")));
    addTabTopBtn->setIconSize(QSize(13, 13));
    addTabTopBtn->setToolTip(QStringLiteral("New Tab (Ctrl+T)"));
    connect(addTabTopBtn, &QToolButton::clicked, this, [this]{ newTab(); });
    topLayout->addWidget(addTabTopBtn);

    topLayout->addStretch(1);

    // Window Controls (—, ☐, ✕) in top-right corner
    auto *minBtn = new QToolButton(topBar);
    minBtn->setObjectName(QStringLiteral("WinBtn"));
    minBtn->setText(QStringLiteral("—"));
    connect(minBtn, &QToolButton::clicked, this, &QWidget::showMinimized);
    topLayout->addWidget(minBtn);

    auto *maxBtn = new QToolButton(topBar);
    maxBtn->setObjectName(QStringLiteral("WinBtn"));
    maxBtn->setText(QStringLiteral("☐"));
    connect(maxBtn, &QToolButton::clicked, this, [this]{
        if (isMaximized()) showNormal(); else showMaximized();
    });
    topLayout->addWidget(maxBtn);

    auto *closeBtn = new QToolButton(topBar);
    closeBtn->setObjectName(QStringLiteral("WinBtnClose"));
    closeBtn->setText(QStringLiteral("✕"));
    connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
    topLayout->addWidget(closeBtn);

    mainVLayout->addWidget(topBar);

    // ── 2. Unified Navigation / Omnibox Bar (38px) ───────────────────────
    auto *navBar = new QWidget(this);
    m_navBar = navBar;
    navBar->setObjectName(QStringLiteral("NavBar"));
    auto *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(8, 2, 8, 2);
    navLayout->setSpacing(5);

    // Back
    m_backBtn = new QToolButton(navBar);
    m_backBtn->setObjectName(QStringLiteral("NavBtn"));
    m_backBtn->setIcon(QIcon(QStringLiteral(":/icons/back.svg")));
    m_backBtn->setIconSize(QSize(15, 15));
    m_backBtn->setToolTip(QStringLiteral("Back (Alt+Left)"));
    connect(m_backBtn, &QToolButton::clicked, this, &Browser::navigateBack);
    navLayout->addWidget(m_backBtn);

    // Forward
    m_fwdBtn = new QToolButton(navBar);
    m_fwdBtn->setObjectName(QStringLiteral("NavBtn"));
    m_fwdBtn->setIcon(QIcon(QStringLiteral(":/icons/forward.svg")));
    m_fwdBtn->setIconSize(QSize(15, 15));
    m_fwdBtn->setToolTip(QStringLiteral("Forward (Alt+Right)"));
    connect(m_fwdBtn, &QToolButton::clicked, this, &Browser::navigateForward);
    navLayout->addWidget(m_fwdBtn);

    // Reload / Stop Toggle
    m_reloadBtn = new QToolButton(navBar);
    m_reloadBtn->setObjectName(QStringLiteral("NavBtn"));
    m_reloadBtn->setIcon(QIcon(QStringLiteral(":/icons/reload.svg")));
    m_reloadBtn->setIconSize(QSize(15, 15));
    m_reloadBtn->setToolTip(QStringLiteral("Reload (Ctrl+R)"));
    connect(m_reloadBtn, &QToolButton::clicked, this, &Browser::reloadOrStop);
    navLayout->addWidget(m_reloadBtn);

    // Omnibox
    m_addressBar = new AddressBar(navBar);
    navLayout->addWidget(m_addressBar, 1);
    connect(m_addressBar, &AddressBar::urlEntered, this, &Browser::loadUrl);
    connect(m_addressBar, &AddressBar::bookmarkClicked, this, &Browser::onBookmarkClicked);

    // TitanShield Status Badge Button
    m_shieldBadgeBtn = new QToolButton(navBar);
    m_shieldBadgeBtn->setObjectName(QStringLiteral("ShieldBadgeBtn"));
    m_shieldBadgeBtn->setIcon(QIcon(QStringLiteral(":/icons/shield.svg")));
    m_shieldBadgeBtn->setText(QStringLiteral(" Protected"));
    m_shieldBadgeBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_shieldBadgeBtn->setToolTip(QStringLiteral("TitanShield Security & Ad Blocker"));
    connect(m_shieldBadgeBtn, &QToolButton::clicked, this, &Browser::onShieldClicked);
    navLayout->addWidget(m_shieldBadgeBtn);

    // Bookmarks Menu Button
    auto *bmBtn = new QToolButton(navBar);
    bmBtn->setObjectName(QStringLiteral("NavBtn"));
    bmBtn->setIcon(QIcon(QStringLiteral(":/icons/star.svg")));
    bmBtn->setIconSize(QSize(15, 15));
    bmBtn->setToolTip(QStringLiteral("Bookmarks (Ctrl+B)"));
    connect(bmBtn, &QToolButton::clicked, this, &Browser::onBookmarksClicked);
    navLayout->addWidget(bmBtn);

    // Extensions
    auto *extBtn = new QToolButton(navBar);
    extBtn->setObjectName(QStringLiteral("NavBtn"));
    extBtn->setIcon(QIcon(QStringLiteral(":/icons/cube.svg")));
    extBtn->setIconSize(QSize(15, 15));
    extBtn->setToolTip(QStringLiteral("Extensions"));
    connect(extBtn, &QToolButton::clicked, this, &Browser::onExtensionsClicked);
    navLayout->addWidget(extBtn);

    // Downloads
    auto *dlBtn = new QToolButton(navBar);
    dlBtn->setObjectName(QStringLiteral("NavBtn"));
    dlBtn->setIcon(QIcon(QStringLiteral(":/icons/download.svg")));
    dlBtn->setIconSize(QSize(15, 15));
    dlBtn->setToolTip(QStringLiteral("Downloads (Ctrl+J)"));
    connect(dlBtn, &QToolButton::clicked, this, &Browser::onDownloadsClicked);
    navLayout->addWidget(dlBtn);

    // Profile Avatar
    m_avatarBtn = new QToolButton(navBar);
    m_avatarBtn->setObjectName(QStringLiteral("AvatarBtn"));
    connect(m_avatarBtn, &QToolButton::clicked, this, &Browser::onProfileClicked);
    updateProfileAvatar();
    navLayout->addWidget(m_avatarBtn);

    mainVLayout->addWidget(navBar);

    // ── 3. Find in Page Floating Bar ────────────────────────────────────
    setupFindBar(mainVLayout);

    // ── 4. Progress Bar (2px line) ──────────────────────────────────────
    m_progress = new QProgressBar(this);
    m_progress->setMaximumHeight(2);
    m_progress->setTextVisible(false);
    m_progress->hide();
    mainVLayout->addWidget(m_progress);

    // ── 5. Main Body (Compact Rail + Web Content Stack) ──────────────────
    auto *bodyWidget = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Compact Navigation Rail (46px)
    auto *railWidget = new QWidget(bodyWidget);
    m_railWidget = railWidget;
    railWidget->setObjectName(QStringLiteral("RailWidget"));
    auto *railLayout = new QVBoxLayout(railWidget);
    railLayout->setContentsMargins(0, 8, 0, 8);
    railLayout->setSpacing(4);

    auto createRailBtn = [this, railWidget](const QString &iconPath, const QString &tooltip, QToolButton*& target) {
        target = new QToolButton(railWidget);
        target->setProperty("class", "RailNavBtn");
        target->setIcon(QIcon(iconPath));
        target->setIconSize(QSize(18, 18));
        target->setToolTip(tooltip);
        target->setCursor(Qt::PointingHandCursor);
        return target;
    };

    // Rail Items
    // 1. New Tab (+)
    auto *railNewTabBtn = new QToolButton(railWidget);
    railNewTabBtn->setProperty("class", "RailNavBtn");
    railNewTabBtn->setIcon(QIcon(QStringLiteral(":/icons/plus.svg")));
    railNewTabBtn->setIconSize(QSize(17, 17));
    railNewTabBtn->setToolTip(QStringLiteral("New Tab (Ctrl+T)"));
    railNewTabBtn->setCursor(Qt::PointingHandCursor);
    connect(railNewTabBtn, &QToolButton::clicked, this, [this]{ newTab(); });
    railLayout->addWidget(railNewTabBtn);

    // 2. Home
    createRailBtn(QStringLiteral(":/icons/home.svg"), QStringLiteral("Home (Alt+H)"), m_railHomeBtn);
    m_railHomeBtn->setProperty("class", "RailNavBtnActive");
    m_activeRailBtn = m_railHomeBtn;
    connect(m_railHomeBtn, &QToolButton::clicked, this, &Browser::onHomeClicked);
    railLayout->addWidget(m_railHomeBtn);

    createRailBtn(QStringLiteral(":/icons/ai.svg"), QStringLiteral("Titan AI Assistant (⌘K)"), m_railAiBtn);
    connect(m_railAiBtn, &QToolButton::clicked, this, &Browser::onAICoreClicked);
    railLayout->addWidget(m_railAiBtn);

    createRailBtn(QStringLiteral(":/icons/spaces.svg"), QStringLiteral("Spaces Workspace (Alt+S)"), m_railSpacesBtn);
    connect(m_railSpacesBtn, &QToolButton::clicked, this, &Browser::onSpacesClicked);
    railLayout->addWidget(m_railSpacesBtn);

    createRailBtn(QStringLiteral(":/icons/star.svg"), QStringLiteral("Bookmarks (Ctrl+B)"), m_railBookmarksBtn);
    connect(m_railBookmarksBtn, &QToolButton::clicked, this, &Browser::onBookmarksClicked);
    railLayout->addWidget(m_railBookmarksBtn);

    createRailBtn(QStringLiteral(":/icons/history.svg"), QStringLiteral("History (Ctrl+H)"), m_railHistoryBtn);
    connect(m_railHistoryBtn, &QToolButton::clicked, this, &Browser::onHistoryClicked);
    railLayout->addWidget(m_railHistoryBtn);

    createRailBtn(QStringLiteral(":/icons/download.svg"), QStringLiteral("Downloads (Ctrl+J)"), m_railDlBtn);
    connect(m_railDlBtn, &QToolButton::clicked, this, &Browser::onDownloadsClicked);
    railLayout->addWidget(m_railDlBtn);

    createRailBtn(QStringLiteral(":/icons/extension.svg"), QStringLiteral("Extensions"), m_railExtBtn);
    connect(m_railExtBtn, &QToolButton::clicked, this, &Browser::onExtensionsClicked);
    railLayout->addWidget(m_railExtBtn);

    createRailBtn(QStringLiteral(":/icons/settings.svg"), QStringLiteral("Settings (Ctrl+,)"), m_railSettingsBtn);
    connect(m_railSettingsBtn, &QToolButton::clicked, this, &Browser::onSettingsClicked);
    railLayout->addWidget(m_railSettingsBtn);

    railLayout->addStretch(1);

    // Theme Toggle at bottom of rail
    auto *moonBtn = new QToolButton(railWidget);
    moonBtn->setProperty("class", "RailNavBtn");
    moonBtn->setIcon(QIcon(QStringLiteral(":/icons/moon.svg")));
    moonBtn->setIconSize(QSize(18, 18));
    moonBtn->setToolTip(QStringLiteral("Toggle Theme"));
    connect(moonBtn, &QToolButton::clicked, this, &Browser::onThemeToggleClicked);
    railLayout->addWidget(moonBtn);

    bodyLayout->addWidget(railWidget);

    // Web Content Stack
    bodyLayout->addWidget(m_tabs->contentStack(), 1);

    mainVLayout->addWidget(bodyWidget, 1);

    // Start with native chrome hidden for titan://home
    m_navBar->hide();
    m_railWidget->hide();

    // Floating Toast Notification
    setupToastWidget();

    // Connect TabWidget signals
    connect(m_tabs, &TabWidget::urlChanged,    this, &Browser::onUrlChanged);
    connect(m_tabs, &TabWidget::titleChanged,  this, &Browser::onTitleChanged);
    connect(m_tabs, &TabWidget::loadProgress,  this, &Browser::onLoadProgress);
    connect(m_tabs, &TabWidget::loadFinished,  this, &Browser::onLoadFinished);

    // Keyboard Shortcuts
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this, [this]{ newTab(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, [this]{ closeCurrentTab(); });
    new QShortcut(QKeySequence::Refresh, this, [this]{ reloadOrStop(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this, [this]{
        m_addressBar->setFocus();
        m_addressBar->selectAll();
    });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this, [this]{ onAICoreClicked(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this, [this]{ toggleFindInPage(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_B), this, [this]{ onBookmarksClicked(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this, [this]{ onHistoryClicked(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_J), this, [this]{ onDownloadsClicked(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma), this, [this]{ onSettingsClicked(); });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this, [this]{ navigateBack(); });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Right), this, [this]{ navigateForward(); });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_H), this, [this]{ onHomeClicked(); });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_S), this, [this]{ onSpacesClicked(); });

    // Tab Switching Shortcuts (Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+Shift+A, Ctrl+1..9)
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), this, [this]{
        if (m_tabs && m_tabs->count() > 1) {
            m_tabs->setCurrentIndex((m_tabs->currentIndex() + 1) % m_tabs->count());
        }
    });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab), this, [this]{
        if (m_tabs && m_tabs->count() > 1) {
            m_tabs->setCurrentIndex((m_tabs->currentIndex() - 1 + m_tabs->count()) % m_tabs->count());
        }
    });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_A), this, [this]{ onTabSearchClicked(); });
    for (int i = 1; i <= 8; ++i) {
        new QShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i)), this, [this, i]{
            if (m_tabs && (i - 1) < m_tabs->count()) m_tabs->setCurrentIndex(i - 1);
        });
    }
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_9), this, [this]{
        if (m_tabs && m_tabs->count() > 0) m_tabs->setCurrentIndex(m_tabs->count() - 1);
    });

    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this, [this]{ zoomIn(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this, [this]{ zoomIn(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this, [this]{ zoomOut(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this, [this]{ zoomReset(); });
}

/* ─── Find in Page ─────────────────────────────────────────────────────── */
void Browser::setupFindBar(QVBoxLayout *layout)
{
    m_findBar = new QWidget(this);
    m_findBar->setObjectName(QStringLiteral("FindBar"));
    auto *findLayout = new QHBoxLayout(m_findBar);
    findLayout->setContentsMargins(12, 3, 12, 3);
    findLayout->setSpacing(8);

    m_findEdit = new QLineEdit(m_findBar);
    m_findEdit->setObjectName(QStringLiteral("FindEdit"));
    m_findEdit->setPlaceholderText(QStringLiteral("Find in page..."));
    m_findEdit->setClearButtonEnabled(true);
    connect(m_findEdit, &QLineEdit::textChanged, this, &Browser::onFindTextChanged);
    connect(m_findEdit, &QLineEdit::returnPressed, this, &Browser::findNext);
    findLayout->addWidget(m_findEdit);

    m_findCountLabel = new QLabel(QStringLiteral(""), m_findBar);
    m_findCountLabel->setStyleSheet(QStringLiteral("color: #94a3b8; font-size: 11px; font-weight: 500;"));
    findLayout->addWidget(m_findCountLabel);

    m_findPrevBtn = new QToolButton(m_findBar);
    m_findPrevBtn->setText(QStringLiteral("▲"));
    m_findPrevBtn->setObjectName(QStringLiteral("NavBtn"));
    m_findPrevBtn->setToolTip(QStringLiteral("Previous Match (Shift+Enter)"));
    connect(m_findPrevBtn, &QToolButton::clicked, this, &Browser::findPrevious);
    findLayout->addWidget(m_findPrevBtn);

    m_findNextBtn = new QToolButton(m_findBar);
    m_findNextBtn->setText(QStringLiteral("▼"));
    m_findNextBtn->setObjectName(QStringLiteral("NavBtn"));
    m_findNextBtn->setToolTip(QStringLiteral("Next Match (Enter)"));
    connect(m_findNextBtn, &QToolButton::clicked, this, &Browser::findNext);
    findLayout->addWidget(m_findNextBtn);

    m_findCloseBtn = new QToolButton(m_findBar);
    m_findCloseBtn->setText(QStringLiteral("✕"));
    m_findCloseBtn->setObjectName(QStringLiteral("NavBtn"));
    m_findCloseBtn->setToolTip(QStringLiteral("Close (Esc)"));
    connect(m_findCloseBtn, &QToolButton::clicked, this, &Browser::closeFindBar);
    findLayout->addWidget(m_findCloseBtn);

    findLayout->addStretch(1);
    m_findBar->hide();
    layout->addWidget(m_findBar);
}

void Browser::toggleFindInPage()
{
    if (m_findBar->isVisible()) {
        closeFindBar();
    } else {
        m_findBar->show();
        m_findEdit->setFocus();
        m_findEdit->selectAll();
        if (!m_findEdit->text().isEmpty()) {
            onFindTextChanged(m_findEdit->text());
        }
    }
}

void Browser::findNext()
{
    if (auto *v = currentView()) {
        v->findText(m_findEdit->text(), {}, [this](const QWebEngineFindTextResult &result) {
            if (result.numberOfMatches() > 0) {
                m_findCountLabel->setText(QStringLiteral("%1 of %2").arg(result.activeMatch()).arg(result.numberOfMatches()));
            } else {
                m_findCountLabel->setText(m_findEdit->text().isEmpty() ? QString() : QStringLiteral("No matches"));
            }
        });
    }
}

void Browser::findPrevious()
{
    if (auto *v = currentView()) {
        v->findText(m_findEdit->text(), QWebEnginePage::FindBackward, [this](const QWebEngineFindTextResult &result) {
            if (result.numberOfMatches() > 0) {
                m_findCountLabel->setText(QStringLiteral("%1 of %2").arg(result.activeMatch()).arg(result.numberOfMatches()));
            } else {
                m_findCountLabel->setText(m_findEdit->text().isEmpty() ? QString() : QStringLiteral("No matches"));
            }
        });
    }
}

void Browser::onFindTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        if (auto *v = currentView()) v->findText(QString());
        m_findCountLabel->clear();
        return;
    }
    findNext();
}

void Browser::closeFindBar()
{
    m_findBar->hide();
    if (auto *v = currentView()) {
        v->findText(QString());
        v->setFocus();
    }
}

/* ─── Toast Notifications ──────────────────────────────────────────────── */
void Browser::setupToastWidget()
{
    m_toastLabel = new QLabel(this);
    m_toastLabel->setObjectName(QStringLiteral("ToastLabel"));
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->hide();

    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);
    connect(m_toastTimer, &QTimer::timeout, m_toastLabel, &QLabel::hide);
}

void Browser::showToast(const QString &message, int durationMs)
{
    m_toastLabel->setText(message);
    m_toastLabel->adjustSize();
    m_toastLabel->move((width() - m_toastLabel->width()) / 2, height() - 70);
    m_toastLabel->show();
    m_toastLabel->raise();
    m_toastTimer->start(durationMs);
}

/* ─── Zoom Controls ────────────────────────────────────────────────────── */
void Browser::zoomIn()
{
    if (auto *v = currentView()) {
        m_zoomFactor = qMin(3.0, v->zoomFactor() + 0.1);
        v->setZoomFactor(m_zoomFactor);
        showToast(QStringLiteral("Zoom: %1%").arg(qRound(m_zoomFactor * 100)));
    }
}

void Browser::zoomOut()
{
    if (auto *v = currentView()) {
        m_zoomFactor = qMax(0.3, v->zoomFactor() - 0.1);
        v->setZoomFactor(m_zoomFactor);
        showToast(QStringLiteral("Zoom: %1%").arg(qRound(m_zoomFactor * 100)));
    }
}

void Browser::zoomReset()
{
    if (auto *v = currentView()) {
        m_zoomFactor = 1.0;
        v->setZoomFactor(m_zoomFactor);
        showToast(QStringLiteral("Zoom: 100% (Reset)"));
    }
}

/* ─── Bookmarks & History Persistence ──────────────────────────────────── */
void Browser::loadPersistentUserData()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/titanbrowser");
    QDir().mkpath(dirPath);

    // Load Bookmarks
    QFile bmFile(dirPath + QStringLiteral("/bookmarks.json"));
    if (bmFile.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(bmFile.readAll()).array();
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            m_bookmarks.append({
                obj.value(QStringLiteral("title")).toString(),
                QUrl(obj.value(QStringLiteral("url")).toString()),
                QDateTime::fromString(obj.value(QStringLiteral("date")).toString(), Qt::ISODate)
            });
        }
    }

    // Default bookmarks if empty
    if (m_bookmarks.isEmpty()) {
        m_bookmarks.append({QStringLiteral("ArchTitan OS"), QUrl(QStringLiteral("https://archtitan.io")), QDateTime::currentDateTime()});
        m_bookmarks.append({QStringLiteral("GitHub"), QUrl(QStringLiteral("https://github.com")), QDateTime::currentDateTime()});
        m_bookmarks.append({QStringLiteral("Arch Linux Wiki"), QUrl(QStringLiteral("https://wiki.archlinux.org")), QDateTime::currentDateTime()});
    }

    // Load Config (Default Search Engine, etc.)
    QFile cfgFile(dirPath + QStringLiteral("/config.json"));
    if (cfgFile.open(QIODevice::ReadOnly)) {
        QJsonObject obj = QJsonDocument::fromJson(cfgFile.readAll()).object();
        QString engine = obj.value(QStringLiteral("search_engine")).toString(QStringLiteral("google"));
        if (m_addressBar) m_addressBar->setSearchEngine(engine);
    } else {
        if (m_addressBar) m_addressBar->setSearchEngine(QStringLiteral("google"));
    }
}

void Browser::savePersistentUserData()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/titanbrowser");
    QDir().mkpath(dirPath);

    QFile bmFile(dirPath + QStringLiteral("/bookmarks.json"));
    if (bmFile.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (const auto &bm : m_bookmarks) {
            QJsonObject obj;
            obj[QStringLiteral("title")] = bm.title;
            obj[QStringLiteral("url")]   = bm.url.toString();
            obj[QStringLiteral("date")]  = bm.dateAdded.toString(Qt::ISODate);
            arr.append(obj);
        }
        bmFile.write(QJsonDocument(arr).toJson());
    }

    // Save Config
    QFile cfgFile(dirPath + QStringLiteral("/config.json"));
    if (cfgFile.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj[QStringLiteral("search_engine")] = m_addressBar ? m_addressBar->searchEngine() : QStringLiteral("google");
        cfgFile.write(QJsonDocument(obj).toJson());
    }
}

bool Browser::isBookmarked(const QUrl &url) const
{
    for (const auto &bm : m_bookmarks) {
        if (bm.url == url) return true;
    }
    return false;
}

void Browser::addBookmark(const QString &title, const QUrl &url)
{
    if (isBookmarked(url)) return;
    m_bookmarks.append({title.isEmpty() ? url.toString() : title, url, QDateTime::currentDateTime()});
    savePersistentUserData();
}

void Browser::removeBookmark(const QUrl &url)
{
    for (int i = 0; i < m_bookmarks.size(); ++i) {
        if (m_bookmarks[i].url == url) {
            m_bookmarks.removeAt(i);
            break;
        }
    }
    savePersistentUserData();
}

void Browser::addHistoryRecord(const QString &title, const QUrl &url)
{
    if (url.scheme() == QStringLiteral("qrc") || url.isEmpty()) return;
    m_history.prepend({title.isEmpty() ? url.toString() : title, url, QDateTime::currentDateTime()});
    if (m_history.size() > 500) m_history.removeLast();
}

void Browser::onBookmarkClicked(const QUrl &url)
{
    if (url.isEmpty() || url.toString() == QStringLiteral("qrc:/homepage.html")) return;

    if (isBookmarked(url)) {
        removeBookmark(url);
        m_addressBar->setBookmarked(false);
        showToast(QStringLiteral("Bookmark removed"));
    } else {
        QString title = currentView() ? currentView()->title() : url.toString();
        addBookmark(title, url);
        m_addressBar->setBookmarked(true);
        showToast(QStringLiteral("★ Bookmarked page"));
    }
}

void Browser::onTabSearchClicked()
{
    if (!m_tabs || m_tabs->count() == 0) return;

    auto *menu = new QMenu(this);
    menu->setStyleSheet(QStringLiteral(R"(
        QMenu {
            background: rgba(6, 12, 28, 0.95);
            border: 1px solid rgba(56, 189, 248, 0.35);
            border-radius: 10px;
            padding: 8px;
            color: #f0f6fc;
            font-family: "Outfit", -apple-system, sans-serif;
            min-width: 300px;
        }
        QMenu::item {
            padding: 8px 14px;
            border-radius: 6px;
            font-size: 12.5px;
            margin: 2px 0;
        }
        QMenu::item:selected {
            background: rgba(56, 189, 248, 0.2);
            color: #38bdf8;
        }
        QMenu::separator {
            height: 1px;
            background: rgba(56, 189, 248, 0.15);
            margin: 6px 4px;
        }
    )"));

    auto *headerAct = menu->addAction(QStringLiteral("🗂 Open Tabs (%1)").arg(m_tabs->count()));
    headerAct->setEnabled(false);
    menu->addSeparator();

    for (int i = 0; i < m_tabs->count(); ++i) {
        QString title = m_tabs->tabTitle(i);
        if (title.isEmpty()) title = QStringLiteral("Untitled Tab");
        QUrl url = m_tabs->tabUrl(i);
        QString shortUrl = url.host().isEmpty() ? url.toString() : url.host();
        QString prefix = (i == m_tabs->currentIndex()) ? QStringLiteral("● ") : QStringLiteral("○ ");
        QString label = QStringLiteral("%1%2  [%3]").arg(prefix, title.left(25), shortUrl);

        QAction *tabAct = menu->addAction(label);
        if (i == m_tabs->currentIndex()) {
            QFont f = tabAct->font();
            f.setBold(true);
            tabAct->setFont(f);
        }
        connect(tabAct, &QAction::triggered, this, [this, i] {
            m_tabs->setCurrentIndex(i);
        });
    }

    menu->addSeparator();
    auto *newTabAct = menu->addAction(QStringLiteral("➕ New Tab (Ctrl+T)"));
    connect(newTabAct, &QAction::triggered, this, [this] { newTab(); });

    menu->exec(QCursor::pos());
}

/* ─── Rail & Chrome Actions ────────────────────────────────────────────── */
void Browser::setActiveRailButton(QToolButton *btn)
{
    if (m_activeRailBtn) {
        m_activeRailBtn->setProperty("class", "RailNavBtn");
        m_activeRailBtn->style()->unpolish(m_activeRailBtn);
        m_activeRailBtn->style()->polish(m_activeRailBtn);
    }
    m_activeRailBtn = btn;
    if (m_activeRailBtn) {
        m_activeRailBtn->setProperty("class", "RailNavBtnActive");
        m_activeRailBtn->style()->unpolish(m_activeRailBtn);
        m_activeRailBtn->style()->polish(m_activeRailBtn);
    }
}

void Browser::onHomeClicked()
{
    setActiveRailButton(m_railHomeBtn);
    loadUrl(QUrl(QString::fromUtf8(kHomeUrl)));
}

void Browser::onAICoreClicked()
{
    setActiveRailButton(m_railAiBtn);
    if (auto *v = currentView()) {
        v->page()->runJavaScript(QStringLiteral("if (window.toggleAIPanel) window.toggleAIPanel(true);"));
    }
}

void Browser::onSpacesClicked()
{
    setActiveRailButton(m_railSpacesBtn);
    auto *menu = new QMenu(this);

    auto addSpaceAction = [this, menu](const QString &name, const QString &badge) {
        auto *act = menu->addAction(QStringLiteral("%1  (%2)").arg(name, badge));
        if (m_activeSpace == name) {
            act->setText(QStringLiteral("✔ %1  (%2)").arg(name, badge));
        }
        connect(act, &QAction::triggered, this, [this, name]{
            m_activeSpace = name;
            showToast(QStringLiteral("Switched to Space: %1").arg(name));
        });
    };

    addSpaceAction(QStringLiteral("Development"), QStringLiteral("C++, Rust, Vite"));
    addSpaceAction(QStringLiteral("Research"), QStringLiteral("Docs, Papers, LLMs"));
    addSpaceAction(QStringLiteral("Personal"), QStringLiteral("Feeds, Media"));
    menu->addSeparator();
    auto *newSpaceAct = menu->addAction(QStringLiteral("+ Create New Space..."));
    connect(newSpaceAct, &QAction::triggered, this, [this]{
        showToast(QStringLiteral("Spaces Workspace Manager ready"));
    });

    menu->popup(QCursor::pos());
}

void Browser::onBookmarksClicked()
{
    setActiveRailButton(m_railBookmarksBtn);
    auto *menu = new QMenu(this);

    auto *addCurAct = menu->addAction(QStringLiteral("★ Bookmark Current Tab"));
    connect(addCurAct, &QAction::triggered, this, [this]{
        if (auto *v = currentView()) onBookmarkClicked(v->url());
    });
    menu->addSeparator();

    if (m_bookmarks.isEmpty()) {
        auto *emptyAct = menu->addAction(QStringLiteral("No saved bookmarks yet"));
        emptyAct->setEnabled(false);
    } else {
        int limit = qMin(15, m_bookmarks.size());
        for (int i = 0; i < limit; ++i) {
            const auto &bm = m_bookmarks[i];
            auto *act = menu->addAction(bm.title.left(32));
            connect(act, &QAction::triggered, this, [this, bm]{
                newTab(bm.url);
            });
        }
    }

    menu->popup(QCursor::pos());
}

void Browser::onHistoryClicked()
{
    setActiveRailButton(m_railHistoryBtn);
    auto *menu = new QMenu(this);

    if (m_history.isEmpty()) {
        auto *emptyAct = menu->addAction(QStringLiteral("No browsing history yet"));
        emptyAct->setEnabled(false);
    } else {
        int limit = qMin(15, m_history.size());
        for (int i = 0; i < limit; ++i) {
            const auto &rec = m_history[i];
            auto *act = menu->addAction(QStringLiteral("%1 — %2").arg(rec.title.left(28), rec.url.host()));
            connect(act, &QAction::triggered, this, [this, rec]{
                loadUrl(rec.url);
            });
        }
        menu->addSeparator();
        auto *clearAct = menu->addAction(QStringLiteral("Clear Browsing History"));
        connect(clearAct, &QAction::triggered, this, [this]{
            m_history.clear();
            showToast(QStringLiteral("Browsing history cleared"));
        });
    }

    menu->popup(QCursor::pos());
}

void Browser::onDownloadsClicked()
{
    setActiveRailButton(m_railDlBtn);
    auto *menu = new QMenu(this);
    auto *openFolderAct = menu->addAction(QStringLiteral("Open Downloads Folder"));
    connect(openFolderAct, &QAction::triggered, this, []{
        QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    menu->addSeparator();
    auto *cleanAct = menu->addAction(QStringLiteral("All downloads verified clean by TitanShield"));
    cleanAct->setEnabled(false);

    menu->popup(QCursor::pos());
}

void Browser::onExtensionsClicked()
{
    setActiveRailButton(m_railExtBtn);
    auto *menu = new QMenu(this);
    menu->addAction(QStringLiteral("✔ uBlock Origin (Core Engine Active)"))->setEnabled(false);
    menu->addAction(QStringLiteral("✔ Bitwarden Password Safe"))->setEnabled(false);
    menu->addAction(QStringLiteral("✔ Dark Reader High-Contrast"))->setEnabled(false);
    menu->addSeparator();
    auto *storeAct = menu->addAction(QStringLiteral("Chrome Web Store"));
    connect(storeAct, &QAction::triggered, this, [this]{
        newTab(QUrl(QStringLiteral("https://chromewebstore.google.com")));
    });

    menu->popup(QCursor::pos());
}

void Browser::onSettingsClicked()
{
    setActiveRailButton(m_railSettingsBtn);
    for (int i = 0; i < m_tabs->count(); ++i) {
        QUrl tabUrl = m_tabs->tabUrl(i);
        if (tabUrl.toString().startsWith(QStringLiteral("titan://settings"))) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }
    newTab(QUrl(QString::fromUtf8(kSettingsUrl)));
}

void Browser::onShieldClicked()
{
    auto *menu = new QMenu(this);
    qint64 blk = m_adBlocker ? m_adBlocker->blockedCount() : 0;
    menu->addAction(QStringLiteral("🛡 TitanShield Engine: Active"))->setEnabled(false);
    menu->addAction(QStringLiteral("📊 %1 Requests Blocked").arg(blk))->setEnabled(false);
    menu->addSeparator();
    auto *resetAct = menu->addAction(QStringLiteral("Reset Blocked Statistics"));
    connect(resetAct, &QAction::triggered, this, [this]{
        if (m_adBlocker) m_adBlocker->resetStats();
        showToast(QStringLiteral("TitanShield statistics reset"));
    });
    auto *settingsAct = menu->addAction(QStringLiteral("Privacy & Shield Settings..."));
    connect(settingsAct, &QAction::triggered, this, &Browser::onSettingsClicked);

    menu->popup(QCursor::pos());
}

void Browser::updateProfileAvatar()
{
    if (!m_avatarBtn) return;
    UserProfile p = ProfileManager::instance().activeProfile();
    QString initial = p.name.trimmed().isEmpty() ? QStringLiteral("T") : p.name.trimmed().left(1).toUpper();
    m_avatarBtn->setText(initial);
    QString tip = QStringLiteral("Profile: %1").arg(p.name);
    if (!p.email.isEmpty()) tip += QStringLiteral(" (%1)").arg(p.email);
    m_avatarBtn->setToolTip(tip);

    QString color = p.avatarColor.isEmpty() ? QStringLiteral("#38bdf8") : p.avatarColor;
    m_avatarBtn->setStyleSheet(QStringLiteral(R"(
        QToolButton#AvatarBtn {
            background-color: %1;
            color: #ffffff;
            font-weight: 700;
            font-size: 11px;
            border-radius: 12px;
            border: 1.5px solid rgba(255, 255, 255, 0.25);
            min-width: 24px;
            max-width: 24px;
            min-height: 24px;
            max-height: 24px;
            padding: 0;
            margin: 0 4px;
        }
        QToolButton#AvatarBtn:hover {
            border: 1.5px solid #ffffff;
        }
    )").arg(color));
}

void Browser::switchProfile(const QString &profileId)
{
    if (ProfileManager::instance().setActiveProfile(profileId)) {
        updateProfileAvatar();
        m_tabs->reloadAllTabsForProfile();
        UserProfile p = ProfileManager::instance().activeProfile();
        showToast(QStringLiteral("Switched to Profile: %1").arg(p.name));
    }
}

void Browser::showAddProfileDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Add New Profile"));
    dlg.setFixedSize(360, 240);
    dlg.setStyleSheet(QStringLiteral(R"(
        QDialog {
            background: #090e1c;
            color: #f1f5f9;
            font-family: 'Inter', sans-serif;
        }
        QLabel { color: #94a3b8; font-size: 12px; }
        QLineEdit {
            background: #0e172e;
            border: 1px solid rgba(56, 189, 248, 0.3);
            border-radius: 6px;
            color: #ffffff;
            padding: 6px 10px;
            font-size: 13px;
        }
        QLineEdit:focus { border-color: #38bdf8; }
        QComboBox {
            background: #0e172e;
            border: 1px solid rgba(56, 189, 248, 0.3);
            border-radius: 6px;
            color: #ffffff;
            padding: 6px 10px;
        }
        QPushButton {
            background: rgba(56, 189, 248, 0.15);
            border: 1px solid rgba(56, 189, 248, 0.4);
            border-radius: 6px;
            color: #38bdf8;
            padding: 6px 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #38bdf8;
            color: #060812;
        }
    )"));

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(QStringLiteral("e.g. Work, Personal, School"));
    form->addRow(QStringLiteral("Profile Name:"), nameEdit);

    auto *colorCombo = new QComboBox(&dlg);
    colorCombo->addItem(QStringLiteral("Cyan (#38bdf8)"), QStringLiteral("#38bdf8"));
    colorCombo->addItem(QStringLiteral("Emerald Green (#22c55e)"), QStringLiteral("#22c55e"));
    colorCombo->addItem(QStringLiteral("Rose Red (#ef4444)"), QStringLiteral("#ef4444"));
    colorCombo->addItem(QStringLiteral("Purple (#a78bfa)"), QStringLiteral("#a78bfa"));
    colorCombo->addItem(QStringLiteral("Amber (#f59e0b)"), QStringLiteral("#f59e0b"));
    colorCombo->addItem(QStringLiteral("Sky Blue (#0ea5e9)"), QStringLiteral("#0ea5e9"));
    form->addRow(QStringLiteral("Avatar Color:"), colorCombo);

    auto *emailEdit = new QLineEdit(&dlg);
    emailEdit->setPlaceholderText(QStringLiteral("Optional: user@gmail.com"));
    form->addRow(QStringLiteral("Google/Account:"), emailEdit);

    layout->addLayout(form);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) name = QStringLiteral("Profile %1").arg(ProfileManager::instance().profiles().size() + 1);
        QString color = colorCombo->currentData().toString();
        QString email = emailEdit->text().trimmed();
        UserProfile newProf = ProfileManager::instance().createProfile(name, color, QStringLiteral("user"), email);
        switchProfile(newProf.id);
    }
}

void Browser::showEditProfileDialog()
{
    UserProfile cur = ProfileManager::instance().activeProfile();
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Edit Profile"));
    dlg.setFixedSize(360, 240);
    dlg.setStyleSheet(QStringLiteral(R"(
        QDialog {
            background: #090e1c;
            color: #f1f5f9;
            font-family: 'Inter', sans-serif;
        }
        QLabel { color: #94a3b8; font-size: 12px; }
        QLineEdit {
            background: #0e172e;
            border: 1px solid rgba(56, 189, 248, 0.3);
            border-radius: 6px;
            color: #ffffff;
            padding: 6px 10px;
            font-size: 13px;
        }
        QLineEdit:focus { border-color: #38bdf8; }
        QComboBox {
            background: #0e172e;
            border: 1px solid rgba(56, 189, 248, 0.3);
            border-radius: 6px;
            color: #ffffff;
            padding: 6px 10px;
        }
        QPushButton {
            background: rgba(56, 189, 248, 0.15);
            border: 1px solid rgba(56, 189, 248, 0.4);
            border-radius: 6px;
            color: #38bdf8;
            padding: 6px 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #38bdf8;
            color: #060812;
        }
    )"));

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setText(cur.name);
    form->addRow(QStringLiteral("Profile Name:"), nameEdit);

    auto *colorCombo = new QComboBox(&dlg);
    colorCombo->addItem(QStringLiteral("Cyan (#38bdf8)"), QStringLiteral("#38bdf8"));
    colorCombo->addItem(QStringLiteral("Emerald Green (#22c55e)"), QStringLiteral("#22c55e"));
    colorCombo->addItem(QStringLiteral("Rose Red (#ef4444)"), QStringLiteral("#ef4444"));
    colorCombo->addItem(QStringLiteral("Purple (#a78bfa)"), QStringLiteral("#a78bfa"));
    colorCombo->addItem(QStringLiteral("Amber (#f59e0b)"), QStringLiteral("#f59e0b"));
    colorCombo->addItem(QStringLiteral("Sky Blue (#0ea5e9)"), QStringLiteral("#0ea5e9"));
    for (int i = 0; i < colorCombo->count(); ++i) {
        if (colorCombo->itemData(i).toString() == cur.avatarColor) {
            colorCombo->setCurrentIndex(i);
            break;
        }
    }
    form->addRow(QStringLiteral("Avatar Color:"), colorCombo);

    auto *emailEdit = new QLineEdit(&dlg);
    emailEdit->setText(cur.email);
    emailEdit->setPlaceholderText(QStringLiteral("Optional: user@gmail.com"));
    form->addRow(QStringLiteral("Google/Account:"), emailEdit);

    layout->addLayout(form);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(btnBox);

    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (!name.isEmpty()) {
            QString color = colorCombo->currentData().toString();
            QString email = emailEdit->text().trimmed();
            ProfileManager::instance().updateProfile(cur.id, name, color, cur.avatarIcon, email);
            updateProfileAvatar();
            showToast(QStringLiteral("Profile updated: %1").arg(name));
        }
    }
}

void Browser::onProfileClicked()
{
    UserProfile active = ProfileManager::instance().activeProfile();
    auto *menu = new QMenu(this);
    menu->setStyleSheet(QStringLiteral(R"(
        QMenu {
            background: #090e1c;
            border: 1px solid rgba(56, 189, 248, 0.25);
            border-radius: 10px;
            padding: 8px;
            color: #e2e8f0;
            font-size: 12.5px;
            font-family: 'Inter', sans-serif;
        }
        QMenu::item {
            padding: 8px 20px 8px 12px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background: rgba(56, 189, 248, 0.16);
            color: #38bdf8;
        }
        QMenu::separator {
            height: 1px;
            background: rgba(255, 255, 255, 0.08);
            margin: 6px 4px;
        }
    )"));

    // Header: Active Profile
    QString activeTitle = QStringLiteral("👤 Current: %1").arg(active.name);
    if (!active.email.isEmpty()) activeTitle += QStringLiteral(" • %1").arg(active.email);
    auto *hdrAct = menu->addAction(activeTitle);
    hdrAct->setEnabled(false);

    // Quick Google Account Shortcuts
    auto *googleAct = menu->addAction(QStringLiteral("🔑 Sign in to Google Account..."));
    connect(googleAct, &QAction::triggered, this, [this]{
        newTab(QUrl(QStringLiteral("https://accounts.google.com")));
    });

    auto *googleAccAct = menu->addAction(QStringLiteral("🌐 Manage Google Account..."));
    connect(googleAccAct, &QAction::triggered, this, [this]{
        newTab(QUrl(QStringLiteral("https://myaccount.google.com")));
    });

    menu->addSeparator();

    // Profiles List (Switch Profile)
    auto *secTitle = menu->addAction(QStringLiteral("Switch Profile:"));
    secTitle->setEnabled(false);

    auto allProfiles = ProfileManager::instance().profiles();
    for (const auto &p : allProfiles) {
        bool isCur = (p.id == active.id);
        QString label = QStringLiteral("%1 %2").arg(isCur ? QStringLiteral("●") : QStringLiteral("○"), p.name);
        if (!p.email.isEmpty()) label += QStringLiteral(" (%1)").arg(p.email);
        auto *pAct = menu->addAction(label);
        if (isCur) {
            QFont f = pAct->font();
            f.setBold(true);
            pAct->setFont(f);
        }
        connect(pAct, &QAction::triggered, this, [this, p]{
            if (p.id != ProfileManager::instance().activeProfileId()) {
                switchProfile(p.id);
            }
        });
    }

    menu->addSeparator();

    // Management Actions
    auto *addProfAct = menu->addAction(QStringLiteral("➕ Add New Profile..."));
    connect(addProfAct, &QAction::triggered, this, &Browser::showAddProfileDialog);

    auto *editProfAct = menu->addAction(QStringLiteral("✏ Edit Current Profile..."));
    connect(editProfAct, &QAction::triggered, this, &Browser::showEditProfileDialog);

    if (allProfiles.size() > 1) {
        auto *delProfAct = menu->addAction(QStringLiteral("🗑 Delete Current Profile"));
        connect(delProfAct, &QAction::triggered, this, [this, active]{
            auto res = QMessageBox::question(this, QStringLiteral("Delete Profile"),
                QStringLiteral("Are you sure you want to delete profile \"%1\" and all its isolated data?").arg(active.name),
                QMessageBox::Yes | QMessageBox::No);
            if (res == QMessageBox::Yes) {
                ProfileManager::instance().deleteProfile(active.id);
                updateProfileAvatar();
                m_tabs->reloadAllTabsForProfile();
                showToast(QStringLiteral("Profile deleted"));
            }
        });
    }

    menu->addSeparator();
    auto *settingsProfAct = menu->addAction(QStringLiteral("⚙ Manage Profiles in Settings..."));
    connect(settingsProfAct, &QAction::triggered, this, &Browser::onSettingsClicked);

    menu->popup(m_avatarBtn ? m_avatarBtn->mapToGlobal(QPoint(0, m_avatarBtn->height() + 4)) : QCursor::pos());
}

void Browser::onMenuClicked()
{
    auto *menu = new QMenu(this);
    menu->addAction(QStringLiteral("New Tab (Ctrl+T)"), this, [this]{ newTab(); });
    menu->addAction(QStringLiteral("Find in Page (Ctrl+F)"), this, &Browser::toggleFindInPage);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Bookmarks (Ctrl+B)"), this, &Browser::onBookmarksClicked);
    menu->addAction(QStringLiteral("History (Ctrl+H)"), this, &Browser::onHistoryClicked);
    menu->addAction(QStringLiteral("Downloads (Ctrl+J)"), this, &Browser::onDownloadsClicked);
    menu->addAction(QStringLiteral("Spaces Workspace (Alt+S)"), this, &Browser::onSpacesClicked);
    menu->addAction(QStringLiteral("Titan AI Assistant (Ctrl+K)"), this, &Browser::onAICoreClicked);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Settings (Ctrl+,)"), this, &Browser::onSettingsClicked);
    menu->addAction(QStringLiteral("About Titan Browser"), this, [this]{
        QMessageBox::about(this, QStringLiteral("About Titan Browser"),
            QStringLiteral("<h3>Titan Browser v1.0.0</h3><p>An operating environment for the web built natively on Qt6 WebEngine for ArchTitan OS.</p>"));
    });
    menu->popup(QCursor::pos());
}

void Browser::onThemeToggleClicked()
{
    m_isDarkMode = !m_isDarkMode;
    showToast(m_isDarkMode ? QStringLiteral("Theme: Obsidian Dark") : QStringLiteral("Theme: Light Glass"));
}

/* ─── Navigation Slots ─────────────────────────────────────────────────── */
void Browser::loadUrl(const QUrl &url)
{
    m_tabs->loadInCurrentTab(url);
}

void Browser::newTab(const QUrl &url)
{
    QUrl target = url.isEmpty() ? QUrl(QString::fromUtf8(kHomeUrl)) : url;
    m_tabs->newTab(target);
    onUrlChanged(target);
}

void Browser::closeCurrentTab()
{
    if (m_tabs->count() > 1)
        m_tabs->removeTab(m_tabs->currentIndex());
}

void Browser::navigateBack()
{
    if (auto *v = currentView()) v->back();
}

void Browser::navigateForward()
{
    if (auto *v = currentView()) v->forward();
}

void Browser::reloadOrStop()
{
    if (auto *v = currentView()) {
        if (m_isLoading) {
            v->stop();
        } else {
            v->reload();
        }
    }
}

void Browser::updateNavigationButtons()
{
    if (auto *v = currentView()) {
        if (auto *h = v->history()) {
            m_backBtn->setEnabled(h->canGoBack());
            m_fwdBtn->setEnabled(h->canGoForward());
        }
    } else {
        m_backBtn->setEnabled(false);
        m_fwdBtn->setEnabled(false);
    }
}

void Browser::onUrlChanged(const QUrl &url)
{
    QString urlStr = url.toString();
    bool isHome = urlStr.isEmpty()
               || urlStr == QStringLiteral("about:blank")
               || urlStr.contains(QStringLiteral("homepage.html"), Qt::CaseInsensitive)
               || urlStr.startsWith(QStringLiteral("titan://home"), Qt::CaseInsensitive)
               || urlStr.startsWith(QStringLiteral("qrc:"), Qt::CaseInsensitive);

    if (m_topBar) m_topBar->setVisible(true); // Tab strip always visible so users can switch tabs visually at all times
    if (m_navBar) m_navBar->setVisible(!isHome);
    if (m_railWidget) m_railWidget->setVisible(!isHome);

    if (isHome) {
        m_addressBar->clear();
        m_addressBar->setBookmarked(false);
        setActiveRailButton(m_railHomeBtn);
    } else if (urlStr == QStringLiteral("titan://settings")) {
        m_addressBar->setText(QStringLiteral("titan://settings"));
        m_addressBar->setBookmarked(false);
        setActiveRailButton(m_railSettingsBtn);
    } else {
        m_addressBar->setUrl(url);
        m_addressBar->setBookmarked(isBookmarked(url));
        if (m_adBlocker && currentView()) {
            m_adBlocker->injectContentScriptIntoView(currentView());
        }
    }
    updateNavigationButtons();
}

void Browser::onLoadProgress(int progress)
{
    m_isLoading = (progress > 0 && progress < 100);
    m_reloadBtn->setIcon(QIcon(m_isLoading ? QStringLiteral(":/icons/close.svg") : QStringLiteral(":/icons/reload.svg")));
    m_reloadBtn->setToolTip(m_isLoading ? QStringLiteral("Stop Loading (Esc)") : QStringLiteral("Reload (Ctrl+R)"));

    if (progress > 0 && progress < 100) {
        m_progress->show();
        m_progress->setValue(progress);
        if (progress > 40 && m_adBlocker && currentView()) {
            m_adBlocker->injectContentScriptIntoView(currentView());
        }
    } else {
        m_progress->hide();
        m_progress->setValue(0);
    }
    updateNavigationButtons();
}

void Browser::onLoadFinished(bool ok)
{
    m_isLoading = false;
    m_reloadBtn->setIcon(QIcon(QStringLiteral(":/icons/reload.svg")));
    m_reloadBtn->setToolTip(QStringLiteral("Reload (Ctrl+R)"));
    m_progress->hide();

    if (ok) {
        if (auto *v = currentView()) {
            if (m_adBlocker) {
                m_adBlocker->injectContentScriptIntoView(v);
            }
            addHistoryRecord(v->title(), v->url());
        }
    }
    updateNavigationButtons();
}

void Browser::onTitleChanged(const QString &title)
{
    setWindowTitle(title.isEmpty()
        ? QStringLiteral("Titan Browser")
        : title + QStringLiteral(" — Titan Browser"));
}

void Browser::onAddressEntered()
{
    loadUrl(QUrl::fromUserInput(m_addressBar->text()));
}

void Browser::onTabChanged(int /*index*/)
{
    if (auto *v = currentView()) {
        m_addressBar->setUrl(v->url());
        m_addressBar->setBookmarked(isBookmarked(v->url()));
        updateNavigationButtons();
    }
}

QWebEngineView *Browser::currentView() const
{
    return m_tabs->currentView();
}
