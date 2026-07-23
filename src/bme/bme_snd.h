// Sound functions header file

#ifndef BME_SND_H
#define BME_SND_H

#include "bme_main.h"

#include <SDL3/SDL.h>

using CustomMixer = void (*)(Sint32 *dest, unsigned samples);
using Player = void (*)();

bool snd_init(unsigned mixrate, unsigned numsids);
void snd_uninit();
void snd_setcustommixer(CustomMixer custommixer);
void snd_setplayer(Player player);
unsigned getmixrate();

void snd_play();
void snd_stop();

extern int snd_bpmtempo;

#endif
