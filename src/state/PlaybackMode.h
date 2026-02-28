#pragma once

#include <QObject>

class PlaybackModeEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Stopped = 0,
        Playing,
        Paused
    };
    Q_ENUM(Value)

    explicit PlaybackModeEnum(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
};

using PlaybackMode = PlaybackModeEnum::Value;
