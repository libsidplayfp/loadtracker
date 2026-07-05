//
// GOATTRACKER sound routines
//

#define GSOUND_C

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef USE_EXSID
#  include <exSID.h>
#endif

#include "goattrk2.h"

// General / reSID output
int playspeed;
int useexsid = 0;
int initted = 0;
unsigned framerate = PALFRAMERATE;
Sint16 *buffer = NULL;
Sint16 *lbuffer = NULL;
Sint16 *rbuffer = NULL;
FILE *writehandle = NULL;
SDL_TimerID timer = 0;

void sound_playrout(void);
void sound_mixer(Sint32 *dest, unsigned samples);
Uint32 sound_timer(void *userdata, SDL_TimerID timerID, Uint32 interval);

#ifdef USE_EXSID
void* exsidfd = NULL;
unsigned exsidDelay = 0;
#endif

int sound_init(unsigned mr, unsigned writer, unsigned m, unsigned ntsc,
               unsigned multiplier, unsigned interpolate, unsigned customclockrate,
               unsigned exsid, float filterbias, unsigned combwaves)
{
#ifdef __WIN32__
  if (!flushmutex)
      flushmutex = SDL_CreateMutex();
#endif

  sound_uninit();

  if (multiplier)
  {
    if (ntsc)
    {
      framerate = NTSCFRAMERATE * multiplier;
      snd_bpmtempo = 150 * multiplier;
    }
    else
    {
      framerate = PALFRAMERATE * multiplier;
      snd_bpmtempo = 125 * multiplier;
    }
  }
  else
  {
    if (ntsc)
    {
      framerate = NTSCFRAMERATE / 2;
      snd_bpmtempo = 150 / 2;
    }
    else
    {
      framerate = PALFRAMERATE / 2;
      snd_bpmtempo = 125 / 2;
    }
  }

#ifdef USE_EXSID
  if (exsid)
  {
    exsidfd = exSID_new();
    if (!exsidfd)
      return 0;

    if (exSID_init(exsidfd) < 0)
      return 0;

    int model = exSID_hwmodel(exsidfd);
    switch (model)
    {
      case XS_MD_PLUS:
      exSID_audio_op(exsidfd, m == 1 ? XS_AU_8580_8580 : XS_AU_6581_6581);
      exSID_clockselect(exsidfd, ntsc ? XS_CL_NTSC : XS_CL_PAL);
      exSID_audio_op(exsidfd, XS_AU_UNMUTE);
      exsidDelay = ntsc ? NTSCCLOCKRATE : PALCLOCKRATE;
      break;

      case XS_MD_STD:
      exSID_chipselect(exsidfd, m == 1 ? XS_CS_CHIP1 : XS_CS_CHIP0);
      exsidDelay = 1000000;
      break;

      default:
      return 0;
    }

    exsidDelay /= framerate;
    exsidDelay -= SIDWRITEDELAY*NUMSIDREGS;

    useexsid = 1;
    timer = SDL_AddTimer(1000 / framerate, sound_timer, NULL);
    goto SOUNDOK;
  }
#endif

  if (!buffer) buffer = (Sint16*)malloc(MIXBUFFERSIZE * sizeof(Sint16));
  if (!lbuffer) lbuffer = (Sint16*)malloc(MIXBUFFERSIZE * sizeof(Sint16));
  if (!rbuffer) rbuffer = (Sint16*)malloc(MIXBUFFERSIZE * sizeof(Sint16));
  if ((!buffer) || (!lbuffer) || (!rbuffer)) return 0;

  if (writer)
    writehandle = fopen("sidaudio.raw", "wb");

  playspeed = mr;
  if (playspeed < MINMIXRATE) playspeed = MINMIXRATE;
  if (playspeed > MAXMIXRATE) playspeed = MAXMIXRATE;

  if (numsids == 1 && !snd_init(mr, SIXTEENBIT|MONO, 1, 0))
  {
    return 0;
  }
  else if (numsids == 2 && !snd_init(mr, SIXTEENBIT|STEREO, 1, 0))
  {
    return 0;
  }
  playspeed = snd_mixrate;
  sid_init(playspeed, m, ntsc, interpolate, customclockrate, numsids, filterbias, combwaves);

  snd_player = &sound_playrout;
  snd_setcustommixer(sound_mixer);

SOUNDOK:
  initted = 1;
  return 1;
}

void sound_uninit(void)
{
  if (!initted) return;
  initted = 0;

  // Apparently a delay is needed to make sure the sound timer thread is
  // not mixing stuff anymore, and we can safely delete related structures
  SDL_Delay(50);

  if (useexsid)
  {
#ifdef __WIN32__
    if (!playerthread)
    {
      SDL_RemoveTimer(timer);
    }
    else
    {
      runplayerthread = FALSE;
      SDL_WaitThread(playerthread, NULL);
      playerthread = NULL;
    }
#else
    SDL_RemoveTimer(timer);
#endif
  }
  else
  {
    snd_setcustommixer(NULL);
    snd_player = NULL;
  }

  if (writehandle)
  {
    fclose(writehandle);
    writehandle = NULL;
  }

  if (buffer)
  {
    free(buffer);
    buffer = NULL;
  }

  if (lbuffer)
  {
      free(lbuffer);
      lbuffer = NULL;
  }

  if (rbuffer)
  {
      free(rbuffer);
      rbuffer = NULL;
  }

#ifdef USE_EXSID
  if (useexsid)
  {
    if (exsidfd != NULL)
    {
      for (c = 0; c < NUMSIDREGS; c++)
      {
        exSID_clkdwrite(exsidfd, SIDWRITEDELAY, c, 0x00);
      }
    }

    exSID_audio_op(exsidfd, XS_AU_MUTE);
    exSID_exit(exsidfd);

    exSID_free(exsidfd);
    exsidfd = NULL;
  }
#endif
}

void sound_suspend(void)
{
  #ifdef __WIN32__
  SDL_LockMutex(flushmutex);
  suspendplayroutine = TRUE;
  SDL_UnlockMutex(flushmutex);
  #endif
}

void sound_flush(void)
{
  #ifdef __WIN32__
  SDL_LockMutex(flushmutex);
  flushplayerthread = TRUE;
  SDL_UnlockMutex(flushmutex);
  #endif
}

Uint32 sound_timer(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
  if (!initted) return interval;
  sound_playrout();
  return interval;
}

void sound_playrout(void)
{
  if (numsids == 1)
  {
    playroutine();
  }
  else if (numsids == 2)
  {
    playroutine_stereo();
  }

#ifdef USE_EXSID
  if (useexsid)
  {
    exSID_delay(exsidfd, exsidDelay);
    for (c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);
      exSID_clkdwrite(exsidfd, SIDWRITEDELAY, o, sidreg[o]);
    }
  }
#endif
}

void sound_mixer(Sint32 *dest, unsigned samples)
{
  size_t s;
  int c;

  if (!initted) return;
  if (samples > MIXBUFFERSIZE) return;

  if (numsids == 1)
  {
    if (!buffer) return;
    sid_fillbuffer(buffer, samples);
    if (writehandle)
    {
      fwrite(buffer, samples * sizeof(Uint16), 1, writehandle);
    }

    for (c = 0; c < samples; c++)
    {
      dest[c] = buffer[c];
    }
  }
  else if (numsids == 2)
  {
    sid_fillbuffer_stereo(lbuffer, rbuffer, samples);
    if (writehandle)
    {
      for (c = 0; c < samples; c++)
      {
        fwrite(&lbuffer[c], sizeof(Sint16), 1, writehandle);
        fwrite(&rbuffer[c], sizeof(Sint16), 1, writehandle);
      }
    }
    if (monomode)
    {
      for (c = 0; c < samples; c++)
      {
        dest[c*2] = lbuffer[c] / 2 + rbuffer[c] / 2;
        dest[c*2+1] = dest[c*2];
      }
    }
    else
    {
      for (c = 0; c < samples; c++)
      {
        dest[c*2] = lbuffer[c] * panning + rbuffer[c] * (1-panning);
        dest[c*2+1] = rbuffer[c] * panning + lbuffer[c] * (1-panning);
      }
    }
  }
}
