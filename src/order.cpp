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
// orderlist & songname editor
// =============================================================================

#define ORDER_C

#include "order.h"

#include "common.h"
#include "globals.h"
#include "console.h"
#include "display.h"
#include "pattern.h"
#include "play.h"
#include "settings.h"
#include "song.h"

#include <utility>

#include <cstring>

int espos[MAX_CHN];
int esend[MAX_CHN];
int eseditpos;
int esview[MAX_CHN];
int escolumn;
int eschn;
int esnum;
Selection esmark;

void insertorder(unsigned char byte)
{
  int sl = song.len[esnum][eschn];
  if ((sl - eseditpos)-1 >= 0)
  {
    if (sl < MAX_SONGLEN)
    {
      int len = sl+1;
      song.order[esnum][eschn][len+1] = song.order[esnum][eschn][len];
      song.order[esnum][eschn][len] = LOOPSONG;
      if (len) song.order[esnum][eschn][len-1] = byte;
      countthispattern();
    }
    std::memmove(&song.order[esnum][eschn][eseditpos+1],
      &song.order[esnum][eschn][eseditpos],
      (sl - eseditpos)-1);
    song.order[esnum][eschn][eseditpos] = byte;
    int len = sl+1;
    if ((song.order[esnum][eschn][len] > eseditpos) &&
        (song.order[esnum][eschn][len] < (len-2)))
       song.order[esnum][eschn][len]++;
  }
  else
  {
    if (eseditpos > sl)
    {
      if (sl < MAX_SONGLEN)
      {
        song.order[esnum][eschn][eseditpos+1] = song.order[esnum][eschn][eseditpos];
        song.order[esnum][eschn][eseditpos] = LOOPSONG;
        if (eseditpos) song.order[esnum][eschn][eseditpos-1] = byte;
        countthispattern();
        eseditpos = sl+1;
      }
    }
  }
}

void deleteorder()
{
  int sl = song.len[esnum][eschn];
  if ((sl - eseditpos)-1 >= 0)
  {
    std::memmove(&song.order[esnum][eschn][eseditpos],
      &song.order[esnum][eschn][eseditpos+1],
      (sl - eseditpos)-1);
    song.order[esnum][eschn][sl-1] = 0x00;
    if (sl > 0)
    {
      song.order[esnum][eschn][sl-1] = song.order[esnum][eschn][sl];
      song.order[esnum][eschn][sl] = song.order[esnum][eschn][sl+1];
      countthispattern();
    }
    if (eseditpos == sl) eseditpos++;
    int len = sl+1;
    if ((song.order[esnum][eschn][len] > eseditpos) &&
        (song.order[esnum][eschn][len] > 0))
       song.order[esnum][eschn][len]--;
  }
  else
  {
    if (eseditpos > sl)
    {
      if (sl > 0)
      {
        song.order[esnum][eschn][sl-1] = song.order[esnum][eschn][sl];
        song.order[esnum][eschn][sl] = song.order[esnum][eschn][sl+1];
        countthispattern();
        eseditpos = sl+1;
      }
    }
  }
}

void nextsong()
{
  esnum++;
  if (esnum >= MAX_SONGS) esnum = MAX_SONGS - 1;
  songchange();
}

void prevsong()
{
  esnum--;
  if (esnum < 0) esnum = 0;
  songchange();
}

void songchange()
{
  int maxChns = config.getMaxChannels();

  int currentSonglen = song.len[esnum][eschn];

  for (int c = 0; c < maxChns; c++)
  {
    espos[c] = 0;
    esend[c] = 0;
    epnum[c] = c;
  }
  updateviewtopos();

  eppos = 0;
  for (int i=0; i<MAX_CHN; i++)
  {
    epview[i] = - VISIBLEPATTROWS/2;
    esview[i] = 0;
  }
  eseditpos = 0;
  if (eseditpos == currentSonglen) eseditpos++;
  epmark.chn = -1;
  esmark.chn = -1;
  stopsong();
}

void updateviewtopos()
{
  int maxChns = config.getMaxChannels();

  for (int c = 0; c < maxChns; c++)
  {
    int currentSonglen = song.len[esnum][c];
    for (int d = espos[c]; d < currentSonglen; d++)
    {
      int currentSongorder =  song.order[esnum][c][d];
      if (currentSongorder < MAX_PATT)
      {
        epnum[c] = currentSongorder;
        break;
      }
    }
  }
}
