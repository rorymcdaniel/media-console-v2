#include "Logging.h"

Q_LOGGING_CATEGORY(mediaApp, "media.app")
Q_LOGGING_CATEGORY(mediaSpotify, "media.spotify")
Q_LOGGING_CATEGORY(mediaReceiver, "media.receiver")
Q_LOGGING_CATEGORY(mediaAudio, "media.audio")
Q_LOGGING_CATEGORY(mediaHttp, "media.http")
Q_LOGGING_CATEGORY(mediaLidarr, "media.lidarr")
Q_LOGGING_CATEGORY(mediaGpio, "media.gpio")
Q_LOGGING_CATEGORY(mediaCd, "media.cd")

void initLogging(const QString& filterRules)
{
    if (filterRules.isEmpty())
    {
        // Default: info and above for all categories
        QLoggingCategory::setFilterRules(QStringLiteral("media.*.debug=false"));
    }
    else
    {
        QLoggingCategory::setFilterRules(filterRules);
    }
}
