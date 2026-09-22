#include "ipcserver.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

IpcServer::IpcServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &IpcServer::handleNewConnection);
}

IpcServer::~IpcServer()
{
    if (m_server && m_server->isListening()) {
        m_server->close();
        QFile::remove(socketPath());
    }
}

QString IpcServer::socketPath()
{
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        runtimeDir = "/tmp";
    }
    return runtimeDir + "/titan-quicksettings.sock";
}

bool IpcServer::startServer()
{
    QString path = socketPath();
    QFile::remove(path); // Clean up any stale socket

    if (!m_server->listen(path)) {
        qWarning() << "Failed to start IPC server at" << path << ":" << m_server->errorString();
        return false;
    }
    return true;
}

bool IpcServer::sendCommand(const QString &command)
{
    QLocalSocket socket;
    socket.connectToServer(socketPath());
    if (!socket.waitForConnected(500)) {
        return false;
    }

    QByteArray data = command.toUtf8() + "\n";
    socket.write(data);
    socket.flush();
    socket.waitForBytesWritten(500);
    socket.disconnectFromServer();
    return true;
}

void IpcServer::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QLocalSocket *client = m_server->nextPendingConnection();
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            QByteArray raw = client->readAll();
            QString cmd = QString::fromUtf8(raw).trimmed();
            if (cmd.compare("TOGGLE", Qt::CaseInsensitive) == 0) {
                emit toggleRequested();
            } else if (cmd.compare("SHOW", Qt::CaseInsensitive) == 0) {
                emit showRequested();
            } else if (cmd.compare("HIDE", Qt::CaseInsensitive) == 0) {
                emit hideRequested();
            }
            client->disconnectFromServer();
        });
        connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
    }
}
