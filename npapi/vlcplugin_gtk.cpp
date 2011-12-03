#include "vlcplugin_gtk.h"
#include <gdk/gdkx.h>
#include <cstring>

static uint32_t getXid(GtkWidget *widget) {
    GdkDrawable *video_drawable = gtk_widget_get_window(widget);
    return (uint32_t)gdk_x11_drawable_get_xid(video_drawable);
}

VlcPluginGtk::VlcPluginGtk(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode),
    parent(NULL),
    parent_vbox(NULL),
    video(NULL)
{
}

VlcPluginGtk::~VlcPluginGtk()
{
}

void VlcPluginGtk::set_player_window()
{
    libvlc_media_player_set_xwindow(libvlc_media_player,
                                    (uint32_t)getXid(video));
}

void VlcPluginGtk::toggle_fullscreen()
{
    if (playlist_isplaying())
        libvlc_toggle_fullscreen(libvlc_media_player);
}

void VlcPluginGtk::set_fullscreen(int yes)
{
    if (playlist_isplaying())
        libvlc_set_fullscreen(libvlc_media_player, yes);
}

int  VlcPluginGtk::get_fullscreen()
{
    int r = 0;
    if (playlist_isplaying())
        r = libvlc_get_fullscreen(libvlc_media_player);
    return r;
}

void VlcPluginGtk::show_toolbar()
{
    /* TODO */
}

void VlcPluginGtk::hide_toolbar()
{
    /* TODO */
}

void VlcPluginGtk::update_controls()
{
    /* TODO */
}

bool VlcPluginGtk::create_windows()
{
    Display *p_display = ( (NPSetWindowCallbackStruct *)
                           npwindow.ws_info )->display;
    Window socket = (Window) npwindow.window;
    GdkColor color_black;
    gdk_color_parse("black", &color_black);

    parent = gtk_plug_new(socket);
    gtk_widget_modify_bg(parent, GTK_STATE_NORMAL, &color_black);

    parent_vbox = gtk_vbox_new(false, 0);
    gtk_container_add(GTK_CONTAINER(parent), parent_vbox);

    video = gtk_drawing_area_new();
    gtk_widget_modify_bg(video, GTK_STATE_NORMAL, &color_black);
    gtk_box_pack_start(GTK_BOX(parent_vbox), video, true, true, 0);

    gtk_widget_show_all(parent);

    return true;
}

bool VlcPluginGtk::resize_windows()
{
    GtkRequisition req;
    req.width = npwindow.width;
    req.height = npwindow.height;
    gtk_widget_size_request(parent, &req);
#if 0
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
                npwindow.width, ( npwindow.height - i_tb_height ) );
    }

    return true;
#endif
}

bool VlcPluginGtk::destroy_windows()
{
    /* TODO */
}
