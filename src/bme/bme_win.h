// BME windows, input & timing module header file

#ifndef BME_WIN_H
#define BME_WIN_H

#include <SDL3/SDL.h>

int win_openwindow(unsigned xsize, unsigned ysize, const char *appname);
void win_closewindow(void);
int win_getspeed(int framerate);

void mou_init();
void mou_getpos(unsigned *x, unsigned *y);
unsigned mou_getbuttons();

extern bool win_quitted;
extern int win_fullscreen;
extern unsigned char win_keytable[SDL_SCANCODE_COUNT];
extern unsigned char win_keystate[SDL_SCANCODE_COUNT];
extern unsigned char win_asciikey;
extern float win_mouseywheel;
extern SDL_Window *win_window;

#endif
