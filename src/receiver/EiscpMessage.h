#pragma once

#include <QByteArray>
#include <QString>

class EiscpMessage
{
public:
    /// Build an eISCP packet from an ISCP command string (e.g., "PWRQSTN", "MVL1A").
    /// Returns the complete binary packet ready to send over TCP.
    static QByteArray build(const QString& command);

    /// Attempt to parse one complete eISCP packet from the front of buffer.
    /// On success: returns the ISCP command string (e.g., "MVL1A"), removes consumed bytes from buffer.
    /// On failure (incomplete data): returns empty QString, buffer unchanged.
    /// On corruption (invalid magic): clears buffer, returns empty QString.
    static QString parse(QByteArray& buffer);

private:
    static constexpr int kHeaderSize = 16;
    static constexpr char kMagic[] = "ISCP";
    static constexpr quint8 kVersion = 1;
};
