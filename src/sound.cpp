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

#include "sound.h"

#include "configfile.h"
#include "loadtrk.h"
#include "ltmidi.h"
#include "play.h"
#include "sid.h"

#include "bme_snd.h"

#include <SDL3/SDL.h>

#ifdef _WIN32
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
bool initted = false;
unsigned framerate = PALFRAMERATE;
Sint16 *lbuffer = nullptr;
Sint16 *rbuffer = nullptr;
FILE *writehandle = nullptr;
SDL_TimerID stop_timer = 0;

#ifdef USE_EXSID
bool useexsid = false;
SDL_TimerID timer = 0;
void* exsidfd = nullptr;
unsigned exsidDelay = 0;
#endif

void sound_playrout();

void sound_mixer(Sint32 *dest, unsigned samples);
Uint32 sound_timer(void *userdata, SDL_TimerID timerID, Uint32 interval);
Uint32 sound_stop_callback(void *userdata, SDL_TimerID timerID, Uint32 interval);

bool sound_init(unsigned mr, bool writer, unsigned m, unsigned ntsc,
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
      return false;

    if (exSID_init(exsidfd) < 0)
      return false;

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
      return false;
    }

    exsidDelay /= framerate;
    exsidDelay -= SIDWRITEDELAY*NUMSIDREGS;

    useexsid = true;
    timer = SDL_AddTimer(1000 / framerate, sound_timer, nullptr);
    goto SOUNDOK;
  }
#else
    (void)exsid;
#endif

  int playspeed = mr;
  if (playspeed < MINMIXRATE) playspeed = MINMIXRATE;
  if (playspeed > MAXMIXRATE) playspeed = MAXMIXRATE;

  if (!snd_init(playspeed, numsids))
  {
    return false;
  }

  midi_init();

  if (!lbuffer) lbuffer = new Sint16[MIXBUFFERSIZE];
  if (!rbuffer) rbuffer = new Sint16[MIXBUFFERSIZE];

  if (writer)
    writehandle = std::fopen("sidaudio.raw", "wb");

  playspeed = getmixrate();
  sid_init(playspeed, m, ntsc, interpolate, customclockrate, numsids, filterbias, combwaves);

  snd_setplayer(&sound_playrout);
  snd_setcustommixer(sound_mixer);

#ifdef USE_EXSID
SOUNDOK:
#endif
  initted = true;
  return true;
}

void sound_uninit()
{
  if (!initted) return;
  initted = false;

  // Apparently a delay is needed to make sure the sound timer thread is
  // not mixing stuff anymore, and we can safely delete related structures
  SDL_Delay(50);

#ifdef USE_EXSID
  if (useexsid)
  {
    SDL_RemoveTimer(timer);
  }
  else
#endif
  {
    snd_setcustommixer(nullptr);
    snd_setplayer(nullptr);
  }

  if (writehandle)
  {
    std::fclose(writehandle);
    writehandle = nullptr;
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

  snd_uninit();
  midi_uninit();
}

Uint32 sound_timer(void*, SDL_TimerID, Uint32 interval)
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
    if (!lbuffer) return;
    sid_fillbuffer(lbuffer, samples);
    if (writehandle)
    {
      std::fwrite(lbuffer, samples * sizeof(Sint16), 1, writehandle);
    }

    for (unsigned c = 0; c < samples; c++)
    {
      dest[c] = lbuffer[c];
    }
  }
  else if (numsids == 2)
  {
    sid_fillbuffer_stereo(lbuffer, rbuffer, samples);
    if (writehandle)
    {
      for (unsigned c = 0; c < samples; c++)
      {
        // FIXME should be interleaved?
        std::fwrite(&lbuffer[c], sizeof(Sint16), 1, writehandle);
        std::fwrite(&rbuffer[c], sizeof(Sint16), 1, writehandle);
      }
    }
    if (monomode)
    {
      for (unsigned c = 0; c < samples; c++)
      {
        constexpr double SQRT_2 = 1.41421356237;
        Sint32 sample = (lbuffer[c] + rbuffer[c]) / SQRT_2; // FIXME limit insted of scaling
        dest[c*2] = sample;
        dest[c*2+1] = sample;
      }
    }
    else
    {
      for (unsigned c = 0; c < samples; c++)
      {
        Sint32 ls = lbuffer[c];
        Sint32 rs = rbuffer[c];
        dest[c*2] = (ls-rs) * panning + rs;
        dest[c*2+1] = (rs-ls) * panning + ls;
      }
    }
  }
}

void sound_start()
{
    if (stop_timer)
    {
        SDL_RemoveTimer(stop_timer);
        stop_timer = 0;
    }
    snd_play();
}

void sound_stop(bool immediate)
{
    if (immediate)
        snd_stop();
    else
        stop_timer = SDL_AddTimer(30000, sound_stop_callback, NULL);
}

Uint32 sound_stop_callback(void *, SDL_TimerID, Uint32)
{
    snd_stop();
    return 0;
}
