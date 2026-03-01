#include <QCoreApplication>
#include <QSignalSpy>
#include <QStringList>

#include <gtest/gtest.h>

#include "receiver/EiscpConnection.h"
#include "receiver/ReceiverController.h"
#include "state/MediaSource.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/StreamingService.h"
#include "state/UIState.h"

/// Test helper: subclass EiscpConnection to capture sent commands without actual TCP.
class TestableEiscpConnection : public EiscpConnection
{
public:
    explicit TestableEiscpConnection(QObject* parent = nullptr)
        : EiscpConnection(parent)
    {
    }

    QStringList sentCommands;

    /// Override sendCommand to capture without needing a TCP connection.
    /// We shadow the non-virtual base method and also intercept via signal.
    void recordCommand(const QString& command) { sentCommands.append(command); }

    void clearCommands() { sentCommands.clear(); }

    /// Simulate receiving a message from the receiver.
    void simulateMessage(const QString& message) { emit messageReceived(message); }

    /// Simulate connection established.
    void simulateConnected() { emit connected(); }

    /// Simulate disconnection.
    void simulateDisconnected() { emit disconnected(); }
};

/// Since EiscpConnection::sendCommand checks socket state and is not virtual,
/// we use a different approach: intercept at the ReceiverController level.
/// We'll create a standalone test that verifies the command strings the controller
/// would build, and test response parsing via direct message simulation.

class ReceiverControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // QCoreApplication needed for signal/slot processing
        if (QCoreApplication::instance() == nullptr)
        {
            static int argc = 1;
            static char appName[] = "test";
            static char* argv[] = { appName };
            m_app = new QCoreApplication(argc, argv);
        }

        m_connection = new TestableEiscpConnection();
        m_receiverState = new ReceiverState();
        m_playbackState = new PlaybackState();
        m_uiState = new UIState();

        m_controller = new ReceiverController(m_connection, m_receiverState, m_playbackState, m_uiState);
    }

    void TearDown() override
    {
        delete m_controller;
        delete m_uiState;
        delete m_playbackState;
        delete m_receiverState;
        delete m_connection;
    }

    static QCoreApplication* m_app;
    TestableEiscpConnection* m_connection = nullptr;
    ReceiverState* m_receiverState = nullptr;
    PlaybackState* m_playbackState = nullptr;
    UIState* m_uiState = nullptr;
    ReceiverController* m_controller = nullptr;
};

QCoreApplication* ReceiverControllerTest::m_app = nullptr;

// --- Response Parsing Tests ---

// MVL now emits volumeReceivedFromReceiver instead of directly setting ReceiverState.
// The VolumeGestureController (wired in AppBuilder) applies the value; test the signal.
TEST_F(ReceiverControllerTest, ParseMvlEmitsVolumeSignal)
{
    QSignalSpy spy(m_controller, &ReceiverController::volumeReceivedFromReceiver);
    m_connection->simulateMessage("MVL64"); // 0x64 = 100
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 100);
}

TEST_F(ReceiverControllerTest, ParseMvlMaxVolumeSignal)
{
    QSignalSpy spy(m_controller, &ReceiverController::volumeReceivedFromReceiver);
    m_connection->simulateMessage("MVLC8"); // 0xC8 = 200
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 200);
}

TEST_F(ReceiverControllerTest, ParseMvlMinVolumeSignal)
{
    QSignalSpy spy(m_controller, &ReceiverController::volumeReceivedFromReceiver);
    m_connection->simulateMessage("MVL00");
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toInt(), 0);
}

TEST_F(ReceiverControllerTest, ParsePwrOnSetsPowered)
{
    m_connection->simulateMessage("PWR01");
    EXPECT_TRUE(m_receiverState->powered());
}

TEST_F(ReceiverControllerTest, ParsePwrOffSetsPoweredFalse)
{
    m_receiverState->setPowered(true);
    m_connection->simulateMessage("PWR00");
    EXPECT_FALSE(m_receiverState->powered());
}

TEST_F(ReceiverControllerTest, ParseAmtOnSetsMuted)
{
    m_connection->simulateMessage("AMT01");
    EXPECT_TRUE(m_receiverState->muted());
}

TEST_F(ReceiverControllerTest, ParseAmtOffSetsMutedFalse)
{
    m_receiverState->setMuted(true);
    m_connection->simulateMessage("AMT00");
    EXPECT_FALSE(m_receiverState->muted());
}

TEST_F(ReceiverControllerTest, ParseSliStreamingSetsCurrentInput)
{
    m_connection->simulateMessage("SLI2B"); // Streaming
    EXPECT_EQ(m_receiverState->currentInput(), MediaSource::Streaming);
}

TEST_F(ReceiverControllerTest, ParseSliPhonoSetsCurrentInput)
{
    m_connection->simulateMessage("SLI22"); // Phono
    EXPECT_EQ(m_receiverState->currentInput(), MediaSource::Phono);
}

TEST_F(ReceiverControllerTest, ParseSliCdSetsCurrentInput)
{
    m_connection->simulateMessage("SLI23"); // CD (also Library, but default is CD)
    EXPECT_EQ(m_receiverState->currentInput(), MediaSource::CD);
}

TEST_F(ReceiverControllerTest, ParseSliComputerSetsCurrentInput)
{
    m_connection->simulateMessage("SLI05"); // Computer
    EXPECT_EQ(m_receiverState->currentInput(), MediaSource::Computer);
}

TEST_F(ReceiverControllerTest, ParseSliBluetoothSetsCurrentInput)
{
    m_connection->simulateMessage("SLI2E"); // Bluetooth
    EXPECT_EQ(m_receiverState->currentInput(), MediaSource::Bluetooth);
}

// --- Metadata Parsing Tests ---

TEST_F(ReceiverControllerTest, ParseNtiSetsTitle)
{
    m_connection->simulateMessage("NTISong Title");
    EXPECT_EQ(m_receiverState->title(), "Song Title");
}

TEST_F(ReceiverControllerTest, ParseNatSetsArtist)
{
    m_connection->simulateMessage("NATArtist Name");
    EXPECT_EQ(m_receiverState->artist(), "Artist Name");
}

TEST_F(ReceiverControllerTest, ParseNalSetsAlbum)
{
    m_connection->simulateMessage("NALAlbum Name");
    EXPECT_EQ(m_receiverState->album(), "Album Name");
}

TEST_F(ReceiverControllerTest, ParseNjaSetsAlbumArtUrl)
{
    m_connection->simulateMessage("NJAhttp://192.168.68.63/art.jpg");
    EXPECT_EQ(m_receiverState->albumArtUrl(), "http://192.168.68.63/art.jpg");
}

TEST_F(ReceiverControllerTest, ParseNja2SetsAlbumArtUrl)
{
    m_connection->simulateMessage("NJA2http://192.168.68.63/art2.jpg");
    EXPECT_EQ(m_receiverState->albumArtUrl(), "http://192.168.68.63/art2.jpg");
}

TEST_F(ReceiverControllerTest, ParseNfiSetsFileInfo)
{
    m_connection->simulateMessage("NFI44.1kHz/16bit");
    EXPECT_EQ(m_receiverState->fileInfo(), "44.1kHz/16bit");
}

// --- Streaming Service Detection Tests ---

TEST_F(ReceiverControllerTest, ParseNmsSpotifySetsServiceAndName)
{
    m_connection->simulateMessage("NMS0ASomeContent");
    EXPECT_EQ(m_receiverState->streamingService(), StreamingService::Spotify);
    EXPECT_EQ(m_receiverState->serviceName(), "Spotify");
}

TEST_F(ReceiverControllerTest, ParseNmsPandoraSetsServiceAndName)
{
    m_connection->simulateMessage("NMS04SomeContent");
    EXPECT_EQ(m_receiverState->streamingService(), StreamingService::Pandora);
    EXPECT_EQ(m_receiverState->serviceName(), "Pandora");
}

TEST_F(ReceiverControllerTest, ParseNmsAirPlaySetsServiceAndName)
{
    m_connection->simulateMessage("NMS18SomeContent");
    EXPECT_EQ(m_receiverState->streamingService(), StreamingService::AirPlay);
    EXPECT_EQ(m_receiverState->serviceName(), "AirPlay");
}

// --- Playback State Tests ---

TEST_F(ReceiverControllerTest, ParseNstPlayingSetsPlaybackMode)
{
    m_connection->simulateMessage("NSTP--");
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Playing);
}

TEST_F(ReceiverControllerTest, ParseNstPausedSetsPlaybackMode)
{
    m_connection->simulateMessage("NSTp--");
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Paused);
}

TEST_F(ReceiverControllerTest, ParseNstStoppedSetsPlaybackMode)
{
    m_connection->simulateMessage("NSTS--");
    EXPECT_EQ(m_playbackState->playbackMode(), PlaybackMode::Stopped);
}

// --- NTM Time Parsing Tests ---

TEST_F(ReceiverControllerTest, ParseNtmSetsPositionAndDuration)
{
    m_connection->simulateMessage("NTM01:30/04:00");
    EXPECT_EQ(m_playbackState->positionMs(), 90000); // 1:30 = 90 seconds = 90000 ms
    EXPECT_EQ(m_playbackState->durationMs(), 240000); // 4:00 = 240 seconds
}

// --- Connection State Tests ---

TEST_F(ReceiverControllerTest, OnConnectedSetsReceiverConnected)
{
    m_connection->simulateConnected();
    EXPECT_TRUE(m_uiState->receiverConnected());
}

TEST_F(ReceiverControllerTest, OnDisconnectedClearsReceiverConnected)
{
    m_uiState->setReceiverConnected(true);
    m_connection->simulateDisconnected();
    EXPECT_FALSE(m_uiState->receiverConnected());
}

TEST_F(ReceiverControllerTest, OnDisconnectedPreservesReceiverState)
{
    // Set some state
    m_receiverState->setVolume(75);
    m_receiverState->setPowered(true);
    m_receiverState->setTitle("Some Song");

    m_connection->simulateDisconnected();

    // State must be preserved per user decision
    EXPECT_EQ(m_receiverState->volume(), 75);
    EXPECT_TRUE(m_receiverState->powered());
    EXPECT_EQ(m_receiverState->title(), "Some Song");
}

// --- Input Switch Metadata Clearing Tests ---

TEST_F(ReceiverControllerTest, SelectInputClearsMetadata)
{
    // Set some metadata
    m_receiverState->setTitle("Old Song");
    m_receiverState->setArtist("Old Artist");
    m_receiverState->setAlbum("Old Album");
    m_receiverState->setAlbumArtUrl("http://old.url");
    m_receiverState->setFileInfo("old info");
    m_receiverState->setServiceName("OldService");
    m_receiverState->setStreamingService(StreamingService::Spotify);

    m_controller->selectInput(MediaSource::Phono);

    // Metadata should be cleared (except title which gets placeholder)
    EXPECT_EQ(m_receiverState->artist(), "");
    EXPECT_EQ(m_receiverState->album(), "");
    EXPECT_EQ(m_receiverState->albumArtUrl(), "");
    EXPECT_EQ(m_receiverState->fileInfo(), "");
    EXPECT_EQ(m_receiverState->serviceName(), "");
    EXPECT_EQ(m_receiverState->streamingService(), StreamingService::Unknown);
}

TEST_F(ReceiverControllerTest, SelectInputNonStreamingSetsPlaceholderTitle)
{
    m_controller->selectInput(MediaSource::Phono);
    EXPECT_EQ(m_receiverState->title(), "Phono");
}

TEST_F(ReceiverControllerTest, SelectInputCdSetsPlaceholderTitle)
{
    m_controller->selectInput(MediaSource::CD);
    EXPECT_EQ(m_receiverState->title(), "CD");
}

TEST_F(ReceiverControllerTest, SelectInputComputerSetsPlaceholderTitle)
{
    m_controller->selectInput(MediaSource::Computer);
    EXPECT_EQ(m_receiverState->title(), "Computer");
}

TEST_F(ReceiverControllerTest, SelectInputBluetoothSetsPlaceholderTitle)
{
    m_controller->selectInput(MediaSource::Bluetooth);
    EXPECT_EQ(m_receiverState->title(), "Bluetooth");
}

TEST_F(ReceiverControllerTest, SelectInputStreamingDoesNotSetPlaceholder)
{
    m_controller->selectInput(MediaSource::Streaming);
    EXPECT_EQ(m_receiverState->title(), ""); // Cleared, no placeholder
}

// --- Short/Empty Message Handling ---

TEST_F(ReceiverControllerTest, ShortMessageIgnored)
{
    int prevVolume = m_receiverState->volume();
    m_connection->simulateMessage("MV"); // Too short
    EXPECT_EQ(m_receiverState->volume(), prevVolume);
}

TEST_F(ReceiverControllerTest, UnknownCommandIgnored)
{
    int prevVolume = m_receiverState->volume();
    m_connection->simulateMessage("XXXDATA"); // Unknown command
    EXPECT_EQ(m_receiverState->volume(), prevVolume);
}
