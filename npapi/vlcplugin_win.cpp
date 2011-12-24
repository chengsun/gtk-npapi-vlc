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


static LRESULT CALLBACK Manage( HWND p_hwnd, UINT i_msg, WPARAM wpar, LPARAM lpar )
{
    VlcPluginWin *p_plugin = reinterpret_cast<VlcPluginWin *>(GetWindowLongPtr(p_hwnd, GWLP_USERDATA));

    switch( i_msg )
    {
        case WM_ERASEBKGND:
            return 1L;

        case WM_PAINT:
        {
            PAINTSTRUCT paintstruct;
            HDC hdc;
            RECT rect;

            hdc = BeginPaint( p_hwnd, &paintstruct );

            GetClientRect( p_hwnd, &rect );

            FillRect( hdc, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH) );
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(0, 0, 0));
            if( p_plugin->psz_text )
                DrawText( hdc, p_plugin->psz_text, strlen(p_plugin->psz_text), &rect,
                          DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            EndPaint( p_hwnd, &paintstruct );
            return 0L;
        }
        case WM_SIZE:{
            int new_client_width = LOWORD(lpar);
            int new_client_height = HIWORD(lpar);
            //first child will be resized to client area
            HWND hChildWnd = GetWindow(p_hwnd, GW_CHILD);
            if(hChildWnd){
                MoveWindow(hChildWnd, 0, 0, new_client_width, new_client_height, FALSE);
            }
            return 0L;
        }
        case WM_LBUTTONDBLCLK:{
            p_plugin->toggle_fullscreen();
            return 0L;
        }
        default:
            /* delegate to default handler */
            return CallWindowProc( p_plugin->getWindowProc(), p_hwnd,
                                   i_msg, wpar, lpar );
    }
}

VlcPluginWin::VlcPluginWin(NPP instance, NPuint16_t mode) :
    VlcPluginBase(instance, mode),
    pf_wndproc(NULL),
    _WindowsManager(DllGetModule())
{
}

VlcPluginWin::~VlcPluginWin()
{
    destroy_windows();
}

void VlcPluginWin::set_player_window()
{
    _WindowsManager.LibVlcAttach(libvlc_media_player);
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

    /* install our WNDPROC */
    setWindowProc( (WNDPROC)SetWindowLongPtr( drawable,
                                     GWLP_WNDPROC, (LONG_PTR)Manage ) );

    /* change window style to our liking */
    LONG style = GetWindowLong(drawable, GWL_STYLE);
    style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    SetWindowLong(drawable, GWL_STYLE, style);

    _WindowsManager.CreateWindows(drawable);

    return true;
}

bool VlcPluginWin::resize_windows()
{
    /* TODO */
    HWND drawable = (HWND) (getWindow().window);
    /* Redraw window */
    InvalidateRect( drawable, NULL, TRUE );
    UpdateWindow( drawable );
    return true;
}

bool VlcPluginWin::destroy_windows()
{
    _WindowsManager.DestroyWindows();

    HWND oldwin = (HWND)npwindow.window;
    if( oldwin )
    {
        WNDPROC winproc = getWindowProc();
        if( winproc )
        {
            /* reset WNDPROC */
            SetWindowLongPtr( oldwin, GWLP_WNDPROC, (LONG_PTR)winproc );
            setWindowProc(NULL);
        }
        npwindow.window = NULL;
    }

    return true;
}
void VlcPluginWin::on_media_player_release()
{
    _WindowsManager.LibVlcDetach();
}
