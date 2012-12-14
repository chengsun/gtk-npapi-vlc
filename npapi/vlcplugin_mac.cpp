/*****************************************************************************
 * vlcplugin_mac.cpp: a VLC plugin for Mozilla (Mac interface)
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

#include "vlcplugin_mac.h"

#include <npapi.h>

VlcPluginMac::VlcPluginMac(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode)
{
}

VlcPluginMac::~VlcPluginMac()
{
}

void VlcPluginMac::set_player_window()
{
    // XXX FIXME insert appropriate call here
}

void VlcPluginMac::toggle_fullscreen()
{
    if (!get_options().get_enable_fs()) return;
    if (playlist_isplaying())
        libvlc_toggle_fullscreen(getMD());
}

void VlcPluginMac::set_fullscreen(int yes)
{
    if (!get_options().get_enable_fs()) return;
    if (playlist_isplaying())
        libvlc_set_fullscreen(getMD(), yes);
}

int  VlcPluginMac::get_fullscreen()
{
    int r = 0;
    if (playlist_isplaying())
        r = libvlc_get_fullscreen(getMD());
    return r;
}

bool VlcPluginMac::create_windows()
{
    return true;
}

bool VlcPluginMac::resize_windows()
{
    /* as MacOS X video output is windowless, set viewport */
    libvlc_rectangle_t view, clip;

    /* browser sets port origin to top-left location of plugin
     * relative to GrafPort window origin is set relative to document,
     * which of little use for drawing
     */
    view.top	= 0; // ((NP_Port*) (npwindow.window))->porty;
    view.left	= 0; // ((NP_Port*) (npwindow.window))->portx;
    view.bottom  = npwindow.height+view.top;
    view.right   = npwindow.width+view.left;

    /* clipRect coordinates are also relative to GrafPort */
    clip.top     = npwindow.clipRect.top;
    clip.left    = npwindow.clipRect.left;
    clip.bottom  = npwindow.clipRect.bottom;
    clip.right   = npwindow.clipRect.right;

#ifdef NOT_WORKING
    libvlc_video_set_viewport(p_vlc, p_plugin->getMD(), &view, &clip);
#else
#warning disabled code
#endif
    return true;
}

bool VlcPluginMac::destroy_windows()
{
    npwindow.window = NULL;
    return true;
}

bool VlcPluginMac::handle_event(void *event)
{
    // FIXME: implement Cocoa event model, by porting this legacy code:
/*
    static UInt32 lastMouseUp = 0;
    EventRecord *myEvent = (EventRecord*)event;

    switch( myEvent->what )
    {
        case nullEvent:
            return true;
        case mouseDown:
        {
            if( (myEvent->when - lastMouseUp) < GetDblTime() )
            {
                // double click
                p_plugin->toggle_fullscreen();
            }
            return true;
        }
        case mouseUp:
            lastMouseUp = myEvent->when;
            return true;
        case keyUp:
        case keyDown:
        case autoKey:
            return true;
        case updateEvt:
        {
            const NPWindow& npwindow = p_plugin->getWindow();
            if( npwindow.window )
            {
                bool hasVout = false;

                if( p_plugin->playlist_isplaying() )
                {
                    hasVout = p_plugin->player_has_vout();
#if 0
                    if( hasVout )
                    {
                        libvlc_rectangle_t area;
                        area.left = 0;
                        area.top = 0;
                        area.right = npwindow.width;
                        area.bottom = npwindow.height;
                        libvlc_video_redraw_rectangle(p_plugin->getMD(), &area, NULL);
                    }
#else
#warning disabled code
#endif
                }

                if( ! hasVout )
                {
                    // draw the text from get_bg_text()
                    ForeColor(blackColor);
                    PenMode( patCopy );

                    // seems that firefox forgets to set the following
                    // on occasion (reload)
                    SetOrigin(((NP_Port *)npwindow.window)->portx,
                              ((NP_Port *)npwindow.window)->porty);

                    Rect rect;
                    rect.left = 0;
                    rect.top = 0;
                    rect.right = npwindow.width;
                    rect.bottom = npwindow.height;
                    PaintRect( &rect );

                    ForeColor(whiteColor);
                    MoveTo( (npwindow.width-80)/ 2  , npwindow.height / 2 );
                    if( !p_plugin->get_bg_text().empty() )
                        DrawText( p_plugin->get_bg_text().c_str(), 0, p_plugin->get_bg_text().length() );
                }
            }
            return true;
        }
        case activateEvt:
            return false;
        case NPEventType_GetFocusEvent:
        case NPEventType_LoseFocusEvent:
            return true;
        case NPEventType_AdjustCursorEvent:
            return false;
        case NPEventType_MenuCommandEvent:
            return false;
        case NPEventType_ClippingChangedEvent:
            return false;
        case NPEventType_ScrollingBeginsEvent:
            return true;
        case NPEventType_ScrollingEndsEvent:
            return true;
        default:
            ;
    }
*/
    return VlcPluginBase::handle_event(event);
}
