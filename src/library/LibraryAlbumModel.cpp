#include "LibraryAlbumModel.h"

#include "library/LibraryAlbumArtProvider.h"

LibraryAlbumModel::LibraryAlbumModel(LibraryDatabase* database, LibraryAlbumArtProvider* artProvider, QObject* parent)
    : QAbstractListModel(parent)
    , m_database(database)
    , m_artProvider(artProvider)
{
}

int LibraryAlbumModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_albums.size());
}

QVariant LibraryAlbumModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_albums.size())
    {
        return {};
    }

    const auto& album = m_albums[index.row()];

    switch (role)
    {
    case AlbumRole:
        return album.album;
    case YearRole:
        return album.year;
    case TrackCountRole:
        return album.trackCount;
    case ArtPathRole:
    {
        if (m_artProvider)
        {
            auto art = m_artProvider->getCachedArt(m_artistFilter, album.album);
            if (!art.frontPath.isEmpty())
            {
                return "file://" + art.frontPath;
            }
        }
        return QString();
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> LibraryAlbumModel::roleNames() const
{
    return {
        { AlbumRole, "album" }, { YearRole, "year" }, { TrackCountRole, "trackCount" }, { ArtPathRole, "artPath" }
    };
}

QString LibraryAlbumModel::artistFilter() const
{
    return m_artistFilter;
}

void LibraryAlbumModel::setArtistFilter(const QString& artist)
{
    if (m_artistFilter == artist)
    {
        return;
    }
    m_artistFilter = artist;
    refresh();
    emit artistFilterChanged();
}

void LibraryAlbumModel::refresh()
{
    beginResetModel();
    if (m_artistFilter.isEmpty())
    {
        m_albums.clear();
    }
    else
    {
        m_albums = m_database->getAlbumsByArtist(m_artistFilter);
    }
    endResetModel();
}
