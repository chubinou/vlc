/*****************************************************************************
 * playlist/playlist.c
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "playlist.h"

#include <vlc_common.h>

#include "content.h"
#include "item.h"
#include "libvlc.h"
#include "player.h"

vlc_playlist_t *
vlc_playlist_New(vlc_object_t *parent)
{
    vlc_playlist_t *playlist = malloc(sizeof(*playlist));
    if (unlikely(!playlist))
        return NULL;

    bool ok = vlc_playlist_PlayerInit(playlist, parent);
    if (unlikely(!ok))
    {
        free(playlist);
        return NULL;
    }

    vlc_vector_init(&playlist->items);
    randomizer_Init(&playlist->randomizer);
    playlist->current = -1;
    playlist->has_prev = false;
    playlist->has_next = false;
    vlc_list_init(&playlist->listeners);
    playlist->repeat = VLC_PLAYLIST_PLAYBACK_REPEAT_NONE;
    playlist->order = VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL;
    playlist->idgen = 0;

    return playlist;
}

void
vlc_playlist_Delete(vlc_playlist_t *playlist)
{
    assert(vlc_list_is_empty(&playlist->listeners));

    vlc_playlist_PlayerDestroy(playlist);
    randomizer_Destroy(&playlist->randomizer);
    vlc_playlist_ClearItems(playlist);
    free(playlist);
}

void
vlc_playlist_Lock(vlc_playlist_t *playlist)
{
    vlc_player_Lock(playlist->player);
}

void
vlc_playlist_Unlock(vlc_playlist_t *playlist)
{
    vlc_player_Unlock(playlist->player);
}

static void
PlaylistConfigureFromVariables(vlc_playlist_t *playlist, vlc_object_t *obj)
{
    enum vlc_playlist_playback_order order;
    if (var_InheritBool(obj, "random"))
        order = VLC_PLAYLIST_PLAYBACK_ORDER_RANDOM;
    else
        order = VLC_PLAYLIST_PLAYBACK_ORDER_NORMAL;

    /* repeat = repeat current; loop = repeat all */
    enum vlc_playlist_playback_repeat repeat;
    if (var_InheritBool(obj, "repeat"))
        repeat = VLC_PLAYLIST_PLAYBACK_REPEAT_CURRENT;
    else if (var_InheritBool(obj, "loop"))
        repeat = VLC_PLAYLIST_PLAYBACK_REPEAT_ALL;
    else
        repeat = VLC_PLAYLIST_PLAYBACK_REPEAT_NONE;

    enum vlc_player_media_stopped_action media_stopped_action;
    if (var_InheritBool(obj, "play-and-exit"))
        media_stopped_action = VLC_PLAYER_MEDIA_STOPPED_EXIT;
    else if (var_InheritBool(obj, "play-and-stop"))
        media_stopped_action = VLC_PLAYER_MEDIA_STOPPED_STOP;
    else if (var_InheritBool(obj, "play-and-pause"))
        media_stopped_action = VLC_PLAYER_MEDIA_STOPPED_PAUSE;
    else
        media_stopped_action = VLC_PLAYER_MEDIA_STOPPED_CONTINUE;

    bool start_paused = var_InheritBool(obj, "start-paused");

    vlc_playlist_Lock(playlist);
    vlc_playlist_SetPlaybackOrder(playlist, order);
    vlc_playlist_SetPlaybackRepeat(playlist, repeat);

    vlc_player_t *player = vlc_playlist_GetPlayer(playlist);

    /* the playlist and the player share the same lock, and this is not an
     * implementation detail */
    vlc_player_SetMediaStoppedAction(player, media_stopped_action);
    vlc_player_SetStartPaused(player, start_paused);

    vlc_playlist_Unlock(playlist);
}

vlc_playlist_t *
vlc_playlist_GetMainPlaylist(libvlc_int_t *libvlc)
{
    libvlc_priv_t *priv = libvlc_priv(libvlc);

    vlc_mutex_lock(&priv->lock);
    vlc_playlist_t *playlist = priv->main_playlist;
    if (!priv->main_playlist)
    {
        playlist = priv->main_playlist = vlc_playlist_New(VLC_OBJECT(libvlc));
        if (playlist)
            PlaylistConfigureFromVariables(playlist, VLC_OBJECT(libvlc));
    }
    vlc_mutex_unlock(&priv->lock);

    return playlist;
}
