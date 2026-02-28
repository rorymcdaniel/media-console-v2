#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "library/LibraryDatabase.h"

/// QAbstractListModel exposing tracks for a given artist+album, sorted by disc then track number.
/// Setting artistFilter and albumFilter Q_PROPERTYs triggers re-query. Multi-disc albums
/// appear as a flat list ordered by disc number then track number (FLAC-07).
class LibraryTrackModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString artistFilter READ artistFilter WRITE setArtistFilter NOTIFY artistFilterChanged)
    Q_PROPERTY(QString albumFilter READ albumFilter WRITE setAlbumFilter NOTIFY albumFilterChanged)

public:
    enum Roles
    {
        TrackNumberRole = Qt::UserRole + 1,
        DiscNumberRole,
        TitleRole,
        ArtistRole,
        DurationSecondsRole,
        FilePathRole
    };

    explicit LibraryTrackModel(LibraryDatabase* database, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString artistFilter() const;
    void setArtistFilter(const QString& artist);
    QString albumFilter() const;
    void setAlbumFilter(const QString& album);

    /// Get track data at index (for playlist building).
    LibraryTrack trackAt(int index) const;

    /// Get all currently loaded tracks (for playlist building).
    QVector<LibraryTrack> allTracks() const;

public slots:
    void refresh();

signals:
    void artistFilterChanged();
    void albumFilterChanged();

private:
    LibraryDatabase* m_database; // Non-owning
    QString m_artistFilter;
    QString m_albumFilter;
    QVector<LibraryTrack> m_tracks;
};
