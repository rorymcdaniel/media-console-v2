#include <QByteArray>
#include <QStringList>
#include <QtEndian>

#include <gtest/gtest.h>

#include "receiver/EiscpMessage.h"

class EiscpMessageTest : public ::testing::Test
{
protected:
    // Helper: extract big-endian quint32 from byte array at offset
    quint32 readBE32(const QByteArray& data, int offset)
    {
        return qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(data.constData() + offset));
    }
};

// --- build() tests ---

TEST_F(EiscpMessageTest, BuildPwrQstnProducesValidPacket)
{
    QByteArray packet = EiscpMessage::build("PWRQSTN");

    // Header: "ISCP" + headerSize(16) + messageSize + version(1) + reserved(000)
    // Payload: "!1PWRQSTN\r" = 10 bytes
    ASSERT_GE(packet.size(), 16 + 10);

    // Magic
    EXPECT_EQ(packet.mid(0, 4), QByteArray("ISCP"));

    // Header size = 16
    EXPECT_EQ(readBE32(packet, 4), 16u);

    // Message size = 10 ("!1PWRQSTN\r")
    EXPECT_EQ(readBE32(packet, 8), 10u);

    // Version = 1
    EXPECT_EQ(static_cast<quint8>(packet.at(12)), 1);

    // Reserved = 0, 0, 0
    EXPECT_EQ(static_cast<quint8>(packet.at(13)), 0);
    EXPECT_EQ(static_cast<quint8>(packet.at(14)), 0);
    EXPECT_EQ(static_cast<quint8>(packet.at(15)), 0);

    // Payload
    EXPECT_EQ(packet.mid(16), QByteArray("!1PWRQSTN\r"));
}

TEST_F(EiscpMessageTest, BuildMvl1aProducesCorrectMessageSize)
{
    QByteArray packet = EiscpMessage::build("MVL1A");

    // Payload: "!1MVL1A\r" = 8 bytes
    EXPECT_EQ(readBE32(packet, 8), 8u);
    EXPECT_EQ(packet.mid(16), QByteArray("!1MVL1A\r"));
}

TEST_F(EiscpMessageTest, BuildEmptyCommandProducesValidPacket)
{
    QByteArray packet = EiscpMessage::build("");

    // Payload: "!1\r" = 3 bytes
    EXPECT_EQ(readBE32(packet, 8), 3u);
    EXPECT_EQ(packet.mid(16), QByteArray("!1\r"));
}

// --- parse() tests ---

TEST_F(EiscpMessageTest, ParseEmptyBufferReturnsEmpty)
{
    QByteArray buffer;
    QString result = EiscpMessage::parse(buffer);
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(buffer.isEmpty());
}

TEST_F(EiscpMessageTest, ParseInsufficientHeaderReturnsEmptyLeavesBuffer)
{
    QByteArray buffer("ISCP1234"); // Only 8 bytes, need 16
    QByteArray original = buffer;
    QString result = EiscpMessage::parse(buffer);
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(buffer, original);
}

TEST_F(EiscpMessageTest, ParseCompleteHeaderButIncompletePayloadReturnsEmpty)
{
    // Build a valid header indicating 10-byte message, but only provide 5 payload bytes
    QByteArray buffer;
    buffer.append("ISCP");

    quint32 headerSizeBE = qToBigEndian<quint32>(16);
    buffer.append(reinterpret_cast<const char*>(&headerSizeBE), 4);

    quint32 messageSizeBE = qToBigEndian<quint32>(10);
    buffer.append(reinterpret_cast<const char*>(&messageSizeBE), 4);

    buffer.append(char(1)); // version
    buffer.append(3, char(0)); // reserved

    buffer.append("!1PWR"); // Only 5 bytes, need 10

    QByteArray original = buffer;
    QString result = EiscpMessage::parse(buffer);
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(buffer, original);
}

TEST_F(EiscpMessageTest, ParseCompletePacketReturnsCommandAndConsumesBuffer)
{
    QByteArray packet = EiscpMessage::build("PWRQSTN");
    QByteArray buffer = packet;

    QString result = EiscpMessage::parse(buffer);
    EXPECT_EQ(result, "PWRQSTN");
    EXPECT_TRUE(buffer.isEmpty());
}

TEST_F(EiscpMessageTest, ParseCompletePacketFollowedByPartialLeavesPartial)
{
    QByteArray packet1 = EiscpMessage::build("MVL1A");
    QByteArray partial("ISCP1234"); // partial second packet

    QByteArray buffer = packet1 + partial;

    QString result = EiscpMessage::parse(buffer);
    EXPECT_EQ(result, "MVL1A");
    EXPECT_EQ(buffer, partial);
}

TEST_F(EiscpMessageTest, ParseHandlesTrailingLfAndEof)
{
    // Manually construct a packet with CR + LF + EOF at end
    QByteArray iscp = "!1MVL1A\r\n";
    iscp.append(char(0x1A)); // EOF

    QByteArray buffer;
    buffer.append("ISCP");

    quint32 headerSizeBE = qToBigEndian<quint32>(16);
    buffer.append(reinterpret_cast<const char*>(&headerSizeBE), 4);

    quint32 messageSizeBE = qToBigEndian<quint32>(static_cast<quint32>(iscp.size()));
    buffer.append(reinterpret_cast<const char*>(&messageSizeBE), 4);

    buffer.append(char(1));
    buffer.append(3, char(0));
    buffer.append(iscp);

    QString result = EiscpMessage::parse(buffer);
    EXPECT_EQ(result, "MVL1A");
    EXPECT_TRUE(buffer.isEmpty());
}

TEST_F(EiscpMessageTest, ParseInvalidMagicClearsBufferReturnsEmpty)
{
    QByteArray buffer;
    buffer.append("XXXX"); // Invalid magic

    quint32 headerSizeBE = qToBigEndian<quint32>(16);
    buffer.append(reinterpret_cast<const char*>(&headerSizeBE), 4);

    quint32 messageSizeBE = qToBigEndian<quint32>(10);
    buffer.append(reinterpret_cast<const char*>(&messageSizeBE), 4);

    buffer.append(char(1));
    buffer.append(3, char(0));
    buffer.append("!1PWRQSTN\r");

    QString result = EiscpMessage::parse(buffer);
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(buffer.isEmpty()); // Buffer cleared on corruption
}

TEST_F(EiscpMessageTest, ParseTwoConsecutiveCompletePackets)
{
    QByteArray packet1 = EiscpMessage::build("PWR01");
    QByteArray packet2 = EiscpMessage::build("MVL64");
    QByteArray buffer = packet1 + packet2;

    QString result1 = EiscpMessage::parse(buffer);
    EXPECT_EQ(result1, "PWR01");
    EXPECT_FALSE(buffer.isEmpty()); // packet2 still in buffer

    QString result2 = EiscpMessage::parse(buffer);
    EXPECT_EQ(result2, "MVL64");
    EXPECT_TRUE(buffer.isEmpty());
}

TEST_F(EiscpMessageTest, BuildAndParseRoundTrip)
{
    QStringList commands = { "PWRQSTN",
                             "MVL00",
                             "MVLC8",
                             "AMT01",
                             "AMTTG",
                             "SLI2B",
                             "SLI22",
                             "SLI23",
                             "SLI05",
                             "SLI2E",
                             "NTISong Title Here",
                             "NATArtist Name",
                             "NALAlbum Name" };

    for (const QString& cmd : commands)
    {
        QByteArray buffer = EiscpMessage::build(cmd);
        QString parsed = EiscpMessage::parse(buffer);
        EXPECT_EQ(parsed, cmd) << "Round-trip failed for: " << cmd.toStdString();
    }
}
