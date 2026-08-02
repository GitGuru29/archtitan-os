#include "browser.h"
#include "tabwidget.h"
#include "addressbar.h"

#include <QApplication>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QAction>
#include <QShortcut>
#include <QKeySequence>
#include <QWebEngineView>
#include <QWebEngineHistory>
#include <QToolButton>

static const char *kHomeUrl = "qrc:/homepage.html";

static const char *kTheme = R"(
QMainWindow, QWidget {
    background: #0a0b14;
    color: #c0caf5;
    font-family: "JetBrainsMono Nerd Font", "Inter", monospace;
}
QToolBar {
    background: #080c17;
    border-bottom: 1px solid rgba(56, 189, 248, 0.12);
    padding: 4px 10px;
    spacing: 6px;
}
QToolBar#Sidebar {
    background: #070913;
    border-right: 1px solid rgba(56, 189, 248, 0.12);
    border-bottom: none;
    padding: 10px 4px;
    spacing: 14px;
}
QToolButton {
    background: transparent;
    border: none;
    border-radius: 8px;
    color: #94a3b8;
    padding: 4px 8px;
    font-size: 14px;
}
QToolButton:hover {
    background: rgba(255, 255, 255, 0.08);
    color: #f8fafc;
}
QToolButton:disabled { color: #334155; }
QToolButton#AvatarButton {
    background: #6366f1;
    color: #ffffff;
    border-radius: 12px;
    padding: 3px 7px;
}
QToolButton#AvatarButton:hover {
    background: #4f46e5;
}
QTabWidget::pane {
    border: none;
    background: #0a0b14;
}
QTabBar::tab {
    background: rgba(255,255,255,0.03);
    border: 1px solid rgba(255,255,255,0.06);
    border-bottom: none;
    border-radius: 8px 8px 0 0;
    color: #565f89;
    padding: 5px 14px;
    margin-right: 2px;
    font-size: 11px;
    min-width: 80px;
    max-width: 200px;
}
QTabBar::tab:selected {
    background: rgba(122,162,247,0.15);
    border-color: rgba(122,162,247,0.25);
    color: #7aa2f7;
}
QTabBar::tab:hover:!selected {
    background: rgba(122,162,247,0.08);
    color: #a9b1d6;
}
QTabBar::close-button { subcontrol-position: right; }
QProgressBar {
    background: transparent;
    border: none;
    height: 2px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
        stop:0 #7aa2f7, stop:1 #00d4ff);
    border-radius: 1px;
}
QStatusBar {
    background: rgba(10,11,20,0.85);
    color: #565f89;
    font-size: 10px;
    border-top: 1px solid rgba(122,162,247,0.1);
}
)";

Browser::Browser(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("TitanBrowser"));
    resize(1280, 800);
    setStyleSheet(QString::fromUtf8(kTheme));
    setupUi();
    newTab(QUrl(QString::fromUtf8(kHomeUrl)));
}

void Browser::setupUi()
{
    // ── Sidebar (Left) ────────────────────────────────────────
    QToolBar *sidebar = new QToolBar(QStringLiteral("Sidebar"), this);
    sidebar->setObjectName(QStringLiteral("Sidebar"));
    sidebar->setMovable(false);
    sidebar->setFloatable(false);
    sidebar->setOrientation(Qt::Vertical);
    addToolBar(Qt::LeftToolBarArea, sidebar);

    sidebar->addAction(QStringLiteral("A"), this, [this]{ loadUrl(QUrl(QString::fromUtf8(kHomeUrl))); });
    sidebar->addAction(QStringLiteral("󰋜"), this, [this]{ loadUrl(QUrl(QString::fromUtf8(kHomeUrl))); });
    sidebar->addAction(QStringLiteral("󰋚"), this, []{}); // History
    sidebar->addAction(QStringLiteral("󰓋"), this, []{}); // Bookmarks
    sidebar->addAction(QStringLiteral("󰇚"), this, []{}); // Downloads
    sidebar->addAction(QStringLiteral("🛡"), this, []{}); // Shield
    sidebar->addAction(QStringLiteral("󰏖"), this, []{}); // Extensions
    sidebar->addAction(QStringLiteral("󰒓"), this, []{}); // Settings

    QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    sidebar->addWidget(spacer);

    sidebar->addAction(QStringLiteral("󰖔"), this, []{}); // Theme Toggle

    // ── Central widget ────────────────────────────────────────
    m_tabs = new TabWidget(this);
    setCentralWidget(m_tabs);

    connect(m_tabs, &TabWidget::urlChanged,    this, &Browser::onUrlChanged);
    connect(m_tabs, &TabWidget::titleChanged,  this, &Browser::onTitleChanged);
    connect(m_tabs, &TabWidget::loadProgress,  this, &Browser::onLoadProgress);
    connect(m_tabs, &TabWidget::loadFinished,  this, &Browser::onLoadFinished);

    // ── Toolbar ───────────────────────────────────────────────
    QToolBar *bar = addToolBar(QStringLiteral("Navigation"));
    bar->setObjectName(QStringLiteral("Navigation"));
    bar->setMovable(false);
    bar->setFloatable(false);

    m_backAction    = bar->addAction(QStringLiteral("←"), this, &Browser::navigateBack);
    m_forwardAction = bar->addAction(QStringLiteral("→"), this, &Browser::navigateForward);
    m_reloadAction  = bar->addAction(QStringLiteral("↺"), this, &Browser::reload);

    m_backAction->setShortcut(QKeySequence::Back);
    m_forwardAction->setShortcut(QKeySequence::Forward);

    m_addressBar = new AddressBar(this);
    bar->addWidget(m_addressBar);
    connect(m_addressBar, &AddressBar::urlEntered, this, &Browser::loadUrl);

    bar->addAction(QStringLiteral("🛡"), this, []{});
    bar->addAction(QStringLiteral("⬇"), this, []{});
    bar->addAction(QStringLiteral("👤"), this, []{});

    QAction *avatarAction = bar->addAction(QStringLiteral("👤"), this, []{});
    if (auto *btn = qobject_cast<QToolButton*>(bar->widgetForAction(avatarAction))) {
        btn->setObjectName(QStringLiteral("AvatarButton"));
    }

    bar->addAction(QStringLiteral("⋮"), this, []{});

    // ── Progress bar (2px accent line below toolbar) ──────────
    m_progress = new QProgressBar(this);
    m_progress->setMaximumHeight(2);
    m_progress->setTextVisible(false);
    m_progress->hide();

    auto *wrapper = new QWidget(this);
    auto *vl = new QVBoxLayout(wrapper);
    vl->setContentsMargins(0,0,0,0);
    vl->setSpacing(0);
    vl->addWidget(m_progress);
    vl->addWidget(m_tabs);
    setCentralWidget(wrapper);

    // ── Shortcuts ─────────────────────────────────────────────
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_T),
                  this, [this]{ newTab(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W),
                  this, [this]{ closeCurrentTab(); });
    new QShortcut(QKeySequence::Refresh, this, [this]{ reload(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_L),
                  this, [this]{ m_addressBar->setFocus(); m_addressBar->selectAll(); });

    // ── Status bar ────────────────────────────────────────────
    statusBar()->show();
}

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

    if (auto *v = currentView()) {
        m_backAction->setEnabled(v->page()->history()->canGoBack());
        m_forwardAction->setEnabled(v->page()->history()->canGoForward());
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
