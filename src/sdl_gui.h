/*
 * LoadTracker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SDL_GUI_H
#define SDL_GUI_H

#include <SDL3/SDL.h>

constexpr int MAX_COLUMNS = 128;
constexpr int MAX_ROWS    =  40;

enum
{
  MOUSEB_LEFT   = 1,
  MOUSEB_RIGHT  = 2,
  MOUSEB_MIDDLE = 4
};

struct Mouse
{
    unsigned xpos;
    unsigned ypos;
    unsigned buttons;
    float wheel;
};

struct Key
{
    int raw;
    int ascii;
    bool shift;
    bool ctrl;
    bool alt;
};

bool win_openwindow(const char *appname);
void win_closewindow();
void win_seticon(char *iconbuffer, int size);
int win_getspeed(int framerate);
bool win_quit();

bool gfx_init(unsigned xsize, unsigned ysize);
void gfx_uninit();
bool gfx_lock();
void gfx_unlock();
void gfx_flip();

bool gfx_setcolor(int p, int r, int g, int b);
void gfx_setpalette();

bool gfx_loadcursor(const char *name);
void gfx_drawcursor(unsigned x, unsigned y);
void gfx_freecursor();

bool gfx_loadcharset(const char *name, unsigned char *chardata);

Mouse mou_get();

Key key_get();

extern bool gfx_redraw;
extern SDL_Surface *gfx_screen;

// config
extern unsigned xpos;
extern unsigned ypos;
extern unsigned xsize;
extern unsigned ysize;
extern int win_fullscreen;

#endif
