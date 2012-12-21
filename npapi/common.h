/*****************************************************************************
 * vlcplugin.h: a VLC plugin for Mozilla
 *****************************************************************************
 * Copyright (C) 2002-2012 VideoLAN
 * $Id$
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Damien Fouilleul <damienf.fouilleul@laposte.net>
 *          Jean-Paul Saman <jpsaman@videolan.org>
 *          Sergey Radionov <rsatom@gmail.com>
 *          Jean-Baptiste Kempf <jb@videolan.org>
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

#ifndef __VLCPLUGIN_COMMON_H__
#define __VLCPLUGIN_COMMON_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

// Setup XP_MACOSX, XP_UNIX, XP_WIN
#if defined(_WIN32)
#   define XP_WIN 1
#elif defined(__APPLE__)
#   define HAVE_PTHREAD
#   define XP_MACOSX 1
#else
#   define HAVE_PTHREAD
#   define XP_UNIX 1
#   define MOZ_X11 1
#endif

#if !defined(XP_MACOSX) && !defined(XP_UNIX) && !defined(XP_WIN)
#   define XP_UNIX 1
#elif defined(XP_MACOSX)
#   undef XP_UNIX
#endif

#ifndef __MAX
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __MIN
#   define __MIN(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// Include stdint before NP*.h
#include <stdint.h>

// We use <npfunctions.h> insted of including <npapi.h>
// To avoid using Microsoft SDK (rather then from Mozilla SDK),
#include <npfunctions.h>

#if (((NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR) < 20)
    typedef uint16 NPuint16_t;
    typedef int16 NPint16_t;
    typedef int32 NPint32_t;
#else
    typedef uint16_t NPuint16_t;
    typedef int16_t NPint16_t;
    typedef int32_t NPint32_t;
#endif

/* Lock includes */
#ifdef XP_WIN
#   include <windows.h>
#elif defined(HAVE_PTHREAD)
#   include <pthread.h>
#endif
#include <assert.h>

typedef struct {
#if defined(HAVE_PTHREAD)
    pthread_mutex_t mutex;
#elif defined(XP_WIN)
    CRITICAL_SECTION cs;
#else
# warning "locking not implemented in this platform"
#endif
} plugin_lock_t;

/*****************************************************************************
 * Lock utility functions
 *****************************************************************************/
static void plugin_lock_init(plugin_lock_t *lock)
{
    assert(lock);

#if defined(HAVE_PTHREAD)
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

#if defined(HAVE_PTHREAD)
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

#if defined(HAVE_PTHREAD)
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

#if defined(HAVE_PTHREAD)
    pthread_mutex_unlock(&lock->mutex);
#elif defined(XP_WIN)
    LeaveCriticalSection(&lock->cs);
#else
#warning "locking not implemented in this platform"
#endif
}

#endif /* __VLCPLUGIN_H__ */
