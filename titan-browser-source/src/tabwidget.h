#pragma once
#include <QObject>
#include <QTabBar>
#include <QStackedWidget>
#include <QWebEngineView>
#include <QUrl>
#include <QDateTime>
#include <QMap>

class NewTabWidget;
class SettingsWidget;

class TabWidget : public QObject
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);

    QTabBar *tabBar() const { return m_tabBar; }
    QStackedWidget *contentStack() const { return m_stack; }

    QWidget *newTab(const QUrl &url = QUrl());
    void loadInCurrentTab(const QUrl &url);
    void loadInTab(int index, const QUrl &url);

    QWebEngineView *currentView() const;
    QWebEngineView *viewAt(int index) const;
    QWidget *widgetAt(int index) const;

    int count() const { return m_tabBar->count(); }
    int currentIndex() const { return m_tabBar->currentIndex(); }
    void removeTab(int index);
    void setCurrentIndex(int index) { m_tabBar->setCurrentIndex(index); }
    QUrl tabUrl(int index) const;
    QString tabTitle(int index) const;

    void duplicateTab(int index);
    void reloadTab(int index);
    void closeOtherTabs(int index);
    void closeTabsToRight(int index);
    void toggleMuteTab(int index);

    // Profile integration
    void reloadAllTabsForProfile();

    // Memory Saver
    void setMemorySaverEnabled(bool enabled) { m_memorySaverEnabled = enabled; }
    bool isMemorySaverEnabled() const { return m_memorySaverEnabled; }
    void checkAndSuspendInactiveTabs();

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void loadProgress(int progress);
    void loadFinished(bool ok);
    void tabCountChanged(int count);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onTabBarContextMenu(const QPoint &pos);

private:
    void connectView(QWebEngineView *view);
    void updateTabAudioIcon(int index, bool audible, bool muted);

    QTabBar        *m_tabBar = nullptr;
    QStackedWidget *m_stack  = nullptr;

    bool m_memorySaverEnabled = true;
    QMap<QWidget*, QUrl>       m_tabUrls;
    QMap<QWidget*, QDateTime>  m_lastAccessed;
    QMap<QWidget*, bool>       m_isSuspended;
};
