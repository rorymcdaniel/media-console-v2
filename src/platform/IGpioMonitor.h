#pragma once

#include <QObject>

class IGpioMonitor : public QObject
{
    Q_OBJECT

public:
    explicit IGpioMonitor(QObject* parent = nullptr)
        : QObject(parent)
    {
    }
    ~IGpioMonitor() override = default;

    virtual bool start() = 0;
    virtual void stop() = 0;

signals:
    // Volume encoder signal — carries pre-scaled delta (+N/-N per detent)
    void volumeChanged(int delta);
    void muteToggled();

    // Input encoder signals
    void inputNext();
    void inputPrevious();
    void inputSelect();

    // Reed switch signal
    void reedSwitchChanged(bool magnetsApart);
};
