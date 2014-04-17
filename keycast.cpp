// msbuild keycastow.vcxproj
// msbuild keycastow.vcxproj /t:Clean
// cl keycast.cpp keylog.cpp keycastow.res user32.lib shell32.lib gdi32.lib
#include <windows.h>
#include <stdio.h>

#include "resource.h"
#include "timer.h"
CTimer showTimer;
CTimer strokeTimer;

#define MAXCHARSINLINE 64
struct KeyLabel{
    RECT rect;
    char text[MAXCHARSINLINE];
    unsigned int time;
};

int labelFontSize = 46;
int keyStrokeDelay = 500;
int lingerTime = 1;         // 5s by default
int fadeDuration = 1;       // 3s by default
int labelCount = 5;
int labelSpacing = 35;
COLORREF textColor = RGB(0,240, 33);
COLORREF bgColor = RGB(0x7f,0,0x8f);

KeyLabel keyLabels[10];

#include "keycast.h"
#include "keylog.h"

char *szWinName = "KeyCast";
HWND hMainWnd;
HFONT hfFont;
HDC hdcBuffer;
RECT desktopRect;

#define IDI_TRAY       100
#define WM_TRAYMSG     101

#define MENU_CONFIG    32
#define MENU_EXIT      33
void DrawAlphaBlend (HDC hdcwnd, int i)
{
    BLENDFUNCTION bf;      // structure for alpha blending
    HBITMAP hbitmap;       // bitmap handle
    BITMAPINFO bmi;        // bitmap header
    VOID *pvBits;          // pointer to DIB section
    ULONG   ulWindowWidth, ulWindowHeight;      // window width/height
    UINT32   x,y;          // stepping variables
    RECT &rt = keyLabels[i].rect;

    // calculate window width/height
    ulWindowWidth = rt.right - rt.left;
    ulWindowHeight = rt.bottom - rt.top;

    // make sure we have at least some window size
    if ((!ulWindowWidth) || (!ulWindowHeight))
        return;

    TextOut(hdcwnd, 0, keyLabels[i].rect.top, keyLabels[i].text, strlen(keyLabels[i].text));

    // zero the memory for the bitmap info
    ZeroMemory(&bmi, sizeof(BITMAPINFO));

    // setup bitmap info
    // set the bitmap width and height to 60% of the width and height of each of the three horizontal areas. Later on, the blending will occur in the center of each of the three areas.
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = ulWindowWidth;
    bmi.bmiHeader.biHeight = ulWindowHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = ulWindowWidth * ulWindowHeight * 4;

    // create our DIB section and select the bitmap into the dc
    hbitmap = CreateDIBSection(hdcBuffer, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
    SelectObject(hdcBuffer, hbitmap);

    // in top window area, constant alpha = 50%, but no source alpha
    // the color format for each pixel is 0xaarrggbb
    // set all pixels to blue and set source alpha to zero
    for (y = 0; y < ulWindowHeight; y++)
        for (x = 0; x < ulWindowWidth; x++)
            ((UINT32 *)pvBits)[x + y * ulWindowWidth] = 0xffffffff;

    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    int alpha = 255-(int)255*(keyLabels[i].time/fadeDuration/1000.0);
    alpha = (alpha < 0) ? 0: alpha;
    bf.SourceConstantAlpha = alpha;  // half of 0xff = 50% transparency
    bf.AlphaFormat = 0;             // ignore source alpha channel
    //bf.AlphaFormat = AC_SRC_ALPHA;  // use source alpha

    GdiAlphaBlend(hdcwnd, rt.left, rt.top,
                ulWindowWidth, ulWindowHeight,
                hdcBuffer, 0, 0, ulWindowWidth, ulWindowHeight, bf);
    DeleteObject(hbitmap);
}
void updateLabel(int i) {
    RECT box = {};
    DrawText(hdcBuffer, keyLabels[i].text, strlen(keyLabels[i].text), &box, DT_CALCRECT);
    keyLabels[i].rect.right = box.right+18;
}
void drawLabels(HDC hdc) {
    SetTextColor(hdc, textColor);
    SetBkColor (hdc, bgColor);
    //SetBkMode (hdc, TRANSPARENT);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hfFont);
    for(int i = 0; i < labelCount; i ++) {
        DrawAlphaBlend(hdc, i);
    }
}

static void startFade() {
    int i = 0;
    for(i = 0; i < labelCount; i++) {
        if(keyLabels[i].time > fadeDuration*1000) {
            keyLabels[i].time -= 100;
        } else if(keyLabels[i].time > 0) {
            keyLabels[i].time -= 100;
            InvalidateRect(hMainWnd, &keyLabels[i].rect, TRUE);
        }
    }
}

static bool newStroke = true;
static void startNewStroke() {
    newStroke = true;
}

void showText(LPSTR text) {
    HRGN hRgnLabel;
    HRGN hRegion = CreateRectRgn(0,0,0,0);
    if(newStroke) {
        int i;
        for (i = 1; i < labelCount; i++) {
            if(keyLabels[i].time > 0) {
                break;
            }
        }
        for (; i < labelCount; i++) {
            strcpy(keyLabels[i-1].text, keyLabels[i].text);
            keyLabels[i-1].time = keyLabels[i].time;
            keyLabels[i-1].rect.right = keyLabels[i].rect.right;
        }
        strcpy(keyLabels[labelCount-1].text, text);
        keyLabels[labelCount-1].time = (lingerTime+fadeDuration)*1000;
        updateLabel(labelCount-1);

        for(int i = 0; i < labelCount; i ++) {
            hRgnLabel = CreateRoundRectRgn (keyLabels[i].rect.left, keyLabels[i].rect.top, keyLabels[i].rect.right, keyLabels[i].rect.bottom, 15, 15);
            CombineRgn(hRegion, hRegion, hRgnLabel, RGN_OR);
            DeleteObject(hRgnLabel);
        }
        SetWindowRgn(hMainWnd, hRegion, TRUE);
        InvalidateRect(hMainWnd, NULL, TRUE);

        newStroke = false;
        strokeTimer.Start(keyStrokeDelay, false, true);
    } else {
        char tmp[MAXCHARSINLINE];
        strcpy(tmp, keyLabels[labelCount-1].text);
        keyLabels[labelCount-1].time = (lingerTime+fadeDuration)*1000;
        sprintf(keyLabels[labelCount-1].text, "%s%s", tmp, text);
        strokeTimer.Stop();
        strokeTimer.Start(keyStrokeDelay, false, true);

        updateLabel(labelCount-1);
        hRgnLabel = CreateRoundRectRgn (keyLabels[labelCount-1].rect.left, keyLabels[labelCount-1].rect.top, keyLabels[labelCount-1].rect.right, keyLabels[labelCount-1].rect.bottom, 15, 15);
        hRegion = CreateRectRgn(0,0,0,0);
        GetWindowRgn(hMainWnd, hRegion);
        CombineRgn(hRegion, hRegion, hRgnLabel, RGN_OR);
        DeleteObject(hRgnLabel);
        SetWindowRgn(hMainWnd, hRegion, TRUE);
        InvalidateRect(hMainWnd, &keyLabels[labelCount-1].rect, TRUE);
    }
}

BOOL CALLBACK SettingsWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    // Fall through.
                case IDCANCEL:
                    EndDialog(hwndDlg, wParam);
                    return TRUE;
            }
    }
    return FALSE;
}
LRESULT CALLBACK WindowFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static POINT s_last_mouse;
    static HMENU hPopMenu;
    static NOTIFYICONDATA nid;

    PAINTSTRUCT ps;
    HDC hdc;

    switch(message)
    {
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            drawLabels(hdc);
            EndPaint(hWnd, &ps);
            break;
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
                        if (DialogBox(NULL,
                                    MAKEINTRESOURCE(IDD_DIALOG1),
                                    hWnd,
                                    (DLGPROC)SettingsWndProc)==IDOK) {
                            // Complete the command; szItemName contains the
                            // name of the item to delete.
                        } else {
                            // Cancel the command.
                        }
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
    wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);        //Background color
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
            szWinName,            //Title of window - visible
            WS_POPUP,        //Standard window style
            0, 0,            //X and Y position of window
            0, 0,            //Width and height of window
            NULL,                //Handle to parent window
            NULL,                            //No override window
            hThisInst,                    //Handle this instance of program
            NULL                            //No extra arguments
            );
    //SetLayeredWindowAttributes(hMainWnd, 0, (255 * 50) / 100, LWA_ALPHA);
    SetLayeredWindowAttributes (hMainWnd, RGB(255,255,255), 0x1f, LWA_COLORKEY);
    GetWindowRect(GetDesktopWindow(), &desktopRect);

    if( !hMainWnd)    {
        MessageBox(NULL, "Could not create window", "Error", MB_OK);
        return 0;
    }

    hfFont = CreateFont(labelFontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "");
    HDC hdc = GetDC(hMainWnd);
    hdcBuffer = CreateCompatibleDC(hdc);
    ReleaseDC(hMainWnd, hdc);
    HFONT hFontOld = (HFONT)SelectObject(hdcBuffer, hfFont);
    RECT box = {};
    DrawText(hdcBuffer, "A", 1, &box, DT_CALCRECT);
    int maxHeight = (box.bottom+4+labelSpacing)*labelCount;
    int maxWidth = box.right*16+18;
    SetWindowPos(hMainWnd, HWND_TOPMOST, desktopRect.right - maxWidth, desktopRect.bottom - maxHeight, maxWidth, maxHeight, 0);
    UpdateWindow(hMainWnd);
    HRGN hRegion = CreateRectRgn(0,0,0,0);
    SetWindowRgn(hMainWnd, hRegion, TRUE);
    ShowWindow(hMainWnd, SW_SHOW);

    for(int i = 0; i < labelCount; i ++) {
        keyLabels[i].time = 0;
        keyLabels[i].rect.left = 0;
        keyLabels[i].rect.right = 0;
        keyLabels[i].rect.top = (box.bottom+4)*i+labelSpacing*i;
        keyLabels[i].rect.bottom = (box.bottom+4)*(i+1)+labelSpacing*i;
    }
    showTimer.OnTimedEvent = startFade;
    showTimer.Start(100);
    strokeTimer.OnTimedEvent = startNewStroke;

    kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hThisInst, NULL);

    //Establish message loop

    while( GetMessage(&msg, NULL, 0, 0) )    {
        TranslateMessage(&msg);            //Translates some windows messages
        DispatchMessage(&msg);            //Calls window function
    }

    return msg.wParam;
}
