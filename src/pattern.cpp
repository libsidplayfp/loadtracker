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
// pattern editor
// =============================================================================

#define PATTERN_C

#include "globals.h"
#include "console.h"
#include "input.h"
#include "display.h"
#include "instr.h"
#include "order.h"
#include "pattern.h"
#include "reloc.h"
#include "settings.h"
#include "song.h"
#include "table.h"
#include "tuning.h"

#include <utility>

#include <cstring>

int epnum[MAX_CHN];
int eppos;
int epview[MAX_CHN];
int epchn;
int autoadvance = 0;
Selection epmark;

void insertnote(int newnote)
{
    if (recordmode)
    {
        if (eppos < getPattlen(epnum[epchn]))
        {
            song.pattern[epnum[epchn]][eppos*4] = newnote;
            if (newnote < REST)
            {
                song.pattern[epnum[epchn]][eppos*4+1] = einum;
            }
            else
            {
                song.pattern[epnum[epchn]][eppos*4+1] = 0;
            }
            if ((input.shiftpressed) && (newnote == REST))
            {
                song.pattern[epnum[epchn]][eppos*4+2] = 0;
                song.pattern[epnum[epchn]][eppos*4+3] = 0;
            }
        }

        if (autoadvance < 2)
        {
            eppos++;
            if (eppos > getPattlen(epnum[epchn]))
            {
                eppos = 0;
            }
        }
    }
}

void patterndown(int n)
{
  if (input.shiftpressed)
  {
    if ((epmark.chn != epchn) || (eppos != epmark.end))
    {
      epmark.chn = epchn;
      epmark.start = epmark.end = eppos;
    }
  }
  eppos+=n;
  if (eppos > getPattlen(epnum[epchn]))
  {
    eppos = 0;
  }
  if (input.shiftpressed) epmark.end = eppos;
  updateview();
}

void patternup(int n)
{
  if (input.shiftpressed)
  {
    if ((epmark.chn != epchn) || (eppos != epmark.end))
    {
      epmark.chn = epchn;
      epmark.start = epmark.end = eppos;
    }
  }
  eppos-=n;
  if (eppos < 0)
  {
    eppos = getPattlen(epnum[epchn]);
  }
  if (input.shiftpressed) epmark.end = eppos;
  updateview();
}

void prevpattern()
{
  if (epnum[epchn] > 0)
  {
    epnum[epchn]--;
    if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
  }
  if (epchn == epmark.chn) epmark.chn = -1;
}

void nextpattern()
{
  if (epnum[epchn] < MAX_PATT-1)
  {
    epnum[epchn]++;
    if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
  }
  if (epchn == epmark.chn) epmark.chn = -1;
}

void shrinkpattern()
{
  int c = epnum[epchn];

  if (getPattlen(c) < 2) return;

  int l = getPattlen(c);
  int nl = l/2;

  for (int d = 0; d < nl; d++)
  {
    song.pattern[c][d*4] = song.pattern[c][d*2*4];
    song.pattern[c][d*4+1] = song.pattern[c][d*2*4+1];
    song.pattern[c][d*4+2] = song.pattern[c][d*2*4+2];
    song.pattern[c][d*4+3] = song.pattern[c][d*2*4+3];
  }

  song.pattern[c][nl*4] = ENDPATT;
  song.pattern[c][nl*4+1] = 0;
  song.pattern[c][nl*4+2] = 0;
  song.pattern[c][nl*4+3] = 0;

  eppos /= 2;

  countthispattern();
}

void expandpattern()
{
  int c = epnum[epchn];
  int l = getPattlen(c);
  int nl = l*2;
  if (nl > MAX_PATTROWS) return;

  unsigned char temp[MAX_PATTROWS*4+4];
  std::memset(temp, 0, sizeof temp);

  for (int d = 0; d <= nl; d++)
  {
    if (d & 1)
    {
      temp[d*4] = REST;
      temp[d*4+1] = 0;
      temp[d*4+2] = 0;
      temp[d*4+3] = 0;
    }
    else
    {
      temp[d*4] = song.pattern[c][d*2];
      temp[d*4+1] = song.pattern[c][d*2+1];
      temp[d*4+2] = song.pattern[c][d*2+2];
      temp[d*4+3] = song.pattern[c][d*2+3];
    }
  }

  std::memcpy(song.pattern[c], temp, (nl+1)*4);

  eppos *= 2;

  countthispattern();
}

void splitpattern()
{
  int c = epnum[epchn];
  int l = getPattlen(c);
  int maxChns = config.getMaxChannels();

  if ((eppos == 0) || (eppos == l)) return;

  if (insertpattern(c))
  {
    int oldesnum = esnum;
    int oldeschn = eschn;
    int oldeseditpos = eseditpos;

    for (int d = eppos; d <= l; d++)
    {
      song.pattern[c+1][(d-eppos)*4] = song.pattern[c][d*4];
      song.pattern[c+1][(d-eppos)*4+1] = song.pattern[c][d*4+1];
      song.pattern[c+1][(d-eppos)*4+2] = song.pattern[c][d*4+2];
      song.pattern[c+1][(d-eppos)*4+3] = song.pattern[c][d*4+3];
    }
    song.pattern[c][eppos*4] = ENDPATT;
    song.pattern[c][eppos*4+1] = 0;
    song.pattern[c][eppos*4+2] = 0;
    song.pattern[c][eppos*4+3] = 0;

    countpatternlengths();

    for (esnum = 0; esnum < MAX_SONGS; esnum++)
    {
      for (eschn = 0; eschn < maxChns; eschn++)
      {
        for (eseditpos = 0; eseditpos < song.order[esnum][eschn].len; eseditpos++)
        {
          if (song.order[esnum][eschn].data[eseditpos] == c)
          {
            song.order[esnum][eschn].data[eseditpos] = c+1;
            insertorder(c);
          }
        }
      }
    }
    eschn = oldeschn;
    eseditpos = oldeseditpos;
    esnum = oldesnum;
  }
}

void joinpattern()
{
  int c = epnum[epchn];
  int maxChns = config.getMaxChannels();

  if (eschn != epchn) return;
  if (song.order[esnum][epchn].data[eseditpos] != c) return;

  int d = song.order[esnum][epchn].data[eseditpos + 1];
  if (d >= MAX_PATT) return;
  if (getPattlen(c) + getPattlen(d) > MAX_PATTROWS) return;

  if (insertpattern(c))
  {
    int oldesnum = esnum;
    int oldeschn = eschn;
    int oldeseditpos = eseditpos;
    d++;

    int e;
    for (e = 0; e < getPattlen(c); e++)
    {
      song.pattern[c+1][e*4] = song.pattern[c][e*4];
      song.pattern[c+1][e*4+1] = song.pattern[c][e*4+1];
      song.pattern[c+1][e*4+2] = song.pattern[c][e*4+2];
      song.pattern[c+1][e*4+3] = song.pattern[c][e*4+3];
    }
    for (int f = 0; f < getPattlen(d); f++)
    {
      song.pattern[c+1][e*4] = song.pattern[d][f*4];
      song.pattern[c+1][e*4+1] = song.pattern[d][f*4+1];
      song.pattern[c+1][e*4+2] = song.pattern[d][f*4+2];
      song.pattern[c+1][e*4+3] = song.pattern[d][f*4+3];
      e++;
    }
    song.pattern[c+1][e*4] = ENDPATT;
    song.pattern[c+1][e*4+1] = 0;
    song.pattern[c+1][e*4+2] = 0;
    song.pattern[c+1][e*4+3] = 0;

    countpatternlengths();

    for (esnum = 0; esnum < MAX_SONGS; esnum++)
    {
      for (eschn = 0; eschn < maxChns; eschn++)
      {
        for (eseditpos = 0; eseditpos < song.order[esnum][eschn].len; eseditpos++)
        {
          if ((song.order[esnum][eschn].data[eseditpos] == c) && (song.order[esnum][eschn].data[eseditpos+1] == d))
          {
            deleteorder();
            song.order[esnum][eschn].data[eseditpos] = c+1;
          }
        }
      }
    }
    eschn = oldeschn;
    eseditpos = oldeseditpos;
    esnum = oldesnum;

    findusedpatterns();
    {
      bool del1 = pattused.test(c);
      bool del2 = pattused.test(d);

      if (!del1)
      {
        deletepattern(c);
        if (d > c) d--;
      }
      if (!del2) 
        deletepattern(d);
    }
  }
}

void updateview()
{
    for (int i=0; i<MAX_CHN; i++)
        epview[i] = eppos-VISIBLEPATTROWS/2;
}
