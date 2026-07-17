//
// BME (Blasphemous Multimedia Engine) windows & timing module
//

#include "bme_win.h"

#include "bme_main.h"
#include "bme_io.h"

#include <SDL3/SDL.h>

#include <new>

#include <cstdio>
#include <cstdlib>
#include <cstring>

// Colodore palette
unsigned char gfx_palette[MAX_COLORS * 3] =
{
    // Black
    0x00, 0x00, 0x00,
    // White
    0xFF, 0xFF, 0xFF,
    // Red
    0x96, 0x28, 0x2e,
    // Cyan
    0x5b, 0xd6, 0xce,
    // Purple
    0x9f, 0x2d, 0xad,
    // Green
    0x41, 0xb9, 0x36,
    // Blue
    0x27, 0x24, 0xc4,
    // Yellow
    0xef, 0xf3, 0x47,
    // Orange
    0x9f, 0x48, 0x15,
    // Brown
    0x5e, 0x35, 0x00,
    // Light Red
    0xda, 0x5f, 0x66,
    // Dark Gray
    0x47, 0x47, 0x47,
    // Medium Gray
    0x78, 0x78, 0x78,
    // Light Green
    0x91, 0xff, 0x84,
    // Light Blue
    0x68, 0x64, 0xff,
    // Light Gray
    0xae, 0xae, 0xae
};

SDL_Window *win_window = nullptr;

void win_checkmessages();
void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom);
bool gfx_reinit();
void gfx_uninit();

// Global variables

int win_fullscreen = 0; // By default windowed
bool win_windowinitted = false;
bool win_quitted = false;
unsigned char win_keytable[SDL_SCANCODE_COUNT] = {0};
unsigned char win_asciikey = 0;
unsigned win_mousexpos = 0;
unsigned win_mouseypos = 0;
unsigned win_mousexrel = 0;
unsigned win_mouseyrel = 0;
unsigned win_mousebuttons = 0;
float win_mouseywheel = 0.f;
unsigned char win_keystate[SDL_SCANCODE_COUNT] = {0};

bool gfx_initted = false;
bool gfx_redraw = false;
unsigned gfx_virtualxsize;
unsigned gfx_virtualysize;
unsigned gfx_windowxsize;
unsigned gfx_windowysize;
int spr_xsize = 0;
int spr_ysize = 0;
SDL_Surface *gfx_screen = nullptr;
SDL_Renderer *gfx_renderer = nullptr;

unsigned xpos = SDL_WINDOWPOS_UNDEFINED;
unsigned ypos = SDL_WINDOWPOS_UNDEFINED;
unsigned xsize = MAX_COLUMNS * 8;
unsigned ysize = MAX_ROWS * 16;


// Static variables

static Uint64 win_lasttime = 0;
static Uint64 win_currenttime = 0;
static int win_framecounter = 0;
static int win_activateclick = 0;

static bool gfx_initexec = false;
static unsigned gfx_last_xsize;
static unsigned gfx_last_ysize;
static int gfx_cliptop;
static int gfx_clipbottom;
static int gfx_clipleft;
static int gfx_clipright;

static SDL_Surface *gfx_cursor = nullptr;

static SDL_Color gfx_sdlpalette[MAX_COLORS];
static bool gfx_locked = false;
static SDL_Texture *sdlTexture = nullptr;

bool win_openwindow(const char *appname)
{
    if (!win_windowinitted)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        {
            return false;
        }
        std::atexit(SDL_Quit);
        win_windowinitted = true;
    }

    //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    //SDL_EnableUNICODE(1);

    SDL_PropertiesID props{SDL_CreateProperties()};
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, appname);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, xsize);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, ysize);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, xpos);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, ypos);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    if (win_fullscreen)
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);

    win_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!win_window)
    {
        return false;
    }
    SDL_StartTextInput(win_window);
    return true;
}

void win_closewindow()
{
    SDL_StopTextInput(win_window);
    SDL_DestroyWindow(win_window);
}

void win_savepos()
{
    int x, y;
    if (SDL_GetWindowPosition(win_window, &x, &y))
    {
        xpos = (unsigned)x;
        ypos = (unsigned)y;
    }
    if (SDL_GetWindowSize(win_window, &x, &y))
    {
        xsize = (unsigned)x;
        ysize = (unsigned)y;
    }
}

int win_getspeed(int framerate)
{
    // Note: here 1/10000th of a second accuracy is used (although
    // timer resolution is only millisecond) for minimizing
    // inaccuracy in frame duration calculation -> smoother screen
    // update

    int frametime = 10000 / framerate;
    int frames = 0;

    while (frames == 0)
    {
        win_checkmessages();

        win_lasttime = win_currenttime;
        win_currenttime = SDL_GetTicks();

        win_framecounter += (win_currenttime - win_lasttime)*10;
        frames = win_framecounter / frametime;
        win_framecounter -= frames * frametime;

        if (!frames) SDL_Delay((frametime - win_framecounter)/10);
    }

    return frames;
}

// This is the "message pump". Called by following functions:
// win_getspeed();
// kbd_waitkey();
//
// It is recommended to be called in any long loop where those two functions
// are not called.

void win_checkmessages()
{
    SDL_Event event;
    unsigned keynum;

    win_activateclick = 0;
    win_mouseywheel = 0.f;

    SDL_PumpEvents();

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_MOUSE_MOTION:
            win_mousexpos = event.motion.x;
            win_mouseypos = event.motion.y;
            win_mousexrel += event.motion.xrel;
            win_mouseyrel += event.motion.yrel;
            break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch(event.button.button)
            {
                case SDL_BUTTON_LEFT:
                win_mousebuttons |= MOUSEB_LEFT;
                break;

                case SDL_BUTTON_MIDDLE:
                win_mousebuttons |= MOUSEB_MIDDLE;
                break;

                case SDL_BUTTON_RIGHT:
                win_mousebuttons |= MOUSEB_RIGHT;
                break;
            }
            break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
            switch(event.button.button)
            {
                case SDL_BUTTON_LEFT:
                win_mousebuttons &= ~MOUSEB_LEFT;
                break;

                case SDL_BUTTON_MIDDLE:
                win_mousebuttons &= ~MOUSEB_MIDDLE;
                break;

                case SDL_BUTTON_RIGHT:
                win_mousebuttons &= ~MOUSEB_RIGHT;
                break;
            }
            break;

            case SDL_EVENT_MOUSE_WHEEL:
            win_mouseywheel = event.wheel.y;
            break;

            case SDL_EVENT_QUIT:
            win_quitted = true;
            break;

            case SDL_EVENT_TEXT_INPUT:
            win_asciikey = event.text.text[0];
            break;

            case SDL_EVENT_KEY_DOWN:
            keynum = event.key.scancode;
            if (keynum < SDL_SCANCODE_COUNT)
            {
                if ((keynum == SDL_SCANCODE_RETURN) && ((win_keystate[SDL_SCANCODE_LALT])
                    || (win_keystate[SDL_SCANCODE_RALT])))
                {
                    win_fullscreen ^= 1;
                    gfx_reinit();
                    break;
                }
                win_keytable[keynum] = 1;
                win_keystate[keynum] = 1;
            }
            break;

            case SDL_EVENT_KEY_UP:
            keynum = event.key.scancode;
            if (keynum < SDL_SCANCODE_COUNT)
            {
                win_keytable[keynum] = 0;
                win_keystate[keynum] = 0;
            }
            break;

            case SDL_EVENT_WINDOW_RESIZED:
            //case SDL_VIDEOEXPOSE:
            gfx_redraw = true;
            break;
        }
    }
}

bool gfx_init(unsigned xsize, unsigned ysize)
{
    // Prevent re-entry (by window procedure)
    if (gfx_initexec) return true;
    gfx_initexec = true;

    gfx_last_xsize = xsize;
    gfx_last_ysize = ysize;

    SDL_SetWindowFullscreen(win_window, win_fullscreen);

    // Calculate virtual window size

    gfx_virtualxsize = xsize;
    gfx_virtualxsize /= 16;
    gfx_virtualxsize *= 16;
    gfx_virtualysize = ysize;

    if ((!gfx_virtualxsize) || (!gfx_virtualysize))
    {
        gfx_initexec = false;
        gfx_uninit();
        return false;
    }

    // Calculate actual window size (for scanline mode & doublesize mode
    // this is double the virtual)

    gfx_windowxsize = gfx_virtualxsize;
    gfx_windowysize = gfx_virtualysize;

    gfx_setclipregion(0, 0, gfx_virtualxsize, gfx_virtualysize);

    gfx_screen = SDL_CreateSurface(xsize, ysize, SDL_PIXELFORMAT_INDEX8);
    if (!gfx_screen)
        return false;

    gfx_renderer = SDL_CreateRenderer(win_window, nullptr);
    sdlTexture = SDL_CreateTexture(gfx_renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             xsize, ysize);

    SDL_HideCursor();

    gfx_initted = true;

    gfx_setpalette();

    gfx_redraw = true;
    gfx_initexec = false;
    return true;
}

bool gfx_reinit()
{
    gfx_uninit();
    return gfx_init(gfx_last_xsize, gfx_last_ysize);
}

void gfx_uninit()
{
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroySurface(gfx_screen);
    SDL_DestroyRenderer(gfx_renderer);
    gfx_initted = false;
    return;
}

bool gfx_lock()
{
    if (gfx_locked) return true;
    if (!gfx_initted) return false;
    if (SDL_LockSurface(gfx_screen))
    {
        gfx_locked = true;
        return true;
    }
    return false;
}

void gfx_unlock()
{
    if (gfx_locked)
    {
        SDL_UnlockSurface(gfx_screen);
        gfx_locked = false;
    }
}

void gfx_flip()
{
    SDL_Surface* surf = SDL_ConvertSurface(gfx_screen, SDL_PIXELFORMAT_RGBA32);
    if (!surf)
        std::printf("Error: %s\n", SDL_GetError());
    SDL_UpdateTexture(sdlTexture, nullptr, surf->pixels, surf->pitch);
    SDL_DestroySurface(surf);
    SDL_RenderClear(gfx_renderer);
    SDL_RenderTexture(gfx_renderer, sdlTexture, nullptr, nullptr);
    SDL_RenderPresent(gfx_renderer);
    gfx_redraw = false;
}

void gfx_calcpalette()
{
    unsigned char *sptr = gfx_palette;
    for (int c = 0; c < MAX_COLORS; c++)
    {
        gfx_sdlpalette[c].r = *sptr;
        sptr++;

        gfx_sdlpalette[c].g = *sptr;
        sptr++;

        gfx_sdlpalette[c].b = *sptr;
        sptr++;

        gfx_sdlpalette[c].a = 255;
    }
}

void gfx_setpalette()
{
    if (!gfx_initted) return;

    gfx_calcpalette();
    SDL_Palette *palette = SDL_CreateSurfacePalette(gfx_screen);
    SDL_SetPaletteColors(palette, &gfx_sdlpalette[0], 0, MAX_COLORS);
}

void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom)
{
    if (left >= right) return;
    if (top >= bottom) return;
    if (left >= gfx_virtualxsize) return;
    if (top >= gfx_virtualysize) return;
    if (right > gfx_virtualxsize) return;
    if (bottom > gfx_virtualysize) return;

    gfx_clipleft = left;
    gfx_clipright = right;
    gfx_cliptop = top;
    gfx_clipbottom = bottom;
}

bool gfx_loadcursor(const char *name)
{
    gfx_freecursor();

    int handle = io_open(name);
    if (handle == -1) return false;

    int size = io_lseek(handle, 0, SEEK_END);
    io_lseek(handle, 0, SEEK_SET);
    char *iconbuffer = new (std::nothrow) char[size];
    io_read(handle, iconbuffer, size);
    io_close(handle);

    SDL_IOStream *rw = SDL_IOFromMem(iconbuffer, size);
    gfx_cursor = SDL_LoadPNG_IO(rw, true);
    if (!gfx_cursor)
        return false;

    return true;
}

void gfx_freecursor()
{
    if (gfx_cursor)
    {
        SDL_DestroySurface(gfx_cursor);
        gfx_cursor = nullptr;
    }
}

bool gfx_loadcharset(const char *name, unsigned char *chardata)
{
    int handle = io_open(name);
    if (handle == -1) return false;

    int size = io_lseek(handle, 0, SEEK_END);
    char *charbuffer = new (std::nothrow) char[size];
    if (!charbuffer)
    {
        io_close(handle);
        return false;
    }
    io_lseek(handle, 0, SEEK_SET);
    io_read(handle, charbuffer, size);
    io_close(handle);

    SDL_IOStream *rw = SDL_IOFromMem(charbuffer, size);
    SDL_Surface *gfx_chars = SDL_LoadPNG_IO(rw, true);
    if (!gfx_chars)
        return false;

    if ((gfx_chars->w != 8) || (gfx_chars->h != 4096))
        return false;

    if (gfx_chars->format != SDL_PIXELFORMAT_INDEX8)
        return false;

    int i = 0, j = 0;
    for (int y=0; y<4096; y++)
    {
        chardata[j] = 0;
        for (int x=7; x>=0; x--)
        {
            if (((unsigned char*)gfx_chars->pixels)[i])
                chardata[j] |= (1u << x);
            i++;
        }
        j++;
    }
    SDL_DestroySurface(gfx_chars);

    return true;
}

void gfx_drawcursor(int x, int y)
{
    if (!gfx_initted) return;

    if (!gfx_cursor)
    {
        spr_xsize = 0;
        spr_ysize = 0;
        return;
    }

    spr_xsize = gfx_cursor->w;
    spr_ysize = gfx_cursor->h;

    if (x >= gfx_clipright) return;
    if (y >= gfx_clipbottom) return;
    if (x + spr_xsize <= gfx_clipleft) return;
    if (y + spr_ysize <= gfx_cliptop) return;

    SDL_Rect dstrect;
    dstrect.x = x;
    dstrect.y = y;
    SDL_BlitSurface(gfx_cursor, NULL, gfx_screen, &dstrect);
}

void mou_getpos(unsigned *x, unsigned *y)
{
    if (!gfx_initted)
    {
        *x = win_mousexpos;
        *y = win_mouseypos;
    }
    else
    {
        *x = win_mousexpos * gfx_virtualxsize / gfx_windowxsize;
        *y = win_mouseypos * gfx_virtualysize / gfx_windowysize;
    }
}

unsigned mou_getbuttons()
{
    return win_mousebuttons;
}
