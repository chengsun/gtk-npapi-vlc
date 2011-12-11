#include "vlcplugin_gtk.h"
#include <gdk/gdkx.h>
#include <cstring>

static uint32_t get_xid(GtkWidget *widget) {
    GdkDrawable *video_drawable = gtk_widget_get_window(widget);
    return (uint32_t)gdk_x11_drawable_get_xid(video_drawable);
}

VlcPluginGtk::VlcPluginGtk(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode),
    parent(NULL),
    parent_vbox(NULL),
    video_container(NULL),
    toolbar(NULL),
    popupmenu(NULL),
    fullscreen_win(NULL),
    is_fullscreen(false)
{
    memset(&video_xwindow, 0, sizeof(Window));
}

VlcPluginGtk::~VlcPluginGtk()
{
}

Display *VlcPluginGtk::get_display()
{
    return ( (NPSetWindowCallbackStruct *)
             npwindow.ws_info )->display;
}

void VlcPluginGtk::set_player_window()
{
    libvlc_media_player_set_xwindow(libvlc_media_player,
                                    video_xwindow);
    libvlc_video_set_mouse_input(libvlc_media_player, 0);
}

void VlcPluginGtk::toggle_fullscreen()
{
    set_fullscreen(!get_fullscreen());
}

void VlcPluginGtk::set_fullscreen(int yes)
{
    /* we have to reparent windows */
    /* note that the xid of video_container changes after reparenting */
    Display *display = get_display();
    g_signal_handler_block(video_container, video_container_size_handler_id);
    if (yes) {
        XUnmapWindow(display, video_xwindow);
        XReparentWindow(display, video_xwindow,
                        gdk_x11_get_default_root_xwindow(), 0,0);

        gtk_widget_reparent(GTK_WIDGET(parent_vbox), GTK_WIDGET(fullscreen_win));
        gtk_widget_show(fullscreen_win);
        gtk_window_fullscreen(GTK_WINDOW(fullscreen_win));

        XReparentWindow(display, video_xwindow, get_xid(video_container), 0,0);
        XMapWindow(display, video_xwindow);
    } else {
        XUnmapWindow(display, video_xwindow);
        XReparentWindow(display, video_xwindow,
                        gdk_x11_get_default_root_xwindow(), 0,0);

        gtk_widget_hide(fullscreen_win);
        gtk_widget_reparent(GTK_WIDGET(parent_vbox), GTK_WIDGET(parent));
        gtk_widget_show_all(GTK_WIDGET(parent));

        XReparentWindow(display, video_xwindow, get_xid(video_container), 0,0);
        XMapWindow(display, video_xwindow);
    }
//    libvlc_set_fullscreen(libvlc_media_player, yes);
    g_signal_handler_unblock(video_container, video_container_size_handler_id);
    gtk_widget_queue_resize_no_redraw(video_container);

    is_fullscreen = yes;
}

int  VlcPluginGtk::get_fullscreen()
{
    return is_fullscreen;
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

void VlcPluginGtk::resize_video_xwindow(GdkRectangle *rect)
{
    Display *display = get_display();
    XResizeWindow(display, video_xwindow,
                  rect->width, rect->height);
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

static void menu_handler(GtkMenuItem *menuitem, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    const gchar *stock_id = gtk_menu_item_get_label(GTK_MENU_ITEM(menuitem));
    for (int i = 0; i < sizeof(tool_actions)/sizeof(tool_actions_t); ++i) {
        if (!strcmp(stock_id, tool_actions[i].stock_id)) {
            plugin->control_handler(tool_actions[i].clicked);
            return;
        }
    }
    fprintf(stderr, "WARNING: No idea what menu item you just clicked on (%s)\n", stock_id?stock_id:"NULL");
}

void VlcPluginGtk::popup_menu()
{
    /* construct menu */
    GtkWidget *popupmenu = gtk_menu_new();
    GtkWidget *menuitem;

    /* play/pause */
    menuitem = gtk_image_menu_item_new_from_stock(
                        playlist_isplaying() ?
                        GTK_STOCK_MEDIA_PAUSE :
                        GTK_STOCK_MEDIA_PLAY, NULL);
    g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(menu_handler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupmenu), menuitem);
    /* stop */
    menuitem = gtk_image_menu_item_new_from_stock(
                                GTK_STOCK_MEDIA_STOP, NULL);
    g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(menu_handler), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(popupmenu), menuitem);

    gtk_widget_show_all(popupmenu);
    gtk_menu_attach_to_widget(GTK_MENU(popupmenu), video_container, NULL);
    gtk_menu_popup(GTK_MENU(popupmenu), NULL, NULL, NULL, NULL,
                   0, gtk_get_current_event_time());
}

static bool video_button_handler(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        plugin->popup_menu();
        return true;
    }
    if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
        plugin->toggle_fullscreen();
    }
    return false;
}

static bool video_popup_handler(GtkWidget *widget, gpointer user_data) {
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    plugin->popup_menu();
    return true;
}

static bool video_size_handler(GtkWidget *widget, GdkRectangle *rect, gpointer user_data) {
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    plugin->resize_video_xwindow(rect);
    return true;
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

static void fullscreen_win_visibility_handler(GtkWidget *widget, gpointer user_data)
{
    VlcPluginGtk *plugin = (VlcPluginGtk *) user_data;
    plugin->set_fullscreen(gtk_widget_get_visible(widget));
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
    Window socket = (Window) npwindow.window;
    GdkColor color_black;
    gdk_color_parse("black", &color_black);

    parent = gtk_plug_new(socket);
    gtk_widget_modify_bg(parent, GTK_STATE_NORMAL, &color_black);

    parent_vbox = gtk_vbox_new(false, 0);
    gtk_container_add(GTK_CONTAINER(parent), parent_vbox);

    video_container = gtk_drawing_area_new();
    gtk_widget_modify_bg(video_container, GTK_STATE_NORMAL, &color_black);
    gtk_widget_add_events(video_container,
            GDK_BUTTON_PRESS_MASK
          | GDK_BUTTON_RELEASE_MASK);
    g_signal_connect(G_OBJECT(video_container), "button-press-event", G_CALLBACK(video_button_handler), this);
    g_signal_connect(G_OBJECT(video_container), "popup-menu", G_CALLBACK(video_popup_handler), this);
    gtk_box_pack_start(GTK_BOX(parent_vbox), video_container, true, true, 0);

    gtk_widget_show_all(parent);

    /* fullscreen top-level */
    fullscreen_win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(fullscreen_win), false);
    g_signal_connect(G_OBJECT(fullscreen_win), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), this);
    g_signal_connect(G_OBJECT(fullscreen_win), "show", G_CALLBACK(fullscreen_win_visibility_handler), this);
    g_signal_connect(G_OBJECT(fullscreen_win), "hide", G_CALLBACK(fullscreen_win_visibility_handler), this);

    /* actual video window */
    /* libvlc is handed this window's xid. A raw X window is used because
     * GTK+ is incapable of reparenting without changing xid
     */
    Display *display = get_display();
    int blackColor = BlackPixel(display, DefaultScreen(display));
    video_xwindow = XCreateSimpleWindow(display, get_xid(video_container), 0, 0,
                   1, 1,
                   0, blackColor, blackColor);
    XMapWindow(display, video_xwindow);

    /* connect video_container resizes to video_xwindow */
    video_container_size_handler_id = g_signal_connect(
                G_OBJECT(video_container), "size-allocate",
                G_CALLBACK(video_size_handler), this);
    gtk_widget_queue_resize_no_redraw(video_container);

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
}

bool VlcPluginGtk::destroy_windows()
{
    /* TODO */
}
