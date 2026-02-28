#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "library/LibraryDatabase.h"

class LibraryAlbumArtProvider;

/// QAbstractListModel exposing albums for a given artist, sorted by year.
/// Setting the artistFilter Q_PROPERTY triggers re-query of the database (FLAC-07).
class LibraryAlbumModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString artistFilter READ artistFilter WRITE setArtistFilter NOTIFY artistFilterChanged)

public:
    enum Roles
    {
        AlbumRole = Qt::UserRole + 1,
        YearRole,
        TrackCountRole,
        ArtPathRole
    };

    LibraryAlbumModel(LibraryDatabase* database, LibraryAlbumArtProvider* artProvider, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString artistFilter() const;
    void setArtistFilter(const QString& artist);

public slots:
    void refresh();

signals:
    void artistFilterChanged();

private:
    LibraryDatabase* m_database; // Non-owning
    LibraryAlbumArtProvider* m_artProvider; // Non-owning
    QString m_artistFilter;
    QVector<LibraryAlbumEntry> m_albums;
};
