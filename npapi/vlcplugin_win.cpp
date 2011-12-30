/*****************************************************************************
 * vlcplugin_win.cpp: a VLC plugin for Mozilla (Windows interface)
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

#include "vlcplugin_win.h"

#include "../common/win32_fullscreen.h"

static HMODULE hDllModule= 0;

HMODULE DllGetModule()
{
    return hDllModule;
};

extern "C"
BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved )
{
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
            hDllModule = (HMODULE)hModule;
            break;
        default:
            break;
    }
    return TRUE;
};


VlcPluginWin::VlcPluginWin(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode),
    _WindowsManager(DllGetModule())
{
}

VlcPluginWin::~VlcPluginWin()
{
    destroy_windows();
}

void VlcPluginWin::toggle_fullscreen()
{
    _WindowsManager.ToggleFullScreen();
}

void VlcPluginWin::set_fullscreen(int yes)
{
    if(yes){
        _WindowsManager.StartFullScreen();
    }
    else{
        _WindowsManager.EndFullScreen();
    }
}

int  VlcPluginWin::get_fullscreen()
{
    return _WindowsManager.IsFullScreen();
}

void VlcPluginWin::set_toolbar_visible(bool)
{
    /* TODO */
}

bool VlcPluginWin::get_toolbar_visible()
{
    /* TODO */
    return false;
}

void VlcPluginWin::update_controls()
{
    /* TODO */
}

bool VlcPluginWin::create_windows()
{
    HWND drawable = (HWND) (getWindow().window);

    if( GetWindowLongPtr(drawable, GWLP_USERDATA) )
        return false;

    /* attach our plugin object */
    SetWindowLongPtr(drawable, GWLP_USERDATA,
                     (LONG_PTR)this);

    /* change window style to our liking */
    LONG style = GetWindowLong(drawable, GWL_STYLE);
    style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    SetWindowLong(drawable, GWL_STYLE, style);

    _WindowsManager.CreateWindows(drawable);

    return true;
}

bool VlcPluginWin::resize_windows()
{
    HWND drawable = (HWND) (getWindow().window);
    RECT rect;
    GetClientRect(drawable, &rect);
    if(!_WindowsManager.IsFullScreen() && _WindowsManager.getHolderWnd()){
        HWND hHolderWnd = _WindowsManager.getHolderWnd()->getHWND();
        MoveWindow(hHolderWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
    }
    return true;
}

bool VlcPluginWin::destroy_windows()
{
    _WindowsManager.DestroyWindows();
    SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
    return true;
}

void VlcPluginWin::on_media_player_new()
{
    _WindowsManager.LibVlcAttach(libvlc_media_player);
}

void VlcPluginWin::on_media_player_release()
{
    _WindowsManager.LibVlcDetach();
}
