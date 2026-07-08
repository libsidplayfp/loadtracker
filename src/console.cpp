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

#include "loadtrk.h"
#include "bme_main.h"
#include "bme_win.h"
#include "bme_gfx.h"
#include "bme_io.h"

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
int fontwidth = 8;
int fontheight = 14;
int mousesizex = 11;
int mousesizey = 20;

POSITIONS dpos =
{
    MAX_COLUMNS-21,             // channelsX
    MAX_ROWS-2,                 // channelsY
    80,                         // instrumentsX
    10,                         // instrumentsY
    64,                         // loadboxX
    2,                          // loadboxY
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

void loadexternalpalette();
void initicon();

static inline void setcharcolor(unsigned *dptr, short ch, short color)
{
  *dptr = (ch & 0xff) | (color << 16) | (colors.CBKGND << 20);
}

static inline void setcolor(unsigned *dptr, short color)
{
  *dptr = (*dptr & 0xffff) | (color << 16) | (colors.CBKGND << 20);
}

bool initscreen()
{
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    return false;

  unsigned xsize = MAX_COLUMNS * 8;
  unsigned ysize = MAX_ROWS * 16;

  win_openwindow(xsize, ysize, "LoadTracker", nullptr);
  win_setmousemode(MOUSE_ALWAYS_HIDDEN);
  initicon();

  if (!gfx_init(MAX_COLUMNS * fontwidth, MAX_ROWS * fontheight, 60, 0))
  {
    win_fullscreen = 0;
    if (!gfx_init(MAX_COLUMNS * fontwidth, MAX_ROWS * fontheight, 60, 0))
      return false;
  }

  scrbuffer = new unsigned[MAX_COLUMNS * MAX_ROWS * sizeof(unsigned)];
  prevscrbuffer = new unsigned[MAX_COLUMNS * MAX_ROWS * sizeof(unsigned)];

  std::memset(region, 0, sizeof region);

  chardata = new unsigned char[4096];
  int handle = io_open("chargen.bin");
  if (handle == -1) return false;
  io_read(handle, &chardata[0], 4096);
  io_close(handle);

  gfx_loadpalette("palette.bin");
  loadexternalpalette();
  gfx_setpalette();

  gfx_loadcursor("cursor.bin");

  gfxinitted = true;
  clearscreen();
  std::atexit(closescreen);
  return true;
}

void loadexternalpalette()
{
  FILE *ext_f;
  if ((ext_f = std::fopen("custom.pal", "rt")))
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
        while (!feof(ext_f))
        {
          int r, g, b;
          if (!std::fgets(ln, sizeof(ln), ext_f)) break;
          if (std::sscanf(ln, "%d %d %d", &r, &g, &b) == 3)
          {
            // JASC palette is 8-bit and goat palette is 6-bit
            gfx_palette[p++] = r / 4;
            gfx_palette[p++] = g / 4;
            gfx_palette[p++] = b / 4;
          }

          if (p >= 768) break;
        }
        gfx_calcpalette(64, 0, 0, 0);
      }
    }

    std::fclose(ext_f);
  }
}

void initicon()
{
  int handle = io_open("loadtrk.bmp");
  if (handle != -1)
  {
    int size = io_lseek(handle, 0, SEEK_END);
    io_lseek(handle, 0, SEEK_SET);
    char *iconbuffer = new char[size];
    if (iconbuffer)
    {
      io_read(handle, iconbuffer, size);
      io_close(handle);
      SDL_IOStream *rw = SDL_IOFromMem(iconbuffer, size);
      SDL_Surface *icon = SDL_LoadBMP_IO(rw, 0);
      SDL_SetWindowIcon(win_window, icon);
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

  gfxinitted = false;
}

void clearscreen()
{
  if (!gfxinitted) return;

  unsigned *dptr = scrbuffer;

  for (int c = 0; c < MAX_ROWS * MAX_COLUMNS; c++)
  {
    setcharcolor(dptr, 0x20, 0x7);
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
  int x = (MAX_COLUMNS - std::strlen(text)) / 2;

  printtext(x, y, color, text);
}

void printtextcp(int cp, int y, int color, const char *text)
{
  int x = cp - (std::strlen(text) / 2);

  printtext(x, y, color, text);
}


void printblank(int x, int y, int length)
{
  if (!gfxinitted) return;
  if ((y < 0) | (y >= MAX_ROWS)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);

  while (length--)
  {
    setcharcolor(dptr, 0x20, 0x7);
    dptr++;
  }
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
  if (y < 0) return;
  if (y >= MAX_ROWS) return;
  if (y+sy > MAX_ROWS) return;
  if ((!sx) || (!sy)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);
  unsigned *dptr2 = scrbuffer + ((x+sx-1) + y * MAX_COLUMNS);
  int counter = sy;

  while (counter--)
  {
    setcharcolor(dptr, '|', color);
    setcharcolor(dptr2, '|', color);
    dptr += MAX_COLUMNS;
    dptr2 += MAX_COLUMNS;
  }

  dptr = scrbuffer + (x + y * MAX_COLUMNS);
  dptr2 = scrbuffer + (x + (y+sy-1) * MAX_COLUMNS);
  counter = sx;

  while (counter--)
  {
    setcharcolor(dptr, '-', color);
    setcharcolor(dptr2, '-', color);
    dptr++;
    dptr2++;
  }

  dptr = scrbuffer + (x + y * MAX_COLUMNS);
  setcharcolor(dptr, '+', color);

  dptr = scrbuffer + ((x+sx-1) + y * MAX_COLUMNS);
  setcharcolor(dptr, '+', color);

  dptr = scrbuffer + (x + (y+sy-1) * MAX_COLUMNS);
  setcharcolor(dptr, '+', color);

  dptr = scrbuffer + ((x+sx-1) + (y+sy-1) * MAX_COLUMNS);
  setcharcolor(dptr, '+', color);
}

void printbg(int x, int y, int color, int length)
{
  if (!gfxinitted) return;
  if ((y < 0) || (y >= MAX_ROWS)) return;

  unsigned *dptr = scrbuffer + (x + y * MAX_COLUMNS);

  while (length--)
  {
    setcolor(dptr, 15 | (color << 4));
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
          int c;

          unsigned char e = *chptr++;

          for (c = 0; c < 14; c++)
          {
            e = *chptr++;

            if (e & 128) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 64) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 32) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 16) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 8) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 4) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 2) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            if (e & 1) *dptr++ = fgcolor;
            else *dptr++ = bgcolor;
            dptr += gfx_screen->pitch - fontwidth;
          }
        }
      }
      sptr++;
      cmpptr++;
    }
  }

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
  gfx_unlock();
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
  cursorflashdelay += win_getspeed(50);

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
        win_keytable[c] = 0;
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

  if (rawkey == SDL_SCANCODE_KP_0) key = '0';
  if (rawkey == SDL_SCANCODE_KP_1) key = '1';
  if (rawkey == SDL_SCANCODE_KP_2) key = '2';
  if (rawkey == SDL_SCANCODE_KP_3) key = '3';
  if (rawkey == SDL_SCANCODE_KP_4) key = '4';
  if (rawkey == SDL_SCANCODE_KP_5) key = '5';
  if (rawkey == SDL_SCANCODE_KP_6) key = '6';
  if (rawkey == SDL_SCANCODE_KP_7) key = '7';
  if (rawkey == SDL_SCANCODE_KP_8) key = '8';
  if (rawkey == SDL_SCANCODE_KP_9) key = '9';
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
