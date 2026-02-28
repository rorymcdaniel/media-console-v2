#include "orchestration/AlbumArtResolver.h"

#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "utils/Logging.h"

AlbumArtResolver::AlbumArtResolver(PlaybackState* playbackState, ReceiverState* receiverState, QObject* parent)
    : QObject(parent)
    , m_playbackState(playbackState)
    , m_receiverState(receiverState)
{
    if (m_playbackState)
    {
        connect(m_playbackState, &PlaybackState::activeSourceChanged, this, &AlbumArtResolver::resolve);
        connect(m_playbackState, &PlaybackState::albumArtUrlChanged, this, &AlbumArtResolver::resolve);
    }

    if (m_receiverState)
    {
        connect(m_receiverState, &ReceiverState::albumArtUrlChanged, this, &AlbumArtResolver::resolve);
    }

    // Initial resolve
    resolve();

    qCInfo(mediaApp) << "AlbumArtResolver: initialized";
}

QString AlbumArtResolver::albumArtUrl() const
{
    return m_albumArtUrl;
}

void AlbumArtResolver::resolve()
{
    if (!m_playbackState)
        return;

    QString newUrl;
    const auto source = m_playbackState->activeSource();

    switch (source)
    {
    case MediaSource::Streaming:
    case MediaSource::Bluetooth:
        // Receiver CGI provides art for streaming/bluetooth sources
        if (m_receiverState)
            newUrl = m_receiverState->albumArtUrl();
        break;
    case MediaSource::CD:
    case MediaSource::Library:
        // Local controllers set PlaybackState::albumArtUrl from cache
        newUrl = m_playbackState->albumArtUrl();
        break;
    case MediaSource::Phono:
    case MediaSource::Computer:
    case MediaSource::None:
        // No album art -- placeholder handled by QML
        newUrl.clear();
        break;
    }

    if (newUrl != m_albumArtUrl)
    {
        m_albumArtUrl = newUrl;
        emit albumArtUrlChanged(m_albumArtUrl);
    }
}
