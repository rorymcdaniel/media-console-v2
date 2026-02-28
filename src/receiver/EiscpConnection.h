#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class EiscpConnection : public QObject
{
    Q_OBJECT

public:
    explicit EiscpConnection(QObject* parent = nullptr);

    /// Start connecting to the receiver. Will auto-reconnect on failure/disconnect.
    void connectToReceiver(const QString& host, int port);

    /// Disconnect and stop auto-reconnect.
    void disconnect();

    /// Send an ISCP command (e.g., "MVL1A"). Frames as eISCP and writes to socket.
    /// No-op if not connected.
    void sendCommand(const QString& command);

    /// Returns true if the TCP socket is in Connected state.
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& message);
    void connectionError(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReconnectTimer();

private:
    void resetBackoff();

    QTcpSocket m_socket;
    QTimer m_reconnectTimer;
    QByteArray m_readBuffer;
    QString m_host;
    int m_port = 0;
    int m_currentBackoffMs = 1000;
    bool m_autoReconnect = true;

    static constexpr int kInitialBackoffMs = 1000;
    static constexpr int kMaxBackoffMs = 30000;
};
