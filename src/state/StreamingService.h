#pragma once

#include <QObject>

class StreamingServiceEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Unknown = 0,
        Spotify,
        Pandora,
        AirPlay,
        AmazonMusic,
        Chromecast
    };
    Q_ENUM(Value)

    explicit StreamingServiceEnum(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
};

using StreamingService = StreamingServiceEnum::Value;
