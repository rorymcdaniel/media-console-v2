#include "orchestration/AlbumArtResolver.h"

#include "state/PlaybackState.h"
#include "state/ReceiverState.h"

AlbumArtResolver::AlbumArtResolver(PlaybackState* playbackState, ReceiverState* receiverState, QObject* parent)
    : QObject(parent)
    , m_playbackState(playbackState)
    , m_receiverState(receiverState)
{
    // Stub -- tests should fail
}

QString AlbumArtResolver::albumArtUrl() const
{
    return m_albumArtUrl;
}

void AlbumArtResolver::resolve()
{
    // Stub -- does nothing, tests should fail
}
