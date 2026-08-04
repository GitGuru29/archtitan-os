#include "browser.h"
#include "tabwidget.h"
#include "addressbar.h"

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

static const char *kHomeUrl = "qrc:/homepage.html";

/* ─── Theme Stylesheet ────────────────────────────────────────────────── */
static const char *kTheme = R"(

/* Global */
QMainWindow, QWidget {
    background: #070913;
    color: #c0caf5;
    font-family: "Inter", "Segoe UI", system-ui, sans-serif;
    font-size: 13px;
}

/* Top Header Bar (Tabs Area) */
QWidget#TopBar {
    background: #060810;
    border-bottom: 1px solid rgba(56, 189, 248, 0.08);
    min-height: 40px;
    max-height: 40px;
}

QToolButton#LogoButton {
    background: transparent;
    border: none;
    padding: 4px;
    margin-left: 8px;
    margin-right: 6px;
}

/* Tab Bar */
QTabBar {
    background: transparent;
    border: none;
}

QTabBar::tab {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.06);
    border-bottom: none;
    border-radius: 8px 8px 0 0;
    color: #64748b;
    padding: 6px 14px;
    margin-right: 3px;
    font-size: 12px;
    min-width: 120px;
    max-width: 220px;
    height: 24px;
}

QTabBar::tab:selected {
    background: rgba(56, 189, 248, 0.12);
    border-color: rgba(56, 189, 248, 0.25);
    color: #f8fafc;
}

QTabBar::tab:hover:!selected {
    background: rgba(56, 189, 248, 0.05);
    color: #94a3b8;
}

QTabBar::close-button {
    subcontrol-position: right;
    margin-left: 4px;
}

QToolButton#AddTabButton {
    background: transparent;
    border: none;
    border-radius: 6px;
    padding: 4px;
}

QToolButton#AddTabButton:hover {
    background: rgba(255, 255, 255, 0.08);
}

/* Window Control Buttons */
QToolButton#WinBtn {
    background: transparent;
    border: none;
    color: #64748b;
    font-size: 14px;
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

/* Navigation Toolbar */
QWidget#NavBar {
    background: #080c17;
    border-bottom: 1px solid rgba(56, 189, 248, 0.08);
    min-height: 44px;
    max-height: 44px;
    padding: 0 10px;
}

QToolButton#NavBtn {
    background: transparent;
    border: none;
    border-radius: 8px;
    padding: 6px;
    min-width: 28px;
    min-height: 28px;
    max-width: 28px;
    max-height: 28px;
}

QToolButton#NavBtn:hover {
    background: rgba(255, 255, 255, 0.08);
}

QToolButton#NavBtn:disabled {
    opacity: 0.3;
}

QToolButton#AvatarButton {
    background: #6366f1;
    border: none;
    border-radius: 14px;
    padding: 4px;
    min-width: 28px;
    min-height: 28px;
    max-width: 28px;
    max-height: 28px;
}

QToolButton#AvatarButton:hover {
    background: #4f46e5;
}

/* Sidebar */
QWidget#SidebarWidget {
    background: #070913;
    border-right: 1px solid rgba(56, 189, 248, 0.08);
    min-width: 52px;
    max-width: 52px;
}

QToolButton#SidebarBtn {
    background: transparent;
    border: none;
    border-radius: 10px;
    padding: 8px;
    margin: 4px 6px;
    min-width: 36px;
    min-height: 36px;
    max-width: 36px;
    max-height: 36px;
}

QToolButton#SidebarBtn:hover {
    background: rgba(255, 255, 255, 0.07);
}

/* Active Sidebar Item - Matches Blue Rounded Card in Screenshot */
QToolButton#SidebarBtnActive {
    background: #38bdf8;
    border: none;
    border-radius: 10px;
    padding: 8px;
    margin: 4px 6px;
    min-width: 36px;
    min-height: 36px;
    max-width: 36px;
    max-height: 36px;
}

/* Progress Bar */
QProgressBar {
    background: transparent;
    border: none;
    height: 2px;
}

QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #38bdf8, stop:0.5 #00d4ff, stop:1 #7aa2f7);
    border-radius: 1px;
}

)";

/* ─── Constructor ──────────────────────────────────────────────────────── */
Browser::Browser(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TitanBrowser"));
    resize(1280, 800);
    setStyleSheet(QString::fromUtf8(kTheme));
    setupUi();
    newTab(QUrl(QString::fromUtf8(kHomeUrl)));
}

/* ─── UI Setup ─────────────────────────────────────────────────────────── */
void Browser::setupUi()
{
    // Main Container Layout (Vertical)
    auto *centralContainer = new QWidget(this);
    auto *mainVLayout = new QVBoxLayout(centralContainer);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);
    setCentralWidget(centralContainer);

    // ── 1. Top Header Bar (Logo + TabBar + Window Controls) ──────────────
    auto *topBar = new QWidget(this);
    topBar->setObjectName(QStringLiteral("TopBar"));
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(4, 0, 8, 0);
    topLayout->setSpacing(4);

    // ArchTitan Logo
    auto *logoBtn = new QToolButton(topBar);
    logoBtn->setObjectName(QStringLiteral("LogoButton"));
    logoBtn->setIcon(QIcon(QStringLiteral(":/icons/logo.svg")));
    logoBtn->setIconSize(QSize(24, 24));
    topLayout->addWidget(logoBtn);

    // Tab Widget (we customize TabWidget to expose QTabBar)
    m_tabs = new TabWidget(topBar);
    topLayout->addWidget(m_tabs->tabBar(), 1);

    // Add Tab (+) Button
    auto *addTabBtn = new QToolButton(topBar);
    addTabBtn->setObjectName(QStringLiteral("AddTabButton"));
    addTabBtn->setIcon(QIcon(QStringLiteral(":/icons/plus.svg")));
    addTabBtn->setIconSize(QSize(16, 16));
    addTabBtn->setToolTip(QStringLiteral("New Tab (Ctrl+T)"));
    connect(addTabBtn, &QToolButton::clicked, this, [this]{ newTab(); });
    topLayout->addWidget(addTabBtn);

    topLayout->addStretch();

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

    // ── 2. Navigation Bar ────────────────────────────────────────────────
    auto *navBar = new QWidget(this);
    navBar->setObjectName(QStringLiteral("NavBar"));
    auto *navLayout = new QHBoxLayout(navBar);
    navLayout->setContentsMargins(10, 4, 10, 4);
    navLayout->setSpacing(6);

    // Back button
    auto *backBtn = new QToolButton(navBar);
    backBtn->setObjectName(QStringLiteral("NavBtn"));
    backBtn->setIcon(QIcon(QStringLiteral(":/icons/back.svg")));
    backBtn->setIconSize(QSize(18, 18));
    backBtn->setToolTip(QStringLiteral("Back"));
    connect(backBtn, &QToolButton::clicked, this, &Browser::navigateBack);
    navLayout->addWidget(backBtn);

    // Forward button
    auto *fwdBtn = new QToolButton(navBar);
    fwdBtn->setObjectName(QStringLiteral("NavBtn"));
    fwdBtn->setIcon(QIcon(QStringLiteral(":/icons/forward.svg")));
    fwdBtn->setIconSize(QSize(18, 18));
    fwdBtn->setToolTip(QStringLiteral("Forward"));
    connect(fwdBtn, &QToolButton::clicked, this, &Browser::navigateForward);
    navLayout->addWidget(fwdBtn);

    // Reload button
    auto *reloadBtn = new QToolButton(navBar);
    reloadBtn->setObjectName(QStringLiteral("NavBtn"));
    reloadBtn->setIcon(QIcon(QStringLiteral(":/icons/reload.svg")));
    reloadBtn->setIconSize(QSize(18, 18));
    reloadBtn->setToolTip(QStringLiteral("Reload"));
    connect(reloadBtn, &QToolButton::clicked, this, &Browser::reload);
    navLayout->addWidget(reloadBtn);

    // Address bar
    m_addressBar = new AddressBar(navBar);
    navLayout->addWidget(m_addressBar, 1);
    connect(m_addressBar, &AddressBar::urlEntered, this, &Browser::loadUrl);

    // Navigation Right-side Icons
    auto *bookmarkBtn = new QToolButton(navBar);
    bookmarkBtn->setObjectName(QStringLiteral("NavBtn"));
    bookmarkBtn->setIcon(QIcon(QStringLiteral(":/icons/bookmark.svg")));
    bookmarkBtn->setIconSize(QSize(18, 18));
    bookmarkBtn->setToolTip(QStringLiteral("Bookmark page"));
    navLayout->addWidget(bookmarkBtn);

    auto *shieldBtn = new QToolButton(navBar);
    shieldBtn->setObjectName(QStringLiteral("NavBtn"));
    shieldBtn->setIcon(QIcon(QStringLiteral(":/icons/shield.svg")));
    shieldBtn->setIconSize(QSize(18, 18));
    shieldBtn->setToolTip(QStringLiteral("TitanShield Active"));
    navLayout->addWidget(shieldBtn);

    auto *dlBtn = new QToolButton(navBar);
    dlBtn->setObjectName(QStringLiteral("NavBtn"));
    dlBtn->setIcon(QIcon(QStringLiteral(":/icons/download.svg")));
    dlBtn->setIconSize(QSize(18, 18));
    dlBtn->setToolTip(QStringLiteral("Downloads"));
    navLayout->addWidget(dlBtn);

    auto *userBtn = new QToolButton(navBar);
    userBtn->setObjectName(QStringLiteral("NavBtn"));
    userBtn->setIcon(QIcon(QStringLiteral(":/icons/user.svg")));
    userBtn->setIconSize(QSize(18, 18));
    userBtn->setToolTip(QStringLiteral("Profile"));
    navLayout->addWidget(userBtn);

    // Avatar Button (Indigo Circle)
    auto *avatarBtn = new QToolButton(navBar);
    avatarBtn->setObjectName(QStringLiteral("AvatarButton"));
    avatarBtn->setIcon(QIcon(QStringLiteral(":/icons/user.svg")));
    avatarBtn->setIconSize(QSize(16, 16));
    avatarBtn->setToolTip(QStringLiteral("Account"));
    navLayout->addWidget(avatarBtn);

    // Menu Button
    auto *menuBtn = new QToolButton(navBar);
    menuBtn->setObjectName(QStringLiteral("NavBtn"));
    menuBtn->setIcon(QIcon(QStringLiteral(":/icons/menu.svg")));
    menuBtn->setIconSize(QSize(18, 18));
    menuBtn->setToolTip(QStringLiteral("Menu"));
    navLayout->addWidget(menuBtn);

    mainVLayout->addWidget(navBar);

    // ── 3. Progress Bar (2px line) ──────────────────────────────────────
    m_progress = new QProgressBar(this);
    m_progress->setMaximumHeight(2);
    m_progress->setTextVisible(false);
    m_progress->hide();
    mainVLayout->addWidget(m_progress);

    // ── 4. Main Body Area (Sidebar + Web Content) ────────────────────────
    auto *bodyWidget = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    // Sidebar
    auto *sidebarWidget = new QWidget(bodyWidget);
    sidebarWidget->setObjectName(QStringLiteral("SidebarWidget"));
    auto *sidebarLayout = new QVBoxLayout(sidebarWidget);
    sidebarLayout->setContentsMargins(0, 8, 0, 8);
    sidebarLayout->setSpacing(4);

    // Home Button (Active by default, styled as blue card)
    auto *homeBtn = new QToolButton(sidebarWidget);
    homeBtn->setObjectName(QStringLiteral("SidebarBtnActive"));
    homeBtn->setIcon(QIcon(QStringLiteral(":/icons/home.svg")));
    homeBtn->setIconSize(QSize(20, 20));
    homeBtn->setToolTip(QStringLiteral("Home"));
    connect(homeBtn, &QToolButton::clicked, this, [this]{
        loadUrl(QUrl(QString::fromUtf8(kHomeUrl)));
    });
    sidebarLayout->addWidget(homeBtn);

    // History Button
    auto *histBtn = new QToolButton(sidebarWidget);
    histBtn->setObjectName(QStringLiteral("SidebarBtn"));
    histBtn->setIcon(QIcon(QStringLiteral(":/icons/history.svg")));
    histBtn->setIconSize(QSize(20, 20));
    histBtn->setToolTip(QStringLiteral("History"));
    sidebarLayout->addWidget(histBtn);

    // Bookmarks Button
    auto *bmBtn = new QToolButton(sidebarWidget);
    bmBtn->setObjectName(QStringLiteral("SidebarBtn"));
    bmBtn->setIcon(QIcon(QStringLiteral(":/icons/bookmark.svg")));
    bmBtn->setIconSize(QSize(20, 20));
    bmBtn->setToolTip(QStringLiteral("Bookmarks"));
    sidebarLayout->addWidget(bmBtn);

    // Downloads Button
    auto *sDlBtn = new QToolButton(sidebarWidget);
    sDlBtn->setObjectName(QStringLiteral("SidebarBtn"));
    sDlBtn->setIcon(QIcon(QStringLiteral(":/icons/download.svg")));
    sDlBtn->setIconSize(QSize(20, 20));
    sDlBtn->setToolTip(QStringLiteral("Downloads"));
    sidebarLayout->addWidget(sDlBtn);

    // Shield Button
    auto *sShieldBtn = new QToolButton(sidebarWidget);
    sShieldBtn->setObjectName(QStringLiteral("SidebarBtn"));
    sShieldBtn->setIcon(QIcon(QStringLiteral(":/icons/shield.svg")));
    sShieldBtn->setIconSize(QSize(20, 20));
    sShieldBtn->setToolTip(QStringLiteral("TitanShield"));
    sidebarLayout->addWidget(sShieldBtn);

    // Extensions Button
    auto *extBtn = new QToolButton(sidebarWidget);
    extBtn->setObjectName(QStringLiteral("SidebarBtn"));
    extBtn->setIcon(QIcon(QStringLiteral(":/icons/extension.svg")));
    extBtn->setIconSize(QSize(20, 20));
    extBtn->setToolTip(QStringLiteral("Extensions"));
    sidebarLayout->addWidget(extBtn);

    // Settings Button
    auto *setBtn = new QToolButton(sidebarWidget);
    setBtn->setObjectName(QStringLiteral("SidebarBtn"));
    setBtn->setIcon(QIcon(QStringLiteral(":/icons/settings.svg")));
    setBtn->setIconSize(QSize(20, 20));
    setBtn->setToolTip(QStringLiteral("Settings"));
    sidebarLayout->addWidget(setBtn);

    sidebarLayout->addStretch();

    // Moon Button (Dark mode toggle at bottom)
    auto *moonBtn = new QToolButton(sidebarWidget);
    moonBtn->setObjectName(QStringLiteral("SidebarBtn"));
    moonBtn->setIcon(QIcon(QStringLiteral(":/icons/moon.svg")));
    moonBtn->setIconSize(QSize(20, 20));
    moonBtn->setToolTip(QStringLiteral("Toggle Dark Mode"));
    sidebarLayout->addWidget(moonBtn);

    bodyLayout->addWidget(sidebarWidget);

    // Stacked widget for web content
    bodyLayout->addWidget(m_tabs->contentStack(), 1);

    mainVLayout->addWidget(bodyWidget, 1);

    // Connect signals
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
}

/* ─── Slot Implementations ─────────────────────────────────────────────── */

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
        ? QStringLiteral("TitanBrowser")
        : title + QStringLiteral(" — TitanBrowser"));
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
