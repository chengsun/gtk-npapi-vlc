/*****************************************************************************
 * vlcplugin_xlib.h: a VLC plugin for Mozilla (X interface)
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

#ifndef __VLCPLUGIN_XLIB_H__
#define __VLCPLUGIN_XLIB_H__

#include "vlcplugin_base.h"
#include <X11/Xlib.h>
#include "xembed.h"

class VlcPluginXlib : public VlcPluginBase
{
public:
    VlcPluginXlib(NPP, NPuint16_t);
    virtual ~VlcPluginXlib();

    int                 setSize(unsigned width, unsigned height);

    void toggle_fullscreen();
    void set_fullscreen( int );
    int  get_fullscreen();

    bool create_windows();
    bool resize_windows();
    bool destroy_windows();

    void show_toolbar()     {/* STUB */};
    void hide_toolbar()     {/* STUB */};
    void update_controls()  {/* STUB */};
    void popup_menu()       {/* STUB */};

    Display *getDisplay();
private:
    void set_player_window();

    unsigned int     i_width, i_height;
    Window           npparent, npvideo;

    int i_last_position;
};

#endif /* __VLCPLUGIN_XLIB_H__ */
