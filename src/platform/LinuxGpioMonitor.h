#pragma once

#ifdef HAS_GPIOD

#include <QThread>

#include <atomic>
#include <gpiod.hpp>
#include <memory>

#include "app/AppConfig.h"
#include "platform/IGpioMonitor.h"
#include "platform/QuadratureDecoder.h"

/// Real GPIO monitor using libgpiod v2 on Linux.
/// Monitors two rotary encoders (volume + input) and a reed switch via
/// a single line_request with poll()-based edge event monitoring in a
/// background QThread.
class LinuxGpioMonitor : public IGpioMonitor
{
    Q_OBJECT

public:
    explicit LinuxGpioMonitor(const GpioConfig& config, QObject* parent = nullptr);
    ~LinuxGpioMonitor() override;

    bool start() override;
    void stop() override;

private:
    void monitorLoop();
    void handleEvent(const gpiod::edge_event& event);

    GpioConfig m_config;
    std::unique_ptr<gpiod::line_request> m_request;
    QThread m_thread;
    std::atomic<bool> m_stopRequested { false };

    QuadratureDecoder m_volumeDecoder;
    QuadratureDecoder m_inputDecoder;
};

#endif // HAS_GPIOD
