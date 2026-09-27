#include <QGuiApplication>
#include <QQuickView>
#include <QQmlContext>
#include <QQuickItem>
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

    QQuickView view;
    view.setColor(QColor(Qt::transparent));

    // Configure LayerShell BEFORE setSource creates platform surface
    auto *layerWindow = LayerShellQt::Window::get(&view);
    if (layerWindow) {
        layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorRight | LayerShellQt::Window::AnchorBottom));
        layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
        layerWindow->setMargins(QMargins(0, 36, 6, 6)); // Top offset for Waybar height (36px), 6px right/bottom margin
        layerWindow->setExclusiveZone(0);
        layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        layerWindow->setScope("titan-quicksettings");
    }

    auto *context = view.rootContext();
    context->setContextProperty("systemCtrl", &systemCtrl);
    context->setContextProperty("networkCtrl", &netCtrl);
    context->setContextProperty("bluetoothCtrl", &btCtrl);
    context->setContextProperty("audioCtrl", &audioCtrl);
    context->setContextProperty("nightLightCtrl", &nightCtrl);
    context->setContextProperty("notifServer", &notifServer);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/QuickSettingsWindow.qml")));
    view.setResizeMode(QQuickView::SizeRootObjectToView);

    QObject *rootObj = view.rootObject();
    if (rootObj) {
        QObject::connect(&ipc, &IpcServer::toggleRequested, rootObj, [rootObj]() {
            QMetaObject::invokeMethod(rootObj, "toggleDrawer");
        });
        QObject::connect(&ipc, &IpcServer::showRequested, rootObj, [rootObj]() {
            QMetaObject::invokeMethod(rootObj, "openDrawer");
        });
        QObject::connect(&ipc, &IpcServer::hideRequested, rootObj, [rootObj]() {
            QMetaObject::invokeMethod(rootObj, "closeDrawer");
        });

        if (!startHidden) {
            QMetaObject::invokeMethod(rootObj, "openDrawer");
        }
    }

    view.show();

    return app.exec();
}
