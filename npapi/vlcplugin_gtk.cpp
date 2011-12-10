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
    video(NULL),
    toolbar(NULL)
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
    gtk_box_pack_start(GTK_BOX(parent_vbox), toolbar, false, false, 0);
    gtk_widget_show_all(toolbar);
}

void VlcPluginGtk::hide_toolbar()
{
    gtk_widget_hide(toolbar);
    gtk_container_remove(GTK_CONTAINER(parent_vbox), toolbar);
}

struct tool_actions_t
{
    const gchar *stock_id;
    vlc_toolbar_clicked_t clicked;
};
static const tool_actions_t tool_actions[] = {
    {GTK_STOCK_MEDIA_PLAY, clicked_Play},
    {GTK_STOCK_MEDIA_PAUSE, clicked_Pause},
    {GTK_STOCK_MEDIA_STOP, clicked_Stop},
    {"gtk-volume-muted", clicked_Mute},
    {"gtk-volume-unmuted", clicked_Unmute}
};

static void toolbar_handler(GtkToolButton *btn, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    const gchar *stock_id = gtk_tool_button_get_stock_id(btn);
    for (int i = 0; i < sizeof(tool_actions)/sizeof(tool_actions_t); ++i) {
        if (!strcmp(stock_id, tool_actions[i].stock_id)) {
            plugin->control_handler(tool_actions[i].clicked);
            return;
        }
    }
    fprintf(stderr, "WARNING: No idea what toolbar button you just clicked on (%s)\n", stock_id?stock_id:"NULL");
}

static bool time_slider_handler(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    libvlc_media_player_set_position(plugin->getMD(), value/100.0);
    return false;
}

static bool vol_slider_handler(GtkRange *range, GtkScrollType scroll, gdouble value, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    libvlc_audio_set_volume(plugin->getMD(), value);
    return false;
}

void VlcPluginGtk::update_controls()
{
    GtkToolItem *toolbutton;

    /* play/pause button */
    const gchar *stock_id = playlist_isplaying() ? GTK_STOCK_MEDIA_PAUSE : GTK_STOCK_MEDIA_PLAY;
    toolbutton = gtk_toolbar_get_nth_item(GTK_TOOLBAR(toolbar), 0);
    if (strcmp(gtk_tool_button_get_stock_id(GTK_TOOL_BUTTON(toolbutton)), stock_id)) {
        gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(toolbutton), stock_id);
        /* work around firefox not displaying the icon properly after change */
        g_object_ref(toolbutton);
        gtk_container_remove(GTK_CONTAINER(toolbar), GTK_WIDGET(toolbutton));
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolbutton, 0);
        g_object_unref(toolbutton);
    }

    /* time slider */
    if (!libvlc_media_player ||
            !libvlc_media_player_is_seekable(libvlc_media_player)) {
        gtk_widget_set_sensitive(time_slider, false);
        gtk_range_set_value(GTK_RANGE(time_slider), 0);
    } else {
        gtk_widget_set_sensitive(time_slider, true);
        gdouble timepos = 100*libvlc_media_player_get_position(libvlc_media_player);
        gtk_range_set_value(GTK_RANGE(time_slider), timepos);
    }

    gtk_widget_show_all(toolbar);
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


    /*** TOOLBAR ***/

    toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
    GtkToolItem *toolitem;
    /* play/pause */
    toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(toolbar_handler), this);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
    /* stop */
    toolitem = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(toolbar_handler), this);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

    /* time slider */
    toolitem = gtk_tool_item_new();
    time_slider = gtk_hscale_new_with_range(0, 100, 10);
    gtk_scale_set_draw_value(GTK_SCALE(time_slider), false);
    g_signal_connect(G_OBJECT(time_slider), "change-value", G_CALLBACK(time_slider_handler), this);
    gtk_container_add(GTK_CONTAINER(toolitem), time_slider);
    gtk_tool_item_set_expand(toolitem, true);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
    
    /* volume slider */
    toolitem = gtk_tool_item_new();
    GtkWidget *vol_slider = gtk_hscale_new_with_range(0, 200, 10);
    gtk_scale_set_draw_value(GTK_SCALE(vol_slider), false);
    g_signal_connect(G_OBJECT(vol_slider), "change-value", G_CALLBACK(vol_slider_handler), this);
    gtk_range_set_value(GTK_RANGE(vol_slider), 100);
    gtk_widget_set_size_request(vol_slider, 100, -1);
    gtk_container_add(GTK_CONTAINER(toolitem), vol_slider);
    gtk_tool_item_set_expand(toolitem, false);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

    update_controls();
    show_toolbar();

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
