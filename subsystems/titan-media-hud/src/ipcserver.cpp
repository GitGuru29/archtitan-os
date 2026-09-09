#include "ipcserver.h"
#include "mediaislandcontroller.h"

#include <QLocalSocket>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <unistd.h>

IpcServer::IpcServer(MediaIslandController *controller, QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
    , m_controller(controller)
{
    // Build socket path: /run/user/$UID/titan-media-hud.sock
    QString runtimeDir = qEnvironmentVariable("XDG_RUNTIME_DIR");
    if (runtimeDir.isEmpty()) {
        runtimeDir = QStringLiteral("/run/user/%1").arg(getuid());
    }
    m_socketPath = runtimeDir + QStringLiteral("/titan-media-hud.sock");

    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::onNewConnection);
}

IpcServer::~IpcServer()
{
    m_server->close();
    QFile::remove(m_socketPath);
}

bool IpcServer::start()
{
    // Remove stale socket
    QFile::remove(m_socketPath);

    if (!m_server->listen(m_socketPath)) {
        qWarning() << "IpcServer: Failed to listen on" << m_socketPath
                    << ":" << m_server->errorString();
        return false;
    }

    qDebug() << "IpcServer: Listening on" << m_socketPath;
    return true;
}

void IpcServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QLocalSocket *socket = m_server->nextPendingConnection();
        if (!socket) continue;

        // Read command (small payload, blocking is fine)
        socket->waitForReadyRead(100);
        QString cmd = QString::fromUtf8(socket->readAll()).trimmed().toLower();
        socket->disconnectFromServer();
        socket->deleteLater();

        if (cmd == "toggle") {
            m_controller->toggle();
        } else if (cmd == "show") {
            if (!m_controller->isVisible()) {
                m_controller->toggle(); // opens
            }
        } else if (cmd == "hide" || cmd == "dismiss") {
            m_controller->dismiss();
        } else if (cmd == "expand") {
            m_controller->expand();
        } else if (cmd == "compact" || cmd == "collapse") {
            m_controller->collapse();
        } else {
            qDebug() << "IpcServer: Unknown command:" << cmd;
        }
    }
}
