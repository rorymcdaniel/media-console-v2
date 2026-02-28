#include "receiver/EiscpConnection.h"

#include "receiver/EiscpMessage.h"
#include "utils/Logging.h"

EiscpConnection::EiscpConnection(QObject* parent)
    : QObject(parent)
{
    connect(&m_socket, &QTcpSocket::connected, this, &EiscpConnection::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &EiscpConnection::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &EiscpConnection::onReadyRead);
    connect(&m_socket, &QAbstractSocket::errorOccurred, this, &EiscpConnection::onSocketError);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &EiscpConnection::onReconnectTimer);
}

void EiscpConnection::connectToReceiver(const QString& host, int port)
{
    m_host = host;
    m_port = port;
    m_autoReconnect = true;

    qCInfo(mediaReceiver) << "Connecting to" << host << ":" << port;

    m_socket.abort();
    m_socket.connectToHost(host, static_cast<quint16>(port));
}

void EiscpConnection::disconnect()
{
    m_autoReconnect = false;
    m_reconnectTimer.stop();
    m_socket.abort();

    qCInfo(mediaReceiver) << "Disconnected (manual)";
}

void EiscpConnection::sendCommand(const QString& command)
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    QByteArray packet = EiscpMessage::build(command);
    m_socket.write(packet);

    qCDebug(mediaReceiver) << "Sent:" << command;
}

bool EiscpConnection::isConnected() const
{
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void EiscpConnection::onConnected()
{
    resetBackoff();
    m_reconnectTimer.stop();
    m_readBuffer.clear();

    qCInfo(mediaReceiver) << "Connected to" << m_host << ":" << m_port;

    emit connected();
}

void EiscpConnection::onDisconnected()
{
    qCInfo(mediaReceiver) << "Disconnected from receiver";

    if (m_autoReconnect)
    {
        qCInfo(mediaReceiver) << "Reconnecting in" << m_currentBackoffMs << "ms";
        m_reconnectTimer.start(m_currentBackoffMs);
    }

    emit disconnected();
}

void EiscpConnection::onReadyRead()
{
    m_readBuffer.append(m_socket.readAll());

    // Parse all complete messages from the buffer
    while (true)
    {
        QString message = EiscpMessage::parse(m_readBuffer);
        if (message.isEmpty())
        {
            break;
        }

        qCDebug(mediaReceiver) << "Received:" << message;
        emit messageReceived(message);
    }
}

void EiscpConnection::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)

    qCWarning(mediaReceiver) << "Socket error:" << m_socket.errorString();
    emit connectionError(m_socket.errorString());

    if (m_autoReconnect && !m_reconnectTimer.isActive())
    {
        m_reconnectTimer.start(m_currentBackoffMs);
    }
}

void EiscpConnection::onReconnectTimer()
{
    qCInfo(mediaReceiver) << "Reconnect attempt (backoff:" << m_currentBackoffMs << "ms)";

    m_currentBackoffMs = qMin(m_currentBackoffMs * 2, kMaxBackoffMs);

    m_socket.abort();
    m_socket.connectToHost(m_host, static_cast<quint16>(m_port));
}

void EiscpConnection::resetBackoff()
{
    m_currentBackoffMs = kInitialBackoffMs;
}
