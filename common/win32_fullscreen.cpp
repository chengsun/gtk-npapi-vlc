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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>

#include "win32_fullscreen.h"

/////////////////////////////////
//VLCHolderWnd static members
HINSTANCE VLCHolderWnd::_hinstance = 0;
ATOM VLCHolderWnd::_holder_wndclass_atom = 0;

enum{
    WM_TRY_SET_MOUSE_HOOK = WM_USER+1,
    WM_MOUSE_EVENT_NOTIFY = WM_APP+1,
    WM_MOUSE_EVENT_NOTIFY_SUCCESS = 0xFF
};

void VLCHolderWnd::RegisterWndClassName(HINSTANCE hInstance)
{
    //save hInstance for future use
    _hinstance = hInstance;

    WNDCLASS wClass;

    if( ! GetClassInfo(_hinstance, getClassName(), &wClass) )
    {
        wClass.style          = CS_DBLCLKS;
        wClass.lpfnWndProc    = VLCHolderClassWndProc;
        wClass.cbClsExtra     = 0;
        wClass.cbWndExtra     = 0;
        wClass.hInstance      = _hinstance;
        wClass.hIcon          = NULL;
        wClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wClass.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wClass.lpszMenuName   = NULL;
        wClass.lpszClassName  = getClassName();

        _holder_wndclass_atom = RegisterClass(&wClass);
    }
    else
    {
        _holder_wndclass_atom = 0;
    }
}

void VLCHolderWnd::UnRegisterWndClassName()
{
    if(0 != _holder_wndclass_atom){
        UnregisterClass(MAKEINTATOM(_holder_wndclass_atom), _hinstance);
        _holder_wndclass_atom = 0;
    }
}

VLCHolderWnd* VLCHolderWnd::CreateHolderWindow(HWND hParentWnd, VLCWindowsManager* WM)
{
    HWND hWnd = CreateWindow(getClassName(),
                             TEXT("Holder Window"),
                             WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE,
                             0, 0, 0, 0,
                             hParentWnd,
                             0,
                             VLCHolderWnd::_hinstance,
                             (LPVOID)WM
                             );

    if(hWnd)
        return reinterpret_cast<VLCHolderWnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    return 0;
}

LRESULT CALLBACK VLCHolderWnd::VLCHolderClassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VLCHolderWnd* h_data = reinterpret_cast<VLCHolderWnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch( uMsg )
    {
        case WM_CREATE:{
            CREATESTRUCT* CreateStruct = (CREATESTRUCT*)(lParam);
            VLCWindowsManager* WM = (VLCWindowsManager*)CreateStruct->lpCreateParams;

            h_data = new VLCHolderWnd(hWnd, WM);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(h_data));

            RECT ParentClientRect;
            GetClientRect(CreateStruct->hwndParent, &ParentClientRect);
            MoveWindow(hWnd, 0, 0,
                       (ParentClientRect.right-ParentClientRect.left),
                       (ParentClientRect.bottom-ParentClientRect.top), FALSE);
            break;
        }
        case WM_PAINT:{
            PAINTSTRUCT PaintStruct;
            HDC hDC = BeginPaint(hWnd, &PaintStruct);
            RECT rect;
            GetClientRect(hWnd, &rect);
            int IconX = ((rect.right - rect.left) - GetSystemMetrics(SM_CXICON))/2;
            int IconY = ((rect.bottom - rect.top) - GetSystemMetrics(SM_CYICON))/2;
            DrawIcon(hDC, IconX, IconY, h_data->RC().hBackgroundIcon);
            EndPaint(hWnd, &PaintStruct);
            break;
        }
        case WM_NCDESTROY:
            delete h_data;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            break;
        case WM_TRY_SET_MOUSE_HOOK:{
            h_data->MouseHook(true);
            break;
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
            h_data->_WindowsManager->OnMouseEvent(uMsg);
            break;
        case WM_MOUSE_EVENT_NOTIFY:{
            h_data->_WindowsManager->OnMouseEvent(wParam);
            return WM_MOUSE_EVENT_NOTIFY_SUCCESS;
        }
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
};

void VLCHolderWnd::DestroyWindow()
{
    LibVlcDetach();
    if(_hWnd)
        ::DestroyWindow(_hWnd);
};

LRESULT CALLBACK VLCHolderWnd::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    bool AllowReceiveMessage = true;
    if(nCode >= 0){
        switch(wParam){
            case WM_MOUSEMOVE:
            case WM_LBUTTONDBLCLK:{
                MOUSEHOOKSTRUCT* mhs = reinterpret_cast<MOUSEHOOKSTRUCT*>(lParam);

                //try to find HolderWnd and notify it
                HWND hNotifyWnd = mhs->hwnd;
                LRESULT SMRes = ::SendMessage(hNotifyWnd, WM_MOUSE_EVENT_NOTIFY, wParam, 0);
                while( hNotifyWnd && WM_MOUSE_EVENT_NOTIFY_SUCCESS != SMRes){
                    hNotifyWnd = GetParent(hNotifyWnd);
                    SMRes = ::SendMessage(hNotifyWnd, WM_MOUSE_EVENT_NOTIFY, wParam, 0);
                }

                AllowReceiveMessage = WM_MOUSEMOVE==wParam || (WM_MOUSE_EVENT_NOTIFY_SUCCESS != SMRes);
                break;
            }
        }
    }

    LRESULT NHRes = CallNextHookEx(NULL, nCode, wParam, lParam);
    if(AllowReceiveMessage)
        return NHRes;
    else
        return 1;
}

void VLCHolderWnd::MouseHook(bool SetHook)
{
    if(SetHook){
        const HWND hChildWnd = GetWindow(getHWND(), GW_CHILD);
        const DWORD WndThreadID = (hChildWnd) ? GetWindowThreadProcessId(hChildWnd, NULL) : 0;
        if( _hMouseHook &&( !hChildWnd || WndThreadID != _MouseHookThreadId) ){
            //unhook if something changed
            MouseHook(false);
        }

        if(!_hMouseHook && hChildWnd && WndThreadID){
            _MouseHookThreadId = WndThreadID;
            _hMouseHook =
                SetWindowsHookEx(WH_MOUSE, VLCHolderWnd::MouseHookProc,
                                 NULL, WndThreadID);
        }
    }
    else{
        if(_hMouseHook){
            UnhookWindowsHookEx(_hMouseHook);
            _MouseHookThreadId=0;
            _hMouseHook = 0;
        }
    }
}

//libvlc events arrives from separate thread
void VLCHolderWnd::OnLibVlcEvent(const libvlc_event_t* event)
{
    //We need set hook to catch doubleclicking (to switch to fullscreen and vice versa).
    //But libvlc media window may not exist yet,
    //and we don't know when it will be created, nor ThreadId of it.
    //So we try catch events,
    //(suppose wnd will be ever created),
    //and then try set mouse hook.
    const HWND hChildWnd = GetWindow(getHWND(), GW_CHILD);
    const DWORD WndThreadID = (hChildWnd) ? GetWindowThreadProcessId(hChildWnd, NULL) : 0;
    //if no hook, or window thread has changed
    if(!_hMouseHook || (hChildWnd && WndThreadID != _MouseHookThreadId)){
        //libvlc events arrives from separate thread,
        //so we need post message to main thread, to notify it.
        PostMessage(getHWND(), WM_TRY_SET_MOUSE_HOOK, 0, 0);
    }
}

void VLCHolderWnd::LibVlcAttach()
{
    libvlc_media_player_set_hwnd(getMD(), getHWND());
}

void VLCHolderWnd::LibVlcDetach()
{
    libvlc_media_player_t* p_md = getMD();
    if(p_md)
        libvlc_media_player_set_hwnd(p_md, 0);

    MouseHook(false);
}

/////////////////////////////////
//VLCFullScreenWnd static members
HINSTANCE VLCFullScreenWnd::_hinstance = 0;
ATOM VLCFullScreenWnd::_fullscreen_wndclass_atom = 0;
ATOM VLCFullScreenWnd::_fullscreen_controls_wndclass_atom = 0;

void VLCFullScreenWnd::RegisterWndClassName(HINSTANCE hInstance)
{
    //save hInstance for future use
    _hinstance = hInstance;

    WNDCLASS wClass;

    memset(&wClass, 0 , sizeof(wClass));
    if( ! GetClassInfo(_hinstance,  getClassName(), &wClass) )
    {
        wClass.style          = CS_NOCLOSE|CS_DBLCLKS;
        wClass.lpfnWndProc    = FSWndWindowProc;
        wClass.cbClsExtra     = 0;
        wClass.cbWndExtra     = 0;
        wClass.hInstance      = _hinstance;
        wClass.hIcon          = NULL;
        wClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wClass.hbrBackground  = (HBRUSH)(COLOR_3DFACE+1);
        wClass.lpszMenuName   = NULL;
        wClass.lpszClassName  = getClassName();

        _fullscreen_wndclass_atom = RegisterClass(&wClass);
    }
    else
    {
        _fullscreen_wndclass_atom = 0;
    }

    memset(&wClass, 0 , sizeof(wClass));
    if( ! GetClassInfo(_hinstance,  getControlsClassName(), &wClass) )
    {
        wClass.style          = CS_NOCLOSE;
        wClass.lpfnWndProc    = FSControlsWndWindowProc;
        wClass.cbClsExtra     = 0;
        wClass.cbWndExtra     = 0;
        wClass.hInstance      = _hinstance;
        wClass.hIcon          = NULL;
        wClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wClass.hbrBackground  = (HBRUSH)(COLOR_3DFACE+1);
        wClass.lpszMenuName   = NULL;
        wClass.lpszClassName  = getControlsClassName();

        _fullscreen_controls_wndclass_atom = RegisterClass(&wClass);
    }
    else
    {
        _fullscreen_controls_wndclass_atom = 0;
    }
}
void VLCFullScreenWnd::UnRegisterWndClassName()
{
    if(0 != _fullscreen_wndclass_atom){
        UnregisterClass(MAKEINTATOM(_fullscreen_wndclass_atom), _hinstance);
        _fullscreen_wndclass_atom = 0;
    }

    if(0 != _fullscreen_controls_wndclass_atom){
        UnregisterClass(MAKEINTATOM(_fullscreen_controls_wndclass_atom), _hinstance);
        _fullscreen_controls_wndclass_atom = 0;
    }
}

LRESULT CALLBACK VLCFullScreenWnd::FSWndWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VLCFullScreenWnd* fs_data = reinterpret_cast<VLCFullScreenWnd *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch( uMsg )
    {
        case WM_CREATE:{
            CREATESTRUCT* CreateStruct = (CREATESTRUCT*)(lParam);
            VLCWindowsManager* WM = (VLCWindowsManager*)CreateStruct->lpCreateParams;

            fs_data = new VLCFullScreenWnd(hWnd, WM);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(fs_data));

            fs_data->hControlsWnd =
                CreateWindow(fs_data->getControlsClassName(),
                        TEXT("VLC ActiveX Full Screen Controls Window"),
                        WS_VISIBLE|WS_CHILD|WS_CLIPSIBLINGS,
                        0,
                        0,
                        0, 0,
                        hWnd,
                        0,
                        VLCFullScreenWnd::_hinstance,
                        (LPVOID) fs_data);

            break;
        }
        case WM_NCDESTROY:
            delete fs_data;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
            break;
        case WM_SHOWWINDOW:{
            if(FALSE==wParam){ //hiding
                break;
            }

            fs_data->NeedShowControls();

            //simulate lParam for WM_SIZE
            RECT ClientRect;
            GetClientRect(hWnd, &ClientRect);
            lParam = MAKELPARAM(ClientRect.right, ClientRect.bottom);
        }
        case WM_SIZE:{
            if(fs_data->_WindowsManager->IsFullScreen()){
                int new_client_width = LOWORD(lParam);
                int new_client_height = HIWORD(lParam);
                VLCHolderWnd* HolderWnd =  fs_data->_WindowsManager->getHolderWnd();
                SetWindowPos(HolderWnd->getHWND(), HWND_BOTTOM, 0, 0, new_client_width, new_client_height, SWP_NOACTIVATE|SWP_NOOWNERZORDER);
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0L;
};

#define ID_FS_SWITCH_FS 1
#define ID_FS_PLAY_PAUSE 2
#define ID_FS_VIDEO_POS_SCROLL 3
#define ID_FS_MUTE 4
#define ID_FS_VOLUME 5

LRESULT CALLBACK VLCFullScreenWnd::FSControlsWndWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    VLCFullScreenWnd* fs_data = reinterpret_cast<VLCFullScreenWnd *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch( uMsg )
    {
        case WM_CREATE:{
            CREATESTRUCT* CreateStruct = (CREATESTRUCT*)(lParam);
            fs_data = (VLCFullScreenWnd*)CreateStruct->lpCreateParams;
            const VLCViewResources& rc = fs_data->RC();

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(fs_data));

            const int ControlsHeight = 21+2;
            const int ButtonsWidth = ControlsHeight;
            const int ControlsSpace = 5;
            const int ScrollVOffset = (ControlsHeight-GetSystemMetrics(SM_CXHSCROLL))/2;

            int HorizontalOffset = ControlsSpace;
            int ControlWidth = ButtonsWidth;
            fs_data->hFSButton =
                CreateWindow(TEXT("BUTTON"), TEXT("End Full Screen"), WS_CHILD|WS_VISIBLE|BS_BITMAP|BS_FLAT,
                             HorizontalOffset, ControlsSpace, ControlWidth, ControlsHeight, hWnd, (HMENU)ID_FS_SWITCH_FS, 0, 0);
            SendMessage(fs_data->hFSButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hDeFullscreenBitmap);
            HorizontalOffset+=ControlWidth+ControlsSpace;

            ControlWidth = ButtonsWidth;
            fs_data->hPlayPauseButton =
                CreateWindow(TEXT("BUTTON"), TEXT("Play/Pause"), WS_CHILD|WS_VISIBLE|BS_BITMAP|BS_FLAT,
                             HorizontalOffset, ControlsSpace, ControlWidth, ControlsHeight, hWnd, (HMENU)ID_FS_PLAY_PAUSE, 0, 0);
            SendMessage(fs_data->hPlayPauseButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hPauseBitmap);
            HorizontalOffset+=ControlWidth+ControlsSpace;

            ControlWidth = 200;
            int VideoPosControlHeight = 12;
            fs_data->hVideoPosScroll =
                CreateWindow(PROGRESS_CLASS, TEXT("Video Position"), WS_CHILD|WS_DISABLED|WS_VISIBLE|SBS_HORZ|SBS_TOPALIGN|PBS_SMOOTH,
                             HorizontalOffset, ControlsSpace+(ControlsHeight-VideoPosControlHeight)/2, ControlWidth, VideoPosControlHeight, hWnd, (HMENU)ID_FS_VIDEO_POS_SCROLL, 0, 0);
            HMODULE hThModule = LoadLibrary(TEXT("UxTheme.dll"));
            if(hThModule){
                FARPROC proc = GetProcAddress(hThModule, "SetWindowTheme");
                typedef HRESULT (WINAPI* SetWindowThemeProc)(HWND, LPCWSTR, LPCWSTR);
                if(proc){
                    //SetWindowTheme(fs_data->hVideoPosScroll, L"", L"");
                    ((SetWindowThemeProc)proc)(fs_data->hVideoPosScroll, L"", L"");
                }
                FreeLibrary(hThModule);
            }
            HorizontalOffset+=ControlWidth+ControlsSpace;

            ControlWidth = ButtonsWidth;
            fs_data->hMuteButton =
                CreateWindow(TEXT("BUTTON"), TEXT("Mute"), WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX|BS_PUSHLIKE|BS_BITMAP, //BS_FLAT
                             HorizontalOffset, ControlsSpace, ControlWidth, ControlsHeight, hWnd, (HMENU)ID_FS_MUTE, 0, 0);
            SendMessage(fs_data->hMuteButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hVolumeBitmap);
            HorizontalOffset+=ControlWidth+ControlsSpace;

            ControlWidth = 100;
            fs_data->hVolumeSlider =
                CreateWindow(TRACKBAR_CLASS, TEXT("Volume"), WS_CHILD|WS_VISIBLE|TBS_HORZ|TBS_BOTTOM|TBS_AUTOTICKS,
                             HorizontalOffset, ControlsSpace, ControlWidth, 21, hWnd, (HMENU)ID_FS_VOLUME, 0, 0);
            HorizontalOffset+=ControlWidth+ControlsSpace;
            SendMessage(fs_data->hVolumeSlider, TBM_SETRANGE, FALSE, (LPARAM) MAKELONG (0, 100));
            SendMessage(fs_data->hVolumeSlider, TBM_SETTICFREQ, (WPARAM) 10, 0);
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOOWNERZORDER);

            int ControlWndWidth = HorizontalOffset;
            int ControlWndHeight = ControlsSpace+ControlsHeight+ControlsSpace;
            SetWindowPos(hWnd, HWND_TOPMOST, (GetSystemMetrics(SM_CXSCREEN)-ControlWndWidth)/2, 0,
                         ControlWndWidth, ControlWndHeight, SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE);

            //new message blinking timer
            SetTimer(hWnd, 2, 500, NULL);

            fs_data->CreateToolTip();

            break;
        }
        case WM_LBUTTONUP:{
            POINT BtnUpPoint = {LOWORD(lParam), HIWORD(lParam)};
            RECT VideoPosRect;
            GetWindowRect(fs_data->hVideoPosScroll, &VideoPosRect);
            ClientToScreen(hWnd, &BtnUpPoint);
            if(PtInRect(&VideoPosRect, BtnUpPoint)){
                fs_data->SetVideoPos(float(BtnUpPoint.x-VideoPosRect.left)/(VideoPosRect.right-VideoPosRect.left));
            }
            break;
        }
        case WM_TIMER:{
            switch(wParam){
                case 1:{
                    POINT MousePoint;
                    GetCursorPos(&MousePoint);
                    RECT ControlWndRect;
                    GetWindowRect(fs_data->hControlsWnd, &ControlWndRect);
                    if(PtInRect(&ControlWndRect, MousePoint)||GetCapture()==fs_data->hVolumeSlider){
                        //do not allow control window to close while mouse is within
                        fs_data->NeedShowControls();
                    }
                    else{
                        fs_data->NeedHideControls();
                    }
                    break;
                }
                case 2:{
                    const VLCViewResources& rc = fs_data->RC();
                    LRESULT lResult = SendMessage(fs_data->hFSButton, BM_GETIMAGE, (WPARAM)IMAGE_BITMAP, 0);
                    if((HANDLE)lResult == rc.hDeFullscreenBitmap){
                        if(fs_data->_WindowsManager->getNewMessageFlag()){
                            SendMessage(fs_data->hFSButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hNewMessageBitmap);
                            //do not allow control window to close while there are new messages
                            fs_data->NeedShowControls();
                        }
                    }
                    else{
                        SendMessage(fs_data->hFSButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hDeFullscreenBitmap);
                    }

                    break;
                }
            }
            break;
        }
        case WM_SETCURSOR:{
            RECT VideoPosRect;
            GetWindowRect(fs_data->hVideoPosScroll, &VideoPosRect);
            DWORD dwMsgPos = GetMessagePos();
            POINT MsgPosPoint = {LOWORD(dwMsgPos), HIWORD(dwMsgPos)};
            if(PtInRect(&VideoPosRect, MsgPosPoint)){
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
            else{
                return DefWindowProc(hWnd, uMsg, wParam, lParam);
            }
            break;
        }
        case WM_NCDESTROY:
            break;
        case WM_COMMAND:{
            WORD NCode = HIWORD(wParam);
            WORD Control = LOWORD(wParam);
            switch(NCode){
                case BN_CLICKED:{
                    switch(Control){
                        case ID_FS_SWITCH_FS:
                            fs_data->_WindowsManager->ToggleFullScreen();
                            break;
                        case ID_FS_PLAY_PAUSE:{
                            libvlc_media_player_t* p_md = fs_data->getMD();
                            if( p_md ){
                                if(fs_data->IsPlaying()) libvlc_media_player_pause(p_md);
                                else libvlc_media_player_play(p_md);
                            }
                            break;
                        }
                        case ID_FS_MUTE:{
                            libvlc_media_player_t* p_md = fs_data->getMD();
                            if( p_md ){
                                libvlc_audio_set_mute(p_md, IsDlgButtonChecked(hWnd, ID_FS_MUTE));
                                fs_data->SyncVolumeSliderWithVLCVolume();
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
        case WM_HSCROLL:
        case WM_VSCROLL: {
            libvlc_media_player_t* p_md = fs_data->getMD();
            if( p_md ){
                if(fs_data->hVolumeSlider==(HWND)lParam){
                    LRESULT SliderPos = SendMessage(fs_data->hVolumeSlider, (UINT) TBM_GETPOS, 0, 0);
                    fs_data->SetVLCVolumeBySliderPos(SliderPos);
                }
            }
            break;
        }
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0L;
};

VLCFullScreenWnd* VLCFullScreenWnd::CreateFSWindow(VLCWindowsManager* WM)
{
    HWND hWnd = CreateWindow(getClassName(),
                TEXT("VLC ActiveX Full Screen Window"),
                WS_POPUP|WS_CLIPCHILDREN,
                0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                0,
                0,
                VLCFullScreenWnd::_hinstance,
                (LPVOID)WM
                );
    if(hWnd)
        return reinterpret_cast<VLCFullScreenWnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    return 0;
}

/////////////////////////////////////
//VLCFullScreenWnd members
VLCFullScreenWnd::~VLCFullScreenWnd()
{
    if(hToolTipWnd){
        ::DestroyWindow(hToolTipWnd);
        hToolTipWnd = 0;
    }
}

void VLCFullScreenWnd::NeedShowControls()
{
    if(!IsWindowVisible(hControlsWnd)){
        libvlc_media_player_t* p_md = getMD();
        if( p_md ){
            if(hVideoPosScroll){
                SetVideoPosScrollRangeByVideoLen();
                SyncVideoPosScrollPosWithVideoPos();
            }
            if(hVolumeSlider){
                SyncVolumeSliderWithVLCVolume();
            }
            if(hPlayPauseButton){
                HANDLE hBmp = IsPlaying() ? RC().hPauseBitmap : RC().hPlayBitmap;
                SendMessage(hPlayPauseButton, BM_SETIMAGE,
                            (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
            }
        }
        ShowWindow(hControlsWnd, SW_SHOW);
    }
    //hide controls after 2 seconds
    SetTimer(hControlsWnd, 1, 2*1000, NULL);
}

void VLCFullScreenWnd::NeedHideControls()
{
    KillTimer(hControlsWnd, 1);
    ShowWindow(hControlsWnd, SW_HIDE);
}

void VLCFullScreenWnd::SyncVideoPosScrollPosWithVideoPos()
{
    libvlc_media_player_t* p_md = getMD();
    if( p_md ){
        libvlc_time_t pos = libvlc_media_player_get_time(p_md);
        SetVideoPosScrollPosByVideoPos(pos);
    }
}

void VLCFullScreenWnd::SetVideoPosScrollRangeByVideoLen()
{
    libvlc_media_player_t* p_md = getMD();
    if( p_md ){
        libvlc_time_t MaxLen = libvlc_media_player_get_length(p_md);
        VideoPosShiftBits = 0;
        while(MaxLen>0xffff){
            MaxLen >>= 1;
            ++VideoPosShiftBits;
        };
        SendMessage(hVideoPosScroll, (UINT)PBM_SETRANGE, 0, MAKELPARAM(0, MaxLen));
    }
}

void VLCFullScreenWnd::SetVideoPosScrollPosByVideoPos(libvlc_time_t CurScrollPos)
{
    SendMessage(hVideoPosScroll, (UINT)PBM_SETPOS, (WPARAM) (CurScrollPos >> VideoPosShiftBits), 0);
}

void VLCFullScreenWnd::SetVideoPos(float Pos) //0-start, 1-end
{
    libvlc_media_player_t* p_md = getMD();
    if( p_md ){
        libvlc_media_player_set_time(p_md, libvlc_media_player_get_length(p_md)*Pos);
        SyncVideoPosScrollPosWithVideoPos();
    }
}

void VLCFullScreenWnd::SyncVolumeSliderWithVLCVolume()
{
    libvlc_media_player_t* p_md = getMD();
    if( p_md ){
        int vol = libvlc_audio_get_volume(p_md);
        const LRESULT SliderPos = SendMessage(hVolumeSlider, (UINT) TBM_GETPOS, 0, 0);
        if(SliderPos!=vol)
            SendMessage(hVolumeSlider, (UINT) TBM_SETPOS, (WPARAM) TRUE, (LPARAM) vol);

        bool muted = libvlc_audio_get_mute(p_md);
        int MuteButtonState = SendMessage(hMuteButton, (UINT) BM_GETCHECK, 0, 0);
        if((muted&&(BST_UNCHECKED==MuteButtonState))||(!muted&&(BST_CHECKED==MuteButtonState))){
            SendMessage(hMuteButton, BM_SETCHECK, (WPARAM)(muted?BST_CHECKED:BST_UNCHECKED), 0);
        }
        LRESULT lResult = SendMessage(hMuteButton, BM_GETIMAGE, (WPARAM)IMAGE_BITMAP, 0);
        if( (muted && ((HANDLE)lResult == RC().hVolumeBitmap)) ||
            (!muted&&((HANDLE)lResult == RC().hVolumeMutedBitmap)) )
        {
            HANDLE hBmp = muted ? RC().hVolumeMutedBitmap : RC().hVolumeBitmap ;
            SendMessage(hMuteButton, BM_SETIMAGE,
                        (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
        }
    }
}

void VLCFullScreenWnd::SetVLCVolumeBySliderPos(int CurPos)
{
    libvlc_media_player_t* p_md = getMD();
    if( p_md ){
        libvlc_audio_set_volume(p_md, CurPos);
        if(0==CurPos){
            libvlc_audio_set_mute(p_md, IsDlgButtonChecked(getHWND(), ID_FS_MUTE));
        }
        SyncVolumeSliderWithVLCVolume();
    }
}

void VLCFullScreenWnd::CreateToolTip()
{
    hToolTipWnd = CreateWindowEx(WS_EX_TOPMOST,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            this->getHWND(),
            NULL,
            _hinstance,
            NULL);

    SetWindowPos(hToolTipWnd,
            HWND_TOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);


    TOOLINFO ti;
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS|TTF_IDISHWND;
    ti.hwnd = this->getHWND();
    ti.hinst = _hinstance;

    TCHAR HintText[100];
    RECT ActivateTTRect;

    //end fullscreen button tooltip
    GetWindowRect(this->hFSButton, &ActivateTTRect);
    GetWindowText(this->hFSButton, HintText, sizeof(HintText));
    ti.uId = (UINT_PTR)this->hFSButton;
    ti.rect = ActivateTTRect;
    ti.lpszText = HintText;
    SendMessage(hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

    //play/pause button tooltip
    GetWindowRect(this->hPlayPauseButton, &ActivateTTRect);
    GetWindowText(this->hPlayPauseButton, HintText, sizeof(HintText));
    ti.uId = (UINT_PTR)this->hPlayPauseButton;
    ti.rect = ActivateTTRect;
    ti.lpszText = HintText;
    SendMessage(hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

    //mute button tooltip
    GetWindowRect(this->hMuteButton, &ActivateTTRect);
    GetWindowText(this->hMuteButton, HintText, sizeof(HintText));
    ti.uId = (UINT_PTR)this->hMuteButton;
    ti.rect = ActivateTTRect;
    ti.lpszText = HintText;
    SendMessage(hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
}

/////////////////////////////////////
//VLCFullScreenWnd event handlers
void VLCFullScreenWnd::handle_position_changed_event(const libvlc_event_t* event)
{
    SyncVideoPosScrollPosWithVideoPos();
}

void VLCFullScreenWnd::handle_input_state_event(const libvlc_event_t* event)
{
    const VLCViewResources& rc = RC();
    switch( event->type )
    {
        case libvlc_MediaPlayerPlaying:
            SendMessage(hPlayPauseButton, BM_SETIMAGE,
                        (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hPauseBitmap);
            break;
        case libvlc_MediaPlayerPaused:
            SendMessage(hPlayPauseButton, BM_SETIMAGE,
                        (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hPlayBitmap);
            break;
        case libvlc_MediaPlayerStopped:
            SendMessage(hPlayPauseButton, BM_SETIMAGE,
                        (WPARAM)IMAGE_BITMAP, (LPARAM)rc.hPlayBitmap);
            break;
    }
}

//libvlc events arrives from separate thread
void VLCFullScreenWnd::OnLibVlcEvent(const libvlc_event_t* event)
{
    if( !_WindowsManager->IsFullScreen() )
        return;

    switch(event->type){
        case libvlc_MediaPlayerPlaying:
        case libvlc_MediaPlayerPaused:
        case libvlc_MediaPlayerStopped:
            handle_input_state_event(event);
            break;
        case libvlc_MediaPlayerPositionChanged:
            handle_position_changed_event(event);
            break;
    }
}

///////////////////////
//VLCWindowsManager
///////////////////////
VLCWindowsManager::VLCWindowsManager(HMODULE hModule, const VLCViewResources& rc)
    :_hModule(hModule), _hWindowedParentWnd(0), _p_md(0), _HolderWnd(0), _FSWnd(0),
    _b_new_messages_flag(false), Last_WM_MOUSEMOVE_Pos(0), _rc(rc)
{
    VLCHolderWnd::RegisterWndClassName(hModule);
    VLCFullScreenWnd::RegisterWndClassName(hModule);
}

VLCWindowsManager::~VLCWindowsManager()
{
    VLCHolderWnd::UnRegisterWndClassName();
    VLCFullScreenWnd::UnRegisterWndClassName();
}

void VLCWindowsManager::CreateWindows(HWND hWindowedParentWnd)
{
    _hWindowedParentWnd = hWindowedParentWnd;

    if(!_HolderWnd){
        _HolderWnd = VLCHolderWnd::CreateHolderWindow(hWindowedParentWnd, this);
    }
}

void VLCWindowsManager::DestroyWindows()
{
    if(_HolderWnd){
        _HolderWnd->DestroyWindow();
    }
    _HolderWnd = 0;

    if(_FSWnd){
        _FSWnd->DestroyWindow();
    }
    _FSWnd = 0;
}

void VLCWindowsManager::LibVlcAttach(libvlc_media_player_t* p_md)
{
    if(!_HolderWnd)
        return;//VLCWindowsManager::CreateWindows was not called

    if(_p_md && _p_md != p_md){
        LibVlcDetach();
    }

    if(!_p_md){
        _p_md = p_md;
        VlcEvents(true);
    }

    _HolderWnd->LibVlcAttach();
}

void VLCWindowsManager::LibVlcDetach()
{
    if(_HolderWnd)
        _HolderWnd->LibVlcDetach();

    if(_p_md){
        VlcEvents(false);
        _p_md = 0;
    }
}

void VLCWindowsManager::StartFullScreen()
{
    if(!_HolderWnd)
        return;//VLCWindowsManager::CreateWindows was not called

    if(getMD()&&!IsFullScreen()){
        if(!_FSWnd){
            _FSWnd= VLCFullScreenWnd::CreateFSWindow(this);
        }

        RECT FSRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

        HMONITOR hMonitor = MonitorFromWindow(_hWindowedParentWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO MonInfo;
        memset(&MonInfo, 0, sizeof(MonInfo));
        MonInfo.cbSize = sizeof(MonInfo);
        if( GetMonitorInfo(hMonitor, &MonInfo) ) {
            FSRect = MonInfo.rcMonitor;
        }

#ifdef _DEBUG
        //to simplify debugging in fullscreen mode
        UINT FSFlags = SWP_NOZORDER;
#else
        UINT FSFlags = 0;
#endif

        SetParent(_HolderWnd->getHWND(), _FSWnd->getHWND());
        SetWindowPos(_FSWnd->getHWND(), HWND_TOPMOST,
                     FSRect.left, FSRect.top,
                     FSRect.right - FSRect.left, FSRect.bottom - FSRect.top,
                     FSFlags);

        ShowWindow(_FSWnd->getHWND(), SW_SHOW);
    }
}

void VLCWindowsManager::EndFullScreen()
{
    if(!_HolderWnd)
        return;//VLCWindowsManager::CreateWindows was not called

    if(IsFullScreen()){
        SetParent(_HolderWnd->getHWND(), _hWindowedParentWnd);

        RECT WindowedParentRect;
        GetClientRect(_hWindowedParentWnd, &WindowedParentRect);
        MoveWindow(_HolderWnd->getHWND(), 0, 0, WindowedParentRect.right, WindowedParentRect.bottom, FALSE);

        ShowWindow(_FSWnd->getHWND(), SW_HIDE);

        if(_FSWnd){
            _FSWnd->DestroyWindow();
        }
        _FSWnd = 0;
   }
}

void VLCWindowsManager::ToggleFullScreen()
{
    if(IsFullScreen()){
        EndFullScreen();
    }
    else{
        StartFullScreen();
    }
}

bool VLCWindowsManager::IsFullScreen()
{
    return 0!=_FSWnd && 0!=_HolderWnd && GetParent(_HolderWnd->getHWND())==_FSWnd->getHWND();
}

void VLCWindowsManager::OnMouseEvent(UINT uMouseMsg)
{
    switch(uMouseMsg){
        case WM_LBUTTONDBLCLK:
            ToggleFullScreen();
            break;
        case WM_MOUSEMOVE:{
            DWORD MsgPos = GetMessagePos();
            if(Last_WM_MOUSEMOVE_Pos != MsgPos){
                Last_WM_MOUSEMOVE_Pos = MsgPos;
                if(IsFullScreen()) _FSWnd->NeedShowControls();
            }
            break;
        }
    }
}

//libvlc events arrives from separate thread
void VLCWindowsManager::OnLibVlcEvent_proxy(const libvlc_event_t* event, void *param)
{
    VLCWindowsManager* WM = static_cast<VLCWindowsManager*>(param);
    WM->OnLibVlcEvent(event);
}

void VLCWindowsManager::VlcEvents(bool Attach)
{
    libvlc_media_player_t* p_md = getMD();
    if( !p_md )
        return;

    libvlc_event_manager_t* em =
        libvlc_media_player_event_manager(p_md);
    if(!em)
        return;

    for(int e=libvlc_MediaPlayerMediaChanged; e<=libvlc_MediaPlayerVout; ++e){
        switch(e){
        //case libvlc_MediaPlayerMediaChanged:
        //case libvlc_MediaPlayerNothingSpecial:
        //case libvlc_MediaPlayerOpening:
        //case libvlc_MediaPlayerBuffering:
        case libvlc_MediaPlayerPlaying:
        case libvlc_MediaPlayerPaused:
        case libvlc_MediaPlayerStopped:
        //case libvlc_MediaPlayerForward:
        //case libvlc_MediaPlayerBackward:
        //case libvlc_MediaPlayerEndReached:
        //case libvlc_MediaPlayerEncounteredError:
        //case libvlc_MediaPlayerTimeChanged:
        case libvlc_MediaPlayerPositionChanged:
        //case libvlc_MediaPlayerSeekableChanged:
        //case libvlc_MediaPlayerPausableChanged:
        //case libvlc_MediaPlayerTitleChanged:
        //case libvlc_MediaPlayerSnapshotTaken:
        //case libvlc_MediaPlayerLengthChanged:
        //case libvlc_MediaPlayerVout:
            if(Attach)
                libvlc_event_attach(em, e, OnLibVlcEvent_proxy, this);
            else
                libvlc_event_detach(em, e, OnLibVlcEvent_proxy, this);
            break;
        }
    }
}

void VLCWindowsManager::OnLibVlcEvent(const libvlc_event_t* event)
{
    if(_HolderWnd) _HolderWnd->OnLibVlcEvent(event);
    if(_FSWnd) _FSWnd->OnLibVlcEvent(event);
}
