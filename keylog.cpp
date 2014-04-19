// Copyright © 2014 Brook Hong. All Rights Reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "keylog.h"

struct Key {
    int val;
    LPCWSTR label;
};
struct Key specialKeys[] = {
    {0x08, L"BS"},                 // back
    {0x09, L"TAB"},
    {0x0C, L"CLEAR"},
    {0x0D, L"ENTER"},              // enter
    {0x10, L"SHIFT"},
    {0x11, L"CONTROL"},
    {0x12, L"MENU"},
    {0x13, L"PAUSE"},
    {0x14, L"CAPITAL"},
    {0x15, L"KANA"},
    {0x17, L"JUNJA"},
    {0x18, L"FINAL"},
    {0x19, L"KANJI"},
    {0x1B, L"ESC"},               // escape
    {0x1C, L"CONVERT"},
    {0x1D, L"NONCONVERT"},
    {0x1E, L"ACCEPT"},
    {0x1F, L"MODECHANGE"},
    {0x20, L"\u25ad"},              // space
    {0x21, L"PRIOR"},
    {0x22, L"NEXT"},
    {0x23, L"END"},
    {0x24, L"HOME"},
    {0x25, L"\u2190"},              // left
    {0x26, L"\u2191"},              // up
    {0x27, L"\u2192"},              // right
    {0x28, L"\u2193"},              // down
    {0x29, L"SELECT"},
    {0x2A, L"PRINT"},
    {0x2B, L"EXECUTE"},
    {0x2C, L"SNAPSHOT"},
    {0x2D, L"INSERT"},
    {0x2E, L"DELETE"},
    {0x2F, L"HELP"},
    {0x5B, L"LWIN"},
    {0x5C, L"RWIN"},
    {0x5D, L"APPS"},
    {0x5F, L"SLEEP"},
    {0x6A, L"MULTIPLY"},
    {0x6B, L"ADD"},
    {0x6C, L"SEPARATOR"},
    {0x6D, L"SUBTRACT"},
    {0x6E, L"DECIMAL"},
    {0x6F, L"DIVIDE"},
    {0x70, L"F1"},
    {0x71, L"F2"},
    {0x72, L"F3"},
    {0x73, L"F4"},
    {0x74, L"F5"},
    {0x75, L"F6"},
    {0x76, L"F7"},
    {0x77, L"F8"},
    {0x78, L"F9"},
    {0x79, L"F10"},
    {0x7A, L"F11"},
    {0x7B, L"F12"},
    {0x7C, L"F13"},
    {0x7D, L"F14"},
    {0x7E, L"F15"},
    {0x7F, L"F16"},
    {0x80, L"F17"},
    {0x81, L"F18"},
    {0x82, L"F19"},
    {0x83, L"F20"},
    {0x84, L"F21"},
    {0x85, L"F22"},
    {0x86, L"F23"},
    {0x87, L"F24"},
    {0x90, L"NUMLOCK"},
    {0x91, L"SCROLL"},
    {0xA0, L"LSHIFT"},
    {0xA1, L"RSHIFT"},
    {0xA2, L"Ctrl"},
    {0xA3, L"Ctrl"},
    {0xA4, L"Alt"},
    {0xA5, L"Alt"},
    {0xA6, L"BROWSER_BACK"},
    {0xA7, L"BROWSER_FORWARD"},
    {0xA8, L"BROWSER_REFRESH"},
    {0xA9, L"BROWSER_STOP"},
    {0xAA, L"BROWSER_SEARCH"},
    {0xAB, L"BROWSER_FAVORITES"},
    {0xAC, L"BROWSER_HOME"},
    {0xAD, L"VOLUME_MUTE"},
    {0xAE, L"VOLUME_DOWN"},
    {0xAF, L"VOLUME_UP"},
    {0xB0, L"MEDIA_NEXT_TRACK"},
    {0xB1, L"MEDIA_PREV_TRACK"},
    {0xB2, L"MEDIA_STOP"},
    {0xB3, L"MEDIA_PLAY_PAUSE"},
    {0xB4, L"LAUNCH_MAIL"},
    {0xB5, L"LAUNCH_MEDIA_SELECT"},
    {0xB6, L"LAUNCH_APP1"},
    {0xB7, L"LAUNCH_APP2"},
    {0xBA, L"OEM_1"},
    {0xBB, L"OEM_PLUS"},
    {0xBC, L"OEM_COMMA"},
    {0xBD, L"OEM_MINUS"},
    {0xBE, L"OEM_PERIOD"},
    {0xBF, L"OEM_2"},
    {0xC0, L"OEM_3"},
    {0xDB, L"OEM_4"},
    {0xDC, L"OEM_5"},
    {0xDD, L"OEM_6"},
    {0xDE, L"OEM_7"},
    {0xDF, L"OEM_8"},
    {0xE2, L"OEM_102"},
    {0xE5, L"PROCESSKEY"},
    {0xE7, L"PACKET"},
    {0xF6, L"ATTN"},
    {0xF7, L"CRSEL"},
    {0xF8, L"EXSEL"},
    {0xF9, L"EREOF"},
    {0xFA, L"PLAY"},
    {0xFB, L"ZOOM"},
    {0xFC, L"NONAME"},
    {0xFD, L"PA1"},
    {0xFE, L"OEM_CLEAR"}
};
size_t nSpecialKeys = sizeof(specialKeys) / sizeof(Key);

HHOOK kbdhook;
void showText(LPCWSTR text, BOOL forceNewStroke = FALSE);

WORD GetSymbolFromVK(UINT vk, UINT sc, BOOL mod) {
    BYTE btKeyState[256];
    WORD Symbol = 0;
    HKL hklLayout = GetKeyboardLayout(0);
    if(mod) {
        ZeroMemory(btKeyState, sizeof(btKeyState));
    } else {
        for(int i = 0; i < 256; i++) {
            btKeyState[i] = (BYTE)GetKeyState(i);
        }
    }
    if(ToAsciiEx(vk, sc, btKeyState, &Symbol, 0, hklLayout) == 1) {
        return Symbol;
    }
    return 0;
}
LPCWSTR getSpecialKey(UINT vk) {
    static WCHAR unknown[32];
    for (size_t i=0; i < nSpecialKeys; ++i) {
        if(specialKeys[i].val == vk) {
            return specialKeys[i].label;
        }
    }
    swprintf(unknown, sizeof(unknown), L"0x%02x", vk);
    return unknown;
}

LPCWSTR getModSpecialKey(UINT vk, BOOL mod = FALSE) {
    static WCHAR modsk[64];
    WCHAR tmp[64];
    LPCWSTR sk = getSpecialKey(vk);

    if(GetKeyState(VK_SHIFT) < 0) {
        swprintf(tmp, 64, L"Shift - %s", sk);
        sk= tmp;
    }
    if(!mod) {
        swprintf(modsk, 64, L"<%s>", sk);
    } else {
        swprintf(modsk, 64, L"%s", sk);
    }

    return modsk;
}
LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp)
{
    static WCHAR modifierkey[32] = L"\0";
    KBDLLHOOKSTRUCT k = *(KBDLLHOOKSTRUCT *)lp;
    static WCHAR c[64];
    const WCHAR * theKey = NULL;

    if(nCode < 0)
        return CallNextHookEx(kbdhook, nCode, wp, lp);

    if(wp == WM_KEYUP || wp == WM_SYSKEYUP) {
        if(k.vkCode >= 0xA2 && k.vkCode <= 0xA5) {
            modifierkey[0] = '\0';
            //showText(getSpecialKey(k.vkCode), TRUE);
        }
    } else if(wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) {
        if(k.vkCode >= 0xA2 && k.vkCode <= 0xA5) {
            if(modifierkey[0] == '\0') {
                wcscpy_s(modifierkey, sizeof(modifierkey), getSpecialKey(k.vkCode));
            }
        } else {
            WORD a = 0;
            BOOL fin = FALSE;
            BOOL mod = modifierkey[0] != '\0';
            if(k.vkCode == 0x08 || k.vkCode == 0x09 || k.vkCode == 0x0D || k.vkCode == 0x1B || k.vkCode == 0x20) {
                theKey = getModSpecialKey(k.vkCode, mod);
                fin = TRUE;
            } else if( (a = GetSymbolFromVK(k.vkCode, k.scanCode, mod)) > 0) {
                c[0] = (WCHAR)a;
                c[1] = L'\0';
                theKey = c;
            } else if(k.vkCode != 0xA0 && k.vkCode != 0xA1) {
                theKey = getModSpecialKey(k.vkCode, mod);
                fin = TRUE;
            }

            if(theKey) {
                if(mod) {
                    WCHAR tmp[64];
                    swprintf(tmp, 64, L"<%s - %s>", modifierkey, theKey);
                    showText(tmp, TRUE);
                } else {
                    showText(theKey, fin);
                }
            }
        }
    }

    return CallNextHookEx(kbdhook, nCode, wp, lp);
}