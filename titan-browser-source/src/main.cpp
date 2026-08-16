#include "browser.h"
#include "adblocker.h"
#include "profilemanager.h"
#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(QStringLiteral("TitanBrowser"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("ArchTitan"));
    QApplication::setDesktopFileName(QStringLiteral("titanbrowser"));

    // Force XCB (XWayland) to avoid Wayland GBM/Vulkan crash on NVIDIA systems.
    if (qgetenv("QT_QPA_PLATFORM").isEmpty())
        qputenv("QT_QPA_PLATFORM", "xcb");

    // Performance-optimized Chromium flags:
    // Enables smooth scrolling, disk cache optimization, font antialiasing, and CPU fallback
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--no-sandbox "
            "--disable-gpu "
            "--disable-gpu-sandbox "
            "--disable-dev-shm-usage "
            "--lang=en-US "
            "--enable-smooth-scrolling "
            "--disk-cache-size=536870912 "
            "--disable-features=VaapiVideoDecoder,UseChromeOSDirectVideoDecoder");

    QApplication app(argc, argv);

    // ── Enforce Default English (en-US) Locale & Optimize Profile ─────────
    auto *profile = QWebEngineProfile::defaultProfile();
    profile->setHttpAcceptLanguage(QStringLiteral("en-US,en;q=0.9"));
    profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    profile->setHttpCacheMaximumSize(512 * 1024 * 1024); // 512 MB disk cache
    profile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);

    // Set storage locations in user config directory
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/titanbrowser");
    QDir().mkpath(dataPath);
    profile->setPersistentStoragePath(dataPath);

    // Profile-level WebEngine Settings for smoothness and responsiveness
    auto *settings = profile->settings();
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    settings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    settings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);
    settings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

    // Set English preferences on YouTube & Google
    auto *cookieStore = profile->cookieStore();
    QNetworkCookie ytPref(QByteArrayLiteral("PREF"), QByteArrayLiteral("f6=40000000&hl=en&gl=US"));
    ytPref.setDomain(QStringLiteral(".youtube.com"));
    ytPref.setPath(QStringLiteral("/"));
    ytPref.setExpirationDate(QDateTime::currentDateTime().addYears(5));
    cookieStore->setCookie(ytPref, QUrl(QStringLiteral("https://www.youtube.com")));

    QNetworkCookie googlePref(QByteArrayLiteral("PREF"), QByteArrayLiteral("hl=en&gl=US"));
    googlePref.setDomain(QStringLiteral(".google.com"));
    googlePref.setPath(QStringLiteral("/"));
    googlePref.setExpirationDate(QDateTime::currentDateTime().addYears(5));
    cookieStore->setCookie(googlePref, QUrl(QStringLiteral("https://www.google.com")));

    // ── Install TitanShield Ad Blocker & Content Engine ──────────────────
    // Must be set BEFORE any QWebEngineView is created.
    auto *adBlocker = new AdBlocker(profile);
    profile->setUrlRequestInterceptor(adBlocker);
    adBlocker->installContentScript(profile);

    // Optionally load a user filter list from disk
    const QString userList = QDir::homePath() + QStringLiteral("/.config/titanbrowser/filters.txt");
    if (QFile::exists(userList))
        adBlocker->loadFilterList(userList);

    // ── Initialize Multi-Profile Manager ────────────────────────────────
    ProfileManager::instance().initialize(adBlocker);

    Browser w(adBlocker);
    w.show();

    return app.exec();
}
