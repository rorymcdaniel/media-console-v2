#pragma once

#include <QObject>

class IDisplayControl : public QObject
{
    Q_OBJECT

public:
    explicit IDisplayControl(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~IDisplayControl() override = default;

    virtual bool autoDetectDisplay() = 0;
    virtual bool setPower(bool on) = 0;
    virtual bool setBrightness(int percent) = 0;
    virtual int brightness() const = 0;
    virtual bool isPowered() const = 0;

signals:
    void displayDetected(int busNumber);
    void powerChanged(bool on);
    void brightnessChanged(int percent);
};
