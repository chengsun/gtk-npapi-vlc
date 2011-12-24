#include "vlcplugin_win.h"

#include "../common/win32_fullscreen.h"

static HMODULE hDllModule= 0;

HMODULE DllGetModule()
{
    return hDllModule;
};

extern "C"
BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved )
{
    switch( fdwReason )
    {
        case DLL_PROCESS_ATTACH:
            hDllModule = (HMODULE)hModule;
            break;
        default:
            break;
    }
    return TRUE;
};


VlcPluginWin::VlcPluginWin(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode),
    _WindowsManager(DllGetModule())
{
}

VlcPluginWin::~VlcPluginWin()
{
    destroy_windows();
}

void VlcPluginWin::toggle_fullscreen()
{
    _WindowsManager.ToggleFullScreen();
}

void VlcPluginWin::set_fullscreen(int yes)
{
    if(yes){
        _WindowsManager.StartFullScreen();
    }
    else{
        _WindowsManager.EndFullScreen();
    }
}

int  VlcPluginWin::get_fullscreen()
{
    return _WindowsManager.IsFullScreen();
}

void VlcPluginWin::show_toolbar()
{
    /* TODO */
}

void VlcPluginWin::hide_toolbar()
{
    /* TODO */
}

void VlcPluginWin::update_controls()
{
    /* TODO */
}

bool VlcPluginWin::create_windows()
{
    HWND drawable = (HWND) (getWindow().window);

    /* attach our plugin object */
    SetWindowLongPtr(drawable, GWLP_USERDATA,
                     (LONG_PTR)this);

    /* change window style to our liking */
    LONG style = GetWindowLong(drawable, GWL_STYLE);
    style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    SetWindowLong(drawable, GWL_STYLE, style);

    _WindowsManager.CreateWindows(drawable);

    return true;
}

bool VlcPluginWin::resize_windows()
{
    HWND drawable = (HWND) (getWindow().window);
    RECT rect;
    GetClientRect(drawable, &rect);
    if(!_WindowsManager.IsFullScreen() && _WindowsManager.getHolderWnd()){
        HWND hHolderWnd = _WindowsManager.getHolderWnd()->getHWND();
        MoveWindow(hHolderWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
    }
    return true;
}

bool VlcPluginWin::destroy_windows()
{
    _WindowsManager.DestroyWindows();
    return true;
}

void VlcPluginWin::on_media_player_new()
{
    _WindowsManager.LibVlcAttach(libvlc_media_player);
}

void VlcPluginWin::on_media_player_release()
{
    _WindowsManager.LibVlcDetach();
}
