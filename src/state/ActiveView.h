#pragma once

#include <QObject>

class ActiveViewEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        NowPlaying = 0,
        LibraryBrowser,
        SpotifySearch
    };
    Q_ENUM(Value)

    explicit ActiveViewEnum(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
};

using ActiveView = ActiveViewEnum::Value;
