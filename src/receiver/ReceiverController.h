#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#include "state/MediaSource.h"
#include "state/PlaybackMode.h"
#include "state/StreamingService.h"

class EiscpConnection;
class ReceiverState;
class PlaybackState;
class UIState;

class ReceiverController : public QObject
{
    Q_OBJECT

public:
    ReceiverController(EiscpConnection* connection, ReceiverState* receiverState, PlaybackState* playbackState,
                       UIState* uiState, QObject* parent = nullptr);

    /// Start connection and polling. Call after construction.
    void start(const QString& host, int port);

    /// Stop connection and polling.
    void stop();

public slots:
    void setVolume(int volume);
    void setPower(bool on);
    void selectInput(MediaSource source);
    void toggleMute();

    /// Query all receiver state (MVL, PWR, AMT, SLI). Called on connect and by poll timer.
    void queryAll();

signals:
    /// Emitted when stale data is detected (no messages for 30s during playback).
    void staleDataDetected(bool stale);

private slots:
    void onConnected();
    void onDisconnected();
    void onMessage(const QString& message);
    void onPollTimer();
    void onStaleDataCheck();

private:
    void parseResponse(const QString& message);
    void clearMetadata();
    static QString volumeToHex(int volume);
    static int hexToVolume(const QString& hex);
    static StreamingService detectService(const QString& code);
    static QString serviceCodeToName(const QString& code);
    static QString mediaSourceDisplayName(MediaSource source);

    EiscpConnection* m_connection;
    ReceiverState* m_receiverState;
    PlaybackState* m_playbackState;
    UIState* m_uiState;

    QTimer m_pollTimer;
    QTimer m_staleDataTimer;
    QElapsedTimer m_lastMessageTime;
    bool m_staleWarningActive = false;

    static constexpr int kPollIntervalMs = 2500;
    static constexpr int kStaleDataThresholdMs = 30000;
    static constexpr int kStaleCheckIntervalMs = 5000;
};
