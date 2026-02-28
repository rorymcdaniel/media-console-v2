#ifdef HAS_GPIOD

#include "LinuxGpioMonitor.h"

#include <cerrno>
#include <cstring>
#include <poll.h>

#include "utils/Logging.h"

namespace
{

int gpiodValueToInt(gpiod::line::value v)
{
    return v == gpiod::line::value::ACTIVE ? 1 : 0;
}

} // namespace

LinuxGpioMonitor::LinuxGpioMonitor(const GpioConfig& config, QObject* parent)
    : IGpioMonitor(parent)
    , m_config(config)
{
}

LinuxGpioMonitor::~LinuxGpioMonitor()
{
    stop();
}

bool LinuxGpioMonitor::start()
{
    try
    {
        // Open GPIO chip
        gpiod::chip chip(m_config.chipPath.toStdString());
        qCInfo(mediaGpio) << "LinuxGpioMonitor: opened" << m_config.chipPath;

        // Encoder line settings: BOTH edges, pull-up, NO debounce (state machine handles bounce)
        gpiod::line_settings encoderSettings;
        encoderSettings.set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_bias(gpiod::line::bias::PULL_UP);

        // Push button settings: FALLING edge only, pull-up, 250ms debounce
        gpiod::line_settings buttonSettings;
        buttonSettings.set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::FALLING)
            .set_bias(gpiod::line::bias::PULL_UP)
            .set_debounce_period(std::chrono::milliseconds(m_config.inputButtonDebounceMs));

        // Reed switch settings: BOTH edges, pull-up, 500ms debounce
        gpiod::line_settings reedSettings;
        reedSettings.set_direction(gpiod::line::direction::INPUT)
            .set_edge_detection(gpiod::line::edge::BOTH)
            .set_bias(gpiod::line::bias::PULL_UP)
            .set_debounce_period(std::chrono::milliseconds(m_config.reedSwitchDebounceMs));

        // Build single request with per-line settings
        auto request = chip.prepare_request()
                           .set_consumer("media-console-gpio")
                           // Volume encoder A/B
                           .add_line_settings({ static_cast<unsigned int>(m_config.volumeEncoderPinA),
                                                static_cast<unsigned int>(m_config.volumeEncoderPinB) },
                                              encoderSettings)
                           // Input encoder A/B
                           .add_line_settings({ static_cast<unsigned int>(m_config.inputEncoderPinA),
                                                static_cast<unsigned int>(m_config.inputEncoderPinB) },
                                              encoderSettings)
                           // Input encoder push button (mute/select)
                           .add_line_settings(static_cast<unsigned int>(m_config.inputEncoderButtonPin), buttonSettings)
                           // Reed switch
                           .add_line_settings(static_cast<unsigned int>(m_config.reedSwitchPin), reedSettings)
                           .do_request();

        m_request = std::make_unique<gpiod::line_request>(std::move(request));

        // Read initial encoder states for quadrature decoders
        auto volA = m_request->get_value(static_cast<unsigned int>(m_config.volumeEncoderPinA));
        auto volB = m_request->get_value(static_cast<unsigned int>(m_config.volumeEncoderPinB));
        m_volumeDecoder.reset(gpiodValueToInt(volA), gpiodValueToInt(volB));

        auto inA = m_request->get_value(static_cast<unsigned int>(m_config.inputEncoderPinA));
        auto inB = m_request->get_value(static_cast<unsigned int>(m_config.inputEncoderPinB));
        m_inputDecoder.reset(gpiodValueToInt(inA), gpiodValueToInt(inB));

        // Read initial reed switch state — no ambiguous initial state
        auto reedValue = m_request->get_value(static_cast<unsigned int>(m_config.reedSwitchPin));
        // INACTIVE (high/pulled up) = magnets apart = door open = true
        bool initialDoorOpen = (reedValue == gpiod::line::value::INACTIVE);
        emit reedSwitchChanged(initialDoorOpen);
        qCInfo(mediaGpio) << "LinuxGpioMonitor: initial reed switch state:" << (initialDoorOpen ? "open" : "closed");

        // Start monitoring thread
        m_stopRequested.store(false);
        m_thread = *QThread::create([this]() { monitorLoop(); });
        m_thread.setObjectName("GpioMonitor");
        m_thread.start();

        qCInfo(mediaGpio) << "LinuxGpioMonitor: started monitoring" << 7 << "GPIO lines";
        return true;
    }
    catch (const std::exception& e)
    {
        qCWarning(mediaGpio) << "LinuxGpioMonitor: failed to start:" << e.what();
        return false;
    }
}

void LinuxGpioMonitor::stop()
{
    if (!m_thread.isRunning())
        return;

    m_stopRequested.store(true);
    m_thread.wait();
    m_request.reset();
    m_stopRequested.store(false);

    qCInfo(mediaGpio) << "LinuxGpioMonitor: stopped";
}

void LinuxGpioMonitor::monitorLoop()
{
    gpiod::edge_event_buffer buffer(64);
    struct pollfd pfd;
    pfd.fd = m_request->fd();
    pfd.events = POLLIN;

    while (!m_stopRequested.load(std::memory_order_relaxed))
    {
        int ret = poll(&pfd, 1, 100); // 100ms timeout for stop check
        if (ret < 0)
        {
            if (errno == EINTR)
                continue;
            qCWarning(mediaGpio) << "LinuxGpioMonitor: poll() error:" << strerror(errno);
            break;
        }
        if (ret == 0)
            continue; // timeout, check stop flag

        m_request->read_edge_events(buffer);
        for (const auto& event : buffer)
        {
            handleEvent(event);
        }
    }
}

void LinuxGpioMonitor::handleEvent(const gpiod::edge_event& event)
{
    auto offset = event.line_offset();

    if (offset == static_cast<unsigned int>(m_config.volumeEncoderPinA)
        || offset == static_cast<unsigned int>(m_config.volumeEncoderPinB))
    {
        // Volume encoder — read both channels and decode
        auto valA = m_request->get_value(static_cast<unsigned int>(m_config.volumeEncoderPinA));
        auto valB = m_request->get_value(static_cast<unsigned int>(m_config.volumeEncoderPinB));
        int direction = m_volumeDecoder.update(gpiodValueToInt(valA), gpiodValueToInt(valB));
        if (direction != 0)
        {
            emit volumeChanged(direction * m_config.volumeEncoderDelta);
        }
    }
    else if (offset == static_cast<unsigned int>(m_config.inputEncoderPinA)
             || offset == static_cast<unsigned int>(m_config.inputEncoderPinB))
    {
        // Input encoder — read both channels and decode
        auto valA = m_request->get_value(static_cast<unsigned int>(m_config.inputEncoderPinA));
        auto valB = m_request->get_value(static_cast<unsigned int>(m_config.inputEncoderPinB));
        int direction = m_inputDecoder.update(gpiodValueToInt(valA), gpiodValueToInt(valB));
        if (direction == +1)
        {
            emit inputNext();
        }
        else if (direction == -1)
        {
            emit inputPrevious();
        }
    }
    else if (offset == static_cast<unsigned int>(m_config.inputEncoderButtonPin))
    {
        // Push button — FALLING edge only (configured in line settings)
        // GPIO layer emits inputSelect(); wiring layer routes to mute or carousel confirm
        emit inputSelect();
    }
    else if (offset == static_cast<unsigned int>(m_config.reedSwitchPin))
    {
        // Reed switch — read current value
        auto value = m_request->get_value(static_cast<unsigned int>(m_config.reedSwitchPin));
        // INACTIVE (high/pulled up) = magnets apart = door open = true
        bool magnetsApart = (value == gpiod::line::value::INACTIVE);
        emit reedSwitchChanged(magnetsApart);
    }
}

#endif // HAS_GPIOD
