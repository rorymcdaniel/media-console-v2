#include "display/LinuxDisplayControl.h"

#include <QProcess>
#include <QRegularExpression>

#include "utils/Logging.h"

LinuxDisplayControl::LinuxDisplayControl(QObject* parent)
    : IDisplayControl(parent)
{
}

bool LinuxDisplayControl::autoDetectDisplay()
{
    QProcess process;
    process.start("ddcutil", { "detect", "--brief" });

    if (!process.waitForFinished(kDdcTimeoutMs))
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil detect timed out";
        process.kill();
        return false;
    }

    if (process.exitCode() != 0)
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil detect failed with exit code" << process.exitCode();
        const QString errorOutput = QString::fromUtf8(process.readAllStandardError());
        if (!errorOutput.isEmpty())
        {
            qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil stderr:" << errorOutput;
        }
        return false;
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    qCDebug(mediaApp) << "LinuxDisplayControl: ddcutil detect output:" << output;

    // Parse bus number from output like "/dev/i2c-1"
    static const QRegularExpression busRegex(R"(/dev/i2c-(\d+))");
    const auto match = busRegex.match(output);

    if (!match.hasMatch())
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: no I2C bus found in ddcutil output";
        return false;
    }

    m_busNumber = match.captured(1).toInt();
    qCInfo(mediaApp) << "LinuxDisplayControl: detected display on I2C bus" << m_busNumber;
    emit displayDetected(m_busNumber);
    return true;
}

bool LinuxDisplayControl::setPower(bool on)
{
    if (m_busNumber < 0)
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: no display detected, cannot set power";
        return false;
    }

    // VCP code 0xD6: DPMS control. 01 = on, 04 = off (standby)
    const QString powerValue = on ? "01" : "04";

    QProcess process;
    process.start("ddcutil", { "setvcp", "d6", powerValue, "--bus", QString::number(m_busNumber) });

    if (!process.waitForFinished(kDdcTimeoutMs))
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil setvcp d6 timed out";
        process.kill();
        return false;
    }

    if (process.exitCode() != 0)
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil setvcp d6 failed with exit code" << process.exitCode();
        return false;
    }

    m_powered = on;
    qCInfo(mediaApp) << "LinuxDisplayControl: power" << (on ? "ON" : "OFF");
    emit powerChanged(on);
    return true;
}

bool LinuxDisplayControl::setBrightness(int percent)
{
    if (m_busNumber < 0)
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: no display detected, cannot set brightness";
        return false;
    }

    // Clamp to valid range
    percent = qBound(0, percent, 100);

    QProcess process;
    process.start("ddcutil", { "setvcp", "10", QString::number(percent), "--bus", QString::number(m_busNumber) });

    if (!process.waitForFinished(kDdcTimeoutMs))
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil setvcp 10 timed out";
        process.kill();
        return false;
    }

    if (process.exitCode() != 0)
    {
        qCWarning(mediaApp) << "LinuxDisplayControl: ddcutil setvcp 10 failed with exit code" << process.exitCode();
        return false;
    }

    m_brightness = percent;
    qCInfo(mediaApp) << "LinuxDisplayControl: brightness" << percent << "%";
    emit brightnessChanged(percent);
    return true;
}

int LinuxDisplayControl::brightness() const
{
    return m_brightness;
}

bool LinuxDisplayControl::isPowered() const
{
    return m_powered;
}
