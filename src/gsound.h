#ifndef GSOUND_H
#define GSOUND_H

#define MINMIXRATE 11025
#define MAXMIXRATE 48000
#define DEFAULTMIXRATE 48000

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

#define MIXBUFFERSIZE 65536

int sound_init(unsigned mr, unsigned writer, unsigned m, unsigned ntsc, unsigned multiplier, unsigned interpolate, unsigned customclockrate, unsigned exsid, float filterbias, unsigned combwaves);
void sound_uninit(void);
void sound_suspend(void);
void sound_flush(void);

#endif
