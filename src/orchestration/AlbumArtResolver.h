#pragma once

#include <QObject>
#include <QString>

#include "state/MediaSource.h"

class PlaybackState;
class ReceiverState;

/// Resolves album art URL based on the active media source.
/// Streaming/Bluetooth: uses ReceiverState::albumArtUrl (receiver CGI).
/// CD/Library: uses PlaybackState::albumArtUrl (local cache).
/// Phono/Computer/None: empty string (placeholder handled by QML).
class AlbumArtResolver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString albumArtUrl READ albumArtUrl NOTIFY albumArtUrlChanged)

public:
    AlbumArtResolver(PlaybackState* playbackState, ReceiverState* receiverState, QObject* parent = nullptr);

    QString albumArtUrl() const;

signals:
    void albumArtUrlChanged(const QString& url);

private slots:
    void resolve();

private:
    PlaybackState* m_playbackState;
    ReceiverState* m_receiverState;
    QString m_albumArtUrl;
};
