#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "app/AppBuilder.h"
#include "app/AppConfig.h"
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
