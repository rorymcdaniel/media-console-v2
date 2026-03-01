#include "orchestration/PlaybackRouter.h"

#include "audio/LocalPlaybackController.h"
#include "cd/CdController.h"
#include "receiver/ReceiverController.h"
#include "spotify/SpotifyController.h"
#include "state/PlaybackState.h"
#include "utils/Logging.h"

#ifdef HAS_SNDFILE
#include "library/FlacLibraryController.h"
#endif

namespace
{

QString sourceDisplayName(MediaSource source)
{
    switch (source)
    {
    case MediaSource::None:
        return QStringLiteral("None");
    case MediaSource::Streaming:
        return QStringLiteral("Streaming");
    case MediaSource::Phono:
        return QStringLiteral("Phono");
    case MediaSource::CD:
        return QStringLiteral("CD");
    case MediaSource::Computer:
        return QStringLiteral("Computer");
    case MediaSource::Bluetooth:
        return QStringLiteral("Bluetooth");
    case MediaSource::Library:
        return QStringLiteral("Library");
    }
    return QStringLiteral("Unknown");
}

} // anonymous namespace

PlaybackRouter::PlaybackRouter(PlaybackState* playbackState, ReceiverController* receiverController,
                               CdController* cdController, FlacLibraryController* flacLibraryController,
                               SpotifyController* spotifyController, LocalPlaybackController* localPlaybackController,
                               QObject* parent)
    : QObject(parent)
    , m_playbackState(playbackState)
    , m_receiverController(receiverController)
    , m_cdController(cdController)
    , m_flacLibraryController(flacLibraryController)
    , m_spotifyController(spotifyController)
    , m_localPlaybackController(localPlaybackController)
{
    if (m_playbackState)
    {
        connect(m_playbackState, &PlaybackState::activeSourceChanged, this, &PlaybackRouter::onActiveSourceChanged);
        m_previousSource = m_playbackState->activeSource();
    }

    qCInfo(mediaApp) << "PlaybackRouter: initialized";
}

void PlaybackRouter::play()
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching play() to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        if (m_spotifyController)
            m_spotifyController->play();
        break;
    case MediaSource::CD:
        // CD playback initiated via CdController::playTrack(int) from UI
        qCInfo(mediaApp) << "PlaybackRouter: CD play() is user-initiated via track selection";
        break;
    case MediaSource::Library:
        // Library playback initiated via FlacLibraryController::playTrack(int) from UI
        qCInfo(mediaApp) << "PlaybackRouter: Library play() is user-initiated via track selection";
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        // No local control -- receiver handles these sources
        break;
    }
}

void PlaybackRouter::pause()
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching pause() to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        if (m_spotifyController)
            m_spotifyController->pause();
        break;
    case MediaSource::CD:
        // CD pause handled by LocalPlaybackController internally
        break;
    case MediaSource::Library:
        // Library pause handled by LocalPlaybackController internally
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        break;
    }
}

void PlaybackRouter::stop()
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching stop() to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        if (m_spotifyController)
            m_spotifyController->deactivateSpotify();
        break;
    case MediaSource::CD:
        if (m_cdController)
            m_cdController->stop();
        break;
    case MediaSource::Library:
#ifdef HAS_SNDFILE
        if (m_flacLibraryController)
            m_flacLibraryController->stop();
#endif
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        break;
    }
}

void PlaybackRouter::next()
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching next() to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        if (m_spotifyController)
            m_spotifyController->next();
        break;
    case MediaSource::CD:
        // CdController does not support next track skipping
        break;
    case MediaSource::Library:
#ifdef HAS_SNDFILE
        if (m_flacLibraryController)
            m_flacLibraryController->next();
#endif
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        break;
    }
}

void PlaybackRouter::previous()
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching previous() to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        if (m_spotifyController)
            m_spotifyController->previous();
        break;
    case MediaSource::CD:
        // CdController does not support previous track skipping
        break;
    case MediaSource::Library:
#ifdef HAS_SNDFILE
        if (m_flacLibraryController)
            m_flacLibraryController->previous();
#endif
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        break;
    }
}

void PlaybackRouter::seek(int ms)
{
    if (!m_playbackState)
        return;

    const auto source = m_playbackState->activeSource();
    qCInfo(mediaApp) << "PlaybackRouter: dispatching seek(" << ms << "ms) to" << sourceDisplayName(source);

    switch (source)
    {
    case MediaSource::Streaming:
        // Spotify seek not implemented in Phase 8 controller
        break;
    case MediaSource::CD:
    case MediaSource::Library:
        // Route seek to LocalPlaybackController for local audio sources (ORCH-03)
        if (m_localPlaybackController)
            m_localPlaybackController->seek(static_cast<qint64>(ms));
        break;
    case MediaSource::Phono:
    case MediaSource::Bluetooth:
    case MediaSource::Computer:
    case MediaSource::None:
        break;
    }
}

void PlaybackRouter::onActiveSourceChanged(MediaSource newSource)
{
    qCInfo(mediaApp) << "PlaybackRouter: active source changed from" << sourceDisplayName(m_previousSource) << "to"
                     << sourceDisplayName(newSource);

    // If previous source was playing, stop its controller
    if (m_playbackState && m_playbackState->playbackMode() == PlaybackMode::Playing)
    {
        switch (m_previousSource)
        {
        case MediaSource::Streaming:
            if (m_spotifyController)
            {
                qCInfo(mediaApp) << "PlaybackRouter: stopping Spotify on source switch";
                m_spotifyController->deactivateSpotify();
            }
            break;
        case MediaSource::CD:
            if (m_cdController)
            {
                qCInfo(mediaApp) << "PlaybackRouter: stopping CD on source switch";
                m_cdController->stop();
            }
            break;
        case MediaSource::Library:
#ifdef HAS_SNDFILE
            if (m_flacLibraryController)
            {
                qCInfo(mediaApp) << "PlaybackRouter: stopping Library on source switch";
                m_flacLibraryController->stop();
            }
#endif
            break;
        case MediaSource::Phono:
        case MediaSource::Bluetooth:
        case MediaSource::Computer:
        case MediaSource::None:
            break;
        }
    }

    m_previousSource = newSource;
}
