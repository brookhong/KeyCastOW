// cl keycast.cpp keylog.cpp user32.lib gdi32.lib shell32.lib

#include <windows.h>
#include <stdio.h>

#include "timer.h"
CTimer showTimer;

struct KeyLabel{
    char text[32];
    unsigned int time;
    HWND hwnd;
};

#define MAX_LABELS 5
#define TIME_SLICE 1000
KeyLabel keyLabels[MAX_LABELS];

#include "keycast.h"
#include "keylog.h"

char *szWinName = "KeyCast";
HWND hMainWnd;
HFONT hfFont;
COLORREF textColor = RGB(0,240, 33);

#define IDI_TRAY       100
#define WM_TRAYMSG     101

#define MENU_CONFIG    32
#define MENU_EXIT      33

void updateLabels(int lbl) {
    RECT box = {};
    int maxWidth = 0;
    HRGN hRgnLabel, hRegion = CreateRectRgn(0,0,0,0);
    int spacing = 85;
    for(int i = 0; i < lbl; i ++) {
        // get text rect in selected font
        HDC hdc = GetDC(keyLabels[i].hwnd);
        HFONT hFontOld = (HFONT)SelectObject(hdc, hfFont);
        DrawText(hdc, keyLabels[i].text, strlen(keyLabels[i].text), &box, DT_CALCRECT);
        ReleaseDC(NULL, hdc);
        // construct region for the labels
        hRgnLabel = CreateRoundRectRgn (0, (box.bottom+4)*i+spacing*i, box.right+18, (box.bottom+4)*(i+1)+spacing*i, 15, 15);
        CombineRgn(hRegion, hRegion, hRgnLabel, RGN_OR);
        DeleteObject(hRgnLabel);
        // show the text
        SendMessage(keyLabels[i].hwnd, WM_SETTEXT, NULL, (LPARAM)keyLabels[i].text);
        // set the label on its region
        SetWindowPos(keyLabels[i].hwnd, 0, 0, (box.bottom+4)*i+spacing*i, box.right, box.bottom, SWP_NOZORDER);
        SetLayeredWindowAttributes(keyLabels[i].hwnd, 0, (255 * keyLabels[i].time * 10) / 100, LWA_ALPHA);
        if(box.right+18 > maxWidth) {
            maxWidth = box.right+18;
        }
    }
    SetWindowPos(hMainWnd, HWND_TOPMOST, 0, 0, maxWidth, (box.bottom+4+spacing)*lbl, SWP_NOMOVE);
    SetWindowRgn(hMainWnd, hRegion, TRUE);

    InvalidateRect(hMainWnd, NULL, TRUE);
    UpdateWindow(hMainWnd);
    ShowWindow(hMainWnd, SW_SHOW);
}

/*
 * return 0 ~ MAX_LABELS
 */
int bubbleOut() {
    int i, start = 0, end = -1;
    for(i = 0; i < MAX_LABELS; i ++) {
        if(keyLabels[i].time > 0) {
            start = i;
            break;
        }
    }
    for(i = MAX_LABELS - 1; i >= 0; i --) {
        if(keyLabels[i].time > 0) {
            end = i;
            break;
        }
    }
    if(start > 0) {
        for (i = start; i <= end; i++) {
            strcpy(keyLabels[i-start].text, keyLabels[i].text);
            keyLabels[i-start].time = keyLabels[i].time;
            keyLabels[i].time = 0;
        }
    }
    return (end - start + 1);
}

static void startFade() {
    int i = 0;
    for(i = 0; i < MAX_LABELS; i++) {
        if(keyLabels[i].time > 0) {
            keyLabels[i].time--;
            sprintf(keyLabels[i].text, "%s%d", keyLabels[i].text, keyLabels[i].time);
        }
    }

    int lbl = bubbleOut();
    updateLabels(lbl);
}

void showText(LPSTR text) {
    int lbl = bubbleOut();
    if(lbl == MAX_LABELS) {
        int i;
        for (i = 1; i < MAX_LABELS; i++) {
            strcpy(keyLabels[i-1].text, keyLabels[i].text);
            keyLabels[i-1].time = keyLabels[i].time;
            keyLabels[i].time = 0;
        }
        lbl = MAX_LABELS - 1;
    }
    strcpy(keyLabels[lbl].text, text);
    keyLabels[lbl].time = 10;
    lbl++;

    updateLabels(lbl);
}


LRESULT CALLBACK WindowFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
                nid.hWnd                = hWnd;
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
                            SetForegroundWindow( hWnd ); // needed to get keyboard focus
                            TrackPopupMenu( hPopMenu, TPM_LEFTALIGN, pnt.x, pnt.y, 0, hWnd, NULL );
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
            SetCapture(hWnd);
            GetCursorPos(&s_last_mouse);
            break;
        case WM_MOUSEMOVE:
            if (GetCapture()==hWnd)
            {
                POINT p;
                GetCursorPos(&p);
                int dx= p.x - s_last_mouse.x;
                int dy= p.y - s_last_mouse.y;
                if (dx||dy)
                {
                    s_last_mouse=p;
                    RECT r;
                    GetWindowRect(hWnd,&r);
                    SetWindowPos(hWnd,NULL,r.left+dx,r.top+dy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
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
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
        LPSTR lpszArgs, int nWinMode)
{
    WNDCLASSEX wcl;                //Window class structure
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

    hMainWnd = CreateWindowEx(
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
    SetLayeredWindowAttributes(hMainWnd, 0, (255 * 50) / 100, LWA_ALPHA);

    if( !hMainWnd)    {
        MessageBox(NULL, "Could not create window", "Error", MB_OK);
        return 0;
    }

    hfFont = CreateFont(46, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "");
    for(int i = 0; i < MAX_LABELS; i ++) {
        keyLabels[i].hwnd = CreateWindow("STATIC","",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                9, 0, 0, 0,
                hMainWnd, NULL, hThisInst, NULL);
        keyLabels[i].time = 0;

        SendMessage(keyLabels[i].hwnd, WM_SETFONT, (WPARAM)hfFont, NULL);
    }
    showTimer.OnTimedEvent = startFade;
    showTimer.Start(TIME_SLICE);

    kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hThisInst, NULL);

    //Establish message loop

    while( GetMessage(&msg, NULL, 0, 0) )    {
        TranslateMessage(&msg);            //Translates some windows messages
        DispatchMessage(&msg);            //Calls window function
    }

    return msg.wParam;
}
