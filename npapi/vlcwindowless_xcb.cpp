/*****************************************************************************
 * vlcwindowless_XCB.cpp: a VLC plugin for Mozilla (XCB windowless)
 *****************************************************************************
 * Copyright © 2012 VideoLAN
 * $Id$
 *
 * Authors: Ludovic Fauvet <etix@videolan.org>
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

#include "vlcwindowless_xcb.h"

#include <X11/Xlib-xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_image.h>

#include <cstring>
#include <cstdlib>

VlcWindowlessXCB::VlcWindowlessXCB(NPP instance, NPuint16_t mode) :
    VlcWindowlessBase(instance, mode), m_conn(0), m_screen(0)
{
    //FIXME Avoid using XOpenDisplay in an XCB context
    Display *display = XOpenDisplay(NULL);
    if (!(m_conn = XGetXCBConnection(display)))
    {
        fprintf(stderr, "Can't connect to XCB\n");
        return;
    }

    /* Retrieve the setup */
    const xcb_setup_t *setup;
    if (!(setup = xcb_get_setup(m_conn)))
    {
        fprintf(stderr, "Can't get the XCB setup\n");
        return;
    }

    /* Get the first screen */
    m_screen = xcb_setup_roots_iterator(setup).data;
}

VlcWindowlessXCB::~VlcWindowlessXCB()
{
    xcb_disconnect(m_conn);
}

void VlcWindowlessXCB::drawBackground(xcb_drawable_t drawable)
{
    /* Obtain the background color */
    xcb_colormap_t colormap = m_screen->default_colormap;
    unsigned r = 0, g = 0, b = 0;
    HTMLColor2RGB(get_options().get_bg_color().c_str(), &r, &g, &b);
    xcb_alloc_color_reply_t *reply = xcb_alloc_color_reply(m_conn,
            xcb_alloc_color(m_conn, colormap,
                            (uint16_t) r << 8,
                            (uint16_t) g << 8,
                            (uint16_t) b << 8), NULL);
    uint32_t colorpixel = reply->pixel;
    free(reply);

    /* Prepare to fill the background */
    xcb_gcontext_t background = xcb_generate_id(m_conn);
    uint32_t        mask       = XCB_GC_BACKGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t        values[2]  = {0, colorpixel};
    xcb_create_gc(m_conn, background, drawable, mask, values);
    xcb_rectangle_t rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = npwindow.width;
    rect.height = npwindow.height;

    /* Fill the background */
    xcb_poly_fill_rectangle(m_conn, drawable, background, 1, &rect);
    xcb_free_gc(m_conn, background);
}

bool VlcWindowlessXCB::handle_event(void *event)
{
    XEvent *xevent = static_cast<XEvent *>(event);
    switch (xevent->type) {
    case GraphicsExpose:

        XGraphicsExposeEvent *xgeevent = reinterpret_cast<XGraphicsExposeEvent *>(xevent);

        /* Something went wrong during initialization */
        if (!m_conn || !m_screen)
            break;

        drawBackground(xgeevent->drawable);

        /* Validate video size */
        if (m_media_width == 0 || m_media_height == 0)
            break;

        /* Create our X11 image */
        xcb_image_t *image = xcb_image_create_native(
                            m_conn,
                            m_media_width,
                            m_media_height,
                            XCB_IMAGE_FORMAT_Z_PIXMAP,
                            24,
                            &m_frame_buf[0],
                            m_frame_buf.size(),
                            NULL
                    );

        /* Compute the position of the video */
        int left = (npwindow.width  - m_media_width)  / 2;
        int top  = (npwindow.height - m_media_height) / 2;

        /* TODO Expose:

        xcb_pixmap_t pmap = xcb_generate_id(m_conn);
        xcb_create_pixmap(m_conn, 24, pmap, xgeevent->drawable, m_media_width, m_media_height);

        gc = xcb_generate_id(m_conn);
        xcb_create_gc(m_conn, gc, xgeevent->drawable, 0, NULL);
        xcb_image_put(m_conn, pmap, gc, image, left, top, 0);
        xcb_copy_area(m_conn, pmap, xgeevent->drawable, gc, 0, 0, 0, 0, m_media_width, m_media_height);
        */

        /* Push the frame in X11 */
        xcb_gcontext_t  gc = xcb_generate_id(m_conn);
        xcb_create_gc(m_conn, gc, xgeevent->drawable, 0, NULL);

        //FIXME xcb_put_image_checked is more efficient than xcb_image_*
        xcb_image_put(m_conn, xgeevent->drawable, gc, image, left, top, 0);

        /* Flush the the connection */
        xcb_flush(m_conn);
        xcb_free_gc(m_conn, gc);
    }
    return VlcWindowlessBase::handle_event(event);
}
