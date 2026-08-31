#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QLocalSocket>
#include <unistd.h>
#include <csignal>

#include <LayerShellQt/window.h>

#include "mpriscontroller.h"
#include "mediaislandcontroller.h"
#include "audioprofilecontroller.h"
#include "systemcontroller.h"
#include "ipcserver.h"

int main(int argc, char *argv[])
{
    // Check for CLI command-mode (e.g. titan-media-hud --toggle)
    if (argc > 1) {
        QString arg = QString::fromLatin1(argv[1]).trimmed().toLower();
        QString cmd;
        if (arg == "--toggle" || arg == "-t") cmd = "toggle";
        else if (arg == "--show" || arg == "-s") cmd = "show";
        else if (arg == "--hide" || arg == "-h" || arg == "--dismiss") cmd = "dismiss";
        else if (arg == "--expand" || arg == "-e") cmd = "expand";
        else if (arg == "--collapse" || arg == "-c" || arg == "--compact") cmd = "compact";

        if (!cmd.isEmpty()) {
            QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
            if (runtimeDir.isEmpty()) {
                runtimeDir = QStringLiteral("/run/user/%1").arg(getuid());
            }
            QString sockPath = runtimeDir + QStringLiteral("/titan-media-hud.sock");
            QLocalSocket socket;
            socket.connectToServer(sockPath);
            if (socket.waitForConnected(300)) {
                socket.write((cmd + "\n").toUtf8());
                socket.flush();
                socket.waitForBytesWritten(300);
                return 0;
            }
        }
    }

    // Wayland + transparency
    qputenv("QT_QPA_PLATFORM", "wayland;xcb");
    qputenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1");

    QGuiApplication app(argc, argv);
    app.setApplicationName("titan-media-hud");
    app.setDesktopFileName("titan-media-hud");
    app.setApplicationDisplayName("ArchTitan Media Island");
    app.setOrganizationName("ArchTitan");

    // Graceful signal handling
    std::signal(SIGINT, [](int) { QGuiApplication::quit(); });
    std::signal(SIGTERM, [](int) { QGuiApplication::quit(); });

    // Create controllers
    MprisController mprisCtrl;
    MediaIslandController islandCtrl;
    AudioProfileController audioCtrl;
    SystemController systemCtrl;

    // Connect track changes to auto-popup
    QObject::connect(&mprisCtrl, &MprisController::trackChanged,
                     &islandCtrl, &MediaIslandController::autoPopup);

    // Register types for enum access in QML
    qmlRegisterUncreatableType<MediaIslandController>("ArchTitan.Media", 1, 0, "MediaIslandController",
                                                       "Use the Island singleton");

    // Register QML singletons
    qmlRegisterSingletonInstance<MprisController>("ArchTitan.Media", 1, 0, "Mpris", &mprisCtrl);
    qmlRegisterSingletonInstance<MediaIslandController>("ArchTitan.Media", 1, 0, "Island", &islandCtrl);
    qmlRegisterSingletonInstance<AudioProfileController>("ArchTitan.Media", 1, 0, "AudioProfile", &audioCtrl);
    qmlRegisterSingletonInstance<SystemController>("ArchTitan.Media", 1, 0, "System", &systemCtrl);

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/qml/MediaIsland.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url, &islandCtrl](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
            return;
        }

        // Configure LayerShell on the root window
        QQuickWindow *window = qobject_cast<QQuickWindow *>(obj);
        if (!window) return;

        auto *layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            // Anchor to top edge, centered horizontally (left+right+top = centered top)
            layerWindow->setAnchors(LayerShellQt::Window::AnchorTop);
            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);

            // Position just below Waybar: 4px margin + 34px height + 4px gap = 42px
            layerWindow->setMargins(QMargins(0, 42, 0, 0));

            // Don't reserve exclusive space — this floats over content
            layerWindow->setExclusiveZone(0);

            // Get keyboard when visible (for ESC key)
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);

            layerWindow->setScope("titan-media-island");
        }

        window->show();
    }, Qt::QueuedConnection);

    engine.load(url);

    // Start IPC server for SUPER+M toggle
    IpcServer ipcServer(&islandCtrl);
    if (!ipcServer.start()) {
        qWarning() << "Failed to start IPC server — SUPER+M toggle will not work";
    }

    return app.exec();
}
