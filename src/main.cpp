#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QUrl>

#include "utils/Logging.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("MediaConsole");
    QCoreApplication::setApplicationName("media-console");

    initLogging();
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

    if (engine.rootObjects().isEmpty())
    {
        qCCritical(mediaApp) << "Failed to load QML";
        return -1;
    }

    qCInfo(mediaApp) << "Media Console started successfully";
    return app.exec();
}
