/*****************************************************************************
 * vlcshell.c: a VLC plugin for Mozilla
 *****************************************************************************
 * Copyright (C) 2002 VideoLAN
 * $Id: vlcshell.cpp,v 1.10 2003/02/18 13:13:12 sam Exp $
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdio.h>
#include <string.h>

/* vlc stuff */
#include <vlc/vlc.h>

/* Mozilla stuff */
#include <npapi.h>

#ifdef XP_WIN
    /* Windows stuff */
#endif

#ifdef XP_UNIX
    /* X11 stuff */
#   include <X11/Xlib.h>
#   include <X11/Intrinsic.h>
#   include <X11/StringDefs.h>
#endif

#include "vlcpeer.h"
#include "vlcplugin.h"

/* XXX: disable VLC */
#define USE_LIBVLC 1

#if USE_LIBVLC
#   define WINDOW_TEXT "(no picture)"
#else
#   define WINDOW_TEXT "(no libvlc)"
#endif

/*****************************************************************************
 * Unix-only declarations
******************************************************************************/
#ifdef XP_UNIX
#   define VOUT_PLUGINS "xvideo,x11,dummy"
#   define AOUT_PLUGINS "oss,dummy"

static void Redraw( Widget w, XtPointer closure, XEvent *event );
#endif

/*****************************************************************************
 * Windows-only declarations
 *****************************************************************************/
#ifdef XP_WIN
#   define VOUT_PLUGINS "directx,dummy"
#   define AOUT_PLUGINS "none" /* "directx,waveout,dummy" */

HINSTANCE g_hDllInstance = NULL;

BOOL WINAPI
DllMain( HINSTANCE  hinstDLL,                   // handle of DLL module
                    DWORD  fdwReason,       // reason for calling function
                    LPVOID  lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hDllInstance = hinstDLL;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_PROCESS_DETACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

LRESULT CALLBACK Manage( HWND, UINT, WPARAM, LPARAM );
#endif

/******************************************************************************
 * UNIX-only API calls
 *****************************************************************************/
char * NPP_GetMIMEDescription( void )
{
    return PLUGIN_MIMETYPES;
}

NPError NPP_GetValue( NPP instance, NPPVariable variable, void *value )
{
    static nsIID nsid = VLCINTF_IID;
    static char psz_desc[1000];

    switch( variable )
    {
        case NPPVpluginNameString:
            *((char **)value) = PLUGIN_NAME;
            return NPERR_NO_ERROR;

        case NPPVpluginDescriptionString:
#if USE_LIBVLC
            snprintf( psz_desc, 1000-1, PLUGIN_DESCRIPTION, VLC_Version() );
#else
            snprintf( psz_desc, 1000-1, PLUGIN_DESCRIPTION, "(disabled)" );
#endif
            psz_desc[1000-1] = 0;
            *((char **)value) = psz_desc;
            return NPERR_NO_ERROR;

        default:
            /* go on... */
            break;
    }

    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    VlcPlugin* p_plugin = (VlcPlugin*) instance->pdata;

    switch( variable )
    {
        case NPPVpluginScriptableInstance:
            *(nsISupports**)value = p_plugin->GetPeer();
            if( *(nsISupports**)value == NULL )
            {
                return NPERR_OUT_OF_MEMORY_ERROR;
            }
            break;

        case NPPVpluginScriptableIID:
            *(nsIID**)value = (nsIID*)NPN_MemAlloc( sizeof(nsIID) );
            if( *(nsIID**)value == NULL )
            {
                return NPERR_OUT_OF_MEMORY_ERROR;
            }
            **(nsIID**)value = nsid;
            break;

        default:
            return NPERR_GENERIC_ERROR;
    }

    return NPERR_NO_ERROR;
}

/******************************************************************************
 * General Plug-in Calls
 *****************************************************************************/
NPError NPP_Initialize( void )
{
    return NPERR_NO_ERROR;
}

jref NPP_GetJavaClass( void )
{
    return NULL;
}

void NPP_Shutdown( void )
{
    ;
}

NPError NPP_New( NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc,
                 char* argn[], char* argv[], NPSavedData* saved )
{
    int i;
#if USE_LIBVLC
    vlc_value_t value;
    int i_ret;

    char *ppsz_foo[] =
    {
        "vlc"
        /*, "--plugin-path", "/home/sam/videolan/vlc_MAIN/plugins"*/
    };
#endif

    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    VlcPlugin * p_plugin = new VlcPlugin( instance );

    if( p_plugin == NULL )
    {
        return NPERR_OUT_OF_MEMORY_ERROR;
    }

    instance->pdata = p_plugin;

#ifdef XP_WIN
    p_plugin->p_hwnd = NULL;
    p_plugin->pf_wndproc = NULL;
#endif

#ifdef XP_UNIX
    p_plugin->window = 0;
    p_plugin->p_display = NULL;
#endif

    p_plugin->p_npwin = NULL;
    p_plugin->i_npmode = mode;
    p_plugin->i_width = 0;
    p_plugin->i_height = 0;

#if USE_LIBVLC
    p_plugin->i_vlc = VLC_Create();
    if( p_plugin->i_vlc < 0 )
    {
        p_plugin->i_vlc = 0;
        delete p_plugin;
        p_plugin = NULL;
        return NPERR_GENERIC_ERROR;
    }

    i_ret = VLC_Init( p_plugin->i_vlc, sizeof(ppsz_foo)/sizeof(char*), ppsz_foo );
    if( i_ret )
    {
        VLC_Destroy( p_plugin->i_vlc );
        p_plugin->i_vlc = 0;
        delete p_plugin;
        p_plugin = NULL;
        return NPERR_GENERIC_ERROR;
    }

    value.psz_string = "dummy";
    VLC_Set( p_plugin->i_vlc, "conf::intf", value );
    value.psz_string = VOUT_PLUGINS;
    VLC_Set( p_plugin->i_vlc, "conf::vout", value );
    value.psz_string = AOUT_PLUGINS;
    VLC_Set( p_plugin->i_vlc, "conf::aout", value );

#else
    p_plugin->i_vlc = 1;

#endif /* USE_LIBVLC */

    p_plugin->b_stream = VLC_FALSE;
    p_plugin->b_autoplay = VLC_FALSE;
    p_plugin->psz_target = NULL;

    for( i = 0; i < argc ; i++ )
    {
        if( !strcmp( argn[i], "target" ) )
        {
            p_plugin->psz_target = argv[i];
        }
        else if( !strcmp( argn[i], "autoplay" ) )
        {
            if( !strcmp( argv[i], "yes" ) )
            {
                p_plugin->b_autoplay = 1;
            }
        }
        else if( !strcmp( argn[i], "autostart" ) )
        {
            if( !strcmp( argv[i], "1" ) || !strcmp( argv[i], "true" ) )
            {
                p_plugin->b_autoplay = 1;
            }
        }
        else if( !strcmp( argn[i], "filename" ) )
        {
            p_plugin->psz_target = argv[i];
        }
        else if( !strcmp( argn[i], "src" ) )
        {
            p_plugin->psz_target = argv[i];
        }

#if USE_LIBVLC
        else if( !strcmp( argn[i], "loop" ) )
        {
            if( !strcmp( argv[i], "yes" ) )
            {
                value.b_bool = VLC_TRUE;
                VLC_Set( p_plugin->i_vlc, "conf::loop", value );
            }
        }
#endif
    }

    if( p_plugin->psz_target )
    {
        p_plugin->psz_target = strdup( p_plugin->psz_target );
    }

    return NPERR_NO_ERROR;
}

NPError NPP_Destroy( NPP instance, NPSavedData** save )
{
    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    VlcPlugin* p_plugin = (VlcPlugin*)instance->pdata;

    if( p_plugin != NULL )
    {
        if( p_plugin->i_vlc )
        {
#if USE_LIBVLC
            VLC_Stop( p_plugin->i_vlc );
            VLC_Destroy( p_plugin->i_vlc );
#endif
            p_plugin->i_vlc = 0;
        }

        if( p_plugin->psz_target )
        {
            free( p_plugin->psz_target );
            p_plugin->psz_target = NULL;
        }

        delete p_plugin;
    }

    instance->pdata = NULL;

    return NPERR_NO_ERROR;
}

NPError NPP_SetWindow( NPP instance, NPWindow* window )
{
    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    VlcPlugin* p_plugin = (VlcPlugin*)instance->pdata;

    /* Write the window ID for vlc */
#if USE_LIBVLC
    vlc_value_t value;

    /* FIXME: this cast sucks */
    value.i_int = (int) (ptrdiff_t) (void *) window->window;
    VLC_Set( p_plugin->i_vlc, "drawable", value );
#endif

    /*
     * PLUGIN DEVELOPERS:
     *  Before setting window to point to the
     *  new window, you may wish to compare the new window
     *  info to the previous window (if any) to note window
     *  size changes, etc.
     */

#ifdef XP_WIN
    if( !window || !window->window )
    {
        /* Window was destroyed. Invalidate everything. */
        if( p_plugin->p_npwin )
        {
            SetWindowLong( p_plugin->p_hwnd, GWL_WNDPROC,
                           (LONG)p_plugin->pf_wndproc );
            p_plugin->pf_wndproc = NULL;
            p_plugin->p_hwnd = NULL;
        }

        p_plugin->p_npwin = window;
        return NPERR_NO_ERROR;
    }

    if( p_plugin->p_npwin )
    {
        if( p_plugin->p_hwnd == (HWND)window->window )
        {
            /* Same window, but something may have changed. First we
             * update the plugin structure, then we redraw the window */
            InvalidateRect( p_plugin->p_hwnd, NULL, TRUE );
            p_plugin->i_width = window->width;
            p_plugin->i_height = window->height;
            p_plugin->p_npwin = window;
            UpdateWindow( p_plugin->p_hwnd );
            return NPERR_NO_ERROR;
        }

        /* Window has changed. Destroy the one we have, and go
         * on as if it was a real initialization. */
        SetWindowLong( p_plugin->p_hwnd, GWL_WNDPROC,
                       (LONG)p_plugin->pf_wndproc );
        p_plugin->pf_wndproc = NULL;
        p_plugin->p_hwnd = NULL;
    }

    p_plugin->pf_wndproc = (WNDPROC)SetWindowLong( (HWND)window->window,
                                                   GWL_WNDPROC, (LONG)Manage );
    p_plugin->p_hwnd = (HWND)window->window;
    SetProp( p_plugin->p_hwnd, "w00t", (HANDLE)p_plugin );
    InvalidateRect( p_plugin->p_hwnd, NULL, TRUE );
    UpdateWindow( p_plugin->p_hwnd );
#endif

#ifdef XP_UNIX
    p_plugin->window = (Window) window->window;
    p_plugin->p_display = ((NPSetWindowCallbackStruct *)window->ws_info)->display;

    Widget w = XtWindowToWidget( p_plugin->p_display, p_plugin->window );
    XtAddEventHandler( w, ExposureMask, FALSE,
                       (XtEventHandler)Redraw, p_plugin );
    Redraw( w, (XtPointer)p_plugin, NULL );
#endif

    p_plugin->p_npwin = window;

    p_plugin->i_width = window->width;
    p_plugin->i_height = window->height;

    if( !p_plugin->b_stream )
    {
        int i_mode = PLAYLIST_APPEND;

        if( p_plugin->b_autoplay )
        {
            i_mode |= PLAYLIST_GO;
        }

        if( p_plugin->psz_target )
        {
#if USE_LIBVLC
            VLC_AddTarget( p_plugin->i_vlc, p_plugin->psz_target,
                           i_mode, PLAYLIST_END );
#endif
            p_plugin->b_stream = VLC_TRUE;
        }
    }

    return NPERR_NO_ERROR;
}

NPError NPP_NewStream( NPP instance, NPMIMEType type, NPStream *stream,
                       NPBool seekable, uint16 *stype )
{
    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

#if 0
    VlcPlugin* p_plugin = (VlcPlugin*)instance->pdata;
#endif

    /* fprintf(stderr, "NPP_NewStream - FILE mode !!\n"); */

    /* We want a *filename* ! */
    *stype = NP_ASFILE;

#if 0
    if( !p_plugin->b_stream )
    {
        p_plugin->psz_target = strdup( stream->url );
        p_plugin->b_stream = VLC_TRUE;
    }
#endif

    return NPERR_NO_ERROR;
}

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
                   * mode so we can take any size stream in our
                   * write call (since we ignore it) */

#define SARASS_SIZE (1024*1024)

int32 NPP_WriteReady( NPP instance, NPStream *stream )
{
    VlcPlugin* p_plugin;

    /* fprintf(stderr, "NPP_WriteReady\n"); */

    if (instance != NULL)
    {
        p_plugin = (VlcPlugin*) instance->pdata;
        /* Muahahahahahahaha */
        return STREAMBUFSIZE;
        /*return SARASS_SIZE;*/
    }

    /* Number of bytes ready to accept in NPP_Write() */
    return STREAMBUFSIZE;
    /*return 0;*/
}


int32 NPP_Write( NPP instance, NPStream *stream, int32 offset,
                 int32 len, void *buffer )
{
    /* fprintf(stderr, "NPP_Write %i\n", (int)len); */

    if( instance != NULL )
    {
        /*VlcPlugin* p_plugin = (VlcPlugin*) instance->pdata;*/
    }

    return len;         /* The number of bytes accepted */
}


NPError NPP_DestroyStream( NPP instance, NPStream *stream, NPError reason )
{
    if( instance == NULL )
    {
        return NPERR_INVALID_INSTANCE_ERROR;
    }

    return NPERR_NO_ERROR;
}


void NPP_StreamAsFile( NPP instance, NPStream *stream, const char* fname )
{
    if( instance == NULL )
    {
        return;
    }

    /* fprintf(stderr, "NPP_StreamAsFile %s\n", fname); */

#if USE_LIBVLC
    VlcPlugin* p_plugin = (VlcPlugin*)instance->pdata;

    VLC_AddTarget( p_plugin->i_vlc, fname,
                   PLAYLIST_APPEND | PLAYLIST_GO, PLAYLIST_END );
#endif
}


void NPP_URLNotify( NPP instance, const char* url,
                    NPReason reason, void* notifyData )
{
    /***** Insert NPP_URLNotify code here *****\
    PluginInstance* p_plugin;
    if (instance != NULL)
        p_plugin = (PluginInstance*) instance->pdata;
    \*********************************************/
}


void NPP_Print( NPP instance, NPPrint* printInfo )
{
    if( printInfo == NULL )
    {
        return;
    }

    if( instance != NULL )
    {
        /***** Insert NPP_Print code here *****\
        PluginInstance* p_plugin = (PluginInstance*) instance->pdata;
        \**************************************/

        if( printInfo->mode == NP_FULL )
        {
            /*
             * PLUGIN DEVELOPERS:
             *  If your plugin would like to take over
             *  printing completely when it is in full-screen mode,
             *  set printInfo->pluginPrinted to TRUE and print your
             *  plugin as you see fit.  If your plugin wants Netscape
             *  to handle printing in this case, set
             *  printInfo->pluginPrinted to FALSE (the default) and
             *  do nothing.  If you do want to handle printing
             *  yourself, printOne is true if the print button
             *  (as opposed to the print menu) was clicked.
             *  On the Macintosh, platformPrint is a THPrint; on
             *  Windows, platformPrint is a structure
             *  (defined in npapi.h) containing the printer name, port,
             *  etc.
             */

            /***** Insert NPP_Print code here *****\
            void* platformPrint =
                printInfo->print.fullPrint.platformPrint;
            NPBool printOne =
                printInfo->print.fullPrint.printOne;
            \**************************************/

            /* Do the default*/
            printInfo->print.fullPrint.pluginPrinted = FALSE;
        }
        else
        {
            /* If not fullscreen, we must be embedded */
            /*
             * PLUGIN DEVELOPERS:
             *  If your plugin is embedded, or is full-screen
             *  but you returned false in pluginPrinted above, NPP_Print
             *  will be called with mode == NP_EMBED.  The NPWindow
             *  in the printInfo gives the location and dimensions of
             *  the embedded plugin on the printed page.  On the
             *  Macintosh, platformPrint is the printer port; on
             *  Windows, platformPrint is the handle to the printing
             *  device context.
             */

            /***** Insert NPP_Print code here *****\
            NPWindow* printWindow =
                &(printInfo->print.embedPrint.window);
            void* platformPrint =
                printInfo->print.embedPrint.platformPrint;
            \**************************************/
        }
    }
}

/******************************************************************************
 * Windows-only methods
 *****************************************************************************/
#ifdef XP_WIN
LRESULT CALLBACK Manage( HWND p_hwnd, UINT i_msg, WPARAM wpar, LPARAM lpar )
{
    VlcPlugin* p_plugin = (VlcPlugin*) GetProp( p_hwnd, "w00t" );

    switch( i_msg )
    {
#if !USE_LIBVLC
        case WM_PAINT:
        {
            PAINTSTRUCT paintstruct;
            HDC hdc;
            RECT rect;

            hdc = BeginPaint( p_hwnd, &paintstruct );

            GetClientRect( p_hwnd, &rect );
            FillRect( hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH) );
            TextOut( hdc, p_plugin->i_width / 2 - 40, p_plugin->i_height / 2,
                     WINDOW_TEXT, strlen(WINDOW_TEXT) );

            EndPaint( p_hwnd, &paintstruct );
            break;
        }
#endif
        default:
            p_plugin->pf_wndproc( p_hwnd, i_msg, wpar, lpar );
            break;
    }
    return 0;
}
#endif

/******************************************************************************
 * UNIX-only methods
 *****************************************************************************/
#ifdef XP_UNIX
static void Redraw( Widget w, XtPointer closure, XEvent *event )
{
    VlcPlugin* p_plugin = (VlcPlugin*)closure;
    GC gc;
    XGCValues gcv;

    gcv.foreground = BlackPixel( p_plugin->p_display, 0 );
    gc = XCreateGC( p_plugin->p_display, p_plugin->window, GCForeground, &gcv );

    XFillRectangle( p_plugin->p_display, p_plugin->window, gc,
                    0, 0, p_plugin->i_width, p_plugin->i_height );

    gcv.foreground = WhitePixel( p_plugin->p_display, 0 );
    XChangeGC( p_plugin->p_display, gc, GCForeground, &gcv );

    XDrawString( p_plugin->p_display, p_plugin->window, gc,
                 p_plugin->i_width / 2 - 40, p_plugin->i_height / 2,
                 WINDOW_TEXT, strlen(WINDOW_TEXT) );

    XFreeGC( p_plugin->p_display, gc );
}
#endif

