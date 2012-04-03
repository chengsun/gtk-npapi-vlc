/*****************************************************************************
 * vlcplugin_base.cpp: a VLC plugin for Mozilla
 *****************************************************************************
 * Copyright (C) 2002-2010 the VideoLAN team
 * $Id$
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Damien Fouilleul <damienf.fouilleul@laposte.net>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vlcplugin.h"

#include "control/npolibvlc.h"

#include <cctype>

#if defined(XP_UNIX)
#   include <pthread.h>
#elif defined(XP_WIN)
    /* windows headers */
#   include <winbase.h>
#else
#warning "locking not implemented for this platform"
#endif

#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>

/*****************************************************************************
 * utility functions
 *****************************************************************************/
static void plugin_lock_init(plugin_lock_t *lock)
{
    assert(lock);

#if defined(XP_UNIX)
    pthread_mutex_init(&lock->mutex, NULL);
#elif defined(XP_WIN)
    InitializeCriticalSection(&lock->cs);
#else
#warning "locking not implemented in this platform"
#endif
}

static void plugin_lock_destroy(plugin_lock_t *lock)
{
    assert(lock);

#if defined(XP_UNIX)
    pthread_mutex_destroy(&lock->mutex);
#elif defined(XP_WIN)
    DeleteCriticalSection(&lock->cs);
#else
#warning "locking not implemented in this platform"
#endif
}

static void plugin_lock(plugin_lock_t *lock)
{
    assert(lock);

#if defined(XP_UNIX)
    pthread_mutex_lock(&lock->mutex);
#elif defined(XP_WIN)
    EnterCriticalSection(&lock->cs);
#else
#warning "locking not implemented in this platform"
#endif
}

static void plugin_unlock(plugin_lock_t *lock)
{
    assert(lock);

#if defined(XP_UNIX)
    pthread_mutex_unlock(&lock->mutex);
#elif defined(XP_WIN)
    LeaveCriticalSection(&lock->cs);
#else
#warning "locking not implemented in this platform"
#endif
}

/*****************************************************************************
 * Event Object
 *****************************************************************************/
static void handle_input_event(const libvlc_event_t* event, void *param);
static void handle_changed_event(const libvlc_event_t* event, void *param);

static vlcplugin_event_t vlcevents[] = {
    { "MediaPlayerMediaChanged", libvlc_MediaPlayerMediaChanged, handle_input_event },
    { "MediaPlayerNothingSpecial", libvlc_MediaPlayerNothingSpecial, handle_input_event },
    { "MediaPlayerOpening", libvlc_MediaPlayerOpening, handle_input_event },
    { "MediaPlayerBuffering", libvlc_MediaPlayerBuffering, handle_changed_event },
    { "MediaPlayerPlaying", libvlc_MediaPlayerPlaying, handle_input_event },
    { "MediaPlayerPaused", libvlc_MediaPlayerPaused, handle_input_event },
    { "MediaPlayerStopped", libvlc_MediaPlayerStopped, handle_input_event },
    { "MediaPlayerForward", libvlc_MediaPlayerForward, handle_input_event },
    { "MediaPlayerBackward", libvlc_MediaPlayerBackward, handle_input_event },
    { "MediaPlayerEndReached", libvlc_MediaPlayerEndReached, handle_input_event },
    { "MediaPlayerEncounteredError", libvlc_MediaPlayerEncounteredError, handle_input_event },
    { "MediaPlayerTimeChanged", libvlc_MediaPlayerTimeChanged, handle_changed_event },
    { "MediaPlayerPositionChanged", libvlc_MediaPlayerPositionChanged, handle_changed_event },
    { "MediaPlayerSeekableChanged", libvlc_MediaPlayerSeekableChanged, handle_changed_event },
    { "MediaPlayerPausableChanged", libvlc_MediaPlayerPausableChanged, handle_changed_event },
    { "MediaPlayerTitleChanged", libvlc_MediaPlayerTitleChanged, handle_changed_event },
    { "MediaPlayerLengthChanged", libvlc_MediaPlayerLengthChanged, handle_changed_event },
};

static void handle_input_event(const libvlc_event_t* event, void *param)
{
    VlcPluginBase *plugin = (VlcPluginBase*)param;
    switch( event->type )
    {
        case libvlc_MediaPlayerNothingSpecial:
        case libvlc_MediaPlayerOpening:
        case libvlc_MediaPlayerPlaying:
        case libvlc_MediaPlayerPaused:
        case libvlc_MediaPlayerStopped:
        case libvlc_MediaPlayerForward:
        case libvlc_MediaPlayerBackward:
        case libvlc_MediaPlayerEndReached:
        case libvlc_MediaPlayerEncounteredError:
            plugin->event_callback(event, NULL, 0);
            break;
        default: /* ignore all other libvlc_event_type_t */
            break;
    }
}

static void handle_changed_event(const libvlc_event_t* event, void *param)
{
    uint32_t   npcount = 1;
    NPVariant *npparam = (NPVariant *) NPN_MemAlloc( sizeof(NPVariant) * npcount );

    VlcPluginBase *plugin = (VlcPluginBase*)param;
    switch( event->type )
    {
#ifdef LIBVLC120
        case libvlc_MediaPlayerBuffering:
            DOUBLE_TO_NPVARIANT(event->u.media_player_buffering.new_cache, npparam[0]);
            break;
#endif
        case libvlc_MediaPlayerTimeChanged:
            DOUBLE_TO_NPVARIANT(event->u.media_player_time_changed.new_time, npparam[0]);
            break;
        case libvlc_MediaPlayerPositionChanged:
            DOUBLE_TO_NPVARIANT(event->u.media_player_position_changed.new_position, npparam[0]);
            break;
        case libvlc_MediaPlayerSeekableChanged:
            BOOLEAN_TO_NPVARIANT(event->u.media_player_seekable_changed.new_seekable, npparam[0]);
            break;
        case libvlc_MediaPlayerPausableChanged:
            BOOLEAN_TO_NPVARIANT(event->u.media_player_pausable_changed.new_pausable, npparam[0]);
            break;
        case libvlc_MediaPlayerTitleChanged:
            BOOLEAN_TO_NPVARIANT(event->u.media_player_title_changed.new_title, npparam[0]);
            break;
        case libvlc_MediaPlayerLengthChanged:
            DOUBLE_TO_NPVARIANT(event->u.media_player_length_changed.new_length, npparam[0]);
            break;
        default: /* ignore all other libvlc_event_type_t */
            NPN_MemFree( npparam );
            return;
    }
    plugin->event_callback(event, npparam, npcount);
}

bool EventObj::init()
{
    plugin_lock_init(&lock);
    return true;
}

EventObj::~EventObj()
{
    plugin_lock_destroy(&lock);
}

void EventObj::deliver(NPP browser)
{
    if(_already_in_deliver)
        return;

    plugin_lock(&lock);
    _already_in_deliver = true;

    for( ev_l::iterator iter = _elist.begin(); iter != _elist.end(); ++iter )
    {
        for( lr_l::iterator j = _llist.begin(); j != _llist.end(); ++j )
        {
            if( j->event_type() == iter->event_type() )
            {
                NPVariant result;
                NPVariant *params = iter->params();
                uint32_t   count  = iter->count();

                NPObject *listener = j->listener();
                assert( listener );

                NPN_InvokeDefault( browser, listener, params, count, &result );
                NPN_ReleaseVariantValue( &result );

                for( uint32_t n = 0; n < count; n++ )
                {
                    if( NPVARIANT_IS_STRING(params[n]) )
                        NPN_MemFree( (void*) NPVARIANT_TO_STRING(params[n]).UTF8Characters );
                    else if( NPVARIANT_IS_OBJECT(params[n]) )
                    {
                        NPN_ReleaseObject( NPVARIANT_TO_OBJECT(params[n]) );
                        NPN_MemFree( (void*)NPVARIANT_TO_OBJECT(params[n]) );
                    }
                }
                if (params) NPN_MemFree( params );
            }
        }
    }
    _elist.clear();

    _already_in_deliver = false;
    plugin_unlock(&lock);
}

void EventObj::callback(const libvlc_event_t* event,
                        NPVariant *npparams, uint32_t count)
{
    plugin_lock(&lock);
    _elist.push_back(VLCEvent(event->type, npparams, count));
    plugin_unlock(&lock);
}

vlcplugin_event_t *EventObj::find_event(const char *s) const
{
    for( size_t i = 0; i < ARRAY_SIZE(vlcevents); i++ )
    {
        if( strncmp(vlcevents[i].name, s, strlen(vlcevents[i].name)) == 0 )
            return &vlcevents[i];
    }
    return NULL;
}

const char *EventObj::find_name(const libvlc_event_t *event)
{
    for( size_t i = 0; i < ARRAY_SIZE(vlcevents); i++ )
    {
        if( vlcevents[i].libvlc_type == event->type )
            return vlcevents[i].name;
    }
    return NULL;
}

bool EventObj::insert(const NPString &name, NPObject *listener, bool bubble)
{
    vlcplugin_event_t *event = find_event(name.UTF8Characters);
    if( !event )
        return false;

    for( lr_l::iterator iter = _llist.begin(); iter != _llist.end(); ++iter )
    {
        if( iter->listener() == listener &&
            event->libvlc_type == iter->event_type() &&
            iter->bubble() == bubble )
        {
            return false;
        }
    }

    _llist.push_back( Listener(event, listener, bubble) );
    return true;
}

bool EventObj::remove(const NPString &name, NPObject *listener, bool bubble)
{
    vlcplugin_event_t *event = find_event(name.UTF8Characters);
    if( !event )
        return false;

    for( lr_l::iterator iter = _llist.begin(); iter !=_llist.end(); iter++ )
    {
        if( iter->event_type() == event->libvlc_type &&
            iter->listener() == listener &&
            iter->bubble() == bubble )
        {
            iter = _llist.erase(iter);
            return true;
        }
    }

    return false;
}

void EventObj::hook_manager( libvlc_event_manager_t *em, void *userdata )
{
    _em = em;

    if( _em )
    {
        /* attach all libvlc events we care about */
        for( size_t i = 0; i < ARRAY_SIZE(vlcevents); i++ )
        {
            libvlc_event_attach( _em, vlcevents[i].libvlc_type,
                                      vlcevents[i].libvlc_callback,
                                      userdata );
        }
    }
}

void EventObj::unhook_manager( void *userdata )
{
    if( _em )
    {
        /* detach all libvlc events we cared about */
        for( size_t i = 0; i < ARRAY_SIZE(vlcevents); i++ )
        {
            libvlc_event_detach( _em, vlcevents[i].libvlc_type,
                                      vlcevents[i].libvlc_callback,
                                      userdata );
        }
    }
}

/*****************************************************************************
 * VlcPluginBase constructor and destructor
 *****************************************************************************/
VlcPluginBase::VlcPluginBase( NPP instance, NPuint16_t mode ) :
    i_npmode(mode),
    b_videocompat(0),
    b_stream(0),
    psz_target(NULL),
    playlist_index(-1),
    libvlc_instance(NULL),
    libvlc_media_list(NULL),
    libvlc_media_player(NULL),
    p_scriptClass(NULL),
    p_browser(instance),
    psz_baseURL(NULL)
{
    memset(&npwindow, 0, sizeof(NPWindow));
    _instances.insert(this);
}

static bool boolValue(const char *value) {
    return ( !strcmp(value, "1") ||
             !strcasecmp(value, "true") ||
             !strcasecmp(value, "yes") );
}

std::set<VlcPluginBase*> VlcPluginBase::_instances;

void VlcPluginBase::eventAsync(void *param)
{
    VlcPluginBase *plugin = (VlcPluginBase*)param;
    if( _instances.find(plugin) == _instances.end() )
        return;

    plugin->events.deliver(plugin->getBrowser());
    plugin->update_controls();
}

void VlcPluginBase::event_callback(const libvlc_event_t* event,
                NPVariant *npparams, uint32_t npcount)
{
#if defined(XP_UNIX) || defined(XP_WIN)
    events.callback(event, npparams, npcount);
    NPN_PluginThreadAsyncCall(getBrowser(), eventAsync, this);
#else
#   warning NPN_PluginThreadAsyncCall not implemented yet.
    printf("No NPN_PluginThreadAsyncCall(), doing nothing.\n");
#endif
}

NPError VlcPluginBase::init(int argc, char* const argn[], char* const argv[])
{
    /* prepare VLC command line */
    const char *ppsz_argv[32];
    int ppsz_argc = 0;

#ifndef NDEBUG
    ppsz_argv[ppsz_argc++] = "--no-plugins-cache";
#endif

    /* locate VLC module path */
#ifdef XP_MACOSX
    ppsz_argv[ppsz_argc++] = "--plugin-path=/Library/Internet\\ Plug-Ins/VLC\\ Plugin.plugin/Contents/MacOS/plugins";
    ppsz_argv[ppsz_argc++] = "--vout=minimal_macosx";
#elif defined(XP_WIN)
    HKEY h_key;
    DWORD i_type, i_data = MAX_PATH + 1;
    char p_data[MAX_PATH + 1];
    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, "Software\\VideoLAN\\VLC",
                      0, KEY_READ, &h_key ) == ERROR_SUCCESS )
    {
         if( RegQueryValueEx( h_key, "InstallDir", 0, &i_type,
                              (LPBYTE)p_data, &i_data ) == ERROR_SUCCESS )
         {
             if( i_type == REG_SZ )
             {
                 strcat( p_data, "\\plugins" );
                 ppsz_argv[ppsz_argc++] = "--plugin-path";
                 ppsz_argv[ppsz_argc++] = p_data;
             }
         }
         RegCloseKey( h_key );
    }
    ppsz_argv[ppsz_argc++] = "--no-one-instance";

#endif /* XP_MACOSX */

    /* common settings */
    ppsz_argv[ppsz_argc++] = "-vv";
    ppsz_argv[ppsz_argc++] = "--no-stats";
    ppsz_argv[ppsz_argc++] = "--no-media-library";
    ppsz_argv[ppsz_argc++] = "--intf=dummy";
    ppsz_argv[ppsz_argc++] = "--no-video-title-show";
    ppsz_argv[ppsz_argc++] = "--no-xlib";

    bool b_set_toolbar = false,
         b_set_allowfullscreen = false,
         b_set_autoplay = false,
         b_mute = false,
         b_set_mute = false,
         b_loop = false,
         b_set_loop = false;

    /* parse plugin arguments */
    for( int i = 0; (i < argc) && (ppsz_argc < 32); i++ )
    {
       /* fprintf(stderr, "argn=%s, argv=%s\n", argn[i], argv[i]); */

        if( !strcmp( argn[i], "target" )
         || !strcmp( argn[i], "mrl")
         || !strcmp( argn[i], "filename")
         || !strcmp( argn[i], "src") )
        {
            psz_target = argv[i];
        }
        else if( !strcmp( argn[i], "text" ) )
        {
            set_bg_text( argv[i] );
        }
        else if( !strcmp( argn[i], "videocompat" ) )
        {
            /* use API compatibility wrapper with <video> */
            b_videocompat = boolValue(argv[i]);
        }
        else if( !strcmp( argn[i], "autoplay")
              || !strcmp( argn[i], "autostart") )
        {
            set_autoplay(boolValue(argv[i]));
            b_set_autoplay = true;
        }
        else if( !strcmp( argn[i], "fullscreen" )
              || !strcmp( argn[i], "allowfullscreen" ) )
        {
            set_enable_fs( boolValue(argv[i]) );
            b_set_allowfullscreen = true;
        }
        else if( !strcmp( argn[i], "mute" ) )
        {
            b_mute = boolValue(argv[i]);
            b_set_mute = true;
        }
        else if( !strcmp( argn[i], "loop")
              || !strcmp( argn[i], "autoloop") )
        {
            b_loop = boolValue(argv[i]);
            b_set_loop = true;
        }
        else if( !strcmp( argn[i], "toolbar" ) )
        {
            set_show_toolbar( boolValue(argv[i]) );
            b_set_toolbar = true;
        }
        else if( !strcmp( argn[i], "bgcolor" ) )
        {
            set_bg_color( argv[i] );
        }
    }

    if( b_videocompat ) {
        b_toolbar = b_set_toolbar;
        b_allowfullscreen = b_set_allowfullscreen;
        b_loop = b_set_loop;
        b_autoplay = b_set_autoplay;
        b_mute = b_set_mute;
    }

    if( b_mute )
    {
        ppsz_argv[ppsz_argc++] = "--volume=0";
    }

    if( b_loop )
    {
        ppsz_argv[ppsz_argc++] = "--loop";
    }
    else
    {
        ppsz_argv[ppsz_argc++] = "--no-loop";
    }

    libvlc_instance = libvlc_new(ppsz_argc, ppsz_argv);
    if( !libvlc_instance )
        return NPERR_GENERIC_ERROR;
    libvlc_media_list = libvlc_media_list_new(libvlc_instance);

    /*
    ** fetch plugin base URL, which is the URL of the page containing the plugin
    ** this URL is used for making absolute URL from relative URL that may be
    ** passed as an MRL argument
    */
    NPObject *plugin = NULL;

    if( NPERR_NO_ERROR == NPN_GetValue(p_browser, NPNVWindowNPObject, &plugin) )
    {
        /*
        ** is there a better way to get that info ?
        */
        static const char docLocHref[] = "document.location.href";
        NPString script;
        NPVariant result;

        script.UTF8Characters = docLocHref;
        script.UTF8Length = sizeof(docLocHref)-1;

        if( NPN_Evaluate(p_browser, plugin, &script, &result) )
        {
            if( NPVARIANT_IS_STRING(result) )
            {
                NPString &location = NPVARIANT_TO_STRING(result);

                psz_baseURL = (char *) malloc(location.UTF8Length+1);
                if( psz_baseURL )
                {
                    strncpy(psz_baseURL, location.UTF8Characters, location.UTF8Length);
                    psz_baseURL[location.UTF8Length] = '\0';
                }
            }
            NPN_ReleaseVariantValue(&result);
        }
        NPN_ReleaseObject(plugin);
    }

    if( psz_target )
    {
        // get absolute URL from src
        char *psz_absurl = getAbsoluteURL(psz_target);
        psz_target = psz_absurl ? psz_absurl : strdup(psz_target);
    }

    /* assign plugin script root class */
    if( !b_videocompat )
        /* new APIs */
        p_scriptClass = RuntimeNPClass<LibvlcRootNPObject>::getClass();
    else
        /* <video> compatibility wrapper */
        p_scriptClass = RuntimeNPClass<LibvlcCompatNPObject>::getClass();

    if( !events.init() )
        return NPERR_GENERIC_ERROR;

    return NPERR_NO_ERROR;
}

VlcPluginBase::~VlcPluginBase()
{
    free(psz_baseURL);
    free(psz_target);

    if( libvlc_media_player )
    {
        if( playlist_isplaying() )
            playlist_stop();
        events.unhook_manager( this );
        libvlc_media_player_release( libvlc_media_player );
    }
    if( libvlc_media_list )
        libvlc_media_list_release( libvlc_media_list );
    if( libvlc_instance )
        libvlc_release( libvlc_instance );

    _instances.erase(this);
}

void VlcPluginBase::setWindow(const NPWindow &window)
{
    npwindow = window;
}

/*****************************************************************************
 * VlcPluginBase playlist replacement methods
 *****************************************************************************/
int VlcPluginBase::playlist_add( const char *mrl )
{
    int item = -1;
    libvlc_media_t *p_m = libvlc_media_new_location(libvlc_instance,mrl);
    if( !p_m )
        return -1;
    assert( libvlc_media_list );
    libvlc_media_list_lock(libvlc_media_list);
    if( !libvlc_media_list_add_media(libvlc_media_list,p_m) )
        item = libvlc_media_list_count(libvlc_media_list)-1;
    libvlc_media_list_unlock(libvlc_media_list);

    libvlc_media_release(p_m);

    return item;
}

int VlcPluginBase::playlist_add_extended_untrusted( const char *mrl, const char *,
                    int optc, const char **optv )
{
    libvlc_media_t *p_m;
    int item = -1;

    assert( libvlc_media_list );

    p_m = libvlc_media_new_location(libvlc_instance, mrl);
    if( !p_m )
        return -1;

    for( int i = 0; i < optc; ++i )
        libvlc_media_add_option_flag(p_m, optv[i], libvlc_media_option_unique);

    libvlc_media_list_lock(libvlc_media_list);
    if( !libvlc_media_list_add_media(libvlc_media_list,p_m) )
        item = libvlc_media_list_count(libvlc_media_list)-1;
    libvlc_media_list_unlock(libvlc_media_list);
    libvlc_media_release(p_m);

    return item;
}

bool VlcPluginBase::playlist_select( int idx )
{
    libvlc_media_t *p_m = NULL;

    assert( libvlc_media_list );

    libvlc_media_list_lock(libvlc_media_list);

    int count = libvlc_media_list_count(libvlc_media_list);
    if( idx<0||idx>=count )
        goto bad_unlock;

    playlist_index = idx;

    p_m = libvlc_media_list_item_at_index(libvlc_media_list,playlist_index);
    libvlc_media_list_unlock(libvlc_media_list);

    if( !p_m )
        return false;

    if( libvlc_media_player )
    {
        if( playlist_isplaying() )
            playlist_stop();
        events.unhook_manager( this );
        on_media_player_release();
        libvlc_media_player_release( libvlc_media_player );
        libvlc_media_player = NULL;
    }

    libvlc_media_player = libvlc_media_player_new_from_media( p_m );
    if( libvlc_media_player )
    {
        on_media_player_new();
        set_player_window();

        libvlc_event_manager_t *p_em;
        p_em = libvlc_media_player_event_manager( libvlc_media_player );
        events.hook_manager( p_em, this );
    }

    libvlc_media_release( p_m );
    return true;

bad_unlock:
    libvlc_media_list_unlock( libvlc_media_list );
    return false;
}

int VlcPluginBase::playlist_delete_item( int idx )
{
    if( !libvlc_media_list )
        return -1;
    libvlc_media_list_lock(libvlc_media_list);
    int ret = libvlc_media_list_remove_index(libvlc_media_list,idx);
    libvlc_media_list_unlock(libvlc_media_list);
    return ret;
}

void VlcPluginBase::playlist_clear()
{
    if( libvlc_media_list )
        libvlc_media_list_release(libvlc_media_list);
    libvlc_media_list = libvlc_media_list_new(getVLC());
}

int VlcPluginBase::playlist_count()
{
    int items_count = 0;
    if( !libvlc_media_list )
        return items_count;
    libvlc_media_list_lock(libvlc_media_list);
    items_count = libvlc_media_list_count(libvlc_media_list);
    libvlc_media_list_unlock(libvlc_media_list);
    return items_count;
}


bool  VlcPluginBase::player_has_vout()
{
    bool r = false;
    if( playlist_isplaying() )
        r = libvlc_media_player_has_vout(libvlc_media_player);
    return r;
}

/*****************************************************************************
 * VlcPluginBase methods
 *****************************************************************************/

char *VlcPluginBase::getAbsoluteURL(const char *url)
{
    if( NULL != url )
    {
        // check whether URL is already absolute
        const char *end=strchr(url, ':');
        if( (NULL != end) && (end != url) )
        {
            // validate protocol header
            const char *start = url;
            char c = *start;
            if( isalpha(c) )
            {
                ++start;
                while( start != end )
                {
                    c  = *start;
                    if( ! (isalnum(c)
                       || ('-' == c)
                       || ('+' == c)
                       || ('.' == c)
                       || ('/' == c)) ) /* VLC uses / to allow user to specify a demuxer */
                        // not valid protocol header, assume relative URL
                        goto relativeurl;
                    ++start;
                }
                /* we have a protocol header, therefore URL is absolute */
                return strdup(url);
            }
            // not a valid protocol header, assume relative URL
        }

relativeurl:

        if( psz_baseURL )
        {
            size_t baseLen = strlen(psz_baseURL);
            char *href = (char *) malloc(baseLen+strlen(url)+1);
            if( href )
            {
                /* prepend base URL */
                memcpy(href, psz_baseURL, baseLen+1);

                /*
                ** relative url could be empty,
                ** in which case return base URL
                */
                if( '\0' == *url )
                    return href;

                /*
                ** locate pathname part of base URL
                */

                /* skip over protocol part  */
                char *pathstart = strchr(href, ':');
                char *pathend = href+baseLen;
                if( pathstart )
                {
                    if( '/' == *(++pathstart) )
                    {
                        if( '/' == *(++pathstart) )
                        {
                            ++pathstart;
                        }
                    }
                    /* skip over host part */
                    pathstart = strchr(pathstart, '/');
                    if( ! pathstart )
                    {
                        // no path, add a / past end of url (over '\0')
                        pathstart = pathend;
                        *pathstart = '/';
                    }
                }
                else
                {
                    /* baseURL is just a UNIX path */
                    if( '/' != *href )
                    {
                        /* baseURL is not an absolute path */
                        free(href);
                        return NULL;
                    }
                    pathstart = href;
                }

                /* relative URL made of an absolute path ? */
                if( '/' == *url )
                {
                    /* replace path completely */
                    strcpy(pathstart, url);
                    return href;
                }

                /* find last path component and replace it */
                while( '/' != *pathend)
                    --pathend;

                /*
                ** if relative url path starts with one or more '../',
                ** factor them out of href so that we return a
                ** normalized URL
                */
                while( pathend != pathstart )
                {
                    const char *p = url;
                    if( '.' != *p )
                        break;
                    ++p;
                    if( '\0' == *p  )
                    {
                        /* relative url is just '.' */
                        url = p;
                        break;
                    }
                    if( '/' == *p  )
                    {
                        /* relative url starts with './' */
                        url = ++p;
                        continue;
                    }
                    if( '.' != *p )
                        break;
                    ++p;
                    if( '\0' == *p )
                    {
                        /* relative url is '..' */
                    }
                    else
                    {
                        if( '/' != *p )
                            break;
                        /* relative url starts with '../' */
                        ++p;
                    }
                    url = p;
                    do
                    {
                        --pathend;
                    }
                    while( '/' != *pathend );
                }
                /* skip over '/' separator */
                ++pathend;
                /* concatenate remaining base URL and relative URL */
                strcpy(pathend, url);
            }
            return href;
        }
    }
    return NULL;
}

void VlcPluginBase::control_handler(vlc_toolbar_clicked_t clicked)
{
    switch( clicked )
    {
        case clicked_Play:
        {
            playlist_play();
        }
        break;

        case clicked_Pause:
        {
            playlist_pause();
        }
        break;

        case clicked_Stop:
        {
            playlist_stop();
        }
        break;

        case clicked_Fullscreen:
        {
            toggle_fullscreen();
        }
        break;

        case clicked_Mute:
        case clicked_Unmute:
#if 0
        {
            if( p_md )
                libvlc_audio_toggle_mute( p_md );
        }
#endif
        break;

        case clicked_timeline:
#if 0
        {
            /* if a movie is loaded */
            if( p_md )
            {
                int64_t f_length;
                f_length = libvlc_media_player_get_length( p_md ) / 100;

                f_length = (float)f_length *
                        ( ((float)i_xPos-4.0 ) / ( ((float)i_width-8.0)/100) );

                libvlc_media_player_set_time( p_md, f_length );
            }
        }
#endif
        break;

        case clicked_Time:
        {
            /* Not implemented yet*/
        }
        break;

        default: /* button_Unknown */
            fprintf(stderr, "button Unknown!\n");
        break;
    }
}

// Verifies the version of the NPAPI.
// The eventListeners use a NPAPI function available
// since Gecko 1.9.
bool VlcPluginBase::canUseEventListener()
{
    int plugin_major, plugin_minor;
    int browser_major, browser_minor;

    NPN_Version(&plugin_major, &plugin_minor,
                &browser_major, &browser_minor);

    if (browser_minor >= 19 || browser_major > 0)
        return true;
    return false;
}
