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
    {0x08, L"Backspace"},                 // back
    {0x09, L"Tab"},
    {0x0C, L"Clear"},
    {0x0D, L"Enter"},              // enter
    {0x10, L"Shift"},
    {0x11, L"Control"},
    {0x12, L"Menu"},
    {0x13, L"Pause"},
    {0x14, L"CapsLock"},
    {0x15, L"Kana"},
    {0x17, L"Junja"},
    {0x18, L"Final"},
    {0x19, L"Kanji"},
    {0x1B, L"Esc"},               // escape
    {0x1C, L"Convert"},
    {0x1D, L"Nonconvert"},
    {0x1E, L"Accept"},
    {0x1F, L"ModeChange"},
    {0x20, L"Space"},              // space
    {0x21, L"PageUp"},
    {0x22, L"PageDown"},
    {0x23, L"End"},
    {0x24, L"Home"},
    {0x25, L"\u2190"},              // left
    {0x26, L"\u2191"},              // up
    {0x27, L"\u2192"},              // right
    {0x28, L"\u2193"},              // down
    {0x29, L"Select"},
    {0x2A, L"Print"},
    {0x2B, L"Execute"},
    {0x2C, L"Snapshot"},
    {0x2D, L"Insert"},
    {0x2E, L"Delete"},
    {0x2F, L"Help"},
    {0x5B, L"Win"},
    {0x5C, L"Win"},
    {0x5D, L"Apps"},
    {0x5F, L"Sleep"},
    {0x6A, L"Multiply"},
    {0x6B, L"Add"},
    {0x6C, L"Separator"},
    {0x6D, L"Subtract"},
    {0x6E, L"Decimal"},
    {0x6F, L"Divide"},
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
    {0x90, L"NumLock"},
    {0x91, L"ScrollLock"},
    {0xA0, L"Shift"},
    {0xA1, L"Shift"},
    {0xA2, L"Ctrl"},
    {0xA3, L"Ctrl"},
    {0xA4, L"Alt"},
    {0xA5, L"Alt"},
    {0xA6, L"BrowserBack"},
    {0xA7, L"BrowserForward"},
    {0xA8, L"BrowserRefresh"},
    {0xA9, L"BrowserStop"},
    {0xAA, L"BrowserSearch"},
    {0xAB, L"BrowserFavorites"},
    {0xAC, L"BrowserHome"},
    {0xAD, L"VolumeMute"},
    {0xAE, L"VolumeDown"},
    {0xAF, L"VolumeUp"},
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

LPCWSTR mouseActions[] = {
    L"MouseMove",
    L"LButtonDown",
    L"LButtonUp",
    L"LButtonDBLCLK",
    L"RButtonDown",
    L"RButtonUp",
    L"RButtonDBLCLK",
    L"MButtonDown",
    L"MButtonUp",
    L"MButtonDBLCLK",
    L"MouseWheel",
    L"XButtonDown",
    L"XButtonUp",
    L"XButtonDBLCLK",
    L"MouseHWheel"
};
LPCWSTR mouseClicks[] = {
    L"LClick",
    L"RClick",
    L"MClick"
};
LPCWSTR mouseDblClicks[] = {
    L"LDblClick",
    L"RDblClick",
    L"MDblClick"
};

size_t nMouseActions = sizeof(mouseActions) / sizeof(LPCWSTR);

extern BOOL visibleShift;
extern BOOL visibleModifier;
extern BOOL mouseCapturing;
extern BOOL mouseCapturingMod;
extern BOOL keyAutoRepeat;
extern BOOL mergeMouseActions;
extern BOOL onlyCommandKeys;
extern WCHAR comboChars[3];
extern BOOL positioning;
extern WCHAR deferredLabel[64];
HHOOK kbdhook, moshook;
void showText(LPCWSTR text, int behavior = 0);
void fadeLastLabel(BOOL weither);
void positionOrigin(int action, POINT &pt);

#ifdef _DEBUG
#include <sstream>
void log(const std::stringstream & line);
#endif
LPCWSTR GetSymbolFromVK(UINT vk, UINT sc, BOOL mod, HKL hklLayout) {
    static WCHAR symbol[32];
    BYTE btKeyState[256];
    WORD Symbol = 0;
    WCHAR cc[2];
    if(mod) {
        ZeroMemory(btKeyState, sizeof(btKeyState));
    } else {
        for(int i = 0; i < 256; i++) {
            btKeyState[i] = (BYTE)GetKeyState(i);
        }
    }
    int rr = ToUnicodeEx(vk, sc, btKeyState, cc, 2, 0, hklLayout);
#ifdef _DEBUG
    WCHAR ss[KL_NAMELENGTH];
    GetKeyboardLayoutName(ss);
    std::wstring wide(ss);
    std::string str( wide.begin(), wide.end() );
    std::stringstream line;
    line << vk << ":" << rr << ":" << sc << "\n";
    // log(line);
#endif
    if(rr > 0) {
        if(!visibleShift && mod && GetKeyState(VK_SHIFT) < 0) {
            // prefix "Shift - " only when Ctrl or Alt is hold (mod as TRUE)
            swprintf(symbol, 32, L"Shift %c %s", comboChars[1], cc);
        } else {
            swprintf(symbol, 32, L"%s", cc);
            symbol[rr] = L'\0';
        }
        return symbol;
    }
    return NULL;
}
LPCWSTR getSpecialKey(UINT vk) {
    static WCHAR unknown[32];
    for (size_t i=0; i < nSpecialKeys; ++i) {
        if(specialKeys[i].val == vk) {
            return specialKeys[i].label;
        }
    }
    swprintf(unknown, 32, L"0x%02x", vk);
    return unknown;
}
void addBracket(LPWSTR str) {
    WCHAR tmp[64];
    if(wcslen(comboChars) > 2) {
        swprintf(tmp, 64, L"%s", str);
        swprintf(str, 64, L"%c%s%c", comboChars[0], tmp, comboChars[2]);
    }
}
LPCWSTR getModSpecialKey(UINT vk, BOOL mod = FALSE) {
    static WCHAR modsk[64];
    if( vk == 0xA0 || vk == 0xA1) {
        if(!mod) {
            // show nothing if press SHIFT only
            return NULL;
        } else {
            wcscpy_s(modsk, 64, L"Shift");
        }
    } else {
        WCHAR tmp[64];
        LPCWSTR sk = getSpecialKey(vk);
        if(!visibleShift && GetKeyState(VK_SHIFT) < 0) {
            // prefix "Shift - "
            swprintf(tmp, 64, L"Shift %c %s", comboChars[1], sk);
            sk= tmp;
        }
        if(!mod && HIBYTE(sk[0]) == 0) {
            // if the special key is not used with modifierkey, and has not been replaced with visible symbol
            // then surround it with <>
            swprintf(modsk, 64, L"%s", sk);
            addBracket(modsk);
        } else {
            swprintf(modsk, 64, L"%s", sk);
        }
    }

    return modsk;
}

// remove a modifier vk from modifierkeys
// for example, remove "Alt" from "Ctrl - Alt"
void cleanModifier(UINT vk, LPWSTR modifierkeys) {
    WCHAR tmp[64];
    LPCWSTR ck = getSpecialKey(vk);
    LPWSTR p = wcsstr(modifierkeys, ck);
    if(p == modifierkeys) {
        if(wcslen(modifierkeys) == wcslen(p)) {
            // current key is the only modifier
            modifierkeys[0] = '\0';
        } else {
            // remove current key and the " - " after it
            // sizeof(" - ") == 4
            wcscpy_s(tmp, 64, modifierkeys+wcslen(ck)+4);
            wcscpy_s(modifierkeys, 64, tmp);
        }
    } else if(p) {
        // get rid of all after current key including the delimiter
        *(p-3) = '\0';
    }
}

static WCHAR modifierkey[64] = L"\0";
static BOOL modifierUsed = FALSE;
LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp)
{
    KBDLLHOOKSTRUCT k = *(KBDLLHOOKSTRUCT *)lp;
    WCHAR c[64] = L"\0";
    WCHAR tmp[64] = L"\0";
    const WCHAR * theKey = NULL;

    if(nCode < 0)
        return CallNextHookEx(kbdhook, nCode, wp, lp);

    static DWORD lastvk = 0;
    UINT spk = visibleShift ? 0xA0 : 0xA2;
    GUITHREADINFO Gti;
    ::ZeroMemory ( &Gti,sizeof(GUITHREADINFO));
    Gti.cbSize = sizeof( GUITHREADINFO );
    ::GetGUIThreadInfo(0,&Gti);
    DWORD dwThread = ::GetWindowThreadProcessId(Gti.hwndActive, 0);
    HKL hklLayout = ::GetKeyboardLayout(dwThread);
    UINT isDeadKey = ((MapVirtualKeyEx(k.vkCode, MAPVK_VK_TO_CHAR, hklLayout) & 0x80000000) >> 31);
    if(isDeadKey) {
        // GetKeyboardState(btKeyState);
        // for(int i = 0; i < 256; i++) {
            // btKeyState[i] = (BYTE)GetKeyState(i);
        // }
        // deadKeyPressed = TRUE;
    } else if(wp == WM_KEYUP || wp == WM_SYSKEYUP) {
        lastvk = 0;
        fadeLastLabel(TRUE);
        if(k.vkCode >= spk && k.vkCode <= 0xA5 ||
                k.vkCode == 0x5B || k.vkCode == 0x5C) {
            cleanModifier(k.vkCode, modifierkey);
            modifierUsed = FALSE;
        }
    } else if(wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) {
        if(!keyAutoRepeat && lastvk == k.vkCode) {
            fadeLastLabel(FALSE);
            return TRUE;
        }
        int fin = 0;
        if(k.vkCode >= spk && k.vkCode <= 0xA5 ||          // ctrl / alt
                k.vkCode == 0x5B || k.vkCode == 0x5C) {     // win
            LPCWSTR ck = getSpecialKey(k.vkCode);
            if(modifierkey[0] == '\0') {
                wcscpy_s(modifierkey, 64, ck);
            } else if(!wcsstr(modifierkey, ck)) {
                wcscpy_s(tmp, 64, modifierkey);
                swprintf(modifierkey, 64, L"%s %c %s", tmp, comboChars[1], ck);
            }
            if(!modifierUsed && visibleModifier) {
                swprintf(c, 64, L"%s", modifierkey);
                addBracket(c);
                if(lastvk == k.vkCode) {
                    showText(c, 2);
                } else {
                    showText(c, 1);
                }
            }
        } else {
            WORD a = 0;
            BOOL mod = modifierkey[0] != '\0';
            if(k.vkCode == 0x08 || k.vkCode == 0x09 || k.vkCode == 0x0D || k.vkCode == 0x1B || k.vkCode == 0x20) {
                // for <BS>/<Tab>/<ENTER>/<ESC>/<SPACE>, treat them as specialKeys
                theKey = getModSpecialKey(k.vkCode, mod);
                fin = 1;
            } else if( !(theKey = GetSymbolFromVK(k.vkCode, k.scanCode, mod, hklLayout))) {
                // otherwise try to translate with ToAsciiEx
                // if fails to translate with ToAsciiEx, then treat it as specialKeys
                theKey = getModSpecialKey(k.vkCode, mod);
                fin = 1;
            }

            if(theKey) {
                if(mod) {
                    fin = 1;
                    swprintf(tmp, 64, L"%s %c %s", modifierkey, comboChars[1], theKey);
                    addBracket(tmp);
                    theKey = tmp;
                }
                if(fin || !onlyCommandKeys) {
                    showText(theKey, fin);
                }
            }
        }
        lastvk = k.vkCode;
    }

    return CallNextHookEx(kbdhook, nCode, wp, lp);
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wp, LPARAM lp)
{
    WCHAR c[64] = L"\0";
    WCHAR tmp[64] = L"\0";

    UINT idx = wp - WM_MOUSEFIRST;
    int behavior = 1;
    static DWORD mouseButtonDown = 0;
    static DWORD lastClick = 0;
    BOOL holdButton = FALSE;
    if(positioning) {
        MSLLHOOKSTRUCT* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lp);
        positionOrigin(idx, ms->pt);
    } else if ((mouseCapturing || mouseCapturingMod) && idx > 0 && idx < nMouseActions && nCode == HC_ACTION) {
        MSLLHOOKSTRUCT* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lp);

        if (!(ms->flags & LLMHF_INJECTED)) {
            if(idx == 10) {
                swprintf(c, 64, (int)(ms->mouseData) > 0 ? L"%sUp" : L"%sDown", mouseActions[idx]);
            } else if(mergeMouseActions) {
                switch (idx) {
                    case 1:
                    case 4:
                    case 7:
                        swprintf(c, 64, mouseActions[idx]);
                        mouseButtonDown = GetTickCount();
                        if(lastClick > 0 && (GetTickCount() - lastClick) <= GetDoubleClickTime()) {
                            behavior = 3;
                            // clear deferred label like LButtonDown/RButtonDown/MButtonDown
                            deferredLabel[0] = '\0';
                        } else {
                            // show it some time later instead of right now
                            behavior = 3;
                        }
                        break;
                    case 2:
                    case 5:
                    case 8:
                        if(GetTickCount() - mouseButtonDown > 200) {
                            swprintf(c, 64, mouseActions[idx]);
                            lastClick = 0;
                        } else {
                            behavior = 2;
                            // clear deferred label like LButtonDown/RButtonDown/MButtonDown
                            deferredLabel[0] = '\0';
                            if(lastClick > 0) {
                                if((GetTickCount() - lastClick) <= GetDoubleClickTime()) {
                                    swprintf(c, 64, mouseDblClicks[(idx-2)/3]);
                                } else {
                                    swprintf(c, 64, mouseClicks[(idx-2)/3]);
                                }
                                lastClick = 0;
                            } else {
                                lastClick = GetTickCount();
                                swprintf(c, 64, mouseClicks[(idx-2)/3]);
                            }
                        }
                        mouseButtonDown = 0;
                        break;
                    default:
                        swprintf(c, 64, mouseActions[idx]);
                        mouseButtonDown = 0;
                        break;
                }
            } else {
                swprintf(c, 64, mouseActions[idx]);
                if (idx == 1 || idx == 4 || idx == 7) {
                    holdButton = TRUE;
                }
            }

            if(modifierkey[0] != '\0') {
                modifierUsed = TRUE;
                swprintf(tmp, 64, L"%s %c %s", modifierkey, comboChars[1], c);
                addBracket(tmp);
                showText(tmp, behavior);
            } else if(GetKeyState(VK_SHIFT) < 0) {
                swprintf(tmp, 64, L"Shift %c %s", comboChars[1], c);
                addBracket(tmp);
                showText(tmp, behavior);
            } else if(!mouseCapturingMod) {
                swprintf(tmp, 64, L"%s", c);
                addBracket(tmp);
                showText(tmp, behavior);
            }

            fadeLastLabel(!holdButton);
        }
    }
    return CallNextHookEx(moshook, nCode, wp, lp);
}
