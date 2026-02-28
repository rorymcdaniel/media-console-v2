#pragma once

#include <QObject>

#include <cstdint>

class MediaSourceEnum : public QObject
{
    Q_OBJECT

public:
    enum Value
    {
        None = 0,
        Streaming,
        Phono,
        CD,
        Computer,
        Bluetooth,
        Library
    };
    Q_ENUM(Value)

    explicit MediaSourceEnum(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
};

using MediaSource = MediaSourceEnum::Value;

/// Convert a user-facing MediaSource to the Onkyo eISCP hex code for input selection.
uint8_t toHexCode(MediaSource source);

/// Convert an Onkyo eISCP hex code to a user-facing MediaSource.
/// Note: Library and CD share hex code 0x23 — fromHexCode returns CD.
/// The application distinguishes them by context (active playback source).
MediaSource fromHexCode(uint8_t code);
