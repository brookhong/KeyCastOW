// cl keycast.cpp keylog.cpp user32.lib gdi32.lib shell32.lib

#include <windows.h>
#include <string.h>
#include <stdio.h>

struct KeyLabel{
    LPSTR text;
    unsigned int time;
    HWND hLabel;
};
#define MAX_LABELS 5
KeyLabel keyLabels[MAX_LABELS];
int getFreeLabel() {
    static int i = 0;
    int ret = (i < MAX_LABELS) ? i : 0;
    i++;
    return ret;
}

#include "keycast.h"
#include "keylog.h"

//extern HHOOK kbdhook;
//extern int capsOn;

char *szWinName = "Basic Messages";
char string[255];
HRGN g_rgnKey;
HWND hwnd;
HFONT hfFont;
COLORREF textColor = RGB(0,240, 33);
HWND hwndLabel;

#define IDI_TRAY       100
#define WM_TRAYMSG     101

#define MAX_MENU_ENTRIES 31
#define MENU_CONFIG      ( MAX_MENU_ENTRIES + 1 )
#define MENU_EXIT        ( MAX_MENU_ENTRIES + 2 )

LRESULT CALLBACK WindowFunc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static POINT s_last_mouse;
    static HMENU hPopMenu;
    static NOTIFYICONDATA nid;

    switch(message)
    {
        case WM_CREATE:
            {
                memset( &nid, 0, sizeof( nid ) );

                nid.cbSize              = sizeof( nid );
                nid.hWnd                = hwnd;
                nid.uID                 = IDI_TRAY;
                nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage    = WM_TRAYMSG;
                TCHAR    szIconFile[512];
                GetSystemDirectory( szIconFile, sizeof( szIconFile ) );
                if ( szIconFile[ lstrlen( szIconFile ) - 1 ] != '\\' )
                    lstrcat( szIconFile, _T("\\") );
                lstrcat( szIconFile, _T("shell32.dll") );
                ExtractIconEx( szIconFile, 18, NULL, &(nid.hIcon), 1 );
                lstrcpy( nid.szTip, "Click on the tray icon\nto activate the menu." );
                Shell_NotifyIcon( NIM_ADD, &nid );

                hPopMenu = CreatePopupMenu();
                AppendMenu( hPopMenu, MF_STRING, MENU_CONFIG,  "More..." );
                AppendMenu( hPopMenu, MF_STRING, MENU_EXIT,    "E&xit" );
            }
            break;
        case WM_TRAYMSG:
            {
                switch ( lParam )
                {
                    case WM_LBUTTONUP:
                    case WM_RBUTTONUP:
                        {
                            POINT pnt;
                            GetCursorPos( &pnt );
                            SetForegroundWindow( hwnd ); // needed to get keyboard focus
                            TrackPopupMenu( hPopMenu, TPM_LEFTALIGN, pnt.x, pnt.y, 0, hwnd, NULL );
                        }
                        break;
                }
            }
            break;
        case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                    case MENU_CONFIG:
                        MessageBox( NULL, "keycast", nid.szTip, MB_SETFOREGROUND );
                        break;

                    case MENU_EXIT:
                        Shell_NotifyIcon( NIM_DELETE, &nid );
                        ExitProcess(0);
                        break;
                    default:
                        break;
                }
            }
            break;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            GetCursorPos(&s_last_mouse);
            break;
        case WM_MOUSEMOVE:
            if (GetCapture()==hwnd)
            {
                POINT p;
                GetCursorPos(&p);
                int dx= p.x - s_last_mouse.x;
                int dy= p.y - s_last_mouse.y;
                if (dx||dy)
                {
                    s_last_mouse=p;
                    RECT r;
                    GetWindowRect(hwnd,&r);
                    SetWindowPos(hwnd,NULL,r.left+dx,r.top+dy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
                }
            }
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_CTLCOLORSTATIC:
            {
                HDC hdcStatic = (HDC) wParam;
                SetTextColor(hdcStatic, textColor);
                SetBkMode (hdcStatic, TRANSPARENT);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}
void showText(LPSTR text) {
    RECT box = {};
    HDC hdc = GetDC(hwndLabel);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hfFont);
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, text, lstrlen(text), &box, DT_CALCRECT);
    ReleaseDC(NULL, hdc);

    SetWindowPos(hwndLabel, 0, 0, 0, box.right, box.bottom, SWP_NOZORDER | SWP_NOMOVE);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, box.right+18, box.bottom+4, SWP_NOMOVE);
    HRGN hRegion = CreateRoundRectRgn (0, 0, box.right+18, box.bottom+4, 15, 15);
    SetWindowRgn(hwnd, hRegion, TRUE);
    SendMessage(hwndLabel, WM_SETTEXT, NULL, (LPARAM) text);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOW);
}
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
        LPSTR lpszArgs, int nWinMode)
{
    WNDCLASSEX wcl;                //Window class structure
    //HWND       hwnd;               //Handle to window
    MSG        msg;                //Message structure

    wcl.cbSize = sizeof(WNDCLASSEX);            //Size of window class - helps identify it
    wcl.hInstance = hThisInst;                    //Handle this instance of program
    wcl.lpszClassName = szWinName;            //Name of window class
    wcl.lpfnWndProc = WindowFunc;                //Address of window function
    wcl.style = CS_DBLCLKS;                        //Allow double clicks
    wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);            //Normal icon
    wcl.hIconSm = LoadIcon(NULL, IDI_WINLOGO);            //Windows logo
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);                //Normal cursor
    wcl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);        //Background color
    wcl.lpszMenuName = NULL;                    //No menu
    wcl.cbWndExtra = 0;                //No extra--------
    wcl.cbClsExtra = 0;                //information needed

    if(!RegisterClassEx(&wcl) )    {
        MessageBox(NULL, "Could not register window class", "Error", MB_OK);
        return 0;
    }

    hwnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            szWinName,                    //Name of window class
            "Basic Windows Messages",            //Title of window - visible
            WS_POPUP,        //Standard window style
            0, 0,            //X and Y position of window
            0, 0,            //Width and height of window
            NULL,                //Handle to parent window
            NULL,                            //No override window
            hThisInst,                    //Handle this instance of program
            NULL                            //No extra arguments
            );
    SetLayeredWindowAttributes(hwnd, 0, (255 * 50) / 100, LWA_ALPHA);

    if( !hwnd)    {
        MessageBox(NULL, "Could not create window", "Error", MB_OK);
        return 0;
    }

    hwndLabel = CreateWindow("STATIC","",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            9, 0, 0, 0,
            hwnd, NULL, hThisInst, NULL);
    hfFont = CreateFont(46, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "");
    SendMessage(hwndLabel, WM_SETFONT, (WPARAM)hfFont, NULL);

    HDC hdc = GetDC(hwndLabel);
    SetBkMode(hdc, TRANSPARENT);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hfFont);
    //hfFont = (HFONT)SelectObject(hdc, hFontOld);
    ReleaseDC(NULL, hdc);

    kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hThisInst, NULL);
    ShowWindow(hwnd, nWinMode);            //Show window
    UpdateWindow(hwnd);                        //Sends window first WM_PAINT message

    //Establish message loop

    while( GetMessage(&msg, NULL, 0, 0) )    {
        TranslateMessage(&msg);            //Translates some windows messages
        DispatchMessage(&msg);            //Calls window function
    }

    return msg.wParam;
}
