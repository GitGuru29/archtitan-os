#pragma once
#include <QMainWindow>
#include <QWebEngineView>
#include <QLineEdit>
#include <QProgressBar>
#include <QTabBar>
#include <QStackedWidget>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QList>
#include <QVBoxLayout>
#include <QHBoxLayout>

class TabWidget;
class AddressBar;
class AdBlocker;

struct BookmarkItem {
    QString   title;
    QUrl      url;
    QDateTime dateAdded;
};

struct HistoryRecord {
    QString   title;
    QUrl      url;
    QDateTime timestamp;
};

class Browser : public QMainWindow
{
    Q_OBJECT
public:
    explicit Browser(AdBlocker *adBlocker = nullptr, QWidget *parent = nullptr);
    void loadUrl(const QUrl &url);

    // Bookmarks and history helpers
    bool isBookmarked(const QUrl &url) const;
    void addBookmark(const QString &title, const QUrl &url);
    void removeBookmark(const QUrl &url);
    void addHistoryRecord(const QString &title, const QUrl &url);

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
    void reloadOrStop();
    void onTabChanged(int index);
    void onBookmarkClicked(const QUrl &url);

    // Navigation rail & chrome flyouts
    void onHomeClicked();
    void onAICoreClicked();
    void onSpacesClicked();
    void onHistoryClicked();
    void onDownloadsClicked();
    void onBookmarksClicked();
    void onExtensionsClicked();
    void onSettingsClicked();
    void onShieldClicked();
    void onProfileClicked();
    void onMenuClicked();
    void onThemeToggleClicked();

    // Find in Page
    void toggleFindInPage();
    void findNext();
    void findPrevious();
    void onFindTextChanged(const QString &text);
    void closeFindBar();

    // Zoom
    void zoomIn();
    void zoomOut();
    void zoomReset();

private:
    void setupUi();
    void setupFindBar(QVBoxLayout *layout);
    void setupToastWidget();
    void setActiveRailButton(QToolButton *btn);
    void showToast(const QString &message, int durationMs = 2800);
    void updateNavigationButtons();
    void loadPersistentUserData();
    void savePersistentUserData();
    QWebEngineView *currentView() const;

    TabWidget      *m_tabs             = nullptr;
    AddressBar     *m_addressBar       = nullptr;
    QProgressBar   *m_progress         = nullptr;

    // Navigation buttons
    QToolButton    *m_backBtn          = nullptr;
    QToolButton    *m_fwdBtn           = nullptr;
    QToolButton    *m_reloadBtn        = nullptr;

    // Compact Navigation Rail Buttons
    QToolButton    *m_railHomeBtn      = nullptr;
    QToolButton    *m_railAiBtn        = nullptr;
    QToolButton    *m_railSpacesBtn    = nullptr;
    QToolButton    *m_railBookmarksBtn = nullptr;
    QToolButton    *m_railHistoryBtn   = nullptr;
    QToolButton    *m_railDlBtn        = nullptr;
    QToolButton    *m_railExtBtn       = nullptr;
    QToolButton    *m_railSettingsBtn  = nullptr;
    QToolButton    *m_activeRailBtn    = nullptr;

    // TitanShield & Status
    AdBlocker      *m_adBlocker        = nullptr;
    QTimer         *m_statsTimer       = nullptr;
    QToolButton    *m_shieldBadgeBtn   = nullptr;

    // Find in page bar
    QWidget        *m_findBar          = nullptr;
    QLineEdit      *m_findEdit         = nullptr;
    QLabel         *m_findCountLabel   = nullptr;
    QToolButton    *m_findPrevBtn      = nullptr;
    QToolButton    *m_findNextBtn      = nullptr;
    QToolButton    *m_findCloseBtn     = nullptr;

    // Floating toast overlay
    QLabel         *m_toastLabel       = nullptr;
    QTimer         *m_toastTimer       = nullptr;

    // Persistent storage
    QList<BookmarkItem>   m_bookmarks;
    QList<HistoryRecord>  m_history;
    QString               m_activeSpace = QStringLiteral("Development");

    bool            m_isDarkMode       = true;
    bool            m_isLoading        = false;
    qreal           m_zoomFactor       = 1.0;
};
