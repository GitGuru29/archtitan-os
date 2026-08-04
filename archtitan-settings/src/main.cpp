#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFontDatabase>
#include <QIcon>
#include <QFile>
#include <QSurfaceFormat>

#include "settingsbackend.h"
#include "systeminfo.h"
#include "networkmanager.h"
#include "displaymanager.h"
#include "audiobackend.h"
#include "wallpapermanager.h"

int main(int argc, char *argv[]) {
    // ── Enable Wayland Alpha/Transparency ────────────────────────
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

    app.setApplicationName("ArchTitan Settings");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ArchTitan");
    app.setOrganizationDomain("archtitan.os");
    // Icon is optional, skip if resource missing
    // app.setWindowIcon(QIcon(":/assets/icons/LOGO.png"));

    QQuickStyle::setStyle("Basic");

    // ── Load Inter font if available ─────────────────────────────
    // Try multiple common font paths on Arch Linux
    QStringList fontDirs = {
        "/usr/share/fonts/inter",
        "/usr/share/fonts/TTF",
        "/usr/share/fonts/truetype/inter"
    };
    QStringList fontWeights = {"Regular", "Medium", "SemiBold", "Bold"};
    for (const QString &dir : fontDirs) {
        for (const QString &w : fontWeights) {
            QFontDatabase::addApplicationFont(dir + "/Inter-" + w + ".otf");
            QFontDatabase::addApplicationFont(dir + "/Inter-" + w + ".ttf");
        }
    }

    QQmlApplicationEngine engine;

    // ── Instantiate singletons and expose via context ─────────────
    // Using setContextProperty is the reliable Qt6 way when
    // QML_SINGLETON macros conflict with module-based registration.
    SettingsBackend settingsBackend;
    SystemInfo      systemInfo;
    NetworkManager  networkManager;
    DisplayManager  displayManager;
    AudioBackend    audioBackend;
    WallpaperManager wallpaperManager;

    engine.rootContext()->setContextProperty("SettingsBackend",   &settingsBackend);
    engine.rootContext()->setContextProperty("SystemInfo",        &systemInfo);
    engine.rootContext()->setContextProperty("NetworkManager",    &networkManager);
    engine.rootContext()->setContextProperty("DisplayManager",    &displayManager);
    engine.rootContext()->setContextProperty("AudioBackend",      &audioBackend);
    engine.rootContext()->setContextProperty("WallpaperManager",  &wallpaperManager);

    // ── QML URL: filesystem (dev) or embedded resource ────────────
    using namespace Qt::StringLiterals;

    // Check for dev-mode override via QML_SOURCE_DIR env variable
    QString qmlSourceDir = qEnvironmentVariable("QML_SOURCE_DIR");
    QUrl url;
    if (!qmlSourceDir.isEmpty() && QFile::exists(qmlSourceDir + "/Main.qml")) {
        url = QUrl::fromLocalFile(qmlSourceDir + "/Main.qml");
        engine.addImportPath(qmlSourceDir);
        engine.addImportPath(qmlSourceDir + "/..");
    } else {
        engine.addImportPath(u"qrc:/ArchTitanSettings"_s);
        url = QUrl(u"qrc:/ArchTitanSettings/qml/Main.qml"_s);
    }

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
