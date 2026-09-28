#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QCommandLineParser>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <csignal>
#include <iostream>

#include <LayerShellQt/window.h>

// ── Config helpers ────────────────────────────────────────────────────────────
// Edit ~/.config/titan-quicksettings/config.ini to adjust layout without
// recompiling. Restart the daemon after saving.
static QString configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
           + "/config.ini";
}

static void ensureDefaultConfig()
{
    QString path = configPath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!QFile::exists(path)) {
        QSettings cfg(path, QSettings::IniFormat);
        cfg.beginGroup("layout");
        cfg.setValue("top_margin",    0);   // px above panel (0 = use exclusive zone)
        cfg.setValue("right_margin",  6);   // px from right screen edge
        cfg.setValue("panel_width",   420); // panel width in px
        cfg.setValue("panel_height",  860); // panel height in px
        cfg.endGroup();
        cfg.sync();
        std::cout << "[titan-qs] Created default config: "
                  << path.toStdString() << "\n";
    }
}

struct LayoutConfig {
    int topMargin;
    int rightMargin;
    int panelWidth;
    int panelHeight;
};

static LayoutConfig readConfig()
{
    QSettings cfg(configPath(), QSettings::IniFormat);
    cfg.beginGroup("layout");
    LayoutConfig lc;
    lc.topMargin   = cfg.value("top_margin",    0).toInt();
    lc.rightMargin = cfg.value("right_margin",  6).toInt();
    lc.panelWidth  = cfg.value("panel_width",   420).toInt();
    lc.panelHeight = cfg.value("panel_height",  860).toInt();
    cfg.endGroup();
    return lc;
}

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
                return 0;
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
        } else if (arg == "--set-margin" && i + 1 < argc) {
            // Quick CLI override: titan-quicksettings --set-margin top=N
            // Writes to config.ini so it persists on next daemon restart.
            QString kv = QString::fromUtf8(argv[++i]);
            QStringList parts = kv.split('=');
            if (parts.size() == 2) {
                ensureDefaultConfig();
                QSettings cfg(configPath(), QSettings::IniFormat);
                cfg.beginGroup("layout");
                QString key = parts[0].trimmed();
                int val = parts[1].trimmed().toInt();
                if (key == "top")    cfg.setValue("top_margin",    val);
                if (key == "right")  cfg.setValue("right_margin",  val);
                if (key == "width")  cfg.setValue("panel_width",   val);
                if (key == "height") cfg.setValue("panel_height",  val);
                cfg.endGroup(); cfg.sync();
                std::cout << "[titan-qs] Set " << key.toStdString()
                          << " = " << val << " in " << configPath().toStdString()
                          << ". Restart the daemon to apply.\n";
            }
            return 0;
        } else if (arg == "--config") {
            ensureDefaultConfig();
            std::cout << "Config: " << configPath().toStdString() << "\n";
            auto lc = readConfig();
            std::cout << "  top_margin    = " << lc.topMargin   << "\n"
                      << "  right_margin  = " << lc.rightMargin << "\n"
                      << "  panel_width   = " << lc.panelWidth  << "\n"
                      << "  panel_height  = " << lc.panelHeight << "\n";
            return 0;
        }
    }

    ensureDefaultConfig();

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
            auto lc = readConfig();
            // Anchor Top+Right only (no Bottom) so the window uses its natural
            // QML height. top_margin=0 means: let the bar's exclusive zone
            // position us automatically. Increase top_margin in config.ini
            // if you see a gap, decrease if the panel overlaps the bar.
            layerWindow->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorRight));
            layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
            layerWindow->setMargins(QMargins(0, lc.topMargin, lc.rightMargin, 0));
            layerWindow->setExclusiveZone(-1); // respect bar's exclusive zone
            layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
            layerWindow->setScope("titan-quicksettings");
            // Apply panel dimensions from config
            window->setWidth(lc.panelWidth);
            window->setHeight(lc.panelHeight);
            std::cout << "[titan-qs] Layout: top=" << lc.topMargin
                      << " right=" << lc.rightMargin
                      << " size=" << lc.panelWidth << "x" << lc.panelHeight << "\n";
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
