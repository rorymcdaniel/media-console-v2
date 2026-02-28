#pragma once

#include <QObject>
#include <QString>

#include "state/MediaSource.h"
#include "state/PlaybackMode.h"

class PlaybackState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(PlaybackModeEnum::Value playbackMode READ playbackMode WRITE setPlaybackMode NOTIFY playbackModeChanged)
    Q_PROPERTY(MediaSourceEnum::Value activeSource READ activeSource WRITE setActiveSource NOTIFY activeSourceChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs WRITE setPositionMs NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs WRITE setDurationMs NOTIFY durationMsChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QString albumArtUrl READ albumArtUrl WRITE setAlbumArtUrl NOTIFY albumArtUrlChanged)
    Q_PROPERTY(int trackNumber READ trackNumber WRITE setTrackNumber NOTIFY trackNumberChanged)
    Q_PROPERTY(int trackCount READ trackCount WRITE setTrackCount NOTIFY trackCountChanged)

public:
    explicit PlaybackState(QObject* parent = nullptr);

    PlaybackMode playbackMode() const { return m_playbackMode; }
    MediaSource activeSource() const { return m_activeSource; }
    qint64 positionMs() const { return m_positionMs; }
    qint64 durationMs() const { return m_durationMs; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString albumArtUrl() const { return m_albumArtUrl; }
    int trackNumber() const { return m_trackNumber; }
    int trackCount() const { return m_trackCount; }

public slots:
    void setPlaybackMode(PlaybackMode mode);
    void setActiveSource(MediaSource source);
    void setPositionMs(qint64 positionMs);
    void setDurationMs(qint64 durationMs);
    void setTitle(const QString& title);
    void setArtist(const QString& artist);
    void setAlbum(const QString& album);
    void setAlbumArtUrl(const QString& url);
    void setTrackNumber(int trackNumber);
    void setTrackCount(int trackCount);

signals:
    void playbackModeChanged(PlaybackMode mode);
    void activeSourceChanged(MediaSource source);
    void positionMsChanged(qint64 positionMs);
    void durationMsChanged(qint64 durationMs);
    void titleChanged(const QString& title);
    void artistChanged(const QString& artist);
    void albumChanged(const QString& album);
    void albumArtUrlChanged(const QString& url);
    void trackNumberChanged(int trackNumber);
    void trackCountChanged(int trackCount);

private:
    PlaybackMode m_playbackMode = PlaybackMode::Stopped;
    MediaSource m_activeSource = MediaSource::None;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_albumArtUrl;
    int m_trackNumber = 0;
    int m_trackCount = 0;
};
