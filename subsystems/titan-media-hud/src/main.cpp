#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <csignal>
#include "mpriscontroller.h"

int main(int argc, char *argv[])
{
    // Enable Wayland native features & transparency
    qputenv("QT_QPA_PLATFORM", "wayland;xcb");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");

    QGuiApplication app(argc, argv);
    app.setApplicationName("titan-media-hud");
    app.setDesktopFileName("titan-media-hud");
    app.setApplicationDisplayName("ArchTitan Media HUD");
    app.setOrganizationName("ArchTitan");

    std::signal(SIGINT, [](int) { QGuiApplication::quit(); });
    std::signal(SIGTERM, [](int) { QGuiApplication::quit(); });

    MprisController mprisCtrl;

    // Register singleton for declarative QML imports and context property
    qmlRegisterSingletonInstance<MprisController>("ArchTitan.Media", 1, 0, "Mpris", &mprisCtrl);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("mpris", &mprisCtrl);

    const QUrl url(QStringLiteral("qrc:/qml/MediaHudWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
