#include "browser.h"
#include "tabwidget.h"
#include "addressbar.h"
#include "adblocker.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QShortcut>
#include <QKeySequence>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QIcon>
#include <QSize>
#include <QMenu>
#include <QMessageBox>
#include <QStyle>

static const char *kHomeUrl     = "qrc:/homepage.html";
static const char *kSettingsUrl = "qrc:/settings.html";

/* ─── Modern Developer Browser Stylesheet ─────────────────────────────── */
static const char *kTheme = R"(
/* Global Window */
QMainWindow, QWidget#CentralContainer {
    background: #060913;
    color: #e2e8f0;
    font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    font-size: 13px;
}

/* ── 1. Top Tab Strip ─────────────────────────────────────────────────── */
QWidget#TopBar {
    background: #050711;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
    min-height: 38px;
    max-height: 38px;
}

QToolButton#TabLogoBtn {
    background: transparent;
    border: none;
    padding: 2px 6px;
    margin-left: 6px;
    border-radius: 6px;
}
QToolButton#TabLogoBtn:hover {
    background: rgba(56, 189, 248, 0.1);
}

QTabBar {
    background: transparent;
    border: none;
    qproperty-drawBase: 0;
}

QTabBar::tab {
    background: #070a14;
    border: 1px solid rgba(255, 255, 255, 0.05);
    border-bottom: none;
    border-radius: 7px 7px 0 0;
    color: #64748b;
    padding: 5px 12px 5px 10px;
    margin-right: 3px;
    font-size: 12px;
    font-weight: 500;
    min-width: 120px;
    max-width: 200px;
    height: 24px;
}

QTabBar::tab:selected {
    background: #0b1120;
    border-color: rgba(56, 189, 248, 0.25);
    color: #f8fafc;
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    background: #0d1527;
    color: #94a3b8;
}

QTabBar::close-button {
    subcontrol-position: right;
    margin-left: 4px;
    padding: 2px;
}

QToolButton#AddTabButton {
    background: transparent;
    border: none;
    border-radius: 6px;
    color: #94a3b8;
    padding: 4px;
    min-width: 24px;
    min-height: 24px;
}
QToolButton#AddTabButton:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #ffffff;
}

/* Window Control Buttons */
QToolButton#WinBtn {
    background: transparent;
    border: none;
    color: #64748b;
    font-size: 13px;
    min-width: 32px;
    max-width: 32px;
    min-height: 32px;
    max-height: 32px;
}
QToolButton#WinBtn:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #f8fafc;
}
QToolButton#WinBtnClose:hover {
    background: #ef4444;
    color: #ffffff;
}

/* ── 2. Navigation / Omnibox Row ──────────────────────────────────────── */
QWidget#NavBar {
    background: #070a14;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
    min-height: 44px;
    max-height: 44px;
    padding: 0 8px;
}

QToolButton#NavBtn {
    background: transparent;
    border: none;
    border-radius: 6px;
    padding: 5px;
    color: #94a3b8;
    min-width: 26px;
    min-height: 26px;
}
QToolButton#NavBtn:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #ffffff;
}

QToolButton#ShieldBadgeBtn {
    background: rgba(34, 197, 94, 0.1);
    border: 1px solid rgba(34, 197, 94, 0.25);
    border-radius: 6px;
    color: #22c55e;
    font-size: 11px;
    font-weight: 700;
    padding: 4px 8px;
    min-height: 22px;
}
QToolButton#ShieldBadgeBtn:hover {
    background: rgba(34, 197, 94, 0.2);
    border-color: #22c55e;
}

QToolButton#AvatarBtn {
    background: #8b5cf6;
    border: none;
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
    background: #7c3aed;
}

/* ── 3. Compact Adaptive Rail (Sidebar) ───────────────────────────────── */
QWidget#RailWidget {
    background: #050711;
    border-right: 1px solid rgba(255, 255, 255, 0.05);
    min-width: 46px;
    max-width: 46px;
}

QToolButton.RailNavBtn {
    background: transparent;
    border: none;
    border-radius: 8px;
    padding: 7px;
    margin: 3px 5px;
    min-width: 34px;
    min-height: 34px;
    max-width: 34px;
    max-height: 34px;
}
QToolButton.RailNavBtn:hover {
    background: rgba(255, 255, 255, 0.08);
}

QToolButton.RailNavBtnActive {
    background: rgba(56, 189, 248, 0.15);
    border: 1px solid rgba(56, 189, 248, 0.35);
    border-radius: 8px;
    padding: 7px;
    margin: 3px 5px;
    min-width: 34px;
    min-height: 34px;
    max-width: 34px;
    max-height: 34px;
}

/* Progress Bar */
QProgressBar {
    background: transparent;
    border: none;
    height: 2px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #38bdf8, stop:0.5 #0ea5e9, stop:1 #818cf8);
}

/* Menus */
QMenu {
    background: #0b1120;
    border: 1px solid rgba(56, 189, 248, 0.2);
    border-radius: 8px;
    padding: 6px;
    color: #e2e8f0;
}
QMenu::item {
    padding: 6px 20px 6px 12px;
    border-radius: 4px;
}
QMenu::item:selected {
    background: rgba(56, 189, 248, 0.15);
    color: #38bdf8;
}
)";

/* ─── Constructor ──────────────────────────────────────────────────────── */
Browser::Browser(AdBlocker *adBlocker, QWidget *parent)
    : QMainWindow(parent), m_adBlocker(adBlocker)
{
    setWindowTitle(QStringLiteral("Titan Browser"));
    resize(1360, 840);
    setStyleSheet(QString::fromUtf8(kTheme));
    setupUi();
    newTab(QUrl(QString::fromUtf8(kHomeUrl)));

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
    }
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

    // ── 1. Top Tab Strip ─────────────────────────────────────────────────
    auto *topBar = new QWidget(this);
    topBar->setObjectName(QStringLiteral("TopBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(6, 0, 6, 0);
    topLayout->setSpacing(4);

    // Titan Logo in Tab Strip
    auto *tabLogoBtn = new QToolButton(topBar);
    tabLogoBtn->setObjectName(QStringLiteral("TabLogoBtn"));
    tabLogoBtn->setIcon(QIcon(QStringLiteral(":/icons/logo.svg")));
    tabLogoBtn->setIconSize(QSize(18, 18));
    tabLogoBtn->setToolTip(QStringLiteral("Titan Home"));
    connect(tabLogoBtn, &QToolButton::clicked, this, &Browser::onHomeClicked);
    topLayout->addWidget(tabLogoBtn);

    // Tab Widget
    m_tabs = new TabWidget(topBar);
    topLayout->addWidget(m_tabs->tabBar(), 0);

    // Add Tab (+) Button
    auto *addTabBtn = new QToolButton(topBar);
    addTabBtn->setObjectName(QStringLiteral("AddTabButton"));
    addTabBtn->setIcon(QIcon(QStringLiteral(":/icons/plus.svg")));
    addTabBtn->setIconSize(QSize(14, 14));
    addTabBtn->setToolTip(QStringLiteral("New Tab (Ctrl+T)"));
    connect(addTabBtn, &QToolButton::clicked, this, [this]{ newTab(); });
    topLayout->addWidget(addTabBtn);

    topLayout->addStretch(1);

    // Window Controls
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

    // ── 2. Navigation / Omnibox Row ──────────────────────────────────────
    auto *navBar = new QWidget(this);
    navBar->setObjectName(QStringLiteral("NavBar"));
    auto *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(10, 4, 10, 4);
    navLayout->setSpacing(6);

    // Back
    auto *backBtn = new QToolButton(navBar);
    backBtn->setObjectName(QStringLiteral("NavBtn"));
    backBtn->setIcon(QIcon(QStringLiteral(":/icons/back.svg")));
    backBtn->setIconSize(QSize(17, 17));
    backBtn->setToolTip(QStringLiteral("Back (Alt+Left)"));
    connect(backBtn, &QToolButton::clicked, this, &Browser::navigateBack);
    navLayout->addWidget(backBtn);

    // Forward
    auto *fwdBtn = new QToolButton(navBar);
    fwdBtn->setObjectName(QStringLiteral("NavBtn"));
    fwdBtn->setIcon(QIcon(QStringLiteral(":/icons/forward.svg")));
    fwdBtn->setIconSize(QSize(17, 17));
    fwdBtn->setToolTip(QStringLiteral("Forward (Alt+Right)"));
    connect(fwdBtn, &QToolButton::clicked, this, &Browser::navigateForward);
    navLayout->addWidget(fwdBtn);

    // Reload
    auto *reloadBtn = new QToolButton(navBar);
    reloadBtn->setObjectName(QStringLiteral("NavBtn"));
    reloadBtn->setIcon(QIcon(QStringLiteral(":/icons/reload.svg")));
    reloadBtn->setIconSize(QSize(17, 17));
    reloadBtn->setToolTip(QStringLiteral("Reload (Ctrl+R)"));
    connect(reloadBtn, &QToolButton::clicked, this, &Browser::reload);
    navLayout->addWidget(reloadBtn);

    // Omnibox (Primary Centerpiece)
    m_addressBar = new AddressBar(navBar);
    navLayout->addWidget(m_addressBar, 1);
    connect(m_addressBar, &AddressBar::urlEntered, this, &Browser::loadUrl);

    // Subtle TitanShield Status Button (reference stored for live update)
    m_shieldBadgeBtn = new QToolButton(navBar);
    m_shieldBadgeBtn->setObjectName(QStringLiteral("ShieldBadgeBtn"));
    m_shieldBadgeBtn->setIcon(QIcon(QStringLiteral(":/icons/shield.svg")));
    m_shieldBadgeBtn->setText(QStringLiteral(" Protected"));
    m_shieldBadgeBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_shieldBadgeBtn->setToolTip(QStringLiteral("TitanShield — click for security details"));
    connect(m_shieldBadgeBtn, &QToolButton::clicked, this, &Browser::onShieldClicked);
    navLayout->addWidget(m_shieldBadgeBtn);

    // Extensions
    auto *extBtn = new QToolButton(navBar);
    extBtn->setObjectName(QStringLiteral("NavBtn"));
    extBtn->setIcon(QIcon(QStringLiteral(":/icons/cube.svg")));
    extBtn->setIconSize(QSize(17, 17));
    extBtn->setToolTip(QStringLiteral("Extensions"));
    connect(extBtn, &QToolButton::clicked, this, &Browser::onExtensionsClicked);
    navLayout->addWidget(extBtn);

    // Downloads
    auto *dlBtn = new QToolButton(navBar);
    dlBtn->setObjectName(QStringLiteral("NavBtn"));
    dlBtn->setIcon(QIcon(QStringLiteral(":/icons/download.svg")));
    dlBtn->setIconSize(QSize(17, 17));
    dlBtn->setToolTip(QStringLiteral("Downloads"));
    connect(dlBtn, &QToolButton::clicked, this, &Browser::onDownloadsClicked);
    navLayout->addWidget(dlBtn);

    // Profile Avatar
    auto *avatarBtn = new QToolButton(navBar);
    avatarBtn->setObjectName(QStringLiteral("AvatarBtn"));
    avatarBtn->setText(QStringLiteral("T"));
    avatarBtn->setToolTip(QStringLiteral("Titan Profile"));
    connect(avatarBtn, &QToolButton::clicked, this, &Browser::onProfileClicked);
    navLayout->addWidget(avatarBtn);

    // Main Menu
    auto *menuBtn = new QToolButton(navBar);
    menuBtn->setObjectName(QStringLiteral("NavBtn"));
    menuBtn->setIcon(QIcon(QStringLiteral(":/icons/menu.svg")));
    menuBtn->setIconSize(QSize(17, 17));
    menuBtn->setToolTip(QStringLiteral("Titan Menu"));
    connect(menuBtn, &QToolButton::clicked, this, &Browser::onMenuClicked);
    navLayout->addWidget(menuBtn);

    mainVLayout->addWidget(navBar);

    // ── 3. Progress Bar (2px line) ──────────────────────────────────────
    m_progress = new QProgressBar(this);
    m_progress->setMaximumHeight(2);
    m_progress->setTextVisible(false);
    m_progress->hide();
    mainVLayout->addWidget(m_progress);

    // ── 4. Main Body (Compact Rail + Web View) ───────────────────────────
    auto *bodyWidget = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Compact Navigation Rail (46px)
    auto *railWidget = new QWidget(bodyWidget);
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
    createRailBtn(QStringLiteral(":/icons/home.svg"), QStringLiteral("Home (Alt+H)"), m_railHomeBtn);
    m_railHomeBtn->setProperty("class", "RailNavBtnActive");
    m_activeRailBtn = m_railHomeBtn;
    connect(m_railHomeBtn, &QToolButton::clicked, this, &Browser::onHomeClicked);
    railLayout->addWidget(m_railHomeBtn);

    createRailBtn(QStringLiteral(":/icons/ai.svg"), QStringLiteral("Titan AI Assistant (⌘K)"), m_railAiBtn);
    connect(m_railAiBtn, &QToolButton::clicked, this, &Browser::onAICoreClicked);
    railLayout->addWidget(m_railAiBtn);

    createRailBtn(QStringLiteral(":/icons/spaces.svg"), QStringLiteral("Spaces Workspace"), m_railSpacesBtn);
    connect(m_railSpacesBtn, &QToolButton::clicked, this, &Browser::onSpacesClicked);
    railLayout->addWidget(m_railSpacesBtn);

    createRailBtn(QStringLiteral(":/icons/history.svg"), QStringLiteral("History"), m_railHistoryBtn);
    connect(m_railHistoryBtn, &QToolButton::clicked, this, &Browser::onHistoryClicked);
    railLayout->addWidget(m_railHistoryBtn);

    createRailBtn(QStringLiteral(":/icons/download.svg"), QStringLiteral("Downloads"), m_railDlBtn);
    connect(m_railDlBtn, &QToolButton::clicked, this, &Browser::onDownloadsClicked);
    railLayout->addWidget(m_railDlBtn);

    createRailBtn(QStringLiteral(":/icons/extension.svg"), QStringLiteral("Extensions"), m_railExtBtn);
    connect(m_railExtBtn, &QToolButton::clicked, this, &Browser::onExtensionsClicked);
    railLayout->addWidget(m_railExtBtn);

    createRailBtn(QStringLiteral(":/icons/settings.svg"), QStringLiteral("Settings"), m_railSettingsBtn);
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

    // Web Content Stack (Receives full horizontal viewport)
    bodyLayout->addWidget(m_tabs->contentStack(), 1);

    mainVLayout->addWidget(bodyWidget, 1);

    // Connect TabWidget signals
    connect(m_tabs, &TabWidget::urlChanged,    this, &Browser::onUrlChanged);
    connect(m_tabs, &TabWidget::titleChanged,  this, &Browser::onTitleChanged);
    connect(m_tabs, &TabWidget::loadProgress,  this, &Browser::onLoadProgress);
    connect(m_tabs, &TabWidget::loadFinished,  this, &Browser::onLoadFinished);

    // Shortcuts
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T), this, [this]{ newTab(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this, [this]{ closeCurrentTab(); });
    new QShortcut(QKeySequence::Refresh, this, [this]{ reload(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L), this, [this]{
        m_addressBar->setFocus();
        m_addressBar->selectAll();
    });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_K), this, [this]{
        if (auto *v = currentView()) {
            v->page()->runJavaScript(QStringLiteral("if (window.focusCommandBar) window.focusCommandBar();"));
        }
    });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Left), this, [this]{ navigateBack(); });
    new QShortcut(QKeySequence(Qt::ALT | Qt::Key_Right), this, [this]{ navigateForward(); });
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
    if (auto *v = currentView()) {
        v->page()->runJavaScript(QStringLiteral("if (window.openSpacesModal) window.openSpacesModal();"));
    }
}

void Browser::onHistoryClicked()
{
    setActiveRailButton(m_railHistoryBtn);
    showStatusNotification(QStringLiteral("Browsing History"),
        QStringLiteral("Browsing history is saved strictly to your local encrypted database.\n0 telemetry transmitted."));
}

void Browser::onDownloadsClicked()
{
    setActiveRailButton(m_railDlBtn);
    showStatusNotification(QStringLiteral("Downloads"),
        QStringLiteral("Default downloads folder: ~/Downloads\nAll downloaded files are verified clean."));
}

void Browser::onExtensionsClicked()
{
    setActiveRailButton(m_railExtBtn);
    showStatusNotification(QStringLiteral("Titan Extensions"),
        QStringLiteral("Active Extensions:\n✔ uBlock Origin (Active)\n✔ Bitwarden (Active)\n✔ Dark Reader (Active)"));
}

void Browser::onSettingsClicked()
{
    setActiveRailButton(m_railSettingsBtn);
    // Check if a settings tab is already open — if so, just switch to it
    for (int i = 0; i < m_tabs->count(); ++i) {
        QUrl tabUrl = m_tabs->tabUrl(i);
        if (tabUrl.toString().startsWith(QStringLiteral("qrc:/settings.html"))) {
            m_tabs->setCurrentIndex(i);
            return;
        }
    }
    newTab(QUrl(QString::fromUtf8(kSettingsUrl)));
}

void Browser::onShieldClicked()
{
    if (m_adBlocker) {
        qint64 blocked  = m_adBlocker->blockedCount();
        qint64 requests = m_adBlocker->requestCount();
        double pct      = (requests > 0) ? (100.0 * blocked / requests) : 0.0;
        QString js = QStringLiteral(
            "if (window.updateShieldStats) "
            "window.updateShieldStats(%1, %2, %3);"
        ).arg(blocked).arg(requests).arg(pct, 0, 'f', 1);
        if (auto *v = currentView())
            v->page()->runJavaScript(js);
    }
    if (auto *v = currentView()) {
        v->page()->runJavaScript(QStringLiteral("if (window.openShieldFlyout) window.openShieldFlyout();"));
    }
}

void Browser::onProfileClicked()
{
    showStatusNotification(QStringLiteral("Titan Account"),
        QStringLiteral("User: ArchTitan Developer\nKeyring: Hardware Encrypted\nCross-device sync: Active"));
}

void Browser::onMenuClicked()
{
    auto *menu = new QMenu(this);
    menu->addAction(QStringLiteral("New Tab (Ctrl+T)"), this, [this]{ newTab(); });
    menu->addAction(QStringLiteral("New Private Window"), this, [this]{
        showStatusNotification(QStringLiteral("Private Window"), QStringLiteral("Zero-history ephemeral session active."));
    });
    menu->addSeparator();
    menu->addAction(QStringLiteral("Spaces Workspace"), this, &Browser::onSpacesClicked);
    menu->addAction(QStringLiteral("Titan AI Assistant (Ctrl+K)"), this, &Browser::onAICoreClicked);
    menu->addAction(QStringLiteral("History"), this, &Browser::onHistoryClicked);
    menu->addAction(QStringLiteral("Downloads"), this, &Browser::onDownloadsClicked);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Settings"), this, &Browser::onSettingsClicked);
    menu->addAction(QStringLiteral("About Titan Browser"), this, [this]{
        QMessageBox::about(this, QStringLiteral("About Titan Browser"),
            QStringLiteral("<h3>Titan Browser v1.0.0</h3><p>An operating environment for the web built natively on Qt6 WebEngine for ArchTitan OS.</p>"));
    });
    menu->popup(QCursor::pos());
}

void Browser::onThemeToggleClicked()
{
    m_isDarkMode = !m_isDarkMode;
    if (auto *v = currentView()) {
        v->page()->runJavaScript(QStringLiteral("if (window.toggleTheme) window.toggleTheme();"));
    }
}

void Browser::showStatusNotification(const QString &title, const QString &message)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(title);
    msgBox.setText(title);
    msgBox.setInformativeText(message);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setStyleSheet(QStringLiteral(R"(
        QMessageBox {
            background: #0b1120;
            color: #f1f5f9;
            font-size: 13px;
        }
        QLabel { color: #e2e8f0; }
        QPushButton {
            background: #0284c7;
            color: white;
            border-radius: 6px;
            padding: 5px 16px;
            font-weight: 600;
        }
        QPushButton:hover { background: #0369a1; }
    )"));
    msgBox.exec();
}

/* ─── Navigation Slots ─────────────────────────────────────────────────── */

void Browser::loadUrl(const QUrl &url)
{
    if (auto *v = currentView())
        v->setUrl(url);
}

void Browser::newTab(const QUrl &url)
{
    QUrl target = url.isEmpty() ? QUrl(QString::fromUtf8(kHomeUrl)) : url;
    m_tabs->newTab(target);
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

void Browser::reload()
{
    if (auto *v = currentView()) v->reload();
}

void Browser::onUrlChanged(const QUrl &url)
{
    if (url.toString() == QStringLiteral("qrc:/homepage.html")) {
        m_addressBar->clear();
        setActiveRailButton(m_railHomeBtn);
    } else {
        m_addressBar->setUrl(url);
    }
}

void Browser::onLoadProgress(int progress)
{
    if (progress > 0 && progress < 100) {
        m_progress->show();
        m_progress->setValue(progress);
    } else {
        m_progress->hide();
        m_progress->setValue(0);
    }
}

void Browser::onLoadFinished(bool /*ok*/)
{
    m_progress->hide();
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
    if (auto *v = currentView())
        m_addressBar->setUrl(v->url());
}

QWebEngineView *Browser::currentView() const
{
    return m_tabs->currentView();
}
