#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QtQml>

#include "app/AppBuilder.h"
#include "app/AppConfig.h"
#include "state/ActiveView.h"
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
