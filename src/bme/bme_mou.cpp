//
// BME (Blasphemous Multimedia Engine) mouse module
//

#include "bme_main.h"
#include "bme_win.h"
#include "bme_gfx.h"
#include "bme_io.h"
#include "bme_err.h"

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

void mou_init()
{
    win_mousebuttons = 0;
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

unsigned mou_getbuttons(void)
{
    return win_mousebuttons;
}

#ifdef __cplusplus
}
#endif

