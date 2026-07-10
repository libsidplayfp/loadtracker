/*
 * LoadTracker
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// =============================================================================
// sound routines
// =============================================================================

#define SOUND_C

#include "loadtrk.h"
#include "bme_snd.h"

#include <SDL3/SDL.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#ifdef USE_EXSID
#  include <exSID.h>
#endif

#include <cstdlib>
#include <cstdio>

#define MINMIXRATE 11025
#define MAXMIXRATE 48000

#define MIXBUFFERSIZE 65536

// General / reSID output
bool useexsid = false;
bool initted = false;
unsigned framerate = PALFRAMERATE;
Sint16 *buffer = nullptr;
Sint16 *lbuffer = nullptr;
Sint16 *rbuffer = nullptr;
FILE *writehandle = nullptr;
SDL_TimerID timer = 0;

void sound_playrout(void);
void sound_mixer(Sint32 *dest, unsigned samples);
Uint32 sound_timer(void *userdata, SDL_TimerID timerID, Uint32 interval);

#ifdef USE_EXSID
void* exsidfd = nullptr;
unsigned exsidDelay = 0;
#endif

int sound_init(unsigned mr, bool writer, unsigned m, unsigned ntsc,
               unsigned multiplier, unsigned interpolate, unsigned customclockrate,
               unsigned exsid, float filterbias, unsigned combwaves)
{
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

    useexsid = true;
    timer = SDL_AddTimer(1000 / framerate, sound_timer, nullptr);
    goto SOUNDOK;
  }
#endif

  if (!buffer) buffer = new Sint16[MIXBUFFERSIZE * sizeof(Sint16)];
  if (!lbuffer) lbuffer = new Sint16[MIXBUFFERSIZE * sizeof(Sint16)];
  if (!rbuffer) rbuffer = new Sint16[MIXBUFFERSIZE * sizeof(Sint16)];

  if (writer)
    writehandle = std::fopen("sidaudio.raw", "wb");

  int playspeed = mr;
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

#ifdef USE_EXSID
SOUNDOK:
#endif
  initted = true;
  return 1;
}

void sound_uninit(void)
{
  if (!initted) return;
  initted = false;

  // Apparently a delay is needed to make sure the sound timer thread is
  // not mixing stuff anymore, and we can safely delete related structures
  SDL_Delay(50);

  if (useexsid)
  {
    SDL_RemoveTimer(timer);
  }
  else
  {
    snd_setcustommixer(nullptr);
    snd_player = nullptr;
  }

  if (writehandle)
  {
    std::fclose(writehandle);
    writehandle = nullptr;
  }

  if (buffer)
  {
    delete [] buffer;
    buffer = nullptr;
  }

  if (lbuffer)
  {
    delete [] lbuffer;
    lbuffer = nullptr;
  }

  if (rbuffer)
  {
    delete [] rbuffer;
    rbuffer = nullptr;
  }

#ifdef USE_EXSID
  if (useexsid)
  {
    if (exsidfd != nullptr)
    {
      for (int c = 0; c < NUMSIDREGS; c++)
      {
        exSID_clkdwrite(exsidfd, SIDWRITEDELAY, c, 0x00);
      }
    }

    exSID_audio_op(exsidfd, XS_AU_MUTE);
    exSID_exit(exsidfd);

    exSID_free(exsidfd);
    exsidfd = nullptr;
  }
#endif
}

Uint32 sound_timer(void *userdata, SDL_TimerID timerID, Uint32 interval)
{
  if (!initted) return interval;
  sound_playrout();
  return interval;
}

void sound_playrout()
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
    for (int c = 0; c < NUMSIDREGS; c++)
    {
      unsigned o = sid_getorder(c);
      exSID_clkdwrite(exsidfd, SIDWRITEDELAY, o, sidreg[o]);
    }
  }
#endif
}

void sound_mixer(Sint32 *dest, unsigned samples)
{
  if (!initted) return;
  if (samples > MIXBUFFERSIZE) return;

  if (numsids == 1)
  {
    if (!buffer) return;
    sid_fillbuffer(buffer, samples);
    if (writehandle)
    {
      std::fwrite(buffer, samples * sizeof(Uint16), 1, writehandle);
    }

    for (unsigned c = 0; c < samples; c++)
    {
      dest[c] = buffer[c];
    }
  }
  else if (numsids == 2)
  {
    sid_fillbuffer_stereo(lbuffer, rbuffer, samples);
    if (writehandle)
    {
      for (unsigned c = 0; c < samples; c++)
      {
        std::fwrite(&lbuffer[c], sizeof(Sint16), 1, writehandle);
        std::fwrite(&rbuffer[c], sizeof(Sint16), 1, writehandle);
      }
    }
    if (monomode)
    {
      for (unsigned c = 0; c < samples; c++)
      {
        dest[c*2] = lbuffer[c] / 2 + rbuffer[c] / 2;
        dest[c*2+1] = dest[c*2];
      }
    }
    else
    {
      for (unsigned c = 0; c < samples; c++)
      {
        dest[c*2] = lbuffer[c] * panning + rbuffer[c] * (1-panning);
        dest[c*2+1] = rbuffer[c] * panning + lbuffer[c] * (1-panning);
      }
    }
  }
}
