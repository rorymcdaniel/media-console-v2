#pragma once

#include <QObject>

#include <memory>

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;
struct GpioConfig;

class PlatformFactory
{
public:
    static std::unique_ptr<IAudioOutput> createAudioOutput();
    static std::unique_ptr<ICdDrive> createCdDrive();
    static std::unique_ptr<IGpioMonitor> createGpioMonitor(const GpioConfig& config, QObject* parent = nullptr);
    static std::unique_ptr<IDisplayControl> createDisplayControl(QObject* parent = nullptr);

    static bool isLinux();
};
