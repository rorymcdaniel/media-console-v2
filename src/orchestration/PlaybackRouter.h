#pragma once

#include <QObject>

#include "state/MediaSource.h"
#include "state/PlaybackMode.h"

class PlaybackState;
class ReceiverController;
class CdController;
class FlacLibraryController;
class LocalPlaybackController;
class SpotifyController;

/// Centralizes source-aware playback command dispatch.
/// Routes play/pause/stop/next/previous/seek to the correct controller
/// based on PlaybackState::activeSource. Eliminates duplicated if/else chains.
class PlaybackRouter : public QObject
{
    Q_OBJECT

public:
    PlaybackRouter(PlaybackState* playbackState, ReceiverController* receiverController, CdController* cdController,
                   FlacLibraryController* flacLibraryController, SpotifyController* spotifyController,
                   LocalPlaybackController* localPlaybackController, QObject* parent = nullptr);

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(int ms);

private slots:
    void onActiveSourceChanged(MediaSource newSource);

private:
    PlaybackState* m_playbackState;
    ReceiverController* m_receiverController;
    CdController* m_cdController;
    FlacLibraryController* m_flacLibraryController;
    SpotifyController* m_spotifyController;
    LocalPlaybackController* m_localPlaybackController;
    MediaSource m_previousSource = MediaSource::None;
};
