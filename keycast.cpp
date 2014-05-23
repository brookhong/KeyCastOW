// Copyright © 2014 Brook Hong. All Rights Reserved.
//

// k.vim#cmd msbuild /p:platform=win32 /p:Configuration=Release && .\Release\keycastow.exe
// msbuild keycastow.vcxproj /t:Clean
// rc keycastow.rc && cl -DUNICODE -D_UNICODE keycast.cpp keylog.cpp keycastow.res user32.lib shell32.lib gdi32.lib Comdlg32.lib comctl32.lib

#include <windows.h>
#include <windowsx.h>
#include <Commctrl.h>
#include <stdio.h>

#include <gdiplus.h>
using namespace Gdiplus;

#include "resource.h"
#include "timer.h"
CTimer showTimer;
CTimer previewTimer;

#define MAXCHARS 4096
WCHAR textBuffer[MAXCHARS];
LPCWSTR textBufferEnd = textBuffer + MAXCHARS;

struct KeyLabel {
    RectF rect;
    WCHAR *text;
    DWORD length;
    DWORD time;
    KeyLabel() {
        text = textBuffer;
        length = 0;
    }
};

struct LabelSettings {
    DWORD keyStrokeDelay;
    DWORD lingerTime;
    DWORD fadeDuration;
    LOGFONT labelFont;
    COLORREF bgColor, textColor, borderColor;
    DWORD bgOpacity, textOpacity, borderOpacity;
    int borderSize;
    int cornerSize;
    void reset() {
        keyStrokeDelay = 500;
        lingerTime = 1200;
        fadeDuration = 600;
        bgColor = RGB(75, 75, 75);
        textColor = RGB(255, 255, 255);
        borderColor = RGB(0, 128, 255);
        bgOpacity = textOpacity = borderOpacity = 198;
        borderSize = 8;
        cornerSize = 16;
        memset(&labelFont, 0, sizeof(labelFont));
        labelFont.lfCharSet = DEFAULT_CHARSET;
        labelFont.lfHeight = -37;
        labelFont.lfPitchAndFamily = DEFAULT_PITCH;
        labelFont.lfWeight  = FW_BLACK;
        labelFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
        labelFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        labelFont.lfQuality = ANTIALIASED_QUALITY;
        // incidently clear global variables in debug mode
        wcscpy_s(labelFont.lfFaceName, LF_FACESIZE, TEXT("Arial Black"));
    }
    LabelSettings() {
        reset();
    }
};
LabelSettings labelSettings, previewLabelSettings;
DWORD labelSpacing;
UINT tcModifiers = MOD_ALT;
UINT tcKey = 0x42;      // 0x42 is 'b'
#define BRANDINGMAX 256
WCHAR branding[BRANDINGMAX];
POINT deskOrigin;

#define MAXLABELS 60
KeyLabel keyLabels[MAXLABELS];
DWORD labelCount = 0;
RECT desktopRect;

#include "keycast.h"
#include "keylog.h"

WCHAR *szWinName = L"KeyCastOW";
HWND hMainWnd;
HWND hDlgSettings;
HWND hWndStamp;
HINSTANCE hInstance;
Graphics * g;
Font * fontPlus = NULL;

#define IDI_TRAY       100
#define WM_TRAYMSG     101
#define MENU_CONFIG    32
#define MENU_EXIT      33
#define MENU_RESTORE      34
void stamp(HWND hwnd, LPCWSTR text) {
    RECT rt;
    GetWindowRect(hwnd,&rt);
    HDC hdc = GetDC(hwnd);
    HDC memDC = ::CreateCompatibleDC(hdc);
    HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, desktopRect.right, desktopRect.bottom);
    ::SelectObject(memDC,memBitmap);
    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    g.Clear(Color::Color(0, 0x7f,0,0x8f));

    RectF rc((REAL)labelSettings.borderSize, (REAL)labelSettings.borderSize, 0.0, 0.0);
    SizeF stringSize, layoutSize((REAL)desktopRect.right-2*labelSettings.borderSize, (REAL)desktopRect.bottom-2*labelSettings.borderSize);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    g.MeasureString(text, wcslen(text), fontPlus, layoutSize, &format, &stringSize);
    rc.Width = stringSize.Width;
    rc.Height = stringSize.Height;
    SIZE wndSize = {2*labelSettings.borderSize+(LONG)rc.Width, 2*labelSettings.borderSize+(LONG)rc.Height};
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, wndSize.cx, wndSize.cy, SWP_NOMOVE|SWP_NOACTIVATE);

    SolidBrush bgBrush(Color::Color(0xaf007cfe));
    g.FillRectangle(&bgBrush, rc);
    SolidBrush textBrushPlus(Color(0xaf303030));
    g.DrawString(text, wcslen(text), fontPlus, rc, &format, &textBrushPlus);
    SolidBrush brushPlus(Color::Color(0xaffefefe));
    rc.X += 2;
    rc.Y += 2;
    g.DrawString(text, wcslen(text), fontPlus, rc, &format, &brushPlus);

    POINT ptSrc = {0, 0};
    POINT ptDst = {rt.left, rt.top};
    BLENDFUNCTION blendFunction;
    blendFunction.AlphaFormat = AC_SRC_ALPHA;
    blendFunction.BlendFlags = 0;
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.SourceConstantAlpha = 255;
    ::UpdateLayeredWindow(hwnd,hdc,&ptDst,&wndSize,memDC,&ptSrc,0,&blendFunction,2);
    ::DeleteDC(memDC);
    ::DeleteObject(memBitmap);
    ReleaseDC(hwnd, hdc);
}
void updateLayeredWindow(HWND hwnd) {
    Rect rc(0, 0, desktopRect.right-desktopRect.left, desktopRect.bottom-desktopRect.top);
    POINT ptSrc = {0, 0};
    SIZE wndSize = {rc.Width, rc.Height};
    BLENDFUNCTION blendFunction;
    blendFunction.AlphaFormat = AC_SRC_ALPHA;
    blendFunction.BlendFlags = 0;
    blendFunction.BlendOp = AC_SRC_OVER;
    blendFunction.SourceConstantAlpha = 255;
    HDC hdcBuf = g->GetHDC();
    HDC hdc = GetDC(hwnd);
    ::UpdateLayeredWindow(hwnd,hdc,&ptSrc,&wndSize,hdcBuf,&ptSrc,0,&blendFunction,2);
    ReleaseDC(hwnd, hdc);
    g->ReleaseHDC(hdcBuf);
}
void eraseLabel(int i) {
    RectF &rt = keyLabels[i].rect;
    RectF rc(rt.X-labelSettings.borderSize, rt.Y-labelSettings.borderSize, rt.Width+2*labelSettings.borderSize+1, rt.Height+2*labelSettings.borderSize+1);
    g->SetClip(rc);
    g->Clear(Color::Color(0, 0x7f,0,0x8f));
    g->ResetClip();
}
#define BR(alpha, bgr) (alpha<<24|bgr>>16|(bgr&0x0000ff00)|(bgr&0x000000ff)<<16)
void updateLabel(int i) {
    eraseLabel(i);

    if(keyLabels[i].length > 0) {
        RectF &rc = keyLabels[i].rect;
        REAL r = 1.0f*keyLabels[i].time/labelSettings.fadeDuration;
        r = (r > 1.0f) ? 1.0f : r;
        PointF origin(rc.X, rc.Y);
        g->MeasureString(keyLabels[i].text, keyLabels[i].length, fontPlus, origin, &rc);
        rc.Width = (rc.Width < labelSettings.cornerSize) ? labelSettings.cornerSize : rc.Width;
        rc.Height = (rc.Height < labelSettings.cornerSize) ? labelSettings.cornerSize : rc.Height;
        int bgAlpha = (int)(r*labelSettings.bgOpacity), textAlpha = (int)(r*labelSettings.textOpacity), borderAlpha = (int)(r*labelSettings.borderOpacity);
        GraphicsPath path;
        REAL dx = rc.Width - labelSettings.cornerSize, dy = rc.Height - labelSettings.cornerSize;
        path.AddArc(rc.X, rc.Y, (REAL)labelSettings.cornerSize, (REAL)labelSettings.cornerSize, 170, 90);
        path.AddArc(rc.X + dx, rc.Y, (REAL)labelSettings.cornerSize, (REAL)labelSettings.cornerSize, 270, 90);
        path.AddArc(rc.X + dx, rc.Y + dy, (REAL)labelSettings.cornerSize, (REAL)labelSettings.cornerSize, 0, 90);
        path.AddArc(rc.X, rc.Y + dy, (REAL)labelSettings.cornerSize, (REAL)labelSettings.cornerSize, 90, 90);
        path.AddLine(rc.X, rc.Y + dy, rc.X, rc.Y + labelSettings.cornerSize/2);
        Pen penPlus(Color::Color(BR(borderAlpha, labelSettings.borderColor)), labelSettings.borderSize+0.0f);
        SolidBrush brushPlus(Color::Color(BR(bgAlpha, labelSettings.bgColor)));
        g->DrawPath(&penPlus, &path);
        g->FillPath(&brushPlus, &path);

        SolidBrush textBrushPlus(Color(BR(textAlpha, labelSettings.textColor)));
        g->DrawString( keyLabels[i].text,
                keyLabels[i].length,
                fontPlus,
                origin,
                &textBrushPlus);
    }
}

static int newStrokeCount = 0;
#define SHOWTIMER_INTERVAL 40
static void startFade() {
    if(newStrokeCount > 0) {
        newStrokeCount -= SHOWTIMER_INTERVAL;
    }
    DWORD i = 0;
    BOOL dirty = FALSE;
    for(i = 0; i < labelCount; i++) {
        if(keyLabels[i].time > labelSettings.fadeDuration) {
            keyLabels[i].time -= SHOWTIMER_INTERVAL;
        } else if(keyLabels[i].time > 0) {
            keyLabels[i].time -= SHOWTIMER_INTERVAL;
            updateLabel(i);
            dirty = TRUE;
        } else if(keyLabels[i].length){
            eraseLabel(i);
            keyLabels[i].length = 0;
        }
    }
    if(dirty) {
        updateLayeredWindow(hMainWnd);
    }
}

bool outOfLine(LPCWSTR text) {
    size_t newLen = wcslen(text);
    if(keyLabels[labelCount-1].text+keyLabels[labelCount-1].length+newLen >= textBufferEnd) {
        wcscpy_s(textBuffer, MAXCHARS, keyLabels[labelCount-1].text);
        keyLabels[labelCount-1].text = textBuffer;
    }
    LPWSTR tmp = keyLabels[labelCount-1].text + keyLabels[labelCount-1].length;
    wcscpy_s(tmp, (textBufferEnd-tmp), text);
    RectF box;
    PointF origin(0, 0);
    g->MeasureString(keyLabels[labelCount-1].text, keyLabels[labelCount-1].length, fontPlus, origin, &box);
    return (deskOrigin.x+box.Width+2*labelSettings.cornerSize+labelSettings.borderSize*2 >= desktopRect.right);
}
void showText(LPCWSTR text, BOOL forceNewStroke = FALSE) {
    SetWindowPos(hMainWnd,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
    size_t newLen = wcslen(text);
    if(forceNewStroke || (newStrokeCount <= 0) || outOfLine(text)) {
        DWORD i;
        for (i = 1; i < labelCount; i++) {
            if(keyLabels[i].time > 0) {
                break;
            }
        }
        for (; i < labelCount; i++) {
            eraseLabel(i-1);
            keyLabels[i-1].text = keyLabels[i].text;
            keyLabels[i-1].length = keyLabels[i].length;
            keyLabels[i-1].time = keyLabels[i].time;
            updateLabel(i-1);
            eraseLabel(i);
        }
        keyLabels[labelCount-1].text = keyLabels[labelCount-2].text + keyLabels[labelCount-2].length;
        if(keyLabels[labelCount-1].text+newLen >= textBufferEnd) {
            keyLabels[labelCount-1].text = textBuffer;
        }
        //swprintf(branding, BRANDINGMAX, L"%0x", keyLabels[labelCount-1].text);
        //stamp(hWndStamp, branding);
        wcscpy_s(keyLabels[labelCount-1].text, textBufferEnd-keyLabels[labelCount-1].text, text);
        keyLabels[labelCount-1].length = newLen;

        keyLabels[labelCount-1].time = labelSettings.lingerTime+labelSettings.fadeDuration;
        updateLabel(labelCount-1);

        newStrokeCount = labelSettings.keyStrokeDelay;
    } else {
        LPWSTR tmp = keyLabels[labelCount-1].text + keyLabels[labelCount-1].length;
        if(tmp+newLen >= textBufferEnd) {
            tmp = textBuffer;
            keyLabels[labelCount-1].text = tmp;
            keyLabels[labelCount-1].length = newLen;
        } else {
            keyLabels[labelCount-1].length += newLen;
        }
        wcscpy_s(tmp, (textBufferEnd-tmp), text);
        keyLabels[labelCount-1].time = labelSettings.lingerTime+labelSettings.fadeDuration;
        updateLabel(labelCount-1);

        newStrokeCount = labelSettings.keyStrokeDelay;
    }
    updateLayeredWindow(hMainWnd);
}

BOOL ColorDialog ( HWND hWnd, COLORREF &clr ) {
    DWORD dwCustClrs[16] = {
        RGB(0,0,0),
        RGB(0,0,255),
        RGB(0,255,0),
        RGB(128,255,255),
        RGB(255,0,0),
        RGB(255,0,255),
        RGB(255,255,0),
        RGB(192,192,192),
        RGB(127,127,127),
        RGB(0,0,128),
        RGB(0,128,0),
        RGB(0,255,255),
        RGB(128,0,0),
        RGB(255,0,128),
        RGB(128,128,64),
        RGB(255,255,255)
    };
    CHOOSECOLOR dlgColor;
    dlgColor.lStructSize = sizeof(CHOOSECOLOR);
    dlgColor.hwndOwner = hWnd;
    dlgColor.hInstance = NULL;
    dlgColor.lpTemplateName = NULL;
    dlgColor.rgbResult =  clr;
    dlgColor.lpCustColors =  dwCustClrs;
    dlgColor.Flags = CC_ANYCOLOR|CC_RGBINIT;
    dlgColor.lCustData = 0;
    dlgColor.lpfnHook = NULL;

    if(ChooseColor(&dlgColor)) {
        clr = dlgColor.rgbResult;
    }
    return TRUE;
}
void updateMainWindow() {
    HDC hdc = GetDC(hMainWnd);
    HFONT hlabelFont = CreateFontIndirect(&labelSettings.labelFont);
    HFONT hFontOld = (HFONT)SelectObject(hdc, hlabelFont);
    DeleteObject(hFontOld);

    if(fontPlus) {
        delete fontPlus;
    }
    fontPlus = new Font(hdc, hlabelFont);
    ReleaseDC(hMainWnd, hdc);
    RectF box;
    PointF origin(0, 0);
    g->MeasureString(L"\u263b - KeyCastOW OFF", 16, fontPlus, origin, &box);
    REAL unitH = box.Height+2*labelSettings.borderSize+labelSpacing;
    labelCount = (desktopRect.bottom - desktopRect.top) / (int)unitH;
    if(deskOrigin.x < 0) {
        deskOrigin.x = desktopRect.right-(int)(box.Width+8*labelSettings.borderSize);
    }

    if(labelCount > MAXLABELS)
        labelCount = MAXLABELS;

    g->Clear(Color::Color(0, 0x7f,0,0x8f));
    for(DWORD i = 0; i < labelCount; i ++) {
        keyLabels[i].rect.X = deskOrigin.x+(REAL)labelSettings.borderSize;
        keyLabels[i].rect.Y = deskOrigin.y+unitH*i;
        if(keyLabels[i].time > labelSettings.lingerTime+labelSettings.fadeDuration) {
            keyLabels[i].time = labelSettings.lingerTime+labelSettings.fadeDuration;
        }
        if(keyLabels[i].time > 0) {
            updateLabel(i);
        }
    }

    stamp(hWndStamp, branding);
}
void initSettings() {
    labelSettings.reset();
    previewLabelSettings.reset();
    labelSpacing = 30;
    wcscpy_s(branding, BRANDINGMAX, TEXT("Hi there, press any key to try, double click to configure."));
    deskOrigin.x = -1;
    deskOrigin.y = 0;
    tcModifiers = MOD_ALT;
    tcKey = 0x42;
}
BOOL saveSettings() {
    BOOL res = TRUE;

    HKEY hRootKey, hChildKey;
    if(RegOpenCurrentUser(KEY_WRITE, &hRootKey) != ERROR_SUCCESS)
        return FALSE;

    if(RegCreateKeyEx(hRootKey, L"Software\\KeyCastOW", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hChildKey, NULL) != ERROR_SUCCESS) {
        RegCloseKey(hRootKey);
        return FALSE;
    }

    if(RegSetKeyValue(hChildKey, NULL, L"keyStrokeDelay", REG_DWORD, (LPCVOID)&labelSettings.keyStrokeDelay, sizeof(labelSettings.keyStrokeDelay)) != ERROR_SUCCESS) {
        res = FALSE;
    }

    RegSetKeyValue(hChildKey, NULL, L"lingerTime", REG_DWORD, (LPCVOID)&labelSettings.lingerTime, sizeof(labelSettings.lingerTime));
    RegSetKeyValue(hChildKey, NULL, L"fadeDuration", REG_DWORD, (LPCVOID)&labelSettings.fadeDuration, sizeof(labelSettings.fadeDuration));
    RegSetKeyValue(hChildKey, NULL, L"bgColor", REG_DWORD, (LPCVOID)&labelSettings.bgColor, sizeof(labelSettings.bgColor));
    RegSetKeyValue(hChildKey, NULL, L"textColor", REG_DWORD, (LPCVOID)&labelSettings.textColor, sizeof(labelSettings.textColor));
    RegSetKeyValue(hChildKey, NULL, L"labelFont", REG_BINARY, (LPCVOID)&labelSettings.labelFont, sizeof(labelSettings.labelFont));
    RegSetKeyValue(hChildKey, NULL, L"bgOpacity", REG_DWORD, (LPCVOID)&labelSettings.bgOpacity, sizeof(labelSettings.bgOpacity));
    RegSetKeyValue(hChildKey, NULL, L"textOpacity", REG_DWORD, (LPCVOID)&labelSettings.textOpacity, sizeof(labelSettings.textOpacity));
    RegSetKeyValue(hChildKey, NULL, L"borderOpacity", REG_DWORD, (LPCVOID)&labelSettings.borderOpacity, sizeof(labelSettings.borderOpacity));
    RegSetKeyValue(hChildKey, NULL, L"borderColor", REG_DWORD, (LPCVOID)&labelSettings.borderColor, sizeof(labelSettings.borderColor));
    RegSetKeyValue(hChildKey, NULL, L"borderSize", REG_DWORD, (LPCVOID)&labelSettings.borderSize, sizeof(labelSettings.borderSize));
    RegSetKeyValue(hChildKey, NULL, L"cornerSize", REG_DWORD, (LPCVOID)&labelSettings.cornerSize, sizeof(labelSettings.cornerSize));
    RegSetKeyValue(hChildKey, NULL, L"labelSpacing", REG_DWORD, (LPCVOID)&labelSpacing, sizeof(labelSpacing));
    RegSetKeyValue(hChildKey, NULL, L"offsetX", REG_DWORD, (LPCVOID)&deskOrigin.x, sizeof(deskOrigin.x));
    RegSetKeyValue(hChildKey, NULL, L"offsetY", REG_DWORD, (LPCVOID)&deskOrigin.y, sizeof(deskOrigin.y));
    RegSetKeyValue(hChildKey, NULL, L"tcModifiers", REG_DWORD, (LPCVOID)&tcModifiers, sizeof(tcModifiers));
    RegSetKeyValue(hChildKey, NULL, L"tcKey", REG_DWORD, (LPCVOID)&tcKey, sizeof(tcKey));
    RegSetKeyValue(hChildKey, NULL, L"branding", REG_SZ, (LPCVOID)branding, (wcslen(branding)+1)*sizeof(WCHAR));

    RegCloseKey(hRootKey);
    RegCloseKey(hChildKey);
    return res;
}
BOOL loadSettings() {
    BOOL res = TRUE;
    HKEY hRootKey, hChildKey;
    DWORD disposition; // For checking if key was created or only opened
    initSettings();
    if(RegOpenCurrentUser(KEY_WRITE | KEY_READ, &hRootKey) != ERROR_SUCCESS)
        return FALSE;
    if(RegCreateKeyEx(hRootKey, TEXT("SOFTWARE\\KeyCastOW"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                NULL, &hChildKey, &disposition) != ERROR_SUCCESS) {
        RegCloseKey(hRootKey);
        return FALSE;
    }

    DWORD size = sizeof(DWORD);
    if(disposition == REG_OPENED_EXISTING_KEY) {
        RegGetValue(hChildKey, NULL, L"keyStrokeDelay", RRF_RT_DWORD, NULL, &labelSettings.keyStrokeDelay, &size);
        RegGetValue(hChildKey, NULL, L"lingerTime", RRF_RT_DWORD, NULL, &labelSettings.lingerTime, &size);
        RegGetValue(hChildKey, NULL, L"fadeDuration", RRF_RT_DWORD, NULL, &labelSettings.fadeDuration, &size);
        RegGetValue(hChildKey, NULL, L"bgColor", RRF_RT_DWORD, NULL, &labelSettings.bgColor, &size);
        RegGetValue(hChildKey, NULL, L"textColor", RRF_RT_DWORD, NULL, &labelSettings.textColor, &size);
        RegGetValue(hChildKey, NULL, L"bgOpacity", RRF_RT_DWORD, NULL, &labelSettings.bgOpacity, &size);
        RegGetValue(hChildKey, NULL, L"textOpacity", RRF_RT_DWORD, NULL, &labelSettings.textOpacity, &size);
        RegGetValue(hChildKey, NULL, L"borderOpacity", RRF_RT_DWORD, NULL, &labelSettings.borderOpacity, &size);
        RegGetValue(hChildKey, NULL, L"borderColor", RRF_RT_DWORD, NULL, &labelSettings.borderColor, &size);
        RegGetValue(hChildKey, NULL, L"borderSize", RRF_RT_DWORD, NULL, &labelSettings.borderSize, &size);
        RegGetValue(hChildKey, NULL, L"cornerSize", RRF_RT_DWORD, NULL, &labelSettings.cornerSize, &size);
        RegGetValue(hChildKey, NULL, L"labelSpacing", RRF_RT_DWORD, NULL, &labelSpacing, &size);
        RegGetValue(hChildKey, NULL, L"offsetX", RRF_RT_DWORD, NULL, &deskOrigin.x, &size);
        RegGetValue(hChildKey, NULL, L"offsetY", RRF_RT_DWORD, NULL, &deskOrigin.y, &size);
        RegGetValue(hChildKey, NULL, L"tcModifiers", RRF_RT_DWORD, NULL, &tcModifiers, &size);
        RegGetValue(hChildKey, NULL, L"tcKey", RRF_RT_DWORD, NULL, &tcKey, &size);
        size = sizeof(branding);
        RegGetValue(hChildKey, NULL, L"branding", RRF_RT_REG_SZ, NULL, branding, &size);

        size = sizeof(labelSettings.labelFont);
        RegGetValue(hChildKey, NULL, L"labelFont", RRF_RT_REG_BINARY, NULL, &labelSettings.labelFont, &size);
    } else {
        saveSettings();
    }

    RegCloseKey(hRootKey);
    RegCloseKey(hChildKey);
    previewLabelSettings = labelSettings;
    return res;
}
void renderSettingsData(HWND hwndDlg) {
    WCHAR tmp[256];
    swprintf(tmp, 256, L"%d", previewLabelSettings.keyStrokeDelay);
    SetDlgItemText(hwndDlg, IDC_KEYSTROKEDELAY, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.lingerTime);
    SetDlgItemText(hwndDlg, IDC_LINGERTIME, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.fadeDuration);
    SetDlgItemText(hwndDlg, IDC_FADEDURATION, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.bgOpacity);
    SetDlgItemText(hwndDlg, IDC_BGOPACITY, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.textOpacity);
    SetDlgItemText(hwndDlg, IDC_TEXTOPACITY, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.borderOpacity);
    SetDlgItemText(hwndDlg, IDC_BORDEROPACITY, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.borderSize);
    SetDlgItemText(hwndDlg, IDC_BORDERSIZE, tmp);
    swprintf(tmp, 256, L"%d", previewLabelSettings.cornerSize);
    SetDlgItemText(hwndDlg, IDC_CORNERSIZE, tmp);

    swprintf(tmp, 256, L"%d", labelSpacing);
    SetDlgItemText(hwndDlg, IDC_LABELSPACING, tmp);
    swprintf(tmp, 256, L"%d", deskOrigin.x);
    SetDlgItemText(hwndDlg, IDC_OFFSETX, tmp);
    swprintf(tmp, 256, L"%d", deskOrigin.y);
    SetDlgItemText(hwndDlg, IDC_OFFSETY, tmp);
    SetDlgItemText(hwndDlg, IDC_BRANDING, branding);
    CheckDlgButton(hwndDlg, IDC_MODCTRL, (tcModifiers & MOD_CONTROL) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_MODALT, (tcModifiers & MOD_ALT) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_MODSHIFT, (tcModifiers & MOD_SHIFT) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_MODWIN, (tcModifiers & MOD_WIN) ? BST_CHECKED : BST_UNCHECKED);
    swprintf(tmp, 256, L"%c", MapVirtualKey(tcKey, MAPVK_VK_TO_CHAR));
    SetDlgItemText(hwndDlg, IDC_TCKEY, tmp);
}
void getLabelSettings(HWND hwndDlg, LabelSettings &lblSettings) {
    WCHAR tmp[256];
    GetDlgItemText(hwndDlg, IDC_KEYSTROKEDELAY, tmp, 256);
    lblSettings.keyStrokeDelay = _wtoi(tmp);
    GetDlgItemText(hwndDlg, IDC_LINGERTIME, tmp, 256);
    lblSettings.lingerTime = _wtoi(tmp);
    GetDlgItemText(hwndDlg, IDC_FADEDURATION, tmp, 256);
    lblSettings.fadeDuration = _wtoi(tmp);
    GetDlgItemText(hwndDlg, IDC_BGOPACITY, tmp, 256);
    lblSettings.bgOpacity = _wtoi(tmp);
    lblSettings.bgOpacity = min(lblSettings.bgOpacity, 255);
    GetDlgItemText(hwndDlg, IDC_TEXTOPACITY, tmp, 256);
    lblSettings.textOpacity = _wtoi(tmp);
    lblSettings.textOpacity = min(lblSettings.textOpacity, 255);
    GetDlgItemText(hwndDlg, IDC_BORDEROPACITY, tmp, 256);
    lblSettings.borderOpacity = _wtoi(tmp);
    lblSettings.borderOpacity = min(lblSettings.borderOpacity, 255);
    GetDlgItemText(hwndDlg, IDC_BORDERSIZE, tmp, 256);
    lblSettings.borderSize = _wtoi(tmp);
    GetDlgItemText(hwndDlg, IDC_CORNERSIZE, tmp, 256);
    lblSettings.cornerSize = _wtoi(tmp);
    lblSettings.cornerSize = (lblSettings.cornerSize - lblSettings.borderSize > 0) ? lblSettings.cornerSize : lblSettings.borderSize + 1;
}
DWORD previewTime = 0;
#define PREVIEWTIMER_INTERVAL 5
static void previewLabel() {
    RECT rt = {12, 50, 222, 230};

    getLabelSettings(hDlgSettings, previewLabelSettings);
    DWORD mg = previewLabelSettings.lingerTime+previewLabelSettings.fadeDuration+600;
    double r;
    if(previewTime == 0 || previewTime > mg) {
        previewTime = mg;
    }
    if(previewTime > mg-600) {
        previewTime -= PREVIEWTIMER_INTERVAL;
        r = 0;
    }
    else if(previewTime > previewLabelSettings.fadeDuration) {
        r = 1;
        previewTime -= PREVIEWTIMER_INTERVAL;
    } else if(previewTime > 0) {
        previewTime -= PREVIEWTIMER_INTERVAL;
        r = 1.0*previewTime/previewLabelSettings.fadeDuration;
    }
    HDC hdc = GetDC(hDlgSettings);
    int rtWidth = rt.right-rt.left;
    int rtHeight = rt.bottom-rt.top;
    RectF rc(0, 0, (REAL)rtWidth, (REAL)rtHeight);
    HDC memDC = ::CreateCompatibleDC(hdc);
    HBITMAP memBitmap = ::CreateCompatibleBitmap(hdc, (int)rc.Width, (int)rc.Height);
    ::SelectObject(memDC,memBitmap);
    Graphics g(memDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    //g.Clear(Color::Color(200, 0x7f,0,0x8f));

    WCHAR text[] = L"BH";
    HFONT hFont = CreateFontIndirect(&previewLabelSettings.labelFont);
    SelectObject(memDC, hFont);
    Font font(memDC, hFont);

    PointF origin(rc.X+previewLabelSettings.borderSize, rc.Y+previewLabelSettings.borderSize);
    g.MeasureString(text, 2, &font, origin, &rc);

    rc.X += (rtWidth-(int)rc.Width)/2-previewLabelSettings.borderSize;
    rc.Y += (rtHeight-(int)rc.Height)/2-previewLabelSettings.borderSize;
    origin.X = rc.X;
    origin.Y = rc.Y;

    int bgAlpha = (int)(r*previewLabelSettings.bgOpacity), textAlpha = (int)(r*previewLabelSettings.textOpacity), borderAlpha = (int)(r*previewLabelSettings.borderOpacity);
    GraphicsPath path;
    REAL dx = rc.Width - previewLabelSettings.cornerSize, dy = rc.Height - previewLabelSettings.cornerSize;
    path.AddArc(rc.X, rc.Y, (REAL)previewLabelSettings.cornerSize, (REAL)previewLabelSettings.cornerSize, 170, 90);
    path.AddArc(rc.X + dx, rc.Y, (REAL)previewLabelSettings.cornerSize, (REAL)previewLabelSettings.cornerSize, 270, 90);
    path.AddArc(rc.X + dx, rc.Y + dy, (REAL)previewLabelSettings.cornerSize, (REAL)previewLabelSettings.cornerSize, 0, 90);
    path.AddArc(rc.X, rc.Y + dy, (REAL)previewLabelSettings.cornerSize, (REAL)previewLabelSettings.cornerSize, 90, 90);
    path.AddLine(rc.X, rc.Y + dy, rc.X, rc.Y + previewLabelSettings.cornerSize/2);
    Pen penPlus(Color::Color(BR(borderAlpha, previewLabelSettings.borderColor)), previewLabelSettings.borderSize+0.0f);
    SolidBrush brushPlus(Color::Color(BR(bgAlpha, previewLabelSettings.bgColor)));
    g.DrawPath(&penPlus, &path);
    g.FillPath(&brushPlus, &path);

    SolidBrush textBrushPlus(Color(BR(textAlpha, previewLabelSettings.textColor)));
    g.DrawString(text, wcslen(text), &font, origin, &textBrushPlus);
    BitBlt(hdc, rt.left, rt.top, rtWidth, rtHeight, memDC, 0,0, SRCCOPY);
    DeleteDC(memDC);
    DeleteObject(memBitmap);
    DeleteObject(hFont);
    ReleaseDC(hDlgSettings, hdc);
}

BOOL CALLBACK SettingsWndProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR tmp[256];
    switch (msg)
    {
        case WM_INITDIALOG:
            {
                renderSettingsData(hwndDlg);
                RECT r;
                GetWindowRect(hwndDlg, &r);
                SetWindowPos(hwndDlg, 0, desktopRect.right - r.right + r.left, desktopRect.bottom - r.bottom + r.top, 0, 0, SWP_NOSIZE);
            }
            return TRUE;
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {

                case NM_CLICK:          // Fall through to the next case.
                case NM_RETURN:
                    {
                        PNMLINK pNMLink = (PNMLINK)lParam;
                        LITEM   item    = pNMLink->item;
                        ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
                        break;
                    }
            }

            break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TEXTFONT:
                    {
                        CHOOSEFONT cf ;
                        cf.lStructSize    = sizeof (CHOOSEFONT) ;
                        cf.hwndOwner      = hwndDlg ;
                        cf.hDC            = NULL ;
                        cf.lpLogFont      = &previewLabelSettings.labelFont ;
                        cf.iPointSize     = 0 ;
                        cf.Flags          = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS ;
                        cf.rgbColors      = 0 ;
                        cf.lCustData      = 0 ;
                        cf.lpfnHook       = NULL ;
                        cf.lpTemplateName = NULL ;
                        cf.hInstance      = NULL ;
                        cf.lpszStyle      = NULL ;
                        cf.nFontType      = 0 ;               // Returned from ChooseFont
                        cf.nSizeMin       = 0 ;
                        cf.nSizeMax       = 0 ;

                        if(ChooseFont (&cf)) {
                            updateMainWindow();
                            saveSettings();
                        }
                    }
                    return TRUE;
                case IDC_TEXTCOLOR:
                    if( ColorDialog(hwndDlg, previewLabelSettings.textColor) ) {
                        updateMainWindow();
                        saveSettings();
                    }
                    return TRUE;
                case IDC_BGCOLOR:
                    if( ColorDialog(hwndDlg, previewLabelSettings.bgColor) ) {
                        updateMainWindow();
                        saveSettings();
                    }
                    return TRUE;
                case IDC_BORDERCOLOR:
                    if( ColorDialog(hwndDlg, previewLabelSettings.borderColor) ) {
                        updateMainWindow();
                        saveSettings();
                    }
                    return TRUE;
                case IDOK:
                    labelSettings = previewLabelSettings;
                    GetDlgItemText(hwndDlg, IDC_LABELSPACING, tmp, 256);
                    labelSpacing = _wtoi(tmp);
                    GetDlgItemText(hwndDlg, IDC_OFFSETX, tmp, 256);
                    deskOrigin.x = _wtoi(tmp);
                    GetDlgItemText(hwndDlg, IDC_OFFSETY, tmp, 256);
                    deskOrigin.y = _wtoi(tmp);
                    GetDlgItemText(hwndDlg, IDC_BRANDING, branding, BRANDINGMAX);
                    tcModifiers = 0;
                    if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_MODCTRL)) {
                        tcModifiers |= MOD_CONTROL;
                    }
                    if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_MODALT)) {
                        tcModifiers |= MOD_ALT;
                    }
                    if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_MODSHIFT)) {
                        tcModifiers |= MOD_SHIFT;
                    }
                    if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_MODWIN)) {
                        tcModifiers |= MOD_WIN;
                    }
                    GetDlgItemText(hwndDlg, IDC_TCKEY, tmp, 256);
                    if(tcModifiers != 0 && tmp[0] != '\0') {
                        tcKey = VkKeyScanEx(tmp[0], GetKeyboardLayout(0));
                        UnregisterHotKey(NULL, 1);
                        if (!RegisterHotKey( NULL, 1, tcModifiers | MOD_NOREPEAT, tcKey)) {
                            MessageBox(NULL, L"Unable to register hotkey, you probably need go to settings to redefine your hotkey for toggle capturing.", L"Warning", MB_OK|MB_ICONWARNING);
                        }
                    }
                    updateMainWindow();
                    saveSettings();
                case IDCANCEL:
                    EndDialog(hwndDlg, wParam);
                    previewTimer.Stop();
                    return TRUE;
            }
    }
    return FALSE;
}
LRESULT CALLBACK DraggableWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static POINT s_last_mouse;
    switch(message)
    {
        // hold mouse to move
        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            GetCursorPos(&s_last_mouse);
            showTimer.Stop();
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
                    SetWindowPos(hWnd,HWND_TOPMOST,r.left+dx,r.top+dy,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
                }
            }
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            showTimer.Start(SHOWTIMER_INTERVAL);
            break;
        case WM_LBUTTONDBLCLK:
            SendMessage( hMainWnd, WM_COMMAND, MENU_CONFIG, 0 );
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
LRESULT CALLBACK WindowFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    static POINT s_last_mouse;
    static HMENU hPopMenu;
    static NOTIFYICONDATA nid;

    switch(message) {
        // trayicon
        case WM_CREATE:
            {
                memset( &nid, 0, sizeof( nid ) );

                nid.cbSize              = sizeof( nid );
                nid.hWnd                = hWnd;
                nid.uID                 = IDI_TRAY;
                nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage    = WM_TRAYMSG;
                nid.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON1));
                lstrcpy( nid.szTip, L"KeyCast On Windows by brook hong" );
                Shell_NotifyIcon( NIM_ADD, &nid );

                hPopMenu = CreatePopupMenu();
                AppendMenu( hPopMenu, MF_STRING, MENU_CONFIG,  L"&Settings..." );
                AppendMenu( hPopMenu, MF_STRING, MENU_RESTORE,  L"&Restore default settings" );
                AppendMenu( hPopMenu, MF_STRING, MENU_EXIT,    L"E&xit" );
                SetMenuDefaultItem( hPopMenu, MENU_CONFIG, FALSE );
            }
            break;
        case WM_TRAYMSG:
            {
                switch ( lParam )
                {
                    case WM_RBUTTONUP:
                        {
                            POINT pnt;
                            GetCursorPos( &pnt );
                            SetForegroundWindow( hWnd ); // needed to get keyboard focus
                            TrackPopupMenu( hPopMenu, TPM_LEFTALIGN, pnt.x, pnt.y, 0, hWnd, NULL );
                        }
                        break;
                    case WM_LBUTTONDBLCLK:
                        SendMessage( hWnd, WM_COMMAND, MENU_CONFIG, 0 );
                        return 0;
                }
            }
            break;
        case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                    case MENU_CONFIG:
                        CopyMemory(&previewLabelSettings, &labelSettings, sizeof(previewLabelSettings));
                        renderSettingsData(hDlgSettings);
                        ShowWindow(hDlgSettings, SW_SHOW);
                        SetForegroundWindow(hDlgSettings);
                        previewTimer.Start(PREVIEWTIMER_INTERVAL);
                        break;
                    case MENU_RESTORE:
                        initSettings();
                        saveSettings();
                        updateMainWindow();
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
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
ATOM MyRegisterClassEx(HINSTANCE hInst, LPCWSTR className, WNDPROC wndProc) {
    WNDCLASSEX wcl;
    wcl.cbSize = sizeof(WNDCLASSEX);
    wcl.hInstance = hInst;
    wcl.lpszClassName = className;
    wcl.lpfnWndProc = wndProc;
    wcl.style = CS_DBLCLKS;
    wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcl.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcl.lpszMenuName = NULL;
    wcl.cbWndExtra = 0;
    wcl.cbClsExtra = 0;

    return RegisterClassEx(&wcl);
}
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst,
        LPSTR lpszArgs, int nWinMode)
{
    MSG        msg;

    hInstance = hThisInst;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LINK_CLASS|ICC_LISTVIEW_CLASSES|ICC_PAGESCROLLER_CLASS
        |ICC_PROGRESS_CLASS|ICC_STANDARD_CLASSES|ICC_TAB_CLASSES|ICC_TREEVIEW_CLASSES
        |ICC_UPDOWN_CLASS|ICC_USEREX_CLASSES|ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    if(!MyRegisterClassEx(hThisInst, szWinName, WindowFunc)) {
        MessageBox(NULL, L"Could not register window class", L"Error", MB_OK);
        return 0;
    }

    SystemParametersInfo(SPI_GETWORKAREA,NULL,&desktopRect,NULL);
    hMainWnd = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            szWinName,
            szWinName,
            WS_POPUP,
            0, 0,            //X and Y position of window
            desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top,            //Width and height of window
            NULL,
            NULL,
            hThisInst,
            NULL
            );
    if( !hMainWnd)    {
        MessageBox(NULL, L"Could not create window", L"Error", MB_OK);
        return 0;
    }

    loadSettings();
    MyRegisterClassEx(hThisInst, L"STAMP", DraggableWndProc);
    hWndStamp = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_NOACTIVATE,
            L"STAMP", L"STAMP", WS_VISIBLE|WS_POPUP,
            0, 0, 1, 1,
            NULL, NULL, hThisInst, NULL);

    if (!RegisterHotKey( NULL, 1, tcModifiers | MOD_NOREPEAT, tcKey)) {
        MessageBox(NULL, L"Unable to register hotkey, you probably need go to settings to redefine your hotkey for toggle capturing.", L"Warning", MB_OK|MB_ICONWARNING);
    }
    UpdateWindow(hMainWnd);

    HDC hdc = GetDC(hMainWnd);
    HDC hdcBuffer = CreateCompatibleDC(hdc);
    HBITMAP hbitmap = CreateCompatibleBitmap(hdc, desktopRect.right, desktopRect.bottom);
    HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcBuffer, (HGDIOBJ)hbitmap);
    ReleaseDC(hMainWnd, hdc);
    DeleteObject(hBitmapOld);
    g = new Graphics(hdcBuffer);
    g->SetSmoothingMode(SmoothingModeAntiAlias);
    g->SetTextRenderingHint(TextRenderingHintAntiAlias);

    updateMainWindow();
    ShowWindow(hMainWnd, SW_SHOW);
    hDlgSettings = CreateDialog(hThisInst, MAKEINTRESOURCE(IDD_DLGSETTINGS), NULL, (DLGPROC)SettingsWndProc);
    HFONT hlabelFont = CreateFont(20,10,0,0,FW_BLACK,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
                CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY, VARIABLE_PITCH,TEXT("Arial"));
    HWND hlink = GetDlgItem(hDlgSettings, IDC_SYSLINK1);
    SendMessage(hlink, WM_SETFONT, (WPARAM)hlabelFont, TRUE);

    showTimer.OnTimedEvent = startFade;
    showTimer.Start(SHOWTIMER_INTERVAL);
    previewTimer.OnTimedEvent = previewLabel;

    kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hThisInst, NULL);

    while( GetMessage(&msg, NULL, 0, 0) )    {
        if (msg.message == WM_HOTKEY) {
            if(kbdhook) {
                showText(L"\u263b - KeyCastOW OFF", TRUE);
                UnhookWindowsHookEx(kbdhook);
                kbdhook = NULL;
            } else {
                showText(L"\u263b - KeyCastOW ON", TRUE);
                kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hInstance, NULL);
            }
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnhookWindowsHookEx(kbdhook);
    UnregisterHotKey(NULL, 1);
    delete g;
    delete fontPlus;

    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}
