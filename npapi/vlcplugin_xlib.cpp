/*****************************************************************************
 * vlcplugin_xlib.cpp: a VLC plugin for Mozilla (X interface)
 *****************************************************************************
 * Copyright (C) 2011 the VideoLAN team
 * $Id$
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Damien Fouilleul <damienf@videolan.org>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Cheng Sun <chengsun9@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "vlcplugin_xlib.h"
#include <cstring>

VlcPluginXlib::VlcPluginXlib(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode)
{
    memset(&npparent, 0, sizeof(Window));
    memset(&npvideo, 0, sizeof(Window));
}

VlcPluginXlib::~VlcPluginXlib()
{
}

Display *VlcPluginXlib::getDisplay()
{
        return ((NPSetWindowCallbackStruct *) npwindow.ws_info)->display;
}

void VlcPluginXlib::set_player_window()
{
    libvlc_media_player_set_xwindow(libvlc_media_player,
                                    (uint32_t)npvideo);
}

void VlcPluginXlib::toggle_fullscreen()
{
    if (!b_allowfullscreen) return;
    if (playlist_isplaying())
        libvlc_toggle_fullscreen(libvlc_media_player);
}

void VlcPluginXlib::set_fullscreen(int yes)
{
    if (!b_allowfullscreen) return;
    if (playlist_isplaying())
        libvlc_set_fullscreen(libvlc_media_player,yes);
}

int  VlcPluginXlib::get_fullscreen()
{
    int r = 0;
    if (playlist_isplaying())
        r = libvlc_get_fullscreen(libvlc_media_player);
    return r;
}

bool VlcPluginXlib::create_windows()
{

    Display *p_display = getDisplay();
    Window socket = (Window) npwindow.window;

    unsigned long xembed_info_buf[2] =
            { PLUGIN_XEMBED_PROTOCOL_VERSION, XEMBED_MAPPED };
    Atom xembed_info_atom = XInternAtom( p_display, "_XEMBED_INFO", 1);

    int i_blackColor = BlackPixel(p_display, DefaultScreen(p_display));

    /* create windows */
    npparent = XCreateSimpleWindow( p_display, socket, 0, 0,
                   1, 1,
                   0, i_blackColor, i_blackColor );
    XChangeProperty( p_display, npparent, xembed_info_atom,
                     xembed_info_atom, 32, PropModeReplace,
                     (unsigned char *)xembed_info_buf, 2);

    npvideo = XCreateSimpleWindow( p_display, npparent, 0, 0,
                   1, 1,
                   0, i_blackColor, i_blackColor );

    XMapWindow( p_display, npvideo );

    return true;
}

bool VlcPluginXlib::resize_windows()
{
    Display *p_display = getDisplay();

    int i_ret;

    i_ret = XMoveResizeWindow( p_display, npparent, 0, 0,
                               npwindow.width, npwindow.height );
    i_ret = XMoveResizeWindow( p_display, npvideo, 0, 0,
                               npwindow.width, npwindow.height );

    Window root_return, parent_return, *children_return;
    unsigned int i_nchildren;
    XQueryTree( p_display, npvideo,
                &root_return, &parent_return, &children_return,
                &i_nchildren );

    if( i_nchildren > 0 )
    {
        /* XXX: Make assumptions related to the window parenting structure in
           vlc/modules/video_output/x11/xcommon.c */
        Window base_window = children_return[i_nchildren - 1];

        i_ret = XResizeWindow( p_display, base_window,
                npwindow.width, ( npwindow.height ) );
    }

    return true;
}

bool VlcPluginXlib::destroy_windows()
{
    /* TODO */
}
