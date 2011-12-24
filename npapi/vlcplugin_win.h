/*****************************************************************************
 * vlcplugin_win.h: a VLC plugin for Mozilla
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

#ifndef __VLCPLUGIN_WIN_H__
#define __VLCPLUGIN_WIN_H__

#include "vlcplugin_base.h"

#include <windows.h>
#include <winbase.h>

HMODULE DllGetModule();
#include "../common/win32_fullscreen.h"

class VlcPluginWin : public VlcPluginBase
{
public:
    VlcPluginWin(NPP, NPuint16_t);
    virtual ~VlcPluginWin();

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
    void popup_menu(){};

    WNDPROC             getWindowProc()
                            { return pf_wndproc; };
    void                setWindowProc(WNDPROC wndproc)
                            { pf_wndproc = wndproc; };

protected:
    virtual void on_media_player_new();
    virtual void on_media_player_release();

private:
    void set_player_window(){};

    unsigned int     i_width, i_height;
    unsigned int     i_tb_width, i_tb_height;

    WNDPROC pf_wndproc;
    VLCWindowsManager _WindowsManager;

    int i_last_position;
};

#endif /* __VLCPLUGIN_WIN_H__ */
