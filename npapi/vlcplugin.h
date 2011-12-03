#ifndef __VLCPLUGIN_H__

#include "vlcplugin_base.h"

#if defined(XP_UNIX)
#   if defined(USE_GTK)
#       include "vlcplugin_gtk.h"
        typedef VlcPluginGtk VlcPlugin;
#   else
#       include "vlcplugin_xlib.h"
        typedef VlcPluginXlib VlcPlugin;
#   endif
#elif defined(XP_WIN)
#   include "vlcplugin_win.h"
    typedef VlcPluginWin VlcPlugin;
#elif defined(XP_MAC)
#   include "vlcplugin_mac.h"
    typedef VlcPluginMac VlcPlugin;
#endif

#endif /* __VLCPLUGIN_H__ */
