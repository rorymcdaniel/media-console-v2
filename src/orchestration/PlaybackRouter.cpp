#include "orchestration/PlaybackRouter.h"

#include "cd/CdController.h"
#include "receiver/ReceiverController.h"
#include "spotify/SpotifyController.h"
#include "state/PlaybackState.h"
#include "utils/Logging.h"

#ifdef HAS_SNDFILE
#include "library/FlacLibraryController.h"
#endif

PlaybackRouter::PlaybackRouter(PlaybackState* playbackState, ReceiverController* receiverController,
                               CdController* cdController, FlacLibraryController* flacLibraryController,
                               SpotifyController* spotifyController, QObject* parent)
    : QObject(parent)
    , m_playbackState(playbackState)
    , m_receiverController(receiverController)
    , m_cdController(cdController)
    , m_flacLibraryController(flacLibraryController)
    , m_spotifyController(spotifyController)
{
    // Stub -- tests should fail
}

void PlaybackRouter::play()
{
    // Stub
}

void PlaybackRouter::pause()
{
    // Stub
}

void PlaybackRouter::stop()
{
    // Stub
}

void PlaybackRouter::next()
{
    // Stub
}

void PlaybackRouter::previous()
{
    // Stub
}

void PlaybackRouter::seek(int ms)
{
    Q_UNUSED(ms)
    // Stub
}

void PlaybackRouter::onActiveSourceChanged(MediaSource newSource)
{
    Q_UNUSED(newSource)
    // Stub
}
