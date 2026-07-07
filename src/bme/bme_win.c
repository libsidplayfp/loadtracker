//
// BME (Blasphemous Multimedia Engine) windows & timing module
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include "bme_main.h"
#include "bme_gfx.h"
#include "bme_mou.h"
#include "bme_io.h"
#include "bme_err.h"

SDL_Window *win_window = NULL;

// Prototypes

int win_openwindow(unsigned xsize, unsigned ysize, const char *appname, char *icon);
void win_closewindow(void);
void win_checkmessages(void);
int win_getspeed(int framerate);
void win_setmousemode(int mode);

// Global variables

int win_fullscreen = 0; // By default windowed
int win_windowinitted = 0;
int win_quitted = 0;
unsigned char win_keytable[SDL_SCANCODE_COUNT] = {0};
unsigned char win_asciikey = 0;
unsigned win_virtualkey = 0;
unsigned win_mousexpos = 0;
unsigned win_mouseypos = 0;
unsigned win_mousexrel = 0;
unsigned win_mouseyrel = 0;
unsigned win_mousebuttons = 0;
int win_mousemode = MOUSE_FULLSCREEN_HIDDEN;
float win_mouseywheel = 0.f;
unsigned char win_keystate[SDL_SCANCODE_COUNT] = {0};

// Static variables

static int win_lasttime = 0;
static int win_currenttime = 0;
static int win_framecounter = 0;
static int win_activateclick = 0;

int win_openwindow(unsigned xsize, unsigned ysize, const char *appname, char *icon)
{
    Uint32 flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_RESIZABLE;
    if (win_fullscreen) flags |= SDL_WINDOW_FULLSCREEN;

    if (!win_windowinitted)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK))
        {
            return BME_ERROR;
        }
        atexit(SDL_Quit);
        win_windowinitted = 1;
    }

    //SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
    //SDL_EnableUNICODE(1);
    win_window = SDL_CreateWindow(appname, xsize, ysize, flags);
    if (!win_window)
    {
        return BME_ERROR;
    }
    SDL_StartTextInput(win_window);
    return BME_OK;
}

void win_closewindow(void)
{
    SDL_StopTextInput(win_window);
    SDL_DestroyWindow(win_window);
}

int win_getspeed(int framerate)
{
    // Note: here 1/10000th of a second accuracy is used (although
    // timer resolution is only millisecond) for minimizing
    // inaccuracy in frame duration calculation -> smoother screen
    // update

    int frametime = 10000 / framerate;
    int frames = 0;

    while (!frames)
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

void win_checkmessages(void)
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
            win_quitted = 1;
            break;

            case SDL_EVENT_TEXT_INPUT:
            win_asciikey = event.text.text[0];
            break;

            case SDL_EVENT_KEY_DOWN:
            win_virtualkey = event.key.key;
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
            gfx_redraw = 1;
            break;
        }
    }
}

void win_setmousemode(int mode)
{
    win_mousemode = mode;

    switch(mode)
    {
        case MOUSE_ALWAYS_VISIBLE:
        SDL_ShowCursor();
        break;

        case MOUSE_FULLSCREEN_HIDDEN:
        if (win_fullscreen)
        {
            SDL_HideCursor();
        }
        else
        {
            SDL_ShowCursor();
        }
        break;

        case MOUSE_ALWAYS_HIDDEN:
        SDL_HideCursor();
        break;
    }
}

