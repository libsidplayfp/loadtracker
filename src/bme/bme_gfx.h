// BME graphics module header file

#ifndef BME_GFX_H
#define BME_GFX_H

#include <SDL3/SDL.h>

#define MAX_COLORS 16

bool gfx_init(unsigned xsize, unsigned ysize);
int gfx_reinit();
void gfx_uninit();
bool gfx_lock();
void gfx_unlock();
void gfx_flip();

void gfx_setpalette();

bool gfx_loadcursor(const char *name);
void gfx_drawcursor(int x, int y);
void gfx_freecursor();

extern bool gfx_initted;
extern bool gfx_redraw;
extern unsigned gfx_windowxsize;
extern unsigned gfx_windowysize;
extern unsigned gfx_virtualxsize;
extern unsigned gfx_virtualysize;
extern unsigned char gfx_palette[];
extern SDL_Surface *gfx_screen;

#endif
