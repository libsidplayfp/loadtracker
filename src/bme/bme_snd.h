// Sound functions header file

#ifndef BME_SND_H
#define BME_SND_H

#include "bme_main.h"

#include <SDL3/SDL.h>

bool snd_init(unsigned mixrate, unsigned mixmode, unsigned channels, int usedirectsound);
void snd_uninit();
void snd_setcustommixer(void (*custommixer)(Sint32 *dest, unsigned samples));

extern void (*snd_player)();
extern int snd_bpmtempo;
extern unsigned snd_mixrate;

#endif
