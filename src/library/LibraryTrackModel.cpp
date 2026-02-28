#include "LibraryTrackModel.h"

LibraryTrackModel::LibraryTrackModel(LibraryDatabase* database, QObject* parent)
    : QAbstractListModel(parent)
    , m_database(database)
{
}

int LibraryTrackModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_tracks.size());
}

QVariant LibraryTrackModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
    {
        return {};
    }

    const auto& track = m_tracks[index.row()];

    switch (role)
    {
    case TrackNumberRole:
        return track.trackNumber;
    case DiscNumberRole:
        return track.discNumber;
    case TitleRole:
        return track.title;
    case ArtistRole:
        return track.artist;
    case DurationSecondsRole:
        return track.durationSeconds;
    case FilePathRole:
        return track.filePath;
    default:
        return {};
    }
}

QHash<int, QByteArray> LibraryTrackModel::roleNames() const
{
    return {
        { TrackNumberRole, "trackNumber" }, { DiscNumberRole, "discNumber" },           { TitleRole, "title" },
        { ArtistRole, "artist" },           { DurationSecondsRole, "durationSeconds" }, { FilePathRole, "filePath" }
    };
}

QString LibraryTrackModel::artistFilter() const
{
    return m_artistFilter;
}

void LibraryTrackModel::setArtistFilter(const QString& artist)
{
    if (m_artistFilter == artist)
    {
        return;
    }
    m_artistFilter = artist;
    refresh();
    emit artistFilterChanged();
}

QString LibraryTrackModel::albumFilter() const
{
    return m_albumFilter;
}

void LibraryTrackModel::setAlbumFilter(const QString& album)
{
    if (m_albumFilter == album)
    {
        return;
    }
    m_albumFilter = album;
    refresh();
    emit albumFilterChanged();
}

LibraryTrack LibraryTrackModel::trackAt(int index) const
{
    if (index < 0 || index >= m_tracks.size())
    {
        return {};
    }
    return m_tracks[index];
}

QVector<LibraryTrack> LibraryTrackModel::allTracks() const
{
    return m_tracks;
}

void LibraryTrackModel::refresh()
{
    beginResetModel();
    if (m_artistFilter.isEmpty() || m_albumFilter.isEmpty())
    {
        m_tracks.clear();
    }
    else
    {
        m_tracks = m_database->getTracksByAlbum(m_artistFilter, m_albumFilter);
    }
    endResetModel();
}
