// BME windows, input & timing module header file

int win_openwindow(unsigned xsize, unsigned ysize, const char *appname, char *icon);
void win_closewindow(void);
void win_checkmessages(void);
int win_getspeed(int framerate);
void win_setmousemode(int mode);

extern int win_windowinitted;
extern int win_quitted;
extern int win_fullscreen;
extern unsigned char win_keytable[SDL_SCANCODE_COUNT];
extern unsigned char win_keystate[SDL_SCANCODE_COUNT];
extern unsigned char win_asciikey;
extern unsigned win_virtualkey;
extern unsigned win_mousexpos;
extern unsigned win_mouseypos;
extern unsigned win_mousexrel;
extern unsigned win_mouseyrel;
extern unsigned win_mousebuttons;
extern int win_mousemode;
extern float win_mouseywheel;
extern SDL_Window *win_window;

