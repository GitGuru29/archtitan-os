#include "browser.h"
#include "adblocker.h"
#include <QApplication>
#include <QWebEngineProfile>

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
            "--disable-features=VaapiVideoDecoder,UseChromeOSDirectVideoDecoder");

    QApplication app(argc, argv);

    // ── Install TitanShield Ad Blocker & Content Engine ──────────────────
    // Must be set BEFORE any QWebEngineView is created.
    auto *adBlocker = new AdBlocker(QWebEngineProfile::defaultProfile());
    QWebEngineProfile::defaultProfile()->setUrlRequestInterceptor(adBlocker);
    adBlocker->installContentScript(QWebEngineProfile::defaultProfile());

    // Optionally load a user filter list from disk
    const QString userList = QDir::homePath() + QStringLiteral("/.config/titanbrowser/filters.txt");
    if (QFile::exists(userList))
        adBlocker->loadFilterList(userList);

    Browser w(adBlocker);
    w.show();

    return app.exec();
}
