/*****************************************************************************
 * vgl.c: virtual LibVLC OpenGL extension
 *****************************************************************************
 * Copyright (c) 2018 VLC authors and VideoLAN
 *
 * Authors: Pierre Lamot <pierre@videolabs.io>
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

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_opengl.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_picture.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_renderer_discoverer.h>
#include <vlc/libvlc_media_player.h>

struct vout_display_sys_t
{
    libvlc_video_cleanup_cb cleanupCb;
    libvlc_video_setup_cb setupCb;
    libvlc_video_update_output_cb resizeCb;
    libvlc_video_swap_cb swapCb;
    libvlc_video_makeCurrent_cb makeCurrentCb;
    libvlc_video_getProcAddress_cb getProcAddressCb;

    union
    {
        struct
        {
            void* display;
            void* (*createImageKHR)(void* display, void* ctx, unsigned target, void *buffer, const int32_t *attrib_list);
            void* (*destroyImageKHR)(void* display, void* img);
            const char *(*queryString)(vlc_gl_t *, int32_t name);
        } egl;
        struct
        {
            const char* (*getExtensionsStringEXT)();
        } wgl;
    };

    void* opaque;
    unsigned width;
    unsigned height;
};


static void *OurGetProcAddress(vlc_gl_t *gl, const char *name)
{
    vout_display_sys_t *sys = gl->sys;
    return sys->getProcAddressCb(sys->opaque, name);
}

static int MakeCurrent(vlc_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    bool success = sys->makeCurrentCb(sys->opaque, true);
    return success ? VLC_SUCCESS : VLC_EGENERIC;
}

static void ReleaseCurrent(vlc_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    sys->makeCurrentCb(sys->opaque, false);
}

static void VglSwapBuffers(vlc_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    sys->swapCb(sys->opaque);
}

static void Resize(vlc_gl_t * gl, unsigned w, unsigned h)
{
    vout_display_sys_t *sys = gl->sys;
    if( sys->width == w && sys->height == h )
        return;

    if( !sys->resizeCb )
        return;

    MakeCurrent(gl);
    sys->resizeCb(sys->opaque, w, h);
    ReleaseCurrent(gl);
    sys->width = w;
    sys->height = h;
}

static void Close(vlc_gl_t *gl)
{
    vout_display_sys_t *sys = gl->sys;
    if( sys->cleanupCb )
        sys->cleanupCb(sys->opaque);
}

static const char* EglQueryString(vlc_gl_t *gl, int32_t name )
{
    vout_display_sys_t *sys = gl->sys;
    return sys->egl.queryString(sys->egl.display, name);
}

static void *EglCreateImageKHR(vlc_gl_t *gl, unsigned target, void *buffer,
                            const int32_t *attrib_list)
{
    vout_display_sys_t *sys = gl->sys;
    return sys->egl.createImageKHR(sys->egl.display, NULL, target, buffer, attrib_list);
}

static bool EglDestroyImageKHR(vlc_gl_t *gl, void *image)
{
    vout_display_sys_t *sys = gl->sys;
    return sys->egl.destroyImageKHR(sys->egl.display, image);
}

static const char* WglGetExtensionsString(vlc_gl_t *gl )
{
    vout_display_sys_t *sys = gl->sys;
    return sys->wgl.getExtensionsStringEXT();
}


#define SET_CALLBACK_ADDR(var, varname) \
    do {                                                           \
        var = var_InheritAddress(gl, varname);                     \
        if( !var ) {                                               \
            msg_Err( gl, "%s address is missing", varname );       \
            return VLC_EGENERIC;                                   \
        }                                                          \
    } while( 0 )

static int Open(vlc_gl_t *gl, unsigned width, unsigned height)
{
    vout_display_sys_t * sys;

    /* Allocate structure */
    gl->sys = sys = vlc_obj_calloc(VLC_OBJECT(gl), 1, sizeof(*sys));
    if( !sys )
        return VLC_ENOMEM;

    sys->opaque = var_InheritAddress(gl, "vout-cb-opaque");
    sys->setupCb = var_InheritAddress(gl, "vout-cb-setup");
    sys->cleanupCb = var_InheritAddress(gl, "vout-cb-cleanup");
    sys->resizeCb = var_InheritAddress(gl, "vout-cb-update-output");
    SET_CALLBACK_ADDR(sys->swapCb, "vout-cb-swap");
    SET_CALLBACK_ADDR(sys->makeCurrentCb, "vout-cb-make-current");
    SET_CALLBACK_ADDR(sys->getProcAddressCb, "vout-cb-get-proc-address");

    gl->makeCurrent = MakeCurrent;
    gl->releaseCurrent = ReleaseCurrent;
    gl->resize = Resize;
    gl->swap = VglSwapBuffers;
    gl->getProcAddress = OurGetProcAddress;

    if( sys->setupCb )
        if( !sys->setupCb(sys->opaque) )
        {
            msg_Err( gl, "user setup failed" );
            return VLC_EGENERIC;
        }

    //Detect if we are using EGL to allow HW acceleration
    void* (*eglGetCurrentDisplay)() = sys->getProcAddressCb(sys->opaque, "eglGetCurrentDisplay");
    if (eglGetCurrentDisplay)
    {
        sys->makeCurrentCb(sys->opaque, true);
        void* eglDisplay = eglGetCurrentDisplay();
        sys->makeCurrentCb(sys->opaque, false);
        if (eglDisplay != 0)
        {
            void* eglCreateImageKHR = sys->getProcAddressCb(sys->opaque, "eglCreateImageKHR");
            void* eglDestroyImageKHR = sys->getProcAddressCb(sys->opaque, "eglDestroyImageKHR");
            void* eglQueryString = sys->getProcAddressCb(sys->opaque, "eglQueryString");
            if (eglCreateImageKHR && eglDestroyImageKHR && eglQueryString)
            {
                sys->egl.display = eglDisplay;
                sys->egl.createImageKHR = eglCreateImageKHR;
                sys->egl.destroyImageKHR = eglCreateImageKHR;
                sys->egl.queryString = eglQueryString;

                gl->ext = VLC_GL_EXT_EGL;
                gl->egl.createImageKHR = EglCreateImageKHR;
                gl->egl.destroyImageKHR = EglDestroyImageKHR;
                gl->egl.queryString = EglQueryString;
            }
        }
    }
    else
    {
        //Detect if we are using WGL
        const char* (*wglGetExtensionsStringEXT)() = sys->getProcAddressCb(sys->opaque, "wglGetExtensionsStringEXT");
        if (wglGetExtensionsStringEXT)
        {
            sys->wgl.getExtensionsStringEXT = wglGetExtensionsStringEXT;

            gl->ext = VLC_GL_EXT_WGL;
            gl->wgl.getExtensionsString = WglGetExtensionsString;
        }
    }

    Resize(gl, width, height);
    return VLC_SUCCESS;
}

#undef SET_CALLBACK_ADDR

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/

vlc_module_begin()
    set_shortname("GL texture")
    set_description("GL texture output")
    set_category(CAT_VIDEO)
    set_subcategory(SUBCAT_VIDEO_VOUT)

    set_capability("opengl", 0)
    set_callbacks(Open, Close)
    add_shortcut("vglmem")

    add_submodule()
    set_capability("opengl es2", 0)
    set_callbacks(Open, Close)
    add_shortcut("vglmem")
vlc_module_end()
