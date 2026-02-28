#include "state/MediaSource.h"

uint8_t toHexCode(MediaSource source)
{
    switch (source)
    {
    case MediaSource::Streaming:
        return 0x2B;
    case MediaSource::Phono:
        return 0x22;
    case MediaSource::CD:
        return 0x23;
    case MediaSource::Computer:
        return 0x05;
    case MediaSource::Bluetooth:
        return 0x2E;
    case MediaSource::Library:
        return 0x23; // Library uses the CD input on the receiver
    case MediaSource::None:
    default:
        return 0x00;
    }
}

MediaSource fromHexCode(uint8_t code)
{
    switch (code)
    {
    case 0x2B:
        return MediaSource::Streaming;
    case 0x22:
        return MediaSource::Phono;
    case 0x23:
        return MediaSource::CD; // Ambiguous with Library — CD is default
    case 0x05:
        return MediaSource::Computer;
    case 0x2E:
        return MediaSource::Bluetooth;
    default:
        return MediaSource::None;
    }
}
