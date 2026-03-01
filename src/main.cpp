#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStringList>
#include <QUrl>
#include <QtQml>

#include "app/AppBuilder.h"
#include "app/AppConfig.h"
#include "cd/CdController.h"
#include "display/ScreenTimeoutController.h"
#include "orchestration/AlbumArtResolver.h"
#include "orchestration/PlaybackRouter.h"
#include "receiver/ReceiverController.h"
#include "spotify/CliOAuthReplyHandler.h"
#include "spotify/SpotifyAuth.h"
#include "spotify/SpotifyController.h"
#include "state/ActiveView.h"
#include "state/CommandSource.h"
#include "state/MediaSource.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/StreamingService.h"
#include "state/UIState.h"
#include "utils/Logging.h"

#ifdef HAS_SNDFILE
#include "library/FlacLibraryController.h"
#include "library/LibraryAlbumModel.h"
#include "library/LibraryArtistModel.h"
#include "library/LibraryTrackModel.h"
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MediaConsole");
    QCoreApplication::setApplicationName("media-console");

    // CLI: --spotify-auth mode (headless OAuth flow, no QML)
    if (app.arguments().contains(QStringLiteral("--spotify-auth")))
    {
        auto authConfig = AppConfig::loadFromSettings();

        if (authConfig.spotify.clientId.isEmpty())
        {
            fprintf(stderr,
                    "Error: spotify/client_id not set in config.\n"
                    "Set it in the INI file first.\n");
            return 1;
        }

        auto* auth = new SpotifyAuth(authConfig.spotify, &app);
        auto* cliHandler = auth->useCliReplyHandler(QStringLiteral("https://127.0.0.1:8888/callback"));

        // PKCE URL generation is entirely local — grant() emits authorizationUrlReady
        // synchronously before returning. Capture the URL, then block on stdin before
        // entering the event loop (which handles the async token exchange request).
        QUrl capturedUrl;
        QObject::connect(auth, &SpotifyAuth::authorizationUrlReady,
                         [&capturedUrl](const QUrl& url) { capturedUrl = url; });

        auth->startAuthFlow();

        if (capturedUrl.isEmpty())
        {
            fprintf(stderr, "Failed to generate authorization URL.\n");
            return 1;
        }

        fprintf(stdout,
                "\n"
                "=== Spotify Authorization ===\n"
                "1. Open this URL in your browser:\n\n"
                "   %s\n\n"
                "2. Authorize the app.\n"
                "3. The browser will fail to connect — that's expected.\n"
                "   Copy the full URL from the address bar.\n\n"
                "Paste the redirect URL and press Enter:\n> ",
                qPrintable(capturedUrl.toString()));
        fflush(stdout);

        // Blocking read before the event loop — no threads, no notifiers.
        char buf[4096] = {};
        if (!fgets(buf, sizeof(buf), stdin) || buf[0] == '\n' || buf[0] == '\0')
        {
            fprintf(stderr, "No URL provided.\n");
            return 1;
        }
        const QString redirectUrl = QString::fromLocal8Bit(buf).trimmed();

        fprintf(stdout, "\nExchanging code for tokens...\n");
        fflush(stdout);

        cliHandler->handleRedirectUrl(QUrl(redirectUrl));

        QObject::connect(auth, &SpotifyAuth::authFlowComplete,
                         []()
                         {
                             fprintf(stdout,
                                     "Authorization successful! Tokens saved.\n"
                                     "You can now start media-console normally.\n");
                             QCoreApplication::quit();
                         });

        QObject::connect(auth, &SpotifyAuth::authError,
                         [](const QString& error)
                         {
                             fprintf(stderr, "Authorization failed: %s\n", qPrintable(error));
                             QCoreApplication::exit(1);
                         });

        return app.exec();
    }

    // Normal startup: full QML application
    auto config = AppConfig::loadFromSettings();
    AppBuilder builder(&app);
    auto ctx = builder.build(config);

    qCInfo(mediaApp) << "Media Console" << VERSION_STRING << "starting";

    // Bug 2 (AUDIO-07): Wire Restart Now button action — pluggable restart mechanism.
    // QML calls UIState.restartRequested(); this slot performs the actual quit.
    // Using quit() so systemd restarts the process. Can be changed to systemctl reboot
    // later without any QML changes.
    QObject::connect(ctx.uiState, &UIState::restartRequested, &app,
                     []()
                     {
                         qCCritical(mediaApp)
                             << "Restart requested from AudioErrorDialog — exiting for systemd restart";
                         QCoreApplication::quit();
                     });

    // Register enum types as uncreatable (QML can reference values but not instantiate)
    qmlRegisterUncreatableType<MediaSourceEnum>("MediaConsole", 1, 0, "MediaSource", "MediaSource is an enum type");
    qmlRegisterUncreatableType<PlaybackModeEnum>("MediaConsole", 1, 0, "PlaybackMode", "PlaybackMode is an enum type");
    qmlRegisterUncreatableType<ActiveViewEnum>("MediaConsole", 1, 0, "ActiveView", "ActiveView is an enum type");
    qmlRegisterUncreatableType<StreamingServiceEnum>("MediaConsole", 1, 0, "StreamingService",
                                                     "StreamingService is an enum type");
    qmlRegisterUncreatableType<CommandSourceEnum>("MediaConsole", 1, 0, "CommandSource",
                                                  "CommandSource is an enum type");

    // Register state object singletons (MUST be before engine.load())
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "ReceiverState", ctx.receiverState);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackState", ctx.playbackState);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "UIState", ctx.uiState);

    // Register Phase 10 controller singletons for QML access
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "PlaybackRouter", ctx.playbackRouter);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "AlbumArtResolver", ctx.albumArtResolver);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "SpotifyController", ctx.spotifyController);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "ReceiverController", ctx.receiverController);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "ScreenTimeoutController", ctx.screenTimeoutController);

    // Register Phase 11 controller singletons (UI-13: CdController QML access)
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "CdController", ctx.cdController);

#ifdef HAS_SNDFILE
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "FlacLibraryController", ctx.flacLibraryController);
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryArtistModel", ctx.flacLibraryController->artistModel());
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryAlbumModel", ctx.flacLibraryController->albumModel());
    qmlRegisterSingletonInstance("MediaConsole", 1, 0, "LibraryTrackModel", ctx.flacLibraryController->trackModel());
#endif

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/qt/qml/MediaConsole/src/qml/main.qml"));
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject* obj, const QUrl& objUrl)
        {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
