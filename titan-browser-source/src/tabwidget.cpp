#include "tabwidget.h"
#include "newtabwidget.h"
#include "settingswidget.h"

#include <QTabBar>
#include <QStackedWidget>
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QCursor>

TabWidget::TabWidget(QWidget *parent) : QObject(parent)
{
    m_tabBar = new QTabBar(parent);
    m_tabBar->hide(); // Hide the native tab bar to prevent floating scroll arrows
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setElideMode(Qt::ElideRight);
    m_tabBar->setExpanding(false);
    m_tabBar->setContextMenuPolicy(Qt::CustomContextMenu);

    m_stack = new QStackedWidget(parent);

    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabWidget::onTabCloseRequested);
    connect(m_tabBar, &QTabBar::currentChanged, this, &TabWidget::onCurrentTabChanged);
    connect(m_tabBar, &QTabBar::customContextMenuRequested, this, &TabWidget::onTabBarContextMenu);
}

QWidget *TabWidget::newTab(const QUrl &url)
{
    QWidget *tabContent = nullptr;
    QString title = QStringLiteral("New Tab");
    QIcon icon = QIcon(QStringLiteral(":/icons/home.svg"));
    QUrl finalUrl = url;

    if (url.isEmpty() || url.toString() == QStringLiteral("titan://home") || url.toString() == QStringLiteral("qrc:/homepage.html")) {
        // Load the HTML dashboard via WebEngine instead of native widget
        auto *view = new QWebEngineView(m_stack);
        view->setUrl(QUrl(QStringLiteral("qrc:/homepage.html")));
        connectView(view);
        tabContent = view;
        finalUrl = QUrl(QStringLiteral("titan://home"));
    } else if (url.toString() == QStringLiteral("titan://settings") || url.toString() == QStringLiteral("qrc:/settings.html")) {
        auto *settings = new SettingsWidget(m_stack);
        tabContent = settings;
        title = QStringLiteral("Settings");
        icon = QIcon(QStringLiteral(":/icons/settings.svg"));
        finalUrl = QUrl(QStringLiteral("titan://settings"));
    } else {
        auto *view = new QWebEngineView(m_stack);
        view->setUrl(url);
        connectView(view);
        tabContent = view;
        title = QStringLiteral("Loading...");
    }

    m_tabUrls[tabContent] = finalUrl;
    m_lastAccessed[tabContent] = QDateTime::currentDateTime();
    m_isSuspended[tabContent] = false;

    int stackIndex = m_stack->addWidget(tabContent);
    int tabIndex = m_tabBar->addTab(icon, title);

    m_tabBar->setCurrentIndex(tabIndex);
    m_stack->setCurrentIndex(stackIndex);

    emit urlChanged(finalUrl);
    emit titleChanged(title);
    emit tabCountChanged(count());
    return tabContent;
}

void TabWidget::loadInCurrentTab(const QUrl &url)
{
    loadInTab(currentIndex(), url);
}

void TabWidget::loadInTab(int index, const QUrl &url)
{
    if (index < 0 || index >= m_stack->count()) return;

    QWidget *oldWidget = m_stack->widget(index);
    QString urlStr = url.toString();

    if (urlStr == QStringLiteral("titan://home") || urlStr == QStringLiteral("qrc:/homepage.html") || url.isEmpty()) {
        // If already showing homepage in a WebEngineView, just reload it
        if (auto *existingView = qobject_cast<QWebEngineView*>(oldWidget)) {
            QUrl currentUrl = existingView->url();
            if (currentUrl.toString() == QStringLiteral("qrc:/homepage.html")) return;
        }

        auto *view = new QWebEngineView(m_stack);
        view->setUrl(QUrl(QStringLiteral("qrc:/homepage.html")));
        connectView(view);

        m_stack->removeWidget(oldWidget);
        m_tabUrls.remove(oldWidget);
        m_lastAccessed.remove(oldWidget);
        m_isSuspended.remove(oldWidget);
        oldWidget->deleteLater();

        m_stack->insertWidget(index, view);
        m_tabUrls[view] = QUrl(QStringLiteral("titan://home"));
        m_lastAccessed[view] = QDateTime::currentDateTime();
        m_isSuspended[view] = false;
        m_tabBar->setTabText(index, QStringLiteral("New Tab"));
        m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/home.svg")));
        m_stack->setCurrentIndex(index);

        emit urlChanged(QUrl(QStringLiteral("titan://home")));
        emit titleChanged(QStringLiteral("New Tab"));
    }
    else if (urlStr == QStringLiteral("titan://settings") || urlStr == QStringLiteral("qrc:/settings.html")) {
        if (qobject_cast<SettingsWidget*>(oldWidget)) return;

        auto *settings = new SettingsWidget(m_stack);

        m_stack->removeWidget(oldWidget);
        m_tabUrls.remove(oldWidget);
        m_lastAccessed.remove(oldWidget);
        m_isSuspended.remove(oldWidget);
        oldWidget->deleteLater();

        m_stack->insertWidget(index, settings);
        m_tabUrls[settings] = QUrl(QStringLiteral("titan://settings"));
        m_tabBar->setTabText(index, QStringLiteral("Settings"));
        m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/settings.svg")));
        m_stack->setCurrentIndex(index);

        emit urlChanged(QUrl(QStringLiteral("titan://settings")));
        emit titleChanged(QStringLiteral("Settings"));
    }
    else {
        // External Web URL
        if (auto *existingView = qobject_cast<QWebEngineView*>(oldWidget)) {
            m_tabUrls[existingView] = url;
            existingView->setUrl(url);
        } else {
            // Replace native widget with QWebEngineView
            auto *newView = new QWebEngineView(m_stack);
            newView->setUrl(url);
            connectView(newView);

            m_stack->removeWidget(oldWidget);
            m_tabUrls.remove(oldWidget);
            m_lastAccessed.remove(oldWidget);
            m_isSuspended.remove(oldWidget);
            oldWidget->deleteLater();

            m_stack->insertWidget(index, newView);
            m_tabUrls[newView] = url;
            m_lastAccessed[newView] = QDateTime::currentDateTime();
            m_isSuspended[newView] = false;

            m_stack->setCurrentIndex(index);
            m_tabBar->setTabText(index, QStringLiteral("Loading..."));
            m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/home.svg")));

            emit urlChanged(url);
        }
    }
}

QWebEngineView *TabWidget::currentView() const
{
    return qobject_cast<QWebEngineView *>(m_stack->currentWidget());
}

QWebEngineView *TabWidget::viewAt(int index) const
{
    if (index >= 0 && index < m_stack->count()) {
        return qobject_cast<QWebEngineView *>(m_stack->widget(index));
    }
    return nullptr;
}

QWidget *TabWidget::widgetAt(int index) const
{
    if (index >= 0 && index < m_stack->count()) {
        return m_stack->widget(index);
    }
    return nullptr;
}

QString TabWidget::tabTitle(int index) const
{
    if (index >= 0 && index < m_tabBar->count()) {
        return m_tabBar->tabText(index);
    }
    return QString();
}

void TabWidget::connectView(QWebEngineView *view)
{
    connect(view, &QWebEngineView::urlChanged, this, [this, view](const QUrl &url) {
        QString u = url.toString();
        if (u.startsWith(QStringLiteral("titan://"))) {
            int idx = m_stack->indexOf(view);
            if (idx >= 0) {
                QMetaObject::invokeMethod(this, [this, idx, url]() {
                    loadInTab(idx, url);
                }, Qt::QueuedConnection);
            }
        } else if (u.contains(QStringLiteral("homepage.html")) || u.startsWith(QStringLiteral("qrc:"))) {
            m_tabUrls[view] = QUrl(QStringLiteral("titan://home"));
            if (view == m_stack->currentWidget()) emit urlChanged(QUrl(QStringLiteral("titan://home")));
        } else {
            m_tabUrls[view] = url;
            if (view == m_stack->currentWidget()) emit urlChanged(url);
        }
    });

    connect(view, &QWebEngineView::titleChanged, this, [this, view](const QString &title) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0 && idx < m_tabBar->count()) {
            QString displayTitle = title.trimmed().isEmpty() ? QStringLiteral("New Tab") : title.trimmed();
            m_tabBar->setTabText(idx, displayTitle.left(20));
            m_tabBar->setTabToolTip(idx, title);
        }
        if (view == m_stack->currentWidget()) emit titleChanged(title);
    });

    connect(view, &QWebEngineView::loadProgress, this, [this, view](int p) {
        if (view == m_stack->currentWidget()) emit loadProgress(p);
    });

    connect(view, &QWebEngineView::loadFinished, this, [this, view](bool ok) {
        if (view == m_stack->currentWidget()) emit loadFinished(ok);
    });

    connect(view, &QWebEngineView::iconChanged, this, [this, view](const QIcon &icon) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0 && idx < m_tabBar->count()) {
            if (!icon.isNull()) {
                m_tabBar->setTabIcon(idx, icon);
            }
        }
    });

    connect(view->page(), &QWebEnginePage::recentlyAudibleChanged, this, [this, view](bool audible) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0) updateTabAudioIcon(idx, audible, view->page()->isAudioMuted());
    });

    connect(view->page(), &QWebEnginePage::audioMutedChanged, this, [this, view](bool muted) {
        int idx = m_stack->indexOf(view);
        if (idx >= 0) updateTabAudioIcon(idx, view->page()->recentlyAudible(), muted);
    });
}

void TabWidget::updateTabAudioIcon(int index, bool audible, bool muted)
{
    if (index < 0 || index >= m_tabBar->count()) return;

    if (muted) {
        m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/volume-mute.svg")));
    } else if (audible) {
        m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/volume.svg")));
    } else {
        auto *v = viewAt(index);
        if (v && !v->icon().isNull()) {
            m_tabBar->setTabIcon(index, v->icon());
        } else {
            m_tabBar->setTabIcon(index, QIcon(QStringLiteral(":/icons/home.svg")));
        }
    }
}

void TabWidget::onCurrentTabChanged(int index)
{
    if (index >= 0 && index < m_stack->count()) {
        m_stack->setCurrentIndex(index);
        QWidget *w = m_stack->widget(index);
        if (w) {
            m_lastAccessed[w] = QDateTime::currentDateTime();
            if (auto *v = qobject_cast<QWebEngineView*>(w)) {
                emit urlChanged(v->url());
                emit titleChanged(v->title());
            } else {
                emit urlChanged(m_tabUrls.value(w, QUrl(QStringLiteral("titan://home"))));
                emit titleChanged(m_tabBar->tabText(index));
            }
        }
    }
}

void TabWidget::onTabCloseRequested(int index)
{
    if (m_tabBar->count() <= 1) return; // Keep at least one tab

    auto *w = m_stack->widget(index);
    m_tabBar->removeTab(index);
    if (w) {
        m_tabUrls.remove(w);
        m_lastAccessed.remove(w);
        m_isSuspended.remove(w);
        m_stack->removeWidget(w);
        w->deleteLater();
    }
    emit tabCountChanged(count());
}

void TabWidget::removeTab(int index)
{
    onTabCloseRequested(index);
}

QUrl TabWidget::tabUrl(int index) const
{
    if (index >= 0 && index < m_stack->count()) {
        QWidget *w = m_stack->widget(index);
        if (auto *v = qobject_cast<QWebEngineView*>(w)) return v->url();
        return m_tabUrls.value(w, QUrl(QStringLiteral("titan://home")));
    }
    return QUrl();
}

void TabWidget::duplicateTab(int index)
{
    QUrl url = tabUrl(index);
    newTab(url);
}

void TabWidget::reloadTab(int index)
{
    if (auto *v = viewAt(index)) {
        v->reload();
    }
}

void TabWidget::closeOtherTabs(int index)
{
    for (int i = count() - 1; i >= 0; --i) {
        if (i != index && count() > 1) {
            onTabCloseRequested(i);
        }
    }
}

void TabWidget::closeTabsToRight(int index)
{
    for (int i = count() - 1; i > index; --i) {
        if (count() > 1) {
            onTabCloseRequested(i);
        }
    }
}

void TabWidget::toggleMuteTab(int index)
{
    if (auto *v = viewAt(index)) {
        bool currentMute = v->page()->isAudioMuted();
        v->page()->setAudioMuted(!currentMute);
    }
}

void TabWidget::checkAndSuspendInactiveTabs()
{
    if (!m_memorySaverEnabled) return;

    QDateTime now = QDateTime::currentDateTime();
    auto *active = currentView();

    for (int i = 0; i < m_stack->count(); ++i) {
        auto *v = qobject_cast<QWebEngineView *>(m_stack->widget(i));
        if (!v || v == active) continue;

        if (v->page()->recentlyAudible()) continue;

        QDateTime lastUsed = m_lastAccessed.value(v, now);
        if (lastUsed.secsTo(now) > 900) { // 15 min inactive
            if (!m_isSuspended.value(v, false)) {
                m_isSuspended[v] = true;
                v->page()->runJavaScript(QStringLiteral("if (window.onTabSuspend) window.onTabSuspend();"));
            }
        }
    }
}

void TabWidget::onTabBarContextMenu(const QPoint &pos)
{
    int index = m_tabBar->tabAt(pos);
    if (index < 0) return;

    auto *menu = new QMenu(m_tabBar);
    menu->setStyleSheet(QStringLiteral(R"(
        QMenu {
            background: #090e1c;
            border: 1px solid rgba(56, 189, 248, 0.25);
            border-radius: 8px;
            padding: 5px;
            color: #e2e8f0;
            font-size: 12px;
        }
        QMenu::item {
            padding: 6px 18px 6px 10px;
            border-radius: 5px;
        }
        QMenu::item:selected {
            background: rgba(56, 189, 248, 0.15);
            color: #38bdf8;
        }
    )"));

    auto *reloadAct = menu->addAction(QStringLiteral("Reload Tab"));
    connect(reloadAct, &QAction::triggered, this, [this, index]{ reloadTab(index); });

    auto *dupAct = menu->addAction(QStringLiteral("Duplicate Tab"));
    connect(dupAct, &QAction::triggered, this, [this, index]{ duplicateTab(index); });

    if (auto *v = viewAt(index)) {
        bool isMuted = v->page()->isAudioMuted();
        auto *muteAct = menu->addAction(isMuted ? QStringLiteral("Unmute Tab") : QStringLiteral("Mute Tab"));
        connect(muteAct, &QAction::triggered, this, [this, index]{ toggleMuteTab(index); });
    }

    menu->addSeparator();

    auto *closeAct = menu->addAction(QStringLiteral("Close Tab"));
    connect(closeAct, &QAction::triggered, this, [this, index]{ onTabCloseRequested(index); });

    if (count() > 1) {
        auto *closeOthersAct = menu->addAction(QStringLiteral("Close Other Tabs"));
        connect(closeOthersAct, &QAction::triggered, this, [this, index]{ closeOtherTabs(index); });

        if (index < count() - 1) {
            auto *closeRightAct = menu->addAction(QStringLiteral("Close Tabs to the Right"));
            connect(closeRightAct, &QAction::triggered, this, [this, index]{ closeTabsToRight(index); });
        }
    }

    menu->popup(m_tabBar->mapToGlobal(pos));
}
