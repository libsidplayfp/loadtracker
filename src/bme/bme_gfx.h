// BME graphics module header file

#ifndef BME_GFX_H
#define BME_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

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
void gfx_drawsprite(int x, int y, unsigned num);
void gfx_freesprites(unsigned num);

extern bool gfx_initted;
extern int gfx_scanlinemode;
extern int gfx_fullscreen;
extern bool gfx_redraw;
extern unsigned gfx_windowxsize;
extern unsigned gfx_windowysize;
extern unsigned gfx_virtualxsize;
extern unsigned gfx_virtualysize;
extern unsigned gfx_nblocks;
extern int gfx_blockxsize;
extern int gfx_blockysize;
extern int spr_xsize;
extern int spr_ysize;
extern int spr_xhotspot;
extern int spr_yhotspot;
extern Uint8 *gfx_vscreen;
extern Uint8 *gfx_blocks;
extern Uint8 gfx_palette[];
extern SDL_Surface *gfx_screen;
extern SDL_Renderer *gfx_renderer;

#ifdef __cplusplus
}
#endif

#endif
