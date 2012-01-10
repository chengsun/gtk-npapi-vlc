/*****************************************************************************
 * vlc_win32_fullscreen.h: a VLC plugin for Mozilla
 *****************************************************************************
 * Copyright © 2002-2011 VideoLAN and VLC authors
 * $Id$
 *
 * Authors: Sergey Radionov <rsatom@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_FULLSCREEN_H
#define VLC_FULLSCREEN_H

#ifdef _WIN32

#include <vlc/vlc.h>

#include "win32_vlcwnd.h"
#include "vlc_player_options.h"

struct VLCViewResources
{
    VLCViewResources()
        :hNewMessageBitmap(0), hDeFullscreenBitmap(0), hPauseBitmap(0),
         hPlayBitmap(0), hVolumeBitmap(0), hVolumeMutedBitmap(0),
         hBackgroundIcon(0)
    {};

    HANDLE hNewMessageBitmap;
    HANDLE hFullscreenBitmap;
    HANDLE hDeFullscreenBitmap;
    HANDLE hPauseBitmap;
    HANDLE hPlayBitmap;
    HANDLE hVolumeBitmap;
    HANDLE hVolumeMutedBitmap;
    HICON  hBackgroundIcon;
};

////////////////////////////////////////////////////////////////////////////////
//class VLCControlsWnd
////////////////////////////////////////////////////////////////////////////////
class VLCWindowsManager;
class VLCControlsWnd: public VLCWnd
{
    enum{
        xControlsSpace = 5
    };

    enum{
        ID_FS_SWITCH_FS = 1,
        ID_FS_PLAY_PAUSE = 2,
        ID_FS_VIDEO_POS_SCROLL = 3,
        ID_FS_MUTE = 4,
        ID_FS_VOLUME = 5,
    };

protected:
    VLCControlsWnd(HINSTANCE hInstance, VLCWindowsManager* WM);
    bool Create(HWND hWndParent);

public:
    static VLCControlsWnd*
        CreateControlsWindow(HINSTANCE hInstance,
                             VLCWindowsManager* wm, HWND hWndParent);
    ~VLCControlsWnd();

    void NeedShowControls();

    //libvlc events arrives from separate thread
    void OnLibVlcEvent(const libvlc_event_t* event);

protected:
    virtual void PreRegisterWindowClass(WNDCLASS* wc);
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void SetVideoPosScrollRangeByVideoLen();
    void SyncVideoPosScrollPosWithVideoPos();
    void SetVideoPos(float Pos); //0-start, 1-end

    void SyncVolumeSliderWithVLCVolume();
    void SetVLCVolumeBySliderPos(int CurScrollPos);
    void SetVideoPosScrollPosByVideoPos(libvlc_time_t CurScrollPos);
    void UpdateButtons();

    void NeedHideControls();

    void handle_position_changed_event(const libvlc_event_t* event);
    void handle_input_state_event(const libvlc_event_t* event);

    bool IsPlaying()
    {
        libvlc_media_player_t* mp = MP();
        if( mp )
            return libvlc_media_player_is_playing(mp) != 0;
        return false;
    }

private:
    VLCWindowsManager* _wm;

    VLCWindowsManager& WM() {return *_wm;}
    inline const VLCViewResources& RC();
    inline libvlc_media_player_t* MP() const;
    inline const vlc_player_options* PO() const;

    void CreateToolTip();

private:
    HWND hToolTipWnd;
    HWND hFSButton;
    HWND hPlayPauseButton;
    HWND hVideoPosScroll;
    HWND hMuteButton;
    HWND hVolumeSlider;

    int VideoPosShiftBits;
};

class VLCWindowsManager;
///////////////////////
//VLCHolderWnd
///////////////////////
class VLCHolderWnd
{
public:
    static void RegisterWndClassName(HINSTANCE hInstance);
    static void UnRegisterWndClassName();
    static VLCHolderWnd* CreateHolderWindow(HWND hParentWnd, VLCWindowsManager* WM);
    void DestroyWindow();

    void LibVlcAttach();
    void LibVlcDetach();

    void NeedShowControls()
        { if(_CtrlsWnd) _CtrlsWnd->NeedShowControls(); }

    //libvlc events arrives from separate thread
    void OnLibVlcEvent(const libvlc_event_t* event);

private:
    static LPCTSTR getClassName(void)  { return TEXT("VLC ActiveX Window Holder Class"); };
    static LRESULT CALLBACK VLCHolderClassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    HWND FindMP_hWnd();

    HHOOK _hMouseHook;
    DWORD _MouseHookThreadId;
    void MouseHook(bool SetHook);

     VLCWindowsManager& WM()
        {return *_WindowsManager;}
    inline libvlc_media_player_t* getMD() const;
    inline const VLCViewResources& RC() const;

private:
    static HINSTANCE _hinstance;
    static ATOM _holder_wndclass_atom;

private:
    VLCHolderWnd(HWND hWnd, VLCWindowsManager* WM)
        : _hMouseHook(NULL), _MouseHookThreadId(0), _hWnd(hWnd),
        _WindowsManager(WM), _CtrlsWnd(0) {};

public:
    HWND getHWND() const {return _hWnd;}

private:
    HWND _hWnd;
    VLCWindowsManager* _WindowsManager;
    VLCControlsWnd*    _CtrlsWnd;
};

///////////////////////
//VLCFullScreenWnd
///////////////////////
class VLCFullScreenWnd
{
public:
    static void RegisterWndClassName(HINSTANCE hInstance);
    static void UnRegisterWndClassName();
    static VLCFullScreenWnd* CreateFSWindow(VLCWindowsManager* WM);
    void DestroyWindow()
        {::DestroyWindow(_hWnd);};

private:
    static LPCTSTR getClassName(void) { return TEXT("VLC ActiveX Fullscreen Class"); };
    static LRESULT CALLBACK FSWndWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static HINSTANCE _hinstance;
    static ATOM _fullscreen_wndclass_atom;

private:
    VLCFullScreenWnd(HWND hWnd, VLCWindowsManager* WM)
        :_WindowsManager(WM), _hWnd(hWnd) {};

    ~VLCFullScreenWnd(){};

private:
     VLCWindowsManager& WM()
        {return *_WindowsManager;}
    inline libvlc_media_player_t* getMD() const;
    inline const VLCViewResources& RC() const;

public:
    //libvlc events arrives from separate thread
    void OnLibVlcEvent(const libvlc_event_t* event) {};

private:
    void NeedHideControls();

private:
    void CreateToolTip();

private:
    VLCWindowsManager* _WindowsManager;

public:
    HWND getHWND() const {return _hWnd;}

private:
    HWND _hWnd;

    int VideoPosShiftBits;
};

///////////////////////
//VLCWindowsManager
///////////////////////
class VLCWindowsManager
{
public:
    VLCWindowsManager(HMODULE hModule, const VLCViewResources& rc,
                      const vlc_player_options* = 0);
    ~VLCWindowsManager();

    void CreateWindows(HWND hWindowedParentWnd);
    void DestroyWindows();

    void LibVlcAttach(libvlc_media_player_t* p_md);
    void LibVlcDetach();

    void StartFullScreen();
    void EndFullScreen();
    void ToggleFullScreen();
    bool IsFullScreen();

    HMODULE getHModule() const {return _hModule;};
    VLCHolderWnd* getHolderWnd() const {return _HolderWnd;}
    VLCFullScreenWnd* getFullScreenWnd() const {return _FSWnd;}
    libvlc_media_player_t* getMD() const {return _p_md;}
    const VLCViewResources& RC() const {return _rc;}
    const vlc_player_options* PO() const {return _po;}

public:
    void setNewMessageFlag(bool Yes)
        {_b_new_messages_flag = Yes;};
    bool getNewMessageFlag() const
        {return _b_new_messages_flag;};
public:
    void OnMouseEvent(UINT uMouseMsg);

private:
    void VlcEvents(bool Attach);
    //libvlc events arrives from separate thread
    static void OnLibVlcEvent_proxy(const libvlc_event_t* event, void *param);
    void OnLibVlcEvent(const libvlc_event_t* event);

private:
    const VLCViewResources& _rc;
    HMODULE _hModule;
    const vlc_player_options *const _po;

    HWND _hWindowedParentWnd;

    libvlc_media_player_t* _p_md;

    VLCHolderWnd* _HolderWnd;
    VLCFullScreenWnd* _FSWnd;

    bool _b_new_messages_flag;

private:
    DWORD Last_WM_MOUSEMOVE_Pos;
};

////////////////////////////
//inlines
////////////////////////////
inline libvlc_media_player_t* VLCControlsWnd::MP() const
{
    return _wm->getMD();
}

inline const VLCViewResources& VLCControlsWnd::RC()
{
    return _wm->RC();
}

inline const vlc_player_options* VLCControlsWnd::PO() const
{
    return _wm->PO();
}

inline libvlc_media_player_t* VLCHolderWnd::getMD() const
{
    return _WindowsManager->getMD();
}

inline const VLCViewResources& VLCHolderWnd::RC() const
{
    return _WindowsManager->RC();
}

inline libvlc_media_player_t* VLCFullScreenWnd::getMD() const
{
    return _WindowsManager->getMD();
}

inline const VLCViewResources& VLCFullScreenWnd::RC() const
{
    return _WindowsManager->RC();
}

#endif //_WIN32

#endif //VLC_FULLSCREEN_H
