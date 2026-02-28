#include "AppConfig.h"

#include <QSettings>

AppConfig AppConfig::loadFromSettings()
{
    QSettings settings;
    AppConfig config;

    // Receiver
    config.receiver.host = settings.value("receiver/host", config.receiver.host).toString();
    config.receiver.port = settings.value("receiver/port", config.receiver.port).toInt();

    // Display
    config.display.id = settings.value("display/id", config.display.id).toInt();
    config.display.dimTimeoutSeconds
        = settings.value("display/timeout_to_dim_seconds", config.display.dimTimeoutSeconds).toInt();
    config.display.offTimeoutSeconds
        = settings.value("display/timeout_to_off_seconds", config.display.offTimeoutSeconds).toInt();
    config.display.dimBrightness = settings.value("display/dim_brightness", config.display.dimBrightness).toInt();
    config.display.timeoutEnabled = settings.value("display/timeout_enabled", config.display.timeoutEnabled).toBool();

    // CD
    config.cd.devicePath = settings.value("cd/device", config.cd.devicePath).toString();
    config.cd.pollIntervalMs = settings.value("cd/poll_interval_ms", config.cd.pollIntervalMs).toInt();
    config.cd.audioOnly = settings.value("cd/audio_only", config.cd.audioOnly).toBool();

    // Library
    config.library.rootPath = settings.value("library/root", config.library.rootPath).toString();

    // API
    config.api.port = settings.value("api/port", config.api.port).toInt();

    // Logging
    config.logging.filterRules = settings.value("logging/rules", config.logging.filterRules).toString();

    // Spotify
    config.spotify.clientId = settings.value("spotify/client_id", config.spotify.clientId).toString();
    config.spotify.clientSecret = settings.value("spotify/client_secret", config.spotify.clientSecret).toString();
    config.spotify.desiredDeviceName
        = settings.value("spotify/desired_device_name", config.spotify.desiredDeviceName).toString();

    return config;
}
