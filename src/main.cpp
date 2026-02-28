#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStringList>
#include <QUrl>
#include <QtQml>

#include "app/AppBuilder.h"
#include "app/AppConfig.h"
#include "spotify/SpotifyAuth.h"
#include "state/ActiveView.h"
#include "state/CommandSource.h"
#include "state/MediaSource.h"
#include "state/PlaybackMode.h"
#include "state/PlaybackState.h"
#include "state/ReceiverState.h"
#include "state/StreamingService.h"
#include "state/UIState.h"
#include "utils/Logging.h"

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

        QObject::connect(auth, &SpotifyAuth::authorizationUrlReady,
                         [](const QUrl& url)
                         {
                             fprintf(stdout,
                                     "\n"
                                     "=== Spotify Authorization ===\n"
                                     "Open this URL in a browser:\n\n"
                                     "  %s\n\n"
                                     "Waiting for authorization callback...\n",
                                     qPrintable(url.toString()));
                             fflush(stdout);
                         });

        QObject::connect(auth, &SpotifyAuth::authFlowComplete,
                         [&app]()
                         {
                             fprintf(stdout,
                                     "\nAuthorization successful! Tokens saved.\n"
                                     "You can now start media-console normally.\n");
                             QCoreApplication::quit();
                         });

        QObject::connect(auth, &SpotifyAuth::authError,
                         [](const QString& error)
                         {
                             fprintf(stderr, "\nAuthorization failed: %s\n", qPrintable(error));
                             QCoreApplication::exit(1);
                         });

        auth->startAuthFlow();
        return app.exec();
    }

    // Normal startup: full QML application
    auto config = AppConfig::loadFromSettings();
    AppBuilder builder(&app);
    auto ctx = builder.build(config);

    qCInfo(mediaApp) << "Media Console" << VERSION_STRING << "starting";

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

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/MediaConsole/src/qml/main.qml"));
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
