#pragma once

#include <QObject>
#include <QString>

#include "state/ActiveView.h"

class UIState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ActiveViewEnum::Value activeView READ activeView WRITE setActiveView NOTIFY activeViewChanged)
    Q_PROPERTY(bool volumeOverlayVisible READ volumeOverlayVisible WRITE setVolumeOverlayVisible NOTIFY
                   volumeOverlayVisibleChanged)
    Q_PROPERTY(
        bool errorBannerVisible READ errorBannerVisible WRITE setErrorBannerVisible NOTIFY errorBannerVisibleChanged)
    Q_PROPERTY(bool toastVisible READ toastVisible WRITE setToastVisible NOTIFY toastVisibleChanged)
    Q_PROPERTY(QString toastMessage READ toastMessage WRITE setToastMessage NOTIFY toastMessageChanged)
    Q_PROPERTY(QString toastType READ toastType WRITE setToastType NOTIFY toastTypeChanged)
    Q_PROPERTY(bool receiverConnected READ receiverConnected WRITE setReceiverConnected NOTIFY receiverConnectedChanged)
    Q_PROPERTY(QString audioError READ audioError WRITE setAudioError NOTIFY audioErrorChanged)
    Q_PROPERTY(bool doorOpen READ doorOpen WRITE setDoorOpen NOTIFY doorOpenChanged)
    Q_PROPERTY(bool screenDimmed READ screenDimmed WRITE setScreenDimmed NOTIFY screenDimmedChanged)

public:
    explicit UIState(QObject* parent = nullptr);

    ActiveView activeView() const { return m_activeView; }
    bool volumeOverlayVisible() const { return m_volumeOverlayVisible; }
    bool errorBannerVisible() const { return m_errorBannerVisible; }
    bool toastVisible() const { return m_toastVisible; }
    QString toastMessage() const { return m_toastMessage; }
    QString toastType() const { return m_toastType; }
    bool receiverConnected() const { return m_receiverConnected; }
    QString audioError() const { return m_audioError; }
    bool doorOpen() const { return m_doorOpen; }
    bool screenDimmed() const { return m_screenDimmed; }

public slots:
    void setActiveView(ActiveView view);
    void setVolumeOverlayVisible(bool visible);
    void setErrorBannerVisible(bool visible);
    void setToastVisible(bool visible);
    void setToastMessage(const QString& message);
    void setToastType(const QString& type);
    void setReceiverConnected(bool connected);
    void setAudioError(const QString& error);
    void setDoorOpen(bool open);
    void setScreenDimmed(bool dimmed);

signals:
    void activeViewChanged(ActiveView view);
    void volumeOverlayVisibleChanged(bool visible);
    void errorBannerVisibleChanged(bool visible);
    void toastVisibleChanged(bool visible);
    void toastMessageChanged(const QString& message);
    void toastTypeChanged(const QString& type);
    void receiverConnectedChanged(bool connected);
    void audioErrorChanged(const QString& error);
    void doorOpenChanged(bool open);
    void screenDimmedChanged(bool dimmed);

    /// Transient error signal for toast display — not a property, just an event.
    void showToast(const QString& message, const QString& type);

private:
    ActiveView m_activeView = ActiveView::NowPlaying;
    bool m_volumeOverlayVisible = false;
    bool m_errorBannerVisible = false;
    bool m_toastVisible = false;
    QString m_toastMessage;
    QString m_toastType;
    bool m_receiverConnected = false;
    QString m_audioError;
    bool m_doorOpen = true; // default: door open = display on
    bool m_screenDimmed = false; // set by ScreenTimeoutController
};
