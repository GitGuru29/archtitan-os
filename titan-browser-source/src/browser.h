#pragma once
#include <QMainWindow>
#include <QWebEngineView>
#include <QLineEdit>
#include <QToolBar>
#include <QProgressBar>
#include <QLabel>

class TabWidget;
class AddressBar;

class Browser : public QMainWindow
{
    Q_OBJECT
public:
    explicit Browser(QWidget *parent = nullptr);
    void loadUrl(const QUrl &url);

private slots:
    void onUrlChanged(const QUrl &url);
    void onLoadProgress(int progress);
    void onLoadFinished(bool ok);
    void onTitleChanged(const QString &title);
    void onAddressEntered();
    void newTab(const QUrl &url = QUrl());
    void closeCurrentTab();
    void navigateBack();
    void navigateForward();
    void reload();
    void onTabChanged(int index);

private:
    void setupUi();
    void setupToolbar();
    void applyTheme();
    QWebEngineView *currentView() const;

    TabWidget   *m_tabs       = nullptr;
    AddressBar  *m_addressBar = nullptr;
    QAction     *m_backAction    = nullptr;
    QAction     *m_forwardAction = nullptr;
    QAction     *m_reloadAction  = nullptr;
    QProgressBar *m_progress  = nullptr;
};
