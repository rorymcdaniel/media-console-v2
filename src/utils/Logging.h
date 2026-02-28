#pragma once

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(mediaApp)
Q_DECLARE_LOGGING_CATEGORY(mediaSpotify)
Q_DECLARE_LOGGING_CATEGORY(mediaReceiver)
Q_DECLARE_LOGGING_CATEGORY(mediaAudio)
Q_DECLARE_LOGGING_CATEGORY(mediaHttp)
Q_DECLARE_LOGGING_CATEGORY(mediaLidarr)
Q_DECLARE_LOGGING_CATEGORY(mediaGpio)
Q_DECLARE_LOGGING_CATEGORY(mediaCd)
Q_DECLARE_LOGGING_CATEGORY(mediaLibrary)

// Initialize logging with optional filter rules string.
// If filterRules is empty, defaults to info level (debug disabled).
void initLogging(const QString& filterRules = QString());
