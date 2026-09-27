#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <csignal>
#include <iostream>

#include <LayerShellQt/window.h>

#include "ipcserver.h"
#include "systemcontroller.h"
#include "networkcontroller.h"
#include "bluetoothcontroller.h"
#include "audiocontroller.h"
#include "nightlightcontroller.h"
#include "notificationserver.h"

int main(int argc, char *argv[])
{
    // Enable Wayland native windowing & transparency
    qputenv("QT_QPA_PLATFORM", "wayland;xcb");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");

    // Pre-check CLI arguments for fast socket command dispatch
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--toggle" || arg == "-t") {
            if (IpcServer::sendCommand("TOGGLE")) {
                return 0; // Handled by running daemon!
            }
            break;
        } else if (arg == "--show") {
            if (IpcServer::sendCommand("SHOW")) {
                return 0;
            }
            break;
        } else if (arg == "--hide") {
            if (IpcServer::sendCommand("HIDE")) {
                return 0;
            }
            return 0;
        }
    }

    QGuiApplication app(argc, argv);
    app.setApplicationName("titan-quicksettings");
    app.setApplicationDisplayName("ArchTitan Quick Settings");
    app.setOrganizationName("ArchTitan");

    std::signal(SIGINT, [](int) { QGuiApplication::quit(); });
    std::signal(SIGTERM, [](int) { QGuiApplication::quit(); });

    QCommandLineParser parser;
    parser.setApplicationDescription("ArchTitan Quick Settings & Notification Center");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption daemonOption({"d", "daemon"}, "Start in background daemon mode (hidden)");
    QCommandLineOption toggleOption({"t", "toggle"}, "Toggle visibility of the quick settings drawer");
    QCommandLineOption showOption("show", "Show the quick settings drawer");
    QCommandLineOption hideOption("hide", "Hide the quick settings drawer");

    parser.addOption(daemonOption);
    parser.addOption(toggleOption);
    parser.addOption(showOption);
    parser.addOption(hideOption);
    parser.process(app);

    bool startHidden = parser.isSet(daemonOption);

    // Initialize backend controllers
    IpcServer ipc;
    ipc.startServer();

    SystemController systemCtrl;
    NetworkController netCtrl;
    BluetoothController btCtrl;
    AudioController audioCtrl;
    NightLightController nightCtrl;
    NotificationServer notifServer;

    QQmlApplicationEngine engine;
    auto *context = engine.rootContext();
    context->setContextProperty("systemCtrl", &systemCtrl);
    context->setContextProperty("networkCtrl", &netCtrl);
    context->setContextProperty("bluetoothCtrl", &btCtrl);
    context->setContextProperty("audioCtrl", &audioCtrl);
    context->setContextProperty("nightLightCtrl", &nightCtrl);
    context->setContextProperty("notifServer", &notifServer);

    const QUrl url(QStringLiteral("qrc:/qml/QuickSettingsWindow.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, &ipc, startHidden](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
            return;
        }

        QQuickWindow *window = qobject_cast<QQuickWindow *>(obj);
        if (!window) return;

        // Configure LayerShell on root window
        auto *layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            // Anchor only to Top+Right — no AnchorBottom so the window
            // uses its natural QML height instead of being stretched full-screen.
            // Quickshell bar occupies y=0..73 (73px). Add 4px breathing = 77px total.
            layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorRight));
            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerWindow->setMargins(QMargins(0, 77, 6, 0)); // 77px = 73px bar + 4px gap
            layerWindow->setExclusiveZone(0);
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
            layerWindow->setScope("titan-quicksettings");
        }

        QObject::connect(&ipc, &IpcServer::toggleRequested, window, [window]() {
            QMetaObject::invokeMethod(window, "toggleDrawer");
        });
        QObject::connect(&ipc, &IpcServer::showRequested, window, [window]() {
            QMetaObject::invokeMethod(window, "openDrawer");
        });
        QObject::connect(&ipc, &IpcServer::hideRequested, window, [window]() {
            QMetaObject::invokeMethod(window, "closeDrawer");
        });

        if (!startHidden) {
            QMetaObject::invokeMethod(window, "openDrawer");
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
