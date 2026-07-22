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

// =============================================================================
// "console" output routines
// =============================================================================

#define CONSOLE_C

#include "console.h"

#include "configfile.h"
#include "loadtrk.h"

#include "bme_main.h"
#include "bme_win.h"
#include "bme_io.h"

#include <new>

#include <cstdio>
#include <cstdlib>
#include <cstring>

bool gfxinitted = false;
unsigned *scrbuffer = nullptr;
unsigned *prevscrbuffer = nullptr;
unsigned char *chardata = nullptr;
int key = 0;
int rawkey = 0;
bool shiftpressed = false;
bool altpressed = false;
int cursorflashdelay = 0;
int mouseb = 0;
int prevmouseb = 0;
unsigned mousex = 0;
unsigned mousey = 0;
unsigned mousepixelx = 0;
unsigned mousepixely = 0;
unsigned oldmousepixelx = 0xffffffff;
unsigned oldmousepixely = 0xffffffff;
int mouseheld = 0;
int region[MAX_ROWS];

constexpr int fontwidth = 8;
constexpr int fontheight = 16;
constexpr int mousesizex = 19;
constexpr int mousesizey = 24;

Positions dpos =
{
    MAX_COLUMNS-21,             // channelsX
    MAX_ROWS-2,                 // channelsY
    80,                         // instrumentsX
    10,                         // instrumentsY
    64,                         // loadboxX
    3,                          // loadboxY
    1,                          // octaveX
    MAX_ROWS-2,                 // octaveY
    54,                         // orderlistX
    2,                          // orderlistY
    14,                         // patternsX
    2,                          // patternsY
    ((MAX_COLUMNS/2) - 30),     // statusBottomX
    MAX_ROWS-2,                 // statusBottomY
    MAX_COLUMNS-1,              // statusTopEndX
    80,                         // statusTopFvX
    0,                          // statusTopX
    0                           // statusTopY
};

void closescreen();
void loadexternalpalette();
void initicon();

static inline void setcharcolor(unsigned *dptr, unsigned char ch, unsigned char color)
{
  *dptr = (ch & 0xff) | (color << 16) | (colors.CBKGND << 20);
}

bool initscreen()
{
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    return false;

  if (!win_openwindow("LoadTracker"))
      return false;
  initicon();

  if (!gfx_init(MAX_COLUMNS * fontwidth, MAX_ROWS * fontheight))
  {
    win_fullscreen = 0;
    if (!gfx_init(MAX_COLUMNS * fontwidth, MAX_ROWS * fontheight))
      return false;
  }

  scrbuffer = new unsigned[MAX_COLUMNS * MAX_ROWS];
  prevscrbuffer = new unsigned[MAX_COLUMNS * MAX_ROWS];

  std::memset(region, 0, sizeof region);

  chardata = new unsigned char[4096];
  if (!gfx_loadcharset("font.png", chardata))
      return false;

  loadexternalpalette();
  gfx_setpalette();

  if (!gfx_loadcursor("cursor.png"))
      return false;

  gfxinitted = true;
  clearscreen();
  std::atexit(closescreen);
  return true;
}

void loadexternalpalette()
{
  FILE *ext_f = std::fopen("custom.pal", "rt");
  if (ext_f)
  {
    char ln[100];
    std::strcpy(ln, "");
    std::fgets(ln, sizeof(ln), ext_f);

    if (std::strncmp("JASC-PAL", ln, 8) == 0)
    {
      int colors;
      std::fgets(ln, sizeof(ln), ext_f);
      std::fgets(ln, sizeof(ln), ext_f);
      if (std::sscanf(ln, "%d", &colors) == 1 && colors == 256)
      {
        int p = 0;
        while (!std::feof(ext_f))
        {
          if (!std::fgets(ln, sizeof(ln), ext_f)) break;
          int r, g, b;
          if (std::sscanf(ln, "%d %d %d", &r, &g, &b) == 3)
          {
            gfx_palette[p++] = r;
            gfx_palette[p++] = g;
            gfx_palette[p++] = b;
          }

          if (p >= MAX_COLORS*3) break;
        }
      }
    }

    std::fclose(ext_f);
  }
}

void initicon()
{
  int handle = io_open("loadtrk.png");
  if (handle != -1)
  {
    int size = io_lseek(handle, 0, SEEK_END);
    io_lseek(handle, 0, SEEK_SET);
    char *iconbuffer = new (std::nothrow) char[size];
    if (iconbuffer)
    {
      io_read(handle, iconbuffer, size);
      io_close(handle);
      SDL_IOStream *rw = SDL_IOFromMem(iconbuffer, size);
      SDL_Surface *icon = SDL_LoadPNG_IO(rw, true);
      SDL_SetWindowIcon(win_window, icon);
      SDL_DestroySurface(icon);
      delete [] iconbuffer;
    }
  }
}
void closescreen()
{
  if (scrbuffer)
  {
    delete [] scrbuffer;
    scrbuffer = nullptr;
  }
  if (prevscrbuffer)
  {
    delete [] prevscrbuffer;
    prevscrbuffer = nullptr;
  }
  if (chardata)
  {
    delete [] chardata;
    chardata = nullptr;
  }

  gfx_freecursor();

  gfx_uninit();
  gfxinitted = false;
  win_closewindow();
}

void clearscreen()
{
  if (!gfxinitted) return;

  unsigned *dptr = scrbuffer;

  for (int c = 0; c < MAX_ROWS * MAX_COLUMNS; c++)
  {
    setcharcolor(dptr, 0x20, 0x0);
    dptr++;
  }
}

void printtext(int x, int y, int color, const char *text)
{
  if (!gfxinitted) return;
  if ((y < 0) || (y >= MAX_ROWS)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);

  while (*text)
  {
    setcharcolor(dptr, *text, color);
    dptr++;
    text++;
  }
}

void printtextc(int y, int color, const char *text)
{
  printtextcp(MAX_COLUMNS/2, y, color, text);
}

void printtextcp(int cp, int y, int color, const char *text)
{
  int x = cp - (std::strlen(text) / 2);

  printtext(x, y, color, text);
}


void printblank(int x, int y, int length)
{
  printblankc(x, y, 0, length);
}

void printblankc(int x, int y, int color, int length)
{
  if (!gfxinitted) return;
  if ((y < 0) || (y >= MAX_ROWS)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);

  while (length--)
  {
    setcharcolor(dptr, 0x20, color);
    dptr++;
  }
}

void drawbox(int x, int y, int color, int sx, int sy)
{
  if (!gfxinitted) return;
  if ((y < 0) || (y >= MAX_ROWS)) return;
  if (y+sy > MAX_ROWS) return;
  if ((!sx) || (!sy)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);
  unsigned *dptr2 = scrbuffer + ((x+sx-1) + y * MAX_COLUMNS);
  int counter = sy;

  while (counter--)
  {
    setcharcolor(dptr, '\x95', color);
    setcharcolor(dptr2, '\x95', color);
    dptr += MAX_COLUMNS;
    dptr2 += MAX_COLUMNS;
  }

  dptr = scrbuffer + (x + y * MAX_COLUMNS);
  dptr2 = scrbuffer + (x + (y+sy-1) * MAX_COLUMNS);
  counter = sx;

  while (counter--)
  {
    setcharcolor(dptr, '\x94', color);
    setcharcolor(dptr2, '\x94', color);
    dptr++;
    dptr2++;
  }

  dptr = scrbuffer + (x + y * MAX_COLUMNS);
  setcharcolor(dptr, '\x90', color);

  dptr = scrbuffer + ((x+sx-1) + y * MAX_COLUMNS);
  setcharcolor(dptr, '\x91', color);

  dptr = scrbuffer + (x + (y+sy-1) * MAX_COLUMNS);
  setcharcolor(dptr, '\x92', color);

  dptr = scrbuffer + ((x+sx-1) + (y+sy-1) * MAX_COLUMNS);
  setcharcolor(dptr, '\x93', color);
}

void printbg(int x, int y, int color, int length)
{
  if (!gfxinitted) return;
  if ((y < 0) || (y >= MAX_ROWS)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);

  while (length--)
  {
    *dptr = (*dptr & 0xffff) | (colors.CBKGND << 16) | (color << 20);
    dptr++;
  }
}

void fliptoscreen()
{
  if (!gfxinitted) return;

  // Mark previous mousecursor area changed if mouse moved
  if ((mousepixelx != oldmousepixelx) || (mousepixely != oldmousepixely))
  {
    int sy = oldmousepixely / fontheight;
    int ey = (oldmousepixely + mousesizey - 1) / fontheight;
    int sx = oldmousepixelx / fontwidth;
    int ex = (oldmousepixelx + mousesizex - 1) / fontwidth;

    if (ey >= MAX_ROWS) ey = MAX_ROWS - 1;
    if (ex >= MAX_COLUMNS) ex = MAX_COLUMNS - 1;

    for (int y = sy; y <= ey; y++)
    {
      for (int x = sx; x <= ex; x++)
        prevscrbuffer[y*MAX_COLUMNS+x] = 0xffffffff;
    }
  }

  // If redraw requested, mark whole screen changed
  if (gfx_redraw)
  {
    gfx_redraw = false;
    std::memset(prevscrbuffer, 0xff, MAX_COLUMNS*MAX_ROWS*sizeof(unsigned));
  }

  if (!gfx_lock()) return;

  unsigned *sptr = scrbuffer;
  unsigned *cmpptr = prevscrbuffer;

  bool regionschanged = false;

  // Now redraw text on changed areas
  for (int y = 0; y < MAX_ROWS; y++)
  {
    for (int x = 0; x < MAX_COLUMNS; x++)
    {
      // Check if char changed
      if (*sptr != *cmpptr)
      {
        *cmpptr = *sptr;
        region[y] = 1;
        regionschanged = true;

        {
          unsigned char *chptr = &chardata[(*sptr & 0xffff)*16];
          unsigned char *dptr = (unsigned char*)gfx_screen->pixels + y*fontheight * gfx_screen->pitch + x*fontwidth;
          unsigned char bgcolor = (*sptr) >> 20;
          unsigned char fgcolor = ((*sptr) >> 16) & 0xf;

          for (int c = 0; c < fontheight; c++)
          {
            unsigned char e = *chptr++;
            for (unsigned char m = 0x80; m; m >>=1)
                *dptr++ = (e & m) ? fgcolor : bgcolor;

            dptr += gfx_screen->pitch - fontwidth;
          }
        }
      }
      sptr++;
      cmpptr++;
    }
  }

  gfx_unlock();

  // Redraw mouse if text was redrawn
  if (regionschanged)
  {
    int sy = mousepixely / fontheight;
    int ey = (mousepixely + mousesizey - 1) / fontheight;
    if (ey >= MAX_ROWS) ey = MAX_ROWS - 1;

    gfx_drawcursor(mousepixelx, mousepixely);
    for (int y = sy; y <= ey; y++)
      region[y] = 1;
  }

  // Store current mouse position as old
  oldmousepixelx = mousepixelx;
  oldmousepixely = mousepixely;

  // Redraw changed screen regions
  gfx_flip();
}

void editstring(char *buffer, int maxlength)
{
  int len = std::strlen(buffer);

  if ((key >= 32) && (key < 256))
  {
    if (len < maxlength-1)
    {
      buffer[len] = key;
      buffer[len+1] = 0;
    }
  }

  if ((rawkey == KEY_BACKSPACE) && (len > 0))
  {
    buffer[len-1] = 0;
  }
}

void getkey()
{
  win_asciikey = 0;
  cursorflashdelay += win_getspeed(50); // Updates win_asciikey

  prevmouseb = mouseb;

  mou_getpos(&mousepixelx, &mousepixely);
  mouseb = mou_getbuttons();
  mousex = mousepixelx / fontwidth;
  mousey = mousepixely / fontheight;

  if (mouseb) mouseheld++;
  else mouseheld = 0;

  key = win_asciikey;
  rawkey = 0;
  for (int c = 0; c < SDL_SCANCODE_COUNT; c++)
  {
    if (win_keytable[c])
    {
      if ((c != SDL_SCANCODE_LSHIFT) && (c != SDL_SCANCODE_RSHIFT) &&
          (c != SDL_SCANCODE_LCTRL) && (c != SDL_SCANCODE_RCTRL))
      {
        rawkey = c;
        win_keytable[c] = false;
        break;
      }
    }
  }

  shiftpressed = (win_keystate[SDL_SCANCODE_LSHIFT])||(win_keystate[SDL_SCANCODE_RSHIFT])
                  || (win_keystate[SDL_SCANCODE_LCTRL])||(win_keystate[SDL_SCANCODE_RCTRL]);

  altpressed = (win_keystate[SDL_SCANCODE_LALT])
                || (win_keystate[SDL_SCANCODE_RALT]);

  if (rawkey == SDL_SCANCODE_KP_ENTER)
  {
    key = KEY_ENTER;
    rawkey = SDL_SCANCODE_RETURN;
  }

  switch (rawkey)
  {
    case SDL_SCANCODE_KP_0: key = '0'; break;
    case SDL_SCANCODE_KP_1: key = '1'; break;
    case SDL_SCANCODE_KP_2: key = '2'; break;
    case SDL_SCANCODE_KP_3: key = '3'; break;
    case SDL_SCANCODE_KP_4: key = '4'; break;
    case SDL_SCANCODE_KP_5: key = '5'; break;
    case SDL_SCANCODE_KP_6: key = '6'; break;
    case SDL_SCANCODE_KP_7: key = '7'; break;
    case SDL_SCANCODE_KP_8: key = '8'; break;
    case SDL_SCANCODE_KP_9: key = '9'; break;
  }
}

void initDisplayPositions()
{
    if (numsids == 1)
    {
        dpos.channelsX = MAX_COLUMNS-21;
        dpos.orderlistX = 54;
        dpos.patternsX = 14;
    }
    else if (numsids == 2)
    {
        dpos.channelsX = MAX_COLUMNS-42;
        dpos.orderlistX = 80;
        dpos.patternsX = 1;
    }
}
