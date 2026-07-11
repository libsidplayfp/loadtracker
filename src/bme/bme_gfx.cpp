//
// BME (Blasphemous Multimedia Engine) graphics main module
//

#include "bme_gfx.h"

#include "bme_main.h"
#include "bme_win.h"
#include "bme_io.h"

#include <SDL3/SDL.h>

#include <new>

#include <cstdlib>
#include <cstdio>
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

void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom);

bool gfx_initted = false;
bool gfx_redraw = false;
int gfx_scanlinemode = 0;
unsigned gfx_virtualxsize;
unsigned gfx_virtualysize;
unsigned gfx_windowxsize;
unsigned gfx_windowysize;
int spr_xsize = 0;
int spr_ysize = 0;
SDL_Surface *gfx_screen = nullptr;
SDL_Renderer *gfx_renderer = nullptr;

// Static variables

static bool gfx_initexec = false;
static unsigned gfx_last_xsize;
static unsigned gfx_last_ysize;
static unsigned gfx_last_framerate;
static unsigned gfx_last_flags;
static int gfx_cliptop;
static int gfx_clipbottom;
static int gfx_clipleft;
static int gfx_clipright;

static SDL_Surface *gfx_cursor = nullptr;

static SDL_Color gfx_sdlpalette[MAX_COLORS];
static bool gfx_locked = false;
static SDL_Texture *sdlTexture = nullptr;

bool gfx_init(unsigned xsize, unsigned ysize, unsigned framerate, unsigned flags)
{
    // Prevent re-entry (by window procedure)
    if (gfx_initexec) return true;
    gfx_initexec = true;

    gfx_last_xsize = xsize;
    gfx_last_ysize = ysize;
    gfx_last_framerate = framerate;
    gfx_last_flags = flags & ~(GFX_FULLSCREEN | GFX_WINDOW);

    // Store the options contained in the flags

    gfx_scanlinemode = flags & (GFX_SCANLINES | GFX_DOUBLESIZE);

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
        bme_error = BME_ILLEGAL_CONFIG;
        return false;
    }

    // Calculate actual window size (for scanline mode & doublesize mode
    // this is double the virtual)

    gfx_windowxsize = gfx_virtualxsize;
    gfx_windowysize = gfx_virtualysize;
    if (gfx_scanlinemode)
    {
        gfx_windowxsize <<= 1;
        gfx_windowysize <<= 1;
    }

    gfx_setclipregion(0, 0, gfx_virtualxsize, gfx_virtualysize);

    gfx_renderer = SDL_CreateRenderer(win_window, nullptr);
    gfx_screen = SDL_CreateSurface(xsize, ysize, SDL_PIXELFORMAT_INDEX8);
    sdlTexture = SDL_CreateTexture(gfx_renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             xsize, ysize);

    gfx_initexec = false;
    if (gfx_screen)
    {
        gfx_initted = true;
        gfx_redraw = true;
        gfx_setpalette();
        win_setmousemode(win_mousemode);
        return true;
    }
    else return false;
}

int gfx_reinit()
{
    gfx_uninit();
    return gfx_init(gfx_last_xsize, gfx_last_ysize, gfx_last_framerate,
        gfx_last_flags);
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
    else return false;
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
    bme_error = BME_OPEN_ERROR;

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

    bme_error = BME_OK;
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

void gfx_copyscreen8(Uint8  *destaddress, Uint8  *srcaddress, unsigned pitch)
{
    switch(gfx_scanlinemode)
    {
        default:
        for (unsigned c = 0; c < gfx_virtualysize; c++)
        {
            std::memcpy(destaddress, srcaddress, gfx_virtualxsize);
            destaddress += pitch;
            srcaddress += gfx_virtualxsize;
        }
        break;

        case GFX_SCANLINES:
        for (unsigned c = 0; c < gfx_virtualysize; c++)
        {
            int d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch*2 - (gfx_virtualxsize << 1);
        }
        break;

        case GFX_DOUBLESIZE:
        for (unsigned c = 0; c < gfx_virtualysize; c++)
        {
            int d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch - (gfx_virtualxsize << 1);
            srcaddress -= gfx_virtualxsize;
            d = gfx_virtualxsize;
            while (d--)
            {
                *destaddress = *srcaddress;
                destaddress++;
                *destaddress = *srcaddress;
                destaddress++;
                srcaddress++;
            }
            destaddress += pitch - (gfx_virtualxsize << 1);
        }
        break;
    }
}

void gfx_drawcursor(int x, int y)
{
    if (!gfx_initted) return;
    //if (!gfx_locked) return;

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
    if (!SDL_BlitSurface(gfx_cursor, NULL, gfx_screen, &dstrect))
        printf("Error: %s\n", SDL_GetError());
}
