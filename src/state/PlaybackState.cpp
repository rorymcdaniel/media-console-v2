#include "state/PlaybackState.h"

PlaybackState::PlaybackState(QObject* parent)
    : QObject(parent)
{
}

void PlaybackState::setPlaybackMode(PlaybackMode mode)
{
    if (m_playbackMode == mode)
        return;
    m_playbackMode = mode;
    emit playbackModeChanged(m_playbackMode);
}

void PlaybackState::setActiveSource(MediaSource source)
{
    if (m_activeSource == source)
        return;
    m_activeSource = source;
    emit activeSourceChanged(m_activeSource);
}

void PlaybackState::setPositionMs(qint64 positionMs)
{
    if (m_positionMs == positionMs)
        return;
    m_positionMs = positionMs;
    emit positionMsChanged(m_positionMs);
}

void PlaybackState::setDurationMs(qint64 durationMs)
{
    if (m_durationMs == durationMs)
        return;
    m_durationMs = durationMs;
    emit durationMsChanged(m_durationMs);
}

void PlaybackState::setTitle(const QString& title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit titleChanged(m_title);
}

void PlaybackState::setArtist(const QString& artist)
{
    if (m_artist == artist)
        return;
    m_artist = artist;
    emit artistChanged(m_artist);
}

void PlaybackState::setAlbum(const QString& album)
{
    if (m_album == album)
        return;
    m_album = album;
    emit albumChanged(m_album);
}

void PlaybackState::setAlbumArtUrl(const QString& url)
{
    if (m_albumArtUrl == url)
        return;
    m_albumArtUrl = url;
    emit albumArtUrlChanged(m_albumArtUrl);
}

void PlaybackState::setTrackNumber(int trackNumber)
{
    if (m_trackNumber == trackNumber)
        return;
    m_trackNumber = trackNumber;
    emit trackNumberChanged(m_trackNumber);
}

void PlaybackState::setTrackCount(int trackCount)
{
    if (m_trackCount == trackCount)
        return;
    m_trackCount = trackCount;
    emit trackCountChanged(m_trackCount);
}
