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
// reSIDfp interface
// =============================================================================

#define SID_C

#include "sid.h"

#include "sound.h"

#include <residfp/residfp.h>

#include <cstdlib>

#define SIDWAVEDELAY 4 // and $xxxx,x 4 cycles extra

int clockrate;
int samplerate;
unsigned char sidreg[NUMSIDREGS];
unsigned char sidreg2[NUMSIDREGS];
unsigned char sidorder[] =
  {0x15,0x16,0x18,0x17,
   0x05,0x06,0x02,0x03,0x00,0x01,0x04,
   0x0c,0x0d,0x09,0x0a,0x07,0x08,0x0b,
   0x13,0x14,0x10,0x11,0x0e,0x0f,0x12};

unsigned char altsidorder[] =
  {0x15,0x16,0x18,0x17,
   0x04,0x00,0x01,0x02,0x03,0x05,0x06,
   0x0b,0x07,0x08,0x09,0x0a,0x0c,0x0d,
   0x12,0x0e,0x0f,0x10,0x11,0x13,0x14};

reSIDfp::residfp *sid = nullptr;
reSIDfp::residfp *sid2 = nullptr;

extern unsigned residdelay;
extern unsigned adparam;

void sid_init(int speed, unsigned m,
              unsigned ntsc, unsigned interpolate,
              unsigned customclockrate, unsigned numsids,
              float filterbias, unsigned combwaves)
{
  if (customclockrate)
    clockrate = customclockrate;
  else
    clockrate = ntsc ? NTSCCLOCKRATE : PALCLOCKRATE;

  samplerate = speed;

  if (!sid) sid = new reSIDfp::residfp;
  if (numsids == 2 && !sid2) sid2 = new reSIDfp::residfp;

  switch(interpolate)
  {
    case 0:
        sid->setSamplingParameters(clockrate, reSIDfp::DECIMATE, speed);
        if (sid2) sid2->setSamplingParameters(clockrate, reSIDfp::DECIMATE, speed);
        break;

    default:
    case 1:
        sid->setSamplingParameters(clockrate, reSIDfp::RESAMPLE, speed);
        if (sid2) sid2->setSamplingParameters(clockrate, reSIDfp::RESAMPLE, speed);
        break;
  }

  sid->reset();
  sid->setFilter6581Curve(filterbias);
  sid->setFilter8580Curve(filterbias);
  if (sid2)
  {
    sid2->reset();
    sid2->setFilter6581Curve(filterbias);
    sid2->setFilter8580Curve(filterbias);
  }
  switch(combwaves)
  {
    case 0:
        sid->setCombinedWaveforms(reSIDfp::WEAK);
        if (sid2) sid2->setCombinedWaveforms(reSIDfp::WEAK);
        break;

    default:
    case 1:
        sid->setCombinedWaveforms(reSIDfp::AVERAGE);
        if (sid2) sid2->setCombinedWaveforms(reSIDfp::AVERAGE);
        break;

    case 2:
        sid->setCombinedWaveforms(reSIDfp::STRONG);
        if (sid2) sid2->setCombinedWaveforms(reSIDfp::STRONG);
        break;
  }


  for (int c = 0; c < NUMSIDREGS; c++)
  {
    sidreg[c] = 0x00;
    sidreg2[c] = 0x00;
  }
  if (m == 1)
  {
    sid->setChipModel(reSIDfp::CSG8580);
    if (sid2) sid2->setChipModel(reSIDfp::CSG8580);
  }
  else
  {
    sid->setChipModel(reSIDfp::MOS6581);
    if (sid2) sid2->setChipModel(reSIDfp::MOS6581);
  }
}

unsigned char sid_getorder(unsigned char index)
{
  if (adparam >= 0xf000)
    return altsidorder[index];
  else
    return sidorder[index];
}

int sid_fillbuffer(short *ptr, int samples)
{
  if (!sid)
    return 0;

  int tdelta2;
  int result = 0;
  int total = 0;

  int badline = std::rand() % NUMSIDREGS;

  int tdelta = clockrate * samples / samplerate;
  if (tdelta <= 0) return total;

  for (int c = 0; c < NUMSIDREGS; c++)
  {
    unsigned char o = sid_getorder(c);

    // Possible random badline delay once per writing
    if ((badline == c) && (residdelay))
    {
      tdelta2 = residdelay;
      result = sid->clock(tdelta2, ptr);
      total += result;
      ptr += result;
      samples -= result;
      tdelta -= residdelay;
    }

    sid->write(o, sidreg[o]);

    tdelta2 = SIDWRITEDELAY;
    result = sid->clock(tdelta2, ptr);
    total += result;
    ptr += result;
    samples -= result;
    tdelta -= SIDWRITEDELAY;

    if (tdelta <= 0) return total;
  }

  result = sid->clock(tdelta, ptr);
  total += result;
  ptr += result;
  samples -= result;

  // Loop extra cycles until all samples produced
  while (samples)
  {
    tdelta = clockrate * samples / samplerate;
    if (tdelta <= 0) return total;

    result = sid->clock(tdelta, ptr);
    total += result;
    ptr += result;
    samples -= result;
  }

  return total;
}

int sid_fillbuffer_stereo(short *lptr, short *rptr, int samples)
{
  if (!sid || !sid2)
    return 0;

  int tdelta2;
  int result = 0;
  int total = 0;

  int badline = rand() % NUMSIDREGS;

  int tdelta = clockrate * samples / samplerate;
  if (tdelta <= 0) return total;

  for (int c = 0; c < NUMSIDREGS; c++)
  {
    unsigned char o = sid_getorder(c);

    // Extra delay for loading the waveform (and mt_chngate,x)
    if ((o == 4) || (o == 11) || (o == 18))
    {
        tdelta2 = SIDWAVEDELAY;
        result = sid->clock(tdelta2, lptr);
        tdelta2 = SIDWAVEDELAY;
        sid2->clock(tdelta2, rptr);

        total += result;
        lptr += result;
        rptr += result;
        samples -= result;
        tdelta -= SIDWAVEDELAY;
    }

    // Possible random badline delay once per writing
    if ((badline == c) && (residdelay))
    {
      tdelta2 = residdelay;
      result = sid->clock(tdelta2, lptr);
      tdelta2 = residdelay;
      result = sid2->clock(tdelta2, rptr);
      total += result;
      lptr += result;
      rptr += result;
      samples -= result;
      tdelta -= residdelay;
    }

    sid->write(o, sidreg[o]);
    sid2->write(o, sidreg2[o]);

    tdelta2 = SIDWRITEDELAY-5;
    result = sid->clock(tdelta2, lptr);
    tdelta2 = SIDWRITEDELAY-5;
    result = sid2->clock(tdelta2, rptr);
    total += result;
    lptr += result;
    rptr += result;
    samples -= result;
    tdelta -= SIDWRITEDELAY;

    if (tdelta <= 0) return total;
  }

  tdelta2 = tdelta;
  result = sid->clock(tdelta, lptr);
  tdelta2 = tdelta;
  result = sid2->clock(tdelta, rptr);
  total += result;
  lptr += result;
  rptr += result;
  samples -= result;

  // Loop extra cycles until all samples produced
  while (samples)
  {
    tdelta = clockrate * samples / samplerate;
    if (tdelta <= 0) return total;

    result = sid->clock(tdelta, lptr);
    tdelta = clockrate * samples / samplerate;
    result = sid2->clock(tdelta, rptr);
    total += result;
    lptr += result;
    rptr += result;
    samples -= result;
  }

  return total;
}
