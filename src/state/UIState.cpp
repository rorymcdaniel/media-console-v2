#include "state/UIState.h"

UIState::UIState(QObject* parent)
    : QObject(parent)
{
}

void UIState::setActiveView(ActiveView view)
{
    if (m_activeView == view)
        return;
    m_activeView = view;
    emit activeViewChanged(m_activeView);
}

void UIState::setVolumeOverlayVisible(bool visible)
{
    if (m_volumeOverlayVisible == visible)
        return;
    m_volumeOverlayVisible = visible;
    emit volumeOverlayVisibleChanged(m_volumeOverlayVisible);
}

void UIState::setErrorBannerVisible(bool visible)
{
    if (m_errorBannerVisible == visible)
        return;
    m_errorBannerVisible = visible;
    emit errorBannerVisibleChanged(m_errorBannerVisible);
}

void UIState::setToastVisible(bool visible)
{
    if (m_toastVisible == visible)
        return;
    m_toastVisible = visible;
    emit toastVisibleChanged(m_toastVisible);
}

void UIState::setToastMessage(const QString& message)
{
    if (m_toastMessage == message)
        return;
    m_toastMessage = message;
    emit toastMessageChanged(m_toastMessage);
}

void UIState::setToastType(const QString& type)
{
    if (m_toastType == type)
        return;
    m_toastType = type;
    emit toastTypeChanged(m_toastType);
}

void UIState::setReceiverConnected(bool connected)
{
    if (m_receiverConnected == connected)
        return;
    m_receiverConnected = connected;
    emit receiverConnectedChanged(m_receiverConnected);
}

void UIState::setAudioError(const QString& error)
{
    if (m_audioError == error)
        return;
    m_audioError = error;
    emit audioErrorChanged(m_audioError);
}

void UIState::setDoorOpen(bool open)
{
    if (m_doorOpen == open)
        return;
    m_doorOpen = open;
    emit doorOpenChanged(m_doorOpen);
}
