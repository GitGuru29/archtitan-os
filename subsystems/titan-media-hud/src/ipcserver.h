#pragma once

#include <QObject>
#include <QLocalServer>

class MediaIslandController;

/**
 * IpcServer — Unix domain socket listener for SUPER+M toggle.
 *
 * Listens at /run/user/$UID/titan-media-hud.sock
 * Accepts simple text commands: toggle, show, hide, expand, compact, dismiss
 */
class IpcServer : public QObject
{
    Q_OBJECT

public:
    explicit IpcServer(MediaIslandController *controller, QObject *parent = nullptr);
    ~IpcServer();

    bool start();
    QString socketPath() const { return m_socketPath; }

private slots:
    void onNewConnection();

private:
    QLocalServer *m_server{nullptr};
    MediaIslandController *m_controller{nullptr};
    QString m_socketPath;
};
