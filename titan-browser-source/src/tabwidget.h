#pragma once
#include <QObject>
#include <QTabBar>
#include <QStackedWidget>
#include <QWebEngineView>
#include <QUrl>

class TabWidget : public QObject
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);

    QTabBar *tabBar() const { return m_tabBar; }
    QStackedWidget *contentStack() const { return m_stack; }

    QWebEngineView *newTab(const QUrl &url = QUrl());
    QWebEngineView *currentView() const;
    int count() const { return m_tabBar->count(); }
    int currentIndex() const { return m_tabBar->currentIndex(); }
    void removeTab(int index);

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void loadProgress(int progress);
    void loadFinished(bool ok);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);

private:
    void connectView(QWebEngineView *view);

    QTabBar        *m_tabBar = nullptr;
    QStackedWidget *m_stack  = nullptr;
};
