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

    static AppConfig loadFromSettings();
};
