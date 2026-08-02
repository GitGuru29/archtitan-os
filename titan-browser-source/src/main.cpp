#include "browser.h"
#include <QApplication>
#include <QWebEngineProfile>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(QStringLiteral("TitanBrowser"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("ArchTitan"));
    QApplication::setDesktopFileName(QStringLiteral("titanbrowser"));

    // Enable Wayland/Ozone
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--enable-features=WebRTC,VaapiVideoDecoder "
            "--use-gl=egl");

    QApplication app(argc, argv);

    Browser w;
    w.show();

    return app.exec();
}
