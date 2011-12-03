#include "vlcplugin_mac.h"

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
    if (playlist_isplaying())
        libvlc_toggle_fullscreen(libvlc_media_player);
}

void VlcPluginMac::set_fullscreen(int yes)
{
    if (playlist_isplaying())
        libvlc_set_fullscreen(libvlc_media_player, yes);
}

int  VlcPluginMac::get_fullscreen()
{
    int r = 0;
    if (playlist_isplaying())
        r = libvlc_get_fullscreen(libvlc_media_player);
    return r;
}

void VlcPluginMac::show_toolbar()
{
    // TODO
}

void VlcPluginMac::hide_toolbar()
{
    // TODO
}

void VlcPluginMac::update_controls()
{
    // TODO
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
    view.top     = ((NP_Port*) (npwindow.window))->porty;
    view.left    = ((NP_Port*) (npwindow.window))->portx;
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
}

bool VlcPluginMac::destroy_windows()
{
    window.window = NULL;
}
