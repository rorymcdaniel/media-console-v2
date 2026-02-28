#pragma once

#include <QString>

struct ReceiverConfig
{
    QString host = "192.168.68.63";
    int port = 60128;
};

struct DisplayConfig
{
    int id = 1;
    int dimTimeoutSeconds = 300;
    int offTimeoutSeconds = 1200;
    int dimBrightness = 25;
    bool timeoutEnabled = true;
};

struct CdConfig
{
    QString devicePath = "/dev/cdrom";
    int pollIntervalMs = 1000;
    bool audioOnly = true;
    int idleTimeoutSeconds = 300;
};

struct LibraryConfig
{
    QString rootPath = "/data/media/music/";
};

struct ApiConfig
{
    int port = 8080;
};

struct LoggingConfig
{
    QString filterRules;
};

struct AudioConfig
{
    QString deviceName = "hw:2,0";
};

struct SpotifyConfig
{
    QString clientId;
    QString clientSecret;
    QString desiredDeviceName = "Voice of Music";
};

struct GpioConfig
{
    QString chipPath = "/dev/gpiochip4";
    // Volume encoder (PEC11R-4020F-S0024, 24 PPR, smooth)
    int volumeEncoderPinA = 27;
    int volumeEncoderPinB = 22;
    int volumeEncoderDelta = 2; // volume steps per detent
    // Input encoder (PEC11R-4320F-S0012, 12 PPR, 12 detents)
    int inputEncoderPinA = 16;
    int inputEncoderPinB = 20;
    int inputEncoderButtonPin = 5; // push button (mute/select)
    int inputButtonDebounceMs = 250;
    // Reed switch
    int reedSwitchPin = 17;
    int reedSwitchDebounceMs = 500;
};

struct AppConfig
{
    ReceiverConfig receiver;
    DisplayConfig display;
    CdConfig cd;
    LibraryConfig library;
    AudioConfig audio;
    ApiConfig api;
    LoggingConfig logging;
    SpotifyConfig spotify;
    GpioConfig gpio;

    static AppConfig loadFromSettings();
};
