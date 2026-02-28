#include "state/ReceiverState.h"

ReceiverState::ReceiverState(QObject* parent)
    : QObject(parent)
{
}

void ReceiverState::setVolume(int volume)
{
    if (m_volume == volume)
        return;
    m_volume = volume;
    emit volumeChanged(m_volume);
}

void ReceiverState::setPowered(bool powered)
{
    if (m_powered == powered)
        return;
    m_powered = powered;
    emit poweredChanged(m_powered);
}

void ReceiverState::setMuted(bool muted)
{
    if (m_muted == muted)
        return;
    m_muted = muted;
    emit mutedChanged(m_muted);
}

void ReceiverState::setCurrentInput(MediaSource input)
{
    if (m_currentInput == input)
        return;
    m_currentInput = input;
    emit currentInputChanged(m_currentInput);
}

void ReceiverState::setTitle(const QString& title)
{
    if (m_title == title)
        return;
    m_title = title;
    emit titleChanged(m_title);
}

void ReceiverState::setArtist(const QString& artist)
{
    if (m_artist == artist)
        return;
    m_artist = artist;
    emit artistChanged(m_artist);
}

void ReceiverState::setAlbum(const QString& album)
{
    if (m_album == album)
        return;
    m_album = album;
    emit albumChanged(m_album);
}

void ReceiverState::setAlbumArtUrl(const QString& url)
{
    if (m_albumArtUrl == url)
        return;
    m_albumArtUrl = url;
    emit albumArtUrlChanged(m_albumArtUrl);
}

void ReceiverState::setFileInfo(const QString& fileInfo)
{
    if (m_fileInfo == fileInfo)
        return;
    m_fileInfo = fileInfo;
    emit fileInfoChanged(m_fileInfo);
}

void ReceiverState::setServiceName(const QString& serviceName)
{
    if (m_serviceName == serviceName)
        return;
    m_serviceName = serviceName;
    emit serviceNameChanged(m_serviceName);
}

void ReceiverState::setStreamingService(StreamingService service)
{
    if (m_streamingService == service)
        return;
    m_streamingService = service;
    emit streamingServiceChanged(m_streamingService);
}
