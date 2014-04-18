#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "keylog.h"

struct Key {
    int val;
    const char *label;
};
struct Key specialKeys[] = {
    {0x01, "<LBUTTON>"},
    {0x02, "<RBUTTON>"},
    {0x03, "<CANCEL>"},
    {0x04, "<MBUTTON>"},
    {0x05, "<XBUTTON1>"},
    {0x06, "<XBUTTON2>"},
    {0x08, "<BACK>"},
    {0x09, "<TAB>"},
    {0x0C, "<CLEAR>"},
    {0x0D, "<RETURN>"},
    {0x10, "<SHIFT>"},
    {0x11, "<CONTROL>"},
    {0x12, "<MENU>"},
    {0x13, "<PAUSE>"},
    {0x14, "<CAPITAL>"},
    {0x15, "<KANA>"},
    {0x15, "<HANGUEL>"},
    {0x15, "<HANGUL>"},
    {0x17, "<JUNJA>"},
    {0x18, "<FINAL>"},
    {0x19, "<HANJA>"},
    {0x19, "<KANJI>"},
    {0x1B, "<ESCAPE>"},
    {0x1C, "<CONVERT>"},
    {0x1D, "<NONCONVERT>"},
    {0x1E, "<ACCEPT>"},
    {0x1F, "<MODECHANGE>"},
    {0x20, "<SPACE>"},
    {0x21, "<PRIOR>"},
    {0x22, "<NEXT>"},
    {0x23, "<END>"},
    {0x24, "<HOME>"},
    {0x25, "<LEFT>"},
    {0x26, "<UP>"},
    {0x27, "<RIGHT>"},
    {0x28, "<DOWN>"},
    {0x29, "<SELECT>"},
    {0x2A, "<PRINT>"},
    {0x2B, "<EXECUTE>"},
    {0x2C, "<SNAPSHOT>"},
    {0x2D, "<INSERT>"},
    {0x2E, "<DELETE>"},
    {0x2F, "<HELP>"},
    {0x5B, "<LWIN>"},
    {0x5C, "<RWIN>"},
    {0x5D, "<APPS>"},
    {0x5F, "<SLEEP>"},
    {0x60, "<NUMPAD0>"},
    {0x61, "<NUMPAD1>"},
    {0x62, "<NUMPAD2>"},
    {0x63, "<NUMPAD3>"},
    {0x64, "<NUMPAD4>"},
    {0x65, "<NUMPAD5>"},
    {0x66, "<NUMPAD6>"},
    {0x67, "<NUMPAD7>"},
    {0x68, "<NUMPAD8>"},
    {0x69, "<NUMPAD9>"},
    {0x6A, "<MULTIPLY>"},
    {0x6B, "<ADD>"},
    {0x6C, "<SEPARATOR>"},
    {0x6D, "<SUBTRACT>"},
    {0x6E, "<DECIMAL>"},
    {0x6F, "<DIVIDE>"},
    {0x70, "<F1>"},
    {0x71, "<F2>"},
    {0x72, "<F3>"},
    {0x73, "<F4>"},
    {0x74, "<F5>"},
    {0x75, "<F6>"},
    {0x76, "<F7>"},
    {0x77, "<F8>"},
    {0x78, "<F9>"},
    {0x79, "<F10>"},
    {0x7A, "<F11>"},
    {0x7B, "<F12>"},
    {0x7C, "<F13>"},
    {0x7D, "<F14>"},
    {0x7E, "<F15>"},
    {0x7F, "<F16>"},
    {0x80, "<F17>"},
    {0x81, "<F18>"},
    {0x82, "<F19>"},
    {0x83, "<F20>"},
    {0x84, "<F21>"},
    {0x85, "<F22>"},
    {0x86, "<F23>"},
    {0x87, "<F24>"},
    {0x90, "<NUMLOCK>"},
    {0x91, "<SCROLL>"},
    {0xA0, "<LSHIFT>"},
    {0xA1, "<RSHIFT>"},
    {0xA2, "<LCONTROL>"},
    {0xA3, "<RCONTROL>"},
    {0xA4, "<LMENU>"},
    {0xA5, "<RMENU>"},
    {0xA6, "<BROWSER_BACK>"},
    {0xA7, "<BROWSER_FORWARD>"},
    {0xA8, "<BROWSER_REFRESH>"},
    {0xA9, "<BROWSER_STOP>"},
    {0xAA, "<BROWSER_SEARCH>"},
    {0xAB, "<BROWSER_FAVORITES>"},
    {0xAC, "<BROWSER_HOME>"},
    {0xAD, "<VOLUME_MUTE>"},
    {0xAE, "<VOLUME_DOWN>"},
    {0xAF, "<VOLUME_UP>"},
    {0xB0, "<MEDIA_NEXT_TRACK>"},
    {0xB1, "<MEDIA_PREV_TRACK>"},
    {0xB2, "<MEDIA_STOP>"},
    {0xB3, "<MEDIA_PLAY_PAUSE>"},
    {0xB4, "<LAUNCH_MAIL>"},
    {0xB5, "<LAUNCH_MEDIA_SELECT>"},
    {0xB6, "<LAUNCH_APP1>"},
    {0xB7, "<LAUNCH_APP2>"},
    {0xBA, "<OEM_1>"},
    {0xBB, "<OEM_PLUS>"},
    {0xBC, "<OEM_COMMA>"},
    {0xBD, "<OEM_MINUS>"},
    {0xBE, "<OEM_PERIOD>"},
    {0xBF, "<OEM_2>"},
    {0xC0, "<OEM_3>"},
    {0xDB, "<OEM_4>"},
    {0xDC, "<OEM_5>"},
    {0xDD, "<OEM_6>"},
    {0xDE, "<OEM_7>"},
    {0xDF, "<OEM_8>"},
    {0xE2, "<OEM_102>"},
    {0xE5, "<PROCESSKEY>"},
    {0xE7, "<PACKET>"},
    {0xF6, "<ATTN>"},
    {0xF7, "<CRSEL>"},
    {0xF8, "<EXSEL>"},
    {0xF9, "<EREOF>"},
    {0xFA, "<PLAY>"},
    {0xFB, "<ZOOM>"},
    {0xFC, "<NONAME>"},
    {0xFD, "<PA1>"},
    {0xFE, "<OEM_CLEAR>"}
};
size_t nSpecialKeys = sizeof(specialKeys) / sizeof(Key);

HHOOK kbdhook;
void showText(const char *text, BOOL forceNewStroke = FALSE);

WORD GetSymbolFromVK(UINT vk, UINT sc)
{
    BYTE btKeyState[256];
    WORD Symbol = 0;
    HKL hklLayout = GetKeyboardLayout(0);
    for(int i = 0; i < 256; i++) {
        btKeyState[i] = (BYTE)GetKeyState(i);
    }
    if(ToAsciiEx(vk, sc, btKeyState, &Symbol, 0, hklLayout) == 1) {
        return Symbol;
    }
    return 0;
}

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp)
{
    KBDLLHOOKSTRUCT k = *(KBDLLHOOKSTRUCT *)lp;
    char c[32];

    if(nCode < 0)
        return CallNextHookEx(kbdhook, nCode, wp, lp);

    if(wp == WM_KEYDOWN) {
        if (k.vkCode >= 65 && k.vkCode <= 90 || //between 65 and 90 is a letter key
                k.vkCode >= 48 && k.vkCode <= 57 || //between 48 and 57 is a digit key
                k.vkCode >= 96 && k.vkCode <= 105) //between 48 and 57 is a digit key
        {
            WORD a = GetSymbolFromVK(k.vkCode, k.scanCode);
            if(a > 0) {
                c[0] = (char)a;
                c[1] = '\0';
                showText(c);
            }
        } else {
            for (size_t i=0; i < nSpecialKeys; ++i) {
                if(specialKeys[i].val == k.vkCode) {
                    showText(specialKeys[i].label, TRUE);
                    break;
                }
            }
        }
    }

    return CallNextHookEx(kbdhook, nCode, wp, lp);
}
