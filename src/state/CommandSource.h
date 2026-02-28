#pragma once

#include <QObject>

class CommandSourceEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        Local = 0,
        External,
        API
    };
    Q_ENUM(Value)

    explicit CommandSourceEnum(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
};

using CommandSource = CommandSourceEnum::Value;
