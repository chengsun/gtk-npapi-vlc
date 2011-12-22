/*****************************************************************************
 * vlcplugin_gtk.h: a VLC plugin for Mozilla (GTK+ interface)
 *****************************************************************************
 * Copyright (C) 2011 the VideoLAN team
 * $Id$
 *
 * Authors: Cheng Sun <chengsun9@gmail.com>
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

#ifndef __VLCPLUGIN_GTK_H__
#define __VLCPLUGIN_GTK_H__

#include "vlcplugin_base.h"

#include <gtk/gtk.h>
#include <X11/Xlib.h>

class VlcPluginGtk : public VlcPluginBase
{
public:
    VlcPluginGtk(NPP, NPuint16_t);
    virtual ~VlcPluginGtk();

    int                 setSize(unsigned width, unsigned height);

    void toggle_fullscreen();
    void set_fullscreen( int );
    int  get_fullscreen();

    bool create_windows();
    bool resize_windows();
    bool destroy_windows();

    void show_toolbar();
    void hide_toolbar();
    void update_controls();
    void popup_menu();

    void resize_video_xwindow(GdkRectangle *rect);
private:
    void set_player_window();
    Display *get_display();

    unsigned int     i_width, i_height;
    GtkWidget *parent, *parent_vbox, *video_container;
    GtkWidget *toolbar, *time_slider;
    GtkWidget *fullscreen_win;
    gulong video_container_size_handler_id;

    Window video_xwindow;
    bool is_fullscreen;
};

#endif /* __VLCPLUGIN_GTK_H__ */
