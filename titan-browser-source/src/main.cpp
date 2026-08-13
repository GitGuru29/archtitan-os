#include "browser.h"
#include "adblocker.h"
#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineCookieStore>
#include <QNetworkCookie>
#include <QDateTime>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(QStringLiteral("TitanBrowser"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("ArchTitan"));
    QApplication::setDesktopFileName(QStringLiteral("titanbrowser"));

    // Force XCB (XWayland) to avoid Wayland GBM/Vulkan crash on NVIDIA systems.
    // QtWebEngine's Chromium subprocess segfaults when GBM fails and Vulkan
    // renderer can't initialize on hybrid NVIDIA+Intel laptops.
    if (qgetenv("QT_QPA_PLATFORM").isEmpty())
        qputenv("QT_QPA_PLATFORM", "xcb");

    // Disable GPU + sandbox — required for standalone QtWebEngine binaries.
    // SwiftShader (CPU renderer) is used as fallback; plenty fast for browsing.
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--no-sandbox "
            "--disable-gpu "
            "--disable-gpu-sandbox "
            "--disable-dev-shm-usage "
            "--lang=en-US "
            "--disable-features=VaapiVideoDecoder,UseChromeOSDirectVideoDecoder");

    QApplication app(argc, argv);

    // ── Enforce Default English (en-US) Locale ────────────────────────────
    auto *profile = QWebEngineProfile::defaultProfile();
    profile->setHttpAcceptLanguage(QStringLiteral("en-US,en;q=0.9"));

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

    Browser w(adBlocker);
    w.show();

    return app.exec();
}
