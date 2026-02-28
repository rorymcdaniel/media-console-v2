#include "receiver/EiscpMessage.h"

#include <QtEndian>

QByteArray EiscpMessage::build(const QString& command)
{
    // ISCP payload: "!1" + command + "\r"
    QByteArray iscp = "!1" + command.toLatin1() + "\r";

    // eISCP header: 16 bytes
    QByteArray packet;
    packet.reserve(kHeaderSize + iscp.size());

    // Magic: "ISCP" (4 bytes)
    packet.append(kMagic, 4);

    // Header size: 16 (4 bytes, big-endian)
    quint32 headerSizeBE = qToBigEndian<quint32>(kHeaderSize);
    packet.append(reinterpret_cast<const char*>(&headerSizeBE), 4);

    // Message size (4 bytes, big-endian)
    quint32 messageSizeBE = qToBigEndian<quint32>(static_cast<quint32>(iscp.size()));
    packet.append(reinterpret_cast<const char*>(&messageSizeBE), 4);

    // Version: 1 (1 byte)
    packet.append(static_cast<char>(kVersion));

    // Reserved: 3 zero bytes
    packet.append(3, char(0));

    // ISCP payload
    packet.append(iscp);

    return packet;
}

QString EiscpMessage::parse(QByteArray& buffer)
{
    // Need at least a complete header
    if (buffer.size() < kHeaderSize)
    {
        return {};
    }

    // Validate magic bytes
    if (buffer.left(4) != QByteArray(kMagic, 4))
    {
        // Corrupted data — clear buffer
        buffer.clear();
        return {};
    }

    // Extract message size from bytes 8-11 (big-endian)
    quint32 messageSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(buffer.constData() + 8));

    int totalSize = kHeaderSize + static_cast<int>(messageSize);

    // Need complete packet
    if (buffer.size() < totalSize)
    {
        return {};
    }

    // Extract ISCP payload
    QByteArray iscp = buffer.mid(kHeaderSize, static_cast<int>(messageSize));

    // Remove consumed bytes from buffer
    buffer.remove(0, totalSize);

    // Convert to string and strip framing
    QString msg = QString::fromLatin1(iscp);

    // Strip "!1" prefix
    if (msg.startsWith(QLatin1String("!1")))
    {
        msg = msg.mid(2);
    }

    // Strip trailing CR, LF, and EOF (0x1A) characters
    while (!msg.isEmpty()
           && (msg.endsWith(QLatin1Char('\r')) || msg.endsWith(QLatin1Char('\n')) || msg.endsWith(QChar(0x1A))))
    {
        msg.chop(1);
    }

    return msg;
}
