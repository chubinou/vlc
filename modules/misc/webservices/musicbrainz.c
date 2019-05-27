/*****************************************************************************
 * musicbrainz.c : Musicbrainz API lookup
 *****************************************************************************
 * Copyright (C) 2019 VideoLabs, VLC authors and VideoLAN
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

#include <string.h>
#include <limits.h>

#include <vlc_common.h>
#include <vlc_stream.h>

#include "json.h"
#include "musicbrainz.h"

static const json_value * jsongetbyname(const json_value *object, const char *psz_name)
{
    if (object->type != json_object) return NULL;
    for (unsigned int i=0; i < object->u.object.length; i++)
        if (strcmp(object->u.object.values[i].name, psz_name) == 0)
            return object->u.object.values[i].value;
    return NULL;
}

static char * jsongetstring(const json_value *node, const char *key)
{
    node = jsongetbyname(node, key);
    if (node && node->type == json_string)
        return node->u.string.ptr;
    return NULL;
}

static char * jsondupstring(const json_value *node, const char *key)
{
    const char *str = jsongetstring(node, key);
    return (str) ? strdup(str) : NULL;
}

static json_value * ParseJson(vlc_object_t *p_obj, const char *psz_buffer)
{
    json_settings settings;
    char psz_error[128];
    memset (&settings, 0, sizeof (json_settings));
    json_value *root = json_parse_ex(&settings, psz_buffer, psz_error);
    if (root == NULL)
    {
        msg_Warn(p_obj, "Can't parse json data: %s", psz_error);
        goto error;
    }
    if (root->type != json_object)
    {
        msg_Warn(p_obj, "wrong json root node");
        goto error;
    }

    return root;

error:
    if (root) json_value_free(root);
    return NULL;
}

static void * RetrieveDocument(vlc_object_t *p_obj, const char *psz_url)
{
    msg_Dbg(p_obj, "Querying MB for %s", psz_url);
    bool saved_no_interact = p_obj->no_interact;
    p_obj->no_interact = true;
    stream_t *p_stream = vlc_stream_NewURL(p_obj, psz_url);

    p_obj->no_interact = saved_no_interact;
    if (p_stream == NULL)
        return NULL;

    stream_t *p_chain = vlc_stream_FilterNew(p_stream, "inflate");
    if(p_chain)
        p_stream = p_chain;

    /* read answer */
    char *p_buffer = NULL;
    int i_ret = 0;
    for(;;)
    {
        int i_read = 65536;

        if(i_ret >= INT_MAX - i_read)
            break;

        p_buffer = realloc_or_free(p_buffer, 1 + i_ret + i_read);
        if(unlikely(p_buffer == NULL))
        {
            vlc_stream_Delete(p_stream);
            return NULL;
        }

        i_read = vlc_stream_Read(p_stream, &p_buffer[i_ret], i_read);
        if(i_read <= 0)
            break;

        i_ret += i_read;
    }
    vlc_stream_Delete(p_stream);
    p_buffer[i_ret] = 0;

    return p_buffer;
}

typedef struct
{
    json_value *root;
} musicbrainz_lookup_t;

static void musicbrainz_lookup_release(musicbrainz_lookup_t *p)
{
    if(p && p->root)
        json_value_free(p->root);
    free(p);
}

static musicbrainz_lookup_t * musicbrainz_lookup_new(void)
{
    return calloc(1, sizeof(musicbrainz_lookup_t));
}

static musicbrainz_lookup_t * musicbrainz_lookup(vlc_object_t *p_obj, const char *psz_url)
{
    void *p_buffer = RetrieveDocument(p_obj, psz_url);
    if(!p_buffer)
        return NULL;

    musicbrainz_lookup_t *p_lookup = musicbrainz_lookup_new();
    if(p_lookup)
    {
        p_lookup->root = ParseJson(p_obj, p_buffer);
        if (!p_lookup->root)
            msg_Dbg(p_obj, "No results");
    }
    free(p_buffer);
    return p_lookup;
}

static bool musicbrainz_fill_track(const json_value *tracknode, musicbrainz_track_t *t)
{
    t->psz_title = jsondupstring(tracknode, "title");

    const json_value *node = jsongetbyname(tracknode, "artist-credit");
    if (node && node->type == json_array && node->u.array.length)
        t->psz_artist = jsondupstring(node->u.array.values[0], "name");

    node = jsongetbyname(tracknode, "position");
    if (node && node->type == json_integer)
        t->i_index = node->u.integer;

    return true;
}

static bool musicbrainz_has_cover_in_releasegroup(json_value ** const p_nodes,
                                                  size_t i_nodes,
                                                  const char *psz_group_id)
{
    for(size_t i=0; i<i_nodes; i++)
    {
        const json_value *rgnode = jsongetbyname(p_nodes[i], "release-group");
        if(rgnode)
        {
            const char *psz_id = jsongetstring(rgnode, "id");
            if(!psz_id || strcmp(psz_id, psz_group_id))
                continue;

            const json_value *node = jsongetbyname(p_nodes[i], "cover-art-archive");
            if(!node)
                continue;

            node = jsongetbyname(node, "front");
            if(!node || node->type != json_boolean || !node->u.boolean)
                continue;

            return true;
        }
    }

    return false;
}

static bool musicbrainz_fill_release(const json_value *releasenode, musicbrainz_release_t *r)
{
    const json_value *media = jsongetbyname(releasenode, "media");
    if(!media || media->type != json_array ||
       media->u.array.length == 0)
        return false;
    /* we always use first media */
    media = media->u.array.values[0];

    const json_value *tracks = jsongetbyname(media, "tracks");
    if(!tracks || tracks->type != json_array ||
       tracks->u.array.length == 0)
        return false;

    r->p_tracks = calloc(tracks->u.array.length, sizeof(*r->p_tracks));
    if(!r->p_tracks)
        return false;

    for(size_t i=0; i<tracks->u.array.length; i++)
    {
        if(musicbrainz_fill_track(tracks->u.array.values[i], &r->p_tracks[r->i_tracks]))
            r->i_tracks++;
    }

    r->psz_title = jsondupstring(releasenode, "title");
    r->psz_id = jsondupstring(releasenode, "id");

    const json_value *rgnode = jsongetbyname(releasenode, "release-group");
    if(rgnode)
    {
        r->psz_date = jsondupstring(rgnode, "first-release-date");
        r->psz_group_id = jsondupstring(rgnode, "id");

        const json_value *node = jsongetbyname(rgnode, "artist-credit");
        if (node && node->type == json_array && node->u.array.length)
            r->psz_artist = jsondupstring(node->u.array.values[0], "name");

//        node = jsongetbyname(rgnode, "cover-art-archive");
//        if(node)
//        {
//            node = jsongetbyname(node, "front");
//            if(node && node->type == json_boolean && node->u.boolean)
//            {

//            }
//        }
    }
    else
    {
        const json_value *node = jsongetbyname(releasenode, "artist-credit");
        if (node && node->type == json_array && node->u.array.length)
            r->psz_artist = jsondupstring(node->u.array.values[0], "name");

        node = jsongetbyname(releasenode, "release-events");
        if(node && node->type == json_array && node->u.array.length)
            r->psz_date = jsondupstring(node->u.array.values[0], "date");
    }


    return true;
}

void musicbrainz_recording_release(musicbrainz_recording_t *mbr)
{
    for(size_t i=0; i<mbr->i_release; i++)
    {
        free(mbr->p_releases[i].psz_id);
        free(mbr->p_releases[i].psz_group_id);
        free(mbr->p_releases[i].psz_artist);
        free(mbr->p_releases[i].psz_title);
        free(mbr->p_releases[i].psz_date);
        free(mbr->p_releases[i].psz_coverart_url);
        for(size_t j=0; j<mbr->p_releases[i].i_tracks; j++)
        {
            free(mbr->p_releases[i].p_tracks[j].psz_title);
            free(mbr->p_releases[i].p_tracks[j].psz_artist);
        }
        free(mbr->p_releases[i].p_tracks);
    }
    free(mbr->p_releases);
    free(mbr);
}

static musicbrainz_recording_t *musicbrainz_lookup_recording_by_apiurl(vlc_object_t *obj,
                                                                       const char *psz_url)
{
    musicbrainz_recording_t *r = calloc(1, sizeof(*r));
    if(!r)
        return NULL;

    musicbrainz_lookup_t *lookup = musicbrainz_lookup(obj, psz_url);
    if(!lookup)
    {
        free(r);
        return NULL;
    }

    const json_value *releases = jsongetbyname(lookup->root, "releases");
    if (releases && releases->type == json_array &&
        releases->u.array.length)
    {
        r->p_releases = calloc(releases->u.array.length, sizeof(*r->p_releases));
        if(r->p_releases)
        {
            for(unsigned i=0; i<releases->u.array.length; i++)
            {
                json_value *node = releases->u.array.values[i];
                musicbrainz_release_t *p_mbrel = &r->p_releases[r->i_release];
                if (!node || node->type != json_object ||
                    !musicbrainz_fill_release(node, p_mbrel))
                    continue;

                /* Try to find cover from other releases from the same group */
                if(p_mbrel->psz_group_id && !p_mbrel->psz_coverart_url &&
                   musicbrainz_has_cover_in_releasegroup(releases->u.array.values,
                                                         releases->u.array.length,
                                                         p_mbrel->psz_group_id))
                {
                    char *psz_art = coverartarchive_make_releasegroup_arturl(
                                        COVERARTARCHIVE_DEFAULT_SERVER,
                                        p_mbrel->psz_group_id );
                    if(psz_art)
                        p_mbrel->psz_coverart_url = psz_art;
                }

                r->i_release++;
            }
        }
    }

    musicbrainz_lookup_release(lookup);

    return r;
}

static char *musicbrainz_build_discid_json_url(const char *psz_server,
                                               const char *psz_disc_id,
                                               const char *psz_tail)
{
    char *psz_url;
    if(asprintf(&psz_url,
                "https://%s/ws/2/discid/%s?"
                "fmt=json"
                "&inc=artist-credits+recordings+release-groups"
                "&cdstubs=no"
                "%s%s",
                psz_server, psz_disc_id,
                psz_tail ? "&" : "",
                psz_tail ? psz_tail : "" ) > -1 )
    {
        return psz_url;
    }
    return NULL;
}

musicbrainz_recording_t *musicbrainz_lookup_recording_by_toc(vlc_object_t *obj,
                                                             const char *psz_server,
                                                             const char *psz_toc)
{
    char *psz_url = musicbrainz_build_discid_json_url(psz_server, "-", psz_toc);
    if(!psz_url)
        return NULL;

    musicbrainz_recording_t *r = musicbrainz_lookup_recording_by_apiurl(obj, psz_url);
    free(psz_url);
    return r;
}

musicbrainz_recording_t *musicbrainz_lookup_recording_by_discid(vlc_object_t *obj,
                                                                const char *psz_server,
                                                                const char *psz_disc_id)
{
    char *psz_url = musicbrainz_build_discid_json_url(psz_server, psz_disc_id, NULL);
    if(!psz_url)
        return NULL;

    musicbrainz_recording_t *r = musicbrainz_lookup_recording_by_apiurl(obj, psz_url);
    free(psz_url);
    return r;
}

char * coverartarchive_make_releasegroup_arturl(const char *psz_server, const char *psz_group_id)
{
    char *psz_art;
    if(-1 < asprintf(&psz_art, "https://%s/release-group/%s/front", psz_server, psz_group_id))
        return psz_art;
    return NULL;
}

void musicbrainz_release_covert_art(coverartarchive_t *c)
{
    free(c);
}

coverartarchive_t * coverartarchive_lookup_releasegroup(vlc_object_t *p_obj, const char *psz_id)
{
    coverartarchive_t *c = calloc(1, sizeof(*c));
    if(!c)
        return NULL;

    char *psz_url;
    if(0 < asprintf(&psz_url, "https://" COVERARTARCHIVE_DEFAULT_SERVER
                              "/releasegroup/%s", psz_id ))
    {
        return NULL;
    }

     musicbrainz_lookup_t *p_lookup = musicbrainz_lookup(p_obj, psz_url);
     free(psz_url);

     if(!p_lookup)
     {
         free(c);
         return NULL;
     }

    return c;
}
