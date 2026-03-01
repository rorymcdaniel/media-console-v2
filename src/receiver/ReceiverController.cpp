#include "receiver/ReceiverController.h"

#include "receiver/EiscpConnection.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/UIState.h"
#include "utils/Logging.h"

ReceiverController::ReceiverController(EiscpConnection* connection, ReceiverState* receiverState,
                                       PlaybackState* playbackState, UIState* uiState, QObject* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_receiverState(receiverState)
    , m_playbackState(playbackState)
    , m_uiState(uiState)
{
    connect(m_connection, &EiscpConnection::connected, this, &ReceiverController::onConnected);
    connect(m_connection, &EiscpConnection::disconnected, this, &ReceiverController::onDisconnected);
    connect(m_connection, &EiscpConnection::messageReceived, this, &ReceiverController::onMessage);

    m_pollTimer.setInterval(kPollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, &ReceiverController::onPollTimer);

    m_staleDataTimer.setInterval(kStaleCheckIntervalMs);
    connect(&m_staleDataTimer, &QTimer::timeout, this, &ReceiverController::onStaleDataCheck);
}

void ReceiverController::start(const QString& host, int port)
{
    m_connection->connectToReceiver(host, port);
}

void ReceiverController::stop()
{
    m_pollTimer.stop();
    m_staleDataTimer.stop();
    m_connection->disconnect();
}

void ReceiverController::setVolume(int volume)
{
    m_connection->sendCommand("MVL" + volumeToHex(qBound(0, volume, 200)));
}

void ReceiverController::setPower(bool on)
{
    m_connection->sendCommand(on ? "PWR01" : "PWR00");
}

void ReceiverController::selectInput(MediaSource source)
{
    // Clear metadata before switching — no stale data from previous source
    clearMetadata();

    // Non-streaming inputs get a placeholder title
    if (source != MediaSource::Streaming && source != MediaSource::Library && source != MediaSource::None)
    {
        m_receiverState->setTitle(mediaSourceDisplayName(source));
    }

    uint8_t hexCode = toHexCode(source);
    m_connection->sendCommand("SLI" + QString("%1").arg(hexCode, 2, 16, QChar('0')).toUpper());
}

void ReceiverController::toggleMute()
{
    m_connection->sendCommand("AMTTG");
}

void ReceiverController::inputNext()
{
    // Source cycling order: Streaming, Phono, CD, Computer, Bluetooth, Library
    static constexpr MediaSource kSourceOrder[]
        = { MediaSource::Streaming, MediaSource::Phono,     MediaSource::CD,
            MediaSource::Computer,  MediaSource::Bluetooth, MediaSource::Library };
    static constexpr int kSourceCount = sizeof(kSourceOrder) / sizeof(kSourceOrder[0]);

    MediaSource current = m_receiverState->currentInput();
    int idx = 0;
    for (int i = 0; i < kSourceCount; ++i)
    {
        if (kSourceOrder[i] == current)
        {
            idx = i;
            break;
        }
    }
    int nextIdx = (idx + 1) % kSourceCount;
    selectInput(kSourceOrder[nextIdx]);
}

void ReceiverController::inputPrevious()
{
    static constexpr MediaSource kSourceOrder[]
        = { MediaSource::Streaming, MediaSource::Phono,     MediaSource::CD,
            MediaSource::Computer,  MediaSource::Bluetooth, MediaSource::Library };
    static constexpr int kSourceCount = sizeof(kSourceOrder) / sizeof(kSourceOrder[0]);

    MediaSource current = m_receiverState->currentInput();
    int idx = 0;
    for (int i = 0; i < kSourceCount; ++i)
    {
        if (kSourceOrder[i] == current)
        {
            idx = i;
            break;
        }
    }
    int prevIdx = (idx - 1 + kSourceCount) % kSourceCount;
    selectInput(kSourceOrder[prevIdx]);
}

void ReceiverController::queryAll()
{
    m_connection->sendCommand("MVLQSTN");
    m_connection->sendCommand("PWRQSTN");
    m_connection->sendCommand("AMTQSTN");
    m_connection->sendCommand("SLIQSTN");
}

void ReceiverController::onConnected()
{
    m_uiState->setReceiverConnected(true);
    m_lastMessageTime.start();
    m_staleWarningActive = false;

    queryAll();
    m_pollTimer.start();
    m_staleDataTimer.start();

    qCInfo(mediaReceiver) << "Receiver connected, polling started";
}

void ReceiverController::onDisconnected()
{
    m_uiState->setReceiverConnected(false);
    m_pollTimer.stop();
    m_staleDataTimer.stop();

    // Preserve last-known state — do NOT clear ReceiverState
    qCInfo(mediaReceiver) << "Receiver disconnected, state preserved";
}

void ReceiverController::onMessage(const QString& message)
{
    m_lastMessageTime.restart();

    if (m_staleWarningActive)
    {
        m_staleWarningActive = false;
        emit staleDataDetected(false);
        qCInfo(mediaReceiver) << "Stale data warning cleared";
    }

    parseResponse(message);
}

void ReceiverController::onPollTimer()
{
    queryAll();
}

void ReceiverController::onStaleDataCheck()
{
    if (m_playbackState->playbackMode() == PlaybackMode::Playing && m_lastMessageTime.elapsed() > kStaleDataThresholdMs)
    {
        if (!m_staleWarningActive)
        {
            m_staleWarningActive = true;
            emit staleDataDetected(true);
            qCWarning(mediaReceiver) << "Stale data detected: no messages for" << m_lastMessageTime.elapsed()
                                     << "ms during playback";
        }
    }
    else if (m_staleWarningActive)
    {
        m_staleWarningActive = false;
        emit staleDataDetected(false);
    }
}

void ReceiverController::parseResponse(const QString& message)
{
    if (message.length() < 3)
    {
        return;
    }

    // Check for NJA2 first (4-char prefix) before NJA (3-char prefix)
    if (message.startsWith("NJA2"))
    {
        m_receiverState->setAlbumArtUrl(message.mid(4));
        return;
    }

    QString cmd = message.left(3);
    QString payload = message.mid(3);

    if (cmd == "MVL")
    {
        int vol = hexToVolume(payload);
        if (vol >= 0)
        {
            // Route through VolumeGestureController (gesture suppression + reconciliation snap).
            // VolumeGestureController::onExternalVolumeUpdate() calls m_receiverState->setVolume()
            // after checking whether a gesture is active (gesture has priority).
            emit volumeReceivedFromReceiver(vol);
        }
    }
    else if (cmd == "PWR")
    {
        m_receiverState->setPowered(payload == "01");
    }
    else if (cmd == "AMT")
    {
        m_receiverState->setMuted(payload == "01");
    }
    else if (cmd == "SLI")
    {
        if (payload.length() >= 2)
        {
            bool ok = false;
            uint8_t code = static_cast<uint8_t>(payload.left(2).toInt(&ok, 16));
            if (ok)
            {
                m_receiverState->setCurrentInput(fromHexCode(code));
            }
        }
    }
    else if (cmd == "NTI")
    {
        m_receiverState->setTitle(payload);
    }
    else if (cmd == "NAT")
    {
        m_receiverState->setArtist(payload);
    }
    else if (cmd == "NAL")
    {
        m_receiverState->setAlbum(payload);
    }
    else if (cmd == "NJA")
    {
        m_receiverState->setAlbumArtUrl(payload);
    }
    else if (cmd == "NFI")
    {
        m_receiverState->setFileInfo(payload);
    }
    else if (cmd == "NMS")
    {
        if (payload.length() >= 2)
        {
            QString serviceCode = payload.left(2);
            m_receiverState->setStreamingService(detectService(serviceCode));
            m_receiverState->setServiceName(serviceCodeToName(serviceCode));
        }
    }
    else if (cmd == "NST")
    {
        if (!payload.isEmpty())
        {
            QChar playChar = payload.at(0);
            if (playChar == 'P')
            {
                m_playbackState->setPlaybackMode(PlaybackMode::Playing);
            }
            else if (playChar == 'p')
            {
                m_playbackState->setPlaybackMode(PlaybackMode::Paused);
            }
            else
            {
                m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
            }
        }
    }
    else if (cmd == "NTM")
    {
        // Format: "mm:ss/mm:ss" — position/duration
        QStringList parts = payload.split('/');
        if (parts.size() == 2)
        {
            auto parseTime = [](const QString& timeStr) -> qint64
            {
                QStringList segments = timeStr.split(':');
                if (segments.size() == 2)
                {
                    bool minOk = false;
                    bool secOk = false;
                    int minutes = segments[0].toInt(&minOk);
                    int seconds = segments[1].toInt(&secOk);
                    if (minOk && secOk)
                    {
                        return static_cast<qint64>((minutes * 60 + seconds) * 1000);
                    }
                }
                return 0;
            };

            m_playbackState->setPositionMs(parseTime(parts[0]));
            m_playbackState->setDurationMs(parseTime(parts[1]));
        }
    }
}

void ReceiverController::clearMetadata()
{
    m_receiverState->setTitle(QString());
    m_receiverState->setArtist(QString());
    m_receiverState->setAlbum(QString());
    m_receiverState->setAlbumArtUrl(QString());
    m_receiverState->setFileInfo(QString());
    m_receiverState->setServiceName(QString());
    m_receiverState->setStreamingService(StreamingService::Unknown);
    m_playbackState->setPlaybackMode(PlaybackMode::Stopped);
}

QString ReceiverController::volumeToHex(int volume)
{
    return QString("%1").arg(qBound(0, volume, 200), 2, 16, QChar('0')).toUpper();
}

int ReceiverController::hexToVolume(const QString& hex)
{
    bool ok = false;
    int val = hex.toInt(&ok, 16);
    return ok ? qBound(0, val, 200) : -1;
}

StreamingService ReceiverController::detectService(const QString& code)
{
    if (code == "0A")
    {
        return StreamingService::Spotify;
    }
    if (code == "04")
    {
        return StreamingService::Pandora;
    }
    if (code == "18")
    {
        return StreamingService::AirPlay;
    }
    // Amazon Music and Chromecast codes need hardware verification
    return StreamingService::Unknown;
}

QString ReceiverController::serviceCodeToName(const QString& code)
{
    if (code == "0A")
    {
        return QStringLiteral("Spotify");
    }
    if (code == "04")
    {
        return QStringLiteral("Pandora");
    }
    if (code == "18")
    {
        return QStringLiteral("AirPlay");
    }
    if (code == "13")
    {
        return QStringLiteral("iHeartRadio");
    }
    qCInfo(mediaReceiver) << "Unknown streaming service code:" << code;
    return QStringLiteral("Unknown");
}

QString ReceiverController::mediaSourceDisplayName(MediaSource source)
{
    switch (source)
    {
    case MediaSource::Phono:
        return QStringLiteral("Phono");
    case MediaSource::CD:
        return QStringLiteral("CD");
    case MediaSource::Computer:
        return QStringLiteral("Computer");
    case MediaSource::Bluetooth:
        return QStringLiteral("Bluetooth");
    default:
        return {};
    }
}
