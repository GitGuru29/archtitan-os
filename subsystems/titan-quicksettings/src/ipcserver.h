#pragma once

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

class IpcServer : public QObject
{
    Q_OBJECT
public:
    explicit IpcServer(QObject *parent = nullptr);
    ~IpcServer() override;

    bool startServer();
    static bool sendCommand(const QString &command);
    static QString socketPath();

signals:
    void toggleRequested();
    void showRequested();
    void hideRequested();

private slots:
    void handleNewConnection();

private:
    QLocalServer *m_server;
};
