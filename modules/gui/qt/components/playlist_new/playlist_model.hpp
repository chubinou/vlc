/*****************************************************************************
 * playlist_model.hpp
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_QT_PLAYLIST_NEW_MODEL_HPP_
#define VLC_QT_PLAYLIST_NEW_MODEL_HPP_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <QAbstractListModel>
#include <QVector>
#include "playlist_common.hpp"
#include "playlist_item.hpp"

namespace vlc {
namespace playlist {

class PlaylistListModelPrivate;
class PlaylistListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(PlaylistPtr playlistId READ getPlaylistId WRITE setPlaylistId NOTIFY playlistIdChanged)

public:
    enum Roles
    {
        TitleRole = Qt::UserRole,
        DurationRole,
        IsCurrentRole,
    };

    PlaylistListModel(QObject *parent = nullptr);
    PlaylistListModel(vlc_playlist_t *playlist, QObject *parent = nullptr);
    ~PlaylistListModel();

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

    /* provided for convenience */
    const PlaylistItem &itemAt(int index) const;

    Q_INVOKABLE virtual void removeItems(const QList<int> &indexes);
    Q_INVOKABLE virtual void moveItems(const QList<int> &indexes, int target);

public slots:
    PlaylistPtr getPlaylistId() const;
    void setPlaylistId(PlaylistPtr id);
    void setPlaylistId(vlc_playlist_t* playlist);

signals:
    void playlistIdChanged(const PlaylistPtr& );

private:
    Q_DECLARE_PRIVATE(PlaylistListModel)
    QScopedPointer<PlaylistListModelPrivate> d_ptr;

};


  } // namespace playlist
} // namespace vlc

#endif