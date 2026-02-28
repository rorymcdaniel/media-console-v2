#pragma once

#include <QObject>

#include <memory>

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;

class PlatformFactory
{
public:
    static std::unique_ptr<IAudioOutput> createAudioOutput();
    static std::unique_ptr<ICdDrive> createCdDrive();
    static std::unique_ptr<IGpioMonitor> createGpioMonitor(QObject* parent = nullptr);
    static std::unique_ptr<IDisplayControl> createDisplayControl(QObject* parent = nullptr);

    static bool isLinux();
};
