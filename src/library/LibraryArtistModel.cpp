#include "LibraryArtistModel.h"

LibraryArtistModel::LibraryArtistModel(LibraryDatabase* database, QObject* parent)
    : QAbstractListModel(parent)
    , m_database(database)
{
}

int LibraryArtistModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_artists.size());
}

QVariant LibraryArtistModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_artists.size())
    {
        return {};
    }

    const auto& artist = m_artists[index.row()];

    switch (role)
    {
    case AlbumArtistRole:
        return artist.albumArtist;
    case AlbumCountRole:
        return artist.albumCount;
    default:
        return {};
    }
}

QHash<int, QByteArray> LibraryArtistModel::roleNames() const
{
    return { { AlbumArtistRole, "albumArtist" }, { AlbumCountRole, "albumCount" } };
}

void LibraryArtistModel::refresh()
{
    beginResetModel();
    m_artists = m_database->getArtists();
    endResetModel();
}
