#pragma once
#include <QMainWindow>
#include <QWebEngineView>
#include <QLineEdit>
#include <QProgressBar>
#include <QTabBar>
#include <QStackedWidget>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QTimer>
#include <QDir>
#include <QFile>

class TabWidget;
class AddressBar;
class AdBlocker;


class Browser : public QMainWindow
{
    Q_OBJECT
public:
    explicit Browser(AdBlocker *adBlocker = nullptr, QWidget *parent = nullptr);
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

    // Rail & Chrome Actions
    void onHomeClicked();
    void onAICoreClicked();
    void onSpacesClicked();
    void onHistoryClicked();
    void onDownloadsClicked();
    void onExtensionsClicked();
    void onSettingsClicked();
    void onShieldClicked();
    void onProfileClicked();
    void onMenuClicked();
    void onThemeToggleClicked();

private:
    void setupUi();
    void setActiveRailButton(QToolButton *btn);
    void showStatusNotification(const QString &title, const QString &message);
    QWebEngineView *currentView() const;

    TabWidget      *m_tabs             = nullptr;
    AddressBar     *m_addressBar       = nullptr;
    QProgressBar   *m_progress         = nullptr;

    // Compact Navigation Rail Buttons
    QToolButton    *m_railHomeBtn      = nullptr;
    QToolButton    *m_railAiBtn        = nullptr;
    QToolButton    *m_railSpacesBtn    = nullptr;
    QToolButton    *m_railHistoryBtn   = nullptr;
    QToolButton    *m_railDlBtn        = nullptr;
    QToolButton    *m_railExtBtn       = nullptr;
    QToolButton    *m_railSettingsBtn  = nullptr;
    QToolButton    *m_activeRailBtn    = nullptr;

    AdBlocker      *m_adBlocker      = nullptr;
    QTimer         *m_statsTimer     = nullptr;
    QToolButton    *m_shieldBadgeBtn = nullptr;  // live-updated blocked count

    bool            m_isDarkMode     = true;
};
