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

#include "console.h"
#include "loadtrk.h"
#include "pattern.h"
#include "play.h"
#include "song.h"

#include "bme_main.h"

#include <utility>

#include <cstring>

unsigned char trackcopybuffer[MAX_SONGLEN+2];
int trackcopyrows = 0;
int trackcopywhole;
int trackcopyrpos;

int espos[MAX_CHN];
int esend[MAX_CHN];
int eseditpos;
int esview[MAX_CHN];
int escolumn;
int eschn;
int esnum;
int enpos;
Selection esmark;

void namecommands();
void orderleft();
void orderright();

void orderlistcommands()
{
  int maxChns = getMaxChannels();

  if (hexnybble >= 0)
  {
    if (eseditpos != song.len[esnum][eschn])
    {
      switch(escolumn)
      {
        case 0:
        song.order[esnum][eschn][eseditpos] &= 0x0f;
        song.order[esnum][eschn][eseditpos] |= hexnybble << 4;
        if (eseditpos < song.len[esnum][eschn])
        {
          if (song.order[esnum][eschn][eseditpos] >= MAX_PATT)
            song.order[esnum][eschn][eseditpos] = MAX_PATT - 1;
        }
        else
        {
          if (song.order[esnum][eschn][eseditpos] >= MAX_SONGLEN)
            song.order[esnum][eschn][eseditpos] = MAX_SONGLEN - 1;
        }
        break;

        case 1:
        song.order[esnum][eschn][eseditpos] &= 0xf0;
        if ((song.order[esnum][eschn][eseditpos] & 0xf0) == 0xd0)
        {
          hexnybble--;
          if (hexnybble < 0) hexnybble = 0xf;
        }
        if ((song.order[esnum][eschn][eseditpos] & 0xf0) == 0xe0)
        {
          hexnybble = 16 - hexnybble;
          hexnybble &= 0xf;
        }
        song.order[esnum][eschn][eseditpos] |= hexnybble;

        if (eseditpos < song.len[esnum][eschn])
        {
          if (song.order[esnum][eschn][eseditpos] == LOOPSONG)
            song.order[esnum][eschn][eseditpos] = LOOPSONG-1;
          if (song.order[esnum][eschn][eseditpos] == TRANSDOWN)
            song.order[esnum][eschn][eseditpos] = TRANSDOWN+0x0f;
        }
        else
        {
          if (song.order[esnum][eschn][eseditpos] >= MAX_SONGLEN)
            song.order[esnum][eschn][eseditpos] = MAX_SONGLEN - 1;
        }
        break;
      }
      escolumn++;
      if (escolumn > 1)
      {
        escolumn = 0;
        if (eseditpos < (song.len[esnum][eschn]+1))
        {
          eseditpos++;
          if (eseditpos == song.len[esnum][eschn]) eseditpos++;
        }
      }
    }
  }

  switch(key)
  {
    case 'R':
    if (eseditpos < song.len[esnum][eschn])
    {
      song.order[esnum][eschn][eseditpos] = REPEAT + 0x01;
      escolumn = 1;
    }
    break;

    case '+':
    if (eseditpos < song.len[esnum][eschn])
    {
      song.order[esnum][eschn][eseditpos] = TRANSUP;
      escolumn = 1;
    }
    break;

    case '-':
    if (eseditpos < song.len[esnum][eschn])
    {
      song.order[esnum][eschn][eseditpos] = TRANSDOWN + 0x0F;
      escolumn = 1;
    }
    break;

    case '>':
    case ')':
    case ']':
    nextsong();
    break;

    case '<':
    case '(':
    case '[':
    prevsong();
    break;
  }
  switch(rawkey)
  {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    if (shiftpressed)
    {
      int schn = eschn;
      int tchn = 0;

      esmark.chn = -1;
      if (rawkey == KEY_1) tchn = 0;
      if (rawkey == KEY_2) tchn = 1;
      if (rawkey == KEY_3) tchn = 2;
      if (schn != tchn)
      {
        int lentemp = song.len[esnum][schn];
        song.len[esnum][schn] = song.len[esnum][tchn];
        song.len[esnum][tchn] = lentemp;

        for (int c = 0; c < MAX_SONGLEN+2; c++)
        {
          std::swap(song.order[esnum][schn][c], song.order[esnum][tchn][c]);
        }
      }
    }
    break;

    case KEY_X:
    if (shiftpressed)
    {
      if (esmark.chn != -1)
      {
        int d = 0;

        eschn = esmark.chn;
        if (esmark.start <= esmark.end)
        {
          eseditpos = esmark.start;
          for (int c = esmark.start; c <= esmark.end; c++)
            trackcopybuffer[d++] = song.order[esnum][eschn][c];
          trackcopyrows = d;
        }
        else
        {
          eseditpos = esmark.end;
          for (int c = esmark.end; c <= esmark.start; c++)
            trackcopybuffer[d++] = song.order[esnum][eschn][c];
          trackcopyrows = d;
        }
        if (trackcopyrows == song.len[esnum][eschn])
        {
          trackcopywhole = 1;
          trackcopyrpos = song.order[esnum][eschn][song.len[esnum][eschn]+1];
        }
        else trackcopywhole = 0;
        for (int c = 0; c < trackcopyrows; c++) deleteorder();
        esmark.chn = -1;
      }
    }
    break;

    case KEY_C:
    if (shiftpressed)
    {
      if (esmark.chn != -1)
      {
        int d = 0;
        if (esmark.start <= esmark.end)
        {
          for (int c = esmark.start; c <= esmark.end; c++)
            trackcopybuffer[d++] = song.order[esnum][eschn][c];
          trackcopyrows = d;
        }
        else
        {
          for (int c = esmark.end; c <= esmark.start; c++)
            trackcopybuffer[d++] = song.order[esnum][eschn][c];
          trackcopyrows = d;
        }
        if (trackcopyrows == song.len[esnum][eschn])
        {
          trackcopywhole = 1;
          trackcopyrpos = song.order[esnum][eschn][song.len[esnum][eschn]+1];
        }
        else trackcopywhole = 0;
        esmark.chn = -1;
      }
    }
    break;

    case KEY_V:
    if (shiftpressed)
    {
      int oldlen = song.len[esnum][eschn];

      if (eseditpos < song.len[esnum][eschn])
      {
        for (int c = trackcopyrows-1; c >= 0; c--)
          insertorder(trackcopybuffer[c]);
      }
      else
      {
        for (int c = 0; c < trackcopyrows; c++)
          insertorder(trackcopybuffer[c]);
      }
      if ((trackcopywhole) && (!oldlen))
        song.order[esnum][eschn][song.len[esnum][eschn]+1] = trackcopyrpos;
    }
    break;

    case KEY_L:
    if (shiftpressed)
    {
      if (esmark.chn == -1)
      {
        esmark.chn = eschn;
        esmark.start = 0;
        esmark.end = song.len[esnum][eschn]-1;
      }
      else esmark.chn = -1;
    }
    break;


    case KEY_SPACE:
    if (!shiftpressed)
    {
      if (eseditpos < song.len[esnum][eschn]) espos[eschn] = eseditpos;
      if (esend[eschn] < espos[eschn]) esend[eschn] = 0;
    }
    else
    {
      for (int c = 0; c < maxChns; c++)
      {
        if (eseditpos < song.len[esnum][c]) espos[c] = eseditpos;
        if (esend[c] < espos[c]) esend[c] = 0;
      }
    }
    break;

    case KEY_BACKSPACE:
    if (!shiftpressed)
    {
      if ((esend[eschn] != eseditpos) && (eseditpos > espos[eschn]))
      {
        if (eseditpos < song.len[esnum][eschn]) esend[eschn] = eseditpos;
      }
      else esend[eschn] = 0;
    }
    else
    {
      if ((esend[eschn] != eseditpos) && (eseditpos > espos[eschn]))
      {
        for (int c = 0; c < maxChns; c++)
        {
          if (eseditpos < song.len[esnum][c]) esend[c] = eseditpos;
        }
      }
      else
      {
        for (int c = 0; c < maxChns; c++) esend[c] = 0;
      }
    }
    break;

    case KEY_ENTER:
    if (eseditpos < song.len[esnum][eschn])
    {
      if (!shiftpressed)
      {
        if (song.order[esnum][eschn][eseditpos] < MAX_PATT)
          epnum[eschn] = song.order[esnum][eschn][eseditpos];
      }
      else
      {
        for (int c = 0; c < maxChns; c++)
        {
          int start;

          if (eseditpos != espos[eschn]) start = eseditpos;
          else start = espos[c];

          for (int d = start; d < song.len[esnum][c]; d++)
          {
            if (song.order[esnum][c][d] < MAX_PATT)
            {
              epnum[c] = song.order[esnum][c][d];
              break;
            }
          }
        }
      }
      epmark.chn = -1;
    }
    epchn = eschn;
    epcolumn = 0;
    eppos = 0;
    for (int i=0; i<MAX_CHN; i++)
        epview[i] = - VISIBLEPATTROWS/2;
    editmode = EDIT_PATTERN;
    if (epchn == epmark.chn) epmark.chn = -1;
    break;

    case KEY_DEL:
    esmark.chn = -1;
    deleteorder();
    break;

    case KEY_INS:
    esmark.chn = -1;
    insertorder(0);
    break;

    case KEY_HOME:
    if (song.len[esnum][eschn])
    {
      while ((eseditpos != 0) || (escolumn != 0)) orderleft();
    }
    break;

    case KEY_END:
    while (eseditpos != song.len[esnum][eschn]+1) orderright();
    break;

    case KEY_PGUP:
    for (int scrrep = PGUPDNREPEAT * 2; scrrep; scrrep--)
      orderleft();
    break;

    case KEY_PGDN:
    for (int scrrep = PGUPDNREPEAT * 2; scrrep; scrrep--)
      orderright();
    break;

    case KEY_LEFT:
    orderleft();
    break;

    case KEY_RIGHT:
    orderright();
    break;

    case KEY_UP:
    eschn--;
    if (eschn < 0) eschn = maxChns - 1;
    if ((eseditpos == song.len[esnum][eschn]) || (eseditpos > song.len[esnum][eschn]+1))
    {
      eseditpos = song.len[esnum][eschn]+1;
      escolumn = 0;
    }
    if (shiftpressed) esmark.chn = -1;
    break;

    case KEY_DOWN:
    eschn++;
    if (eschn >= maxChns) eschn = 0;
    if ((eseditpos == song.len[esnum][eschn]) || (eseditpos > song.len[esnum][eschn]+1))
    {
      eseditpos = song.len[esnum][eschn]+1;
      escolumn = 0;
    }
    if (shiftpressed) esmark.chn = -1;
    break;
  }
  if (eseditpos - esview[eschn] < 0)
  {
    esview[eschn] = eseditpos;
  }
  int visibleOrderlist = getVisibleOrderlist();
  if (eseditpos - esview[eschn] >= visibleOrderlist)
  {
    esview[eschn] = eseditpos - visibleOrderlist + 1;
  }
}

void namecommands()
{
  switch(rawkey)
  {
    case KEY_DOWN:
    case KEY_ENTER:
    enpos++;
    if (enpos > 2) enpos = 0;
    break;

    case KEY_UP:
    enpos--;
    if (enpos < 0) enpos = 2;
    break;
  }
  switch(enpos)
  {
    case 0:
    editstring(song.title, MAX_STR);
    break;

    case 1:
    editstring(song.author, MAX_STR);
    break;

    case 2:
    editstring(song.released, MAX_STR);
    break;
  }
}

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

void orderleft()
{
  if ((shiftpressed) && (eseditpos < song.len[esnum][eschn]))
  {
    if ((esmark.chn != eschn) || (eseditpos != esmark.end))
    {
      esmark.chn = eschn;
      esmark.start = esmark.end = eseditpos;
    }
  }
  escolumn--;
  if (escolumn < 0)
  {
    if (eseditpos > 0)
    {
      eseditpos--;
      if (eseditpos == song.len[esnum][eschn]) eseditpos--;
      escolumn = 1;
      if (eseditpos < 0)
      {
        eseditpos = 1;
        escolumn = 0;
      }
    }
    else escolumn = 0;
  }
  if ((shiftpressed) && (eseditpos < song.len[esnum][eschn])) esmark.end = eseditpos;
}

void orderright()
{
  if ((shiftpressed) && (eseditpos < song.len[esnum][eschn]))
  {
    if ((esmark.chn != eschn) || (eseditpos != esmark.end))
    {
      esmark.chn = eschn;
      esmark.start = esmark.end = eseditpos;
    }
  }
  escolumn++;
  if (escolumn > 1)
  {
    escolumn = 0;
    if (eseditpos < (song.len[esnum][eschn]+1))
    {
      eseditpos++;
      if (eseditpos == song.len[esnum][eschn]) eseditpos++;
    }
    else escolumn = 1;
  }
  if ((shiftpressed) && (eseditpos < song.len[esnum][eschn])) esmark.end = eseditpos;
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
  int maxChns = getMaxChannels();

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
  int maxChns = getMaxChannels();

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
