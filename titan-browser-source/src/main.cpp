#include "browser.h"
#include <QApplication>
#include <QWebEngineProfile>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(QStringLiteral("TitanBrowser"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("ArchTitan"));
    QApplication::setDesktopFileName(QStringLiteral("titanbrowser"));

    // Disable sandbox (required for standalone/non-packaged QtWebEngine binaries)
    // Vulkan fallback: disable GPU sandbox and let Chromium pick the renderer
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--no-sandbox "
            "--disable-gpu-sandbox "
            "--disable-features=VaapiVideoDecoder");

    QApplication app(argc, argv);

    Browser w;
    w.show();

    return app.exec();
}
