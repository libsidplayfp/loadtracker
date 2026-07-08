//
// BME (Blasphemous Multimedia Engine) graphics main module
//

#include "bme_main.h"
#include "bme_win.h"
#include "bme_io.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define MAX_COLORS 256          // 8bit oldskool mode

// Prototypes

bool gfx_init(unsigned xsize, unsigned ysize, unsigned framerate, unsigned flags);
int gfx_reinit();
void gfx_uninit();
bool gfx_lock();
void gfx_unlock();
void gfx_flip();
void gfx_setclipregion(unsigned left, unsigned top, unsigned right, unsigned bottom);
void gfx_setmaxspritefiles(unsigned num);
bool gfx_loadpalette(const char *name);
void gfx_calcpalette(int fade, int radd, int gadd, int badd);
void gfx_setpalette();
bool gfx_loadsprites(unsigned num, const char *name);
void gfx_freesprites(unsigned num);

void gfx_drawsprite(int x, int y, unsigned num);
void gfx_getspriteinfo(unsigned num);

bool gfx_initted = false;
bool gfx_redraw = false;
int gfx_scanlinemode = 0;
unsigned gfx_virtualxsize;
unsigned gfx_virtualysize;
unsigned gfx_windowxsize;
unsigned gfx_windowysize;
int gfx_blockxsize = 16;
int gfx_blockysize = 16;
int spr_xsize = 0;
int spr_ysize = 0;
int spr_xhotspot = 0;
int spr_yhotspot = 0;
unsigned gfx_nblocks = 0;
Uint8 gfx_palette[MAX_COLORS * 3] = {0};
SDL_Surface *gfx_screen = NULL;
SDL_Renderer *gfx_renderer = NULL;

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
static int gfx_maxcolors = MAX_COLORS;
static unsigned gfx_maxspritefiles = 0;
static SPRITEHEADER **gfx_spriteheaders = NULL;
static Uint8 **gfx_spritedata = NULL;
static unsigned *gfx_spriteamount = NULL;
static SDL_Color gfx_sdlpalette[MAX_COLORS];
static bool gfx_locked = false;
static SDL_Texture *sdlTexture = NULL;

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

    // Colors 0 & 255 are always black & white
    gfx_sdlpalette[0].r = 0;
    gfx_sdlpalette[0].g = 0;
    gfx_sdlpalette[0].b = 0;
    gfx_sdlpalette[0].a = 255;
    gfx_sdlpalette[255].r = 255;
    gfx_sdlpalette[255].g = 255;
    gfx_sdlpalette[255].b = 255;
    gfx_sdlpalette[255].a = 255;

    gfx_renderer = SDL_CreateRenderer(win_window, NULL);
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
    SDL_UpdateTexture(sdlTexture, NULL, surf->pixels, surf->pitch);
    SDL_DestroySurface(surf);
    SDL_RenderClear(gfx_renderer);
    SDL_RenderTexture(gfx_renderer, sdlTexture, NULL, NULL);
    SDL_RenderPresent(gfx_renderer);
    gfx_redraw = false;
}

bool gfx_loadpalette(const char *name)
{
    int handle = io_open(name);
    if (handle == -1)
    {
        bme_error = BME_OPEN_ERROR;
        return false;
    }
    if (io_read(handle, gfx_palette, sizeof gfx_palette) != sizeof gfx_palette)
    {
        bme_error = BME_READ_ERROR;
        io_close(handle);
        return false;
    }

    io_close(handle);
    gfx_calcpalette(64, 0, 0, 0);
    bme_error = BME_OK;
    return true;
}

void gfx_calcpalette(int fade, int radd, int gadd, int badd)
{
    Uint8  *sptr = &gfx_palette[3];
    int cl;
    if (radd < 0) radd = 0;
    if (gadd < 0) gadd = 0;
    if (badd < 0) badd = 0;

    for (int c = 1; c < 255; c++)
    {
        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += radd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].r = (cl << 2) | (cl & 3);
        sptr++;

        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += gadd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].g = (cl << 2) | (cl & 3);
        sptr++;

        cl = *sptr;
        cl *= fade;
        cl >>= 6;
        cl += badd;
        if (cl > 63) cl = 63;
        if (cl < 0) cl = 0;
        gfx_sdlpalette[c].b = (cl << 2) | (cl & 3);
        sptr++;

        gfx_sdlpalette[c].a = 255;
    }
}

void gfx_setpalette()
{
    if (!gfx_initted) return;

    SDL_Palette *palette = SDL_CreateSurfacePalette(gfx_screen);
    SDL_SetPaletteColors(palette, &gfx_sdlpalette[0], 0, gfx_maxcolors);
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

void gfx_setmaxspritefiles(unsigned num)
{
    if (gfx_spriteheaders) return;

    gfx_spriteheaders = (SPRITEHEADER**)std::malloc(num * sizeof(Uint8 *));
    gfx_spritedata = (Uint8**)std::malloc(num * sizeof(Uint8 *));
    gfx_spriteamount = (unsigned int*)std::malloc(num * sizeof(unsigned));
    if ((gfx_spriteheaders) && (gfx_spritedata) && (gfx_spriteamount))
    {
        unsigned c;

        gfx_maxspritefiles = num;
        for (c = 0; c < num; c++)
        {
            gfx_spriteamount[c] = 0;
            gfx_spritedata[c] = NULL;
            gfx_spriteheaders[c] = NULL;
        }
    }
    else gfx_maxspritefiles = 0;
}

bool gfx_loadsprites(unsigned num, const char *name)
{
    if (!gfx_spriteheaders)
    {
        gfx_setmaxspritefiles(1);
    }

    bme_error = BME_OPEN_ERROR;
    if (num >= gfx_maxspritefiles) return false;

    gfx_freesprites(num);

    int handle = io_open(name);
    if (handle == -1) return false;

    int size = io_lseek(handle, 0, SEEK_END);
    io_lseek(handle, 0, SEEK_SET);

    gfx_spriteamount[num] = io_readle32(handle);

    gfx_spriteheaders[num] = (SPRITEHEADER*)std::malloc(gfx_spriteamount[num] * sizeof(SPRITEHEADER));

    if (!gfx_spriteheaders[num])
    {
        bme_error = BME_OUT_OF_MEMORY;
        io_close(handle);
        return false;
    }

    for (unsigned c = 0; c < gfx_spriteamount[num]; c++)
    {
        SPRITEHEADER *hptr = gfx_spriteheaders[num] + c;

        hptr->xsize = io_readle16(handle);
        hptr->ysize = io_readle16(handle);
        hptr->xhot = io_readle16(handle);
        hptr->yhot = io_readle16(handle);
        hptr->offset = io_readle32(handle);
    }

    int datastart = io_lseek(handle, 0, SEEK_CUR);
    gfx_spritedata[num] = (Uint8*)std::malloc(size - datastart);
    if (!gfx_spritedata[num])
    {
        bme_error = BME_OUT_OF_MEMORY;
        io_close(handle);
        return false;
    }
    io_read(handle, gfx_spritedata[num], size - datastart);
    io_close(handle);
    bme_error = BME_OK;
    return true;
}

void gfx_freesprites(unsigned num)
{
    if (num >= gfx_maxspritefiles) return;

    if (gfx_spritedata[num])
    {
        std::free(gfx_spritedata[num]);
        gfx_spritedata[num] = NULL;
    }
    if (gfx_spriteheaders[num])
    {
        std::free(gfx_spriteheaders[num]);
        gfx_spriteheaders[num] = NULL;
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


void gfx_getspriteinfo(unsigned num)
{
    unsigned sprf = num >> 16;
    unsigned spr = (num & 0xffff) - 1;
    SPRITEHEADER *hptr;

    if ((sprf >= gfx_maxspritefiles) || (!gfx_spriteheaders[sprf]) ||
        (spr >= gfx_spriteamount[sprf])) hptr = nullptr;
    else hptr = gfx_spriteheaders[sprf] + spr;

    if (!hptr)
    {
        spr_xsize = 0;
        spr_ysize = 0;
        spr_xhotspot = 0;
        spr_yhotspot = 0;
        return;
    }

    spr_xsize = hptr->xsize;
    spr_ysize = hptr->ysize;
    spr_xhotspot = hptr->xhot;
    spr_yhotspot = hptr->yhot;
}

void gfx_drawsprite(int x, int y, unsigned num)
{
    unsigned sprf = num >> 16;
    unsigned spr = (num & 0xffff) - 1;
    SPRITEHEADER *hptr;

    if (!gfx_initted) return;
    if (!gfx_locked) return;

    if ((sprf >= gfx_maxspritefiles) || (!gfx_spriteheaders[sprf]) ||
        (spr >= gfx_spriteamount[sprf]))
    {
        spr_xsize = 0;
        spr_ysize = 0;
        spr_xhotspot = 0;
        spr_yhotspot = 0;
        return;
    }
    else hptr = gfx_spriteheaders[sprf] + spr;

    Uint8 *sptr = gfx_spritedata[sprf] + hptr->offset;
    spr_xsize = hptr->xsize;
    spr_ysize = hptr->ysize;
    spr_xhotspot = hptr->xhot;
    spr_yhotspot = hptr->yhot;

    x -= spr_xhotspot;
    y -= spr_yhotspot;

    if (x >= gfx_clipright) return;
    if (y >= gfx_clipbottom) return;
    if (x + spr_xsize <= gfx_clipleft) return;
    if (y + spr_ysize <= gfx_cliptop) return;

    while (y < gfx_cliptop)
    {
        int dec = *sptr++;
        if (dec == 255)
        {
            if (!(*sptr)) return;
            y++;
        }
        else
        {
            if (dec < 128)
            {
                sptr += dec;
            }
        }
    }
    while (y < gfx_clipbottom)
    {
        int cx = x;
        Uint8 *dptr = (Uint8*)gfx_screen->pixels + y * gfx_screen->pitch + x;

        for (;;)
        {
            int dec = *sptr++;

            if (dec == 255)
            {
                if (!(*sptr)) return;
                y++;
                break;
            }
            if (dec < 128)
            {
                if ((cx + dec <= gfx_clipleft) || (cx >= gfx_clipright))
                {
                    goto SKIP;
                }
                if (cx < gfx_clipleft)
                {
                    dec -= (gfx_clipleft - cx);
                    sptr += (gfx_clipleft - cx);
                    dptr += (gfx_clipleft - cx);
                    cx = gfx_clipleft;
                }
                while ((cx < gfx_clipright) && (dec))
                {
                    *dptr = *sptr;
                    cx++;
                    sptr++;
                    dptr++;
                    dec--;
                }
SKIP:
                cx += dec;
                sptr += dec;
                dptr += dec;
            }
            else
            {
                cx += (dec & 0x7f);
                dptr += (dec & 0x7f);
            }
        }
    }
}
