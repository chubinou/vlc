/*****************************************************************************
 * playlist/content.h
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

#ifndef VLC_PLAYLIST_CONTENT_H
#define VLC_PLAYLIST_CONTENT_H

typedef struct vlc_playlist vlc_playlist_t;
typedef struct vlc_playlist_item vlc_playlist_item_t;

/* called by vlc_playlist_Delete() in playlist.c */
void
vlc_playlist_ClearItems(vlc_playlist_t *playlist);

/* called by vlc_playlist_ExpandItem() in preparse.c */
void
vlc_playlist_ItemsInserted(vlc_playlist_t *playlist, size_t index,
                           size_t count);

/* replace the existing item at index by the given item */
void vlc_playlist_ReplaceItem(vlc_playlist_t *playlist, size_t index,
                              vlc_playlist_item_t *item);

#endif
