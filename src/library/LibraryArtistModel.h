#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "library/LibraryDatabase.h"

/// QAbstractListModel exposing artists sorted alphabetically with album counts.
/// Used for the first level of hierarchical Artist > Album > Track browsing (FLAC-07).
class LibraryArtistModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles
    {
        AlbumArtistRole = Qt::UserRole + 1,
        AlbumCountRole
    };

    explicit LibraryArtistModel(LibraryDatabase* database, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();

private:
    LibraryDatabase* m_database; // Non-owning
    QVector<LibraryArtistEntry> m_artists;
};
