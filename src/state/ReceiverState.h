#pragma once

#include <QObject>
#include <QString>

#include "state/MediaSource.h"
#include "state/StreamingService.h"

class ReceiverState : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(MediaSourceEnum::Value currentInput READ currentInput WRITE setCurrentInput NOTIFY currentInputChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString artist READ artist WRITE setArtist NOTIFY artistChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY albumChanged)
    Q_PROPERTY(QString albumArtUrl READ albumArtUrl WRITE setAlbumArtUrl NOTIFY albumArtUrlChanged)
    Q_PROPERTY(QString fileInfo READ fileInfo WRITE setFileInfo NOTIFY fileInfoChanged)
    Q_PROPERTY(QString serviceName READ serviceName WRITE setServiceName NOTIFY serviceNameChanged)
    Q_PROPERTY(StreamingServiceEnum::Value streamingService READ streamingService WRITE setStreamingService NOTIFY
                   streamingServiceChanged)

public:
    explicit ReceiverState(QObject* parent = nullptr);

    int volume() const { return m_volume; }
    bool powered() const { return m_powered; }
    bool muted() const { return m_muted; }
    MediaSource currentInput() const { return m_currentInput; }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString album() const { return m_album; }
    QString albumArtUrl() const { return m_albumArtUrl; }
    QString fileInfo() const { return m_fileInfo; }
    QString serviceName() const { return m_serviceName; }
    StreamingService streamingService() const { return m_streamingService; }

public slots:
    void setVolume(int volume);
    void setPowered(bool powered);
    void setMuted(bool muted);
    void setCurrentInput(MediaSource input);
    void setTitle(const QString& title);
    void setArtist(const QString& artist);
    void setAlbum(const QString& album);
    void setAlbumArtUrl(const QString& url);
    void setFileInfo(const QString& fileInfo);
    void setServiceName(const QString& serviceName);
    void setStreamingService(StreamingService service);

signals:
    void volumeChanged(int volume);
    void poweredChanged(bool powered);
    void mutedChanged(bool muted);
    void currentInputChanged(MediaSource input);
    void titleChanged(const QString& title);
    void artistChanged(const QString& artist);
    void albumChanged(const QString& album);
    void albumArtUrlChanged(const QString& url);
    void fileInfoChanged(const QString& fileInfo);
    void serviceNameChanged(const QString& serviceName);
    void streamingServiceChanged(StreamingService service);

private:
    int m_volume = 0;
    bool m_powered = false;
    bool m_muted = false;
    MediaSource m_currentInput = MediaSource::None;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_albumArtUrl;
    QString m_fileInfo;
    QString m_serviceName;
    StreamingService m_streamingService = StreamingService::Unknown;
};
