#pragma once
#include <QTabWidget>
#include <QWebEngineView>

class TabWidget : public QTabWidget
{
    Q_OBJECT
public:
    explicit TabWidget(QWidget *parent = nullptr);
    QWebEngineView *newTab(const QUrl &url);
    QWebEngineView *currentView() const;

signals:
    void urlChanged(const QUrl &url);
    void titleChanged(const QString &title);
    void loadProgress(int progress);
    void loadFinished(bool ok);

private slots:
    void onTabCloseRequested(int index);

private:
    void connectView(QWebEngineView *view);
};
