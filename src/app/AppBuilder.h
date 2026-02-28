#pragma once

#include <QObject>

#include <memory>

#include "app/AppConfig.h"
#include "app/AppContext.h"

class IAudioOutput;
class ICdDrive;
class IGpioMonitor;
class IDisplayControl;

class AppBuilder : public QObject
{
    Q_OBJECT

public:
    explicit AppBuilder(QObject* parent = nullptr);
    ~AppBuilder() override;

    AppContext build(const AppConfig& config);

private:
    std::unique_ptr<IAudioOutput> m_audioOutput;
    std::unique_ptr<ICdDrive> m_cdDrive;
    std::unique_ptr<IGpioMonitor> m_gpioMonitor;
    std::unique_ptr<IDisplayControl> m_displayControl;
};
