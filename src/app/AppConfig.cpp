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
    config.cd.idleTimeoutSeconds = settings.value("cd/idle_timeout_seconds", config.cd.idleTimeoutSeconds).toInt();

    // Library
    config.library.rootPath = settings.value("library/root", config.library.rootPath).toString();

    // API
    config.api.port = settings.value("api/port", config.api.port).toInt();

    // Audio
    config.audio.deviceName = settings.value("audio/deviceName", config.audio.deviceName).toString();

    // Logging
    config.logging.filterRules = settings.value("logging/rules", config.logging.filterRules).toString();

    // Spotify
    config.spotify.clientId = settings.value("spotify/client_id", config.spotify.clientId).toString();
    config.spotify.clientSecret = settings.value("spotify/client_secret", config.spotify.clientSecret).toString();
    config.spotify.desiredDeviceName
        = settings.value("spotify/desired_device_name", config.spotify.desiredDeviceName).toString();
    config.spotify.redirectPort = settings.value("spotify/redirect_port", config.spotify.redirectPort).toInt();

    // GPIO
    config.gpio.chipPath = settings.value("gpio/chip_path", config.gpio.chipPath).toString();
    config.gpio.volumeEncoderPinA = settings.value("gpio/volume_encoder_pin_a", config.gpio.volumeEncoderPinA).toInt();
    config.gpio.volumeEncoderPinB = settings.value("gpio/volume_encoder_pin_b", config.gpio.volumeEncoderPinB).toInt();
    config.gpio.volumeEncoderDelta
        = settings.value("gpio/volume_encoder_delta", config.gpio.volumeEncoderDelta).toInt();
    config.gpio.inputEncoderPinA = settings.value("gpio/input_encoder_pin_a", config.gpio.inputEncoderPinA).toInt();
    config.gpio.inputEncoderPinB = settings.value("gpio/input_encoder_pin_b", config.gpio.inputEncoderPinB).toInt();
    config.gpio.inputEncoderButtonPin
        = settings.value("gpio/input_encoder_button_pin", config.gpio.inputEncoderButtonPin).toInt();
    config.gpio.inputButtonDebounceMs
        = settings.value("gpio/input_button_debounce_ms", config.gpio.inputButtonDebounceMs).toInt();
    config.gpio.reedSwitchPin = settings.value("gpio/reed_switch_pin", config.gpio.reedSwitchPin).toInt();
    config.gpio.reedSwitchDebounceMs
        = settings.value("gpio/reed_switch_debounce_ms", config.gpio.reedSwitchDebounceMs).toInt();

    return config;
}
