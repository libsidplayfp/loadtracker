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
// instrument editor
// =============================================================================

#define INSTR_C

#include "instr.h"

#include "configfile.h"
#include "console.h"
#include "loadtrk.h"
#include "pattern.h"
#include "play.h"
#include "song.h"
#include "table.h"

#include "bme_main.h"

#include <cstring>

Instr instrcopybuffer;
int cutinstr = -1;

int einum;
int eipos;
int eirow;
int eicolumn;

void instrumentcommands()
{
  switch(rawkey)
  {
    case KEY_CANC:
    case KEY_DEL:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      deleteinstrtable(einum);
      clearinstr(einum);
    }
    break;

    case KEY_X:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      cutinstr = einum;
      std::memcpy(&instrcopybuffer, &song.instr[einum], sizeof(Instr));
      clearinstr(einum);
    }
    break;

    case KEY_C:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      cutinstr = -1;
      std::memcpy(&instrcopybuffer, &song.instr[einum], sizeof(Instr));
    }
    break;

    case KEY_S:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      std::memcpy(&song.instr[einum], &instrcopybuffer, sizeof(Instr));
      if (cutinstr != -1)
      {
        for (int c = 0; c < MAX_PATT; c++)
        {
          for (int d = 0; d < getPattlen(c); d++)
            if (song.pattern[c][d*4+1] == cutinstr) song.pattern[c][d*4+1] = einum;
        }
      }
    }
    break;

    case KEY_V:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      std::memcpy(&song.instr[einum], &instrcopybuffer, sizeof(Instr));
    }
    break;

    case KEY_RIGHT:
    if (eipos < 9)
    {
      eipos++;
      if (eipos >= 9) eipos -= 10;
      if (eipos < 0) eipos = 0;
    }
    break;

    case KEY_LEFT:
    if (eipos < 9)
    {
      eipos--;
      if (eipos < 0) eipos += 10;
      if (eipos > 8) eipos = 8;
    }
    break;

    case KEY_DOWN:
    nextinstr();
    break;

    case KEY_UP:
    previnstr();
    break;

    case KEY_PGUP:
    previnstr(PGUPDNREPEAT);
    break;

    case KEY_PGDN:
    nextinstr(PGUPDNREPEAT);
    break;

    case KEY_HOME:
    previnstr(einum-1);
    break;

    case KEY_END:
    nextinstr(MAX_INSTR-1-einum);
    break;

    case KEY_N:
    if ((eipos != 9) && (shiftpressed))
    {
      eipos = 9;
      return;
    }
    break;

    case KEY_U:
    if (shiftpressed)
    {
      tables.fliplock();
      tables.validatetableview();
    }
    break;

    case KEY_SPACE:
    if (eipos != 9)
    {
      if (!shiftpressed)
        playtestnote(FIRSTNOTE + epoctave * 12, einum, epchn);
      else
        releasenote(epchn);
    }
    break;

    case KEY_ENTER:
    if (!einum) break;
    switch(eipos)
    {
      case 2:
      case 3:
      case 4:
      case 5:
      {
        int pos;

        if (song.instr[einum].ptr[eipos-2])
        {
          if ((eipos == 5) && (shiftpressed))
          {
            song.instr[einum].ptr[STBL] = makespeedtable(song.instr[einum].ptr[STBL], finevibrato, true) + 1;
            break;
          }
          pos = song.instr[einum].ptr[eipos-2] - 1;
        }
        else
        {
          pos = gettablelen(eipos-2);
          if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
          if (shiftpressed) song.instr[einum].ptr[eipos-2] = pos + 1;
        }
        gototable(eipos-2, pos);
      }
      return;

      case 9:
      eipos = 0;
      break;
    }
    break;
  }
  if ((eipos == 9) && (einum)) editstring(song.instr[einum].name, MAX_INSTRNAMELEN);
  if ((hexnybble >= 0) && (eipos < 9) && (einum))
  {
    unsigned char *ptr = &song.instr[einum].ad;
    ptr += eipos;

    switch(eicolumn)
    {
      case 0:
      *ptr &= 0x0f;
      *ptr |= hexnybble << 4;
      eicolumn++;
      break;

      case 1:
      *ptr &= 0xf0;
      *ptr |= hexnybble;
      eicolumn++;
      if (eicolumn > 1)
      {
        eicolumn = 0;
        eipos++;
        if (eipos >= 9) eipos = 0;
      }
      break;
    }
  }
  // Validate instrument parameters
  if (einum)
  {
    if (!(song.instr[einum].gatetimer & 0x3f)) song.instr[einum].gatetimer |= 1;
  }
}


void clearinstr(int num)
{
  std::memset(&song.instr[num], 0, sizeof(Instr));
  if (num)
  {
    if (multiplier)
      song.instr[num].gatetimer = 2 * multiplier;
    else
      song.instr[num].gatetimer = 1;

    song.instr[num].firstwave = 0x9;
  }
}

void clearinstr()
{
    for (int c = 0; c < MAX_INSTR; c++)
      clearinstr(c);
    std::memset(&instrcopybuffer, 0, sizeof(Instr));
    eipos = 0;
    eicolumn = 0;
    eirow = 1;
    einum = 1;
}

void gotoinstr(int i)
{
  if ((i < 0) || (i >= MAX_INSTR)) return;

  einum = i;
  showinstrtable();

  editmode = EDIT_INSTRUMENT;
}

void nextinstr(int n)
{
  einum+=n;
  if (einum >= MAX_INSTR) einum = MAX_INSTR - 1;
  while ((einum - eirow) >= 5) eirow++;
  showinstrtable();
}

void previnstr(int n)
{
  einum-=n;
  if (einum < 1) einum = 1;
  while ((einum - eirow) < 0) eirow--;
  showinstrtable();
}

void showinstrtable()
{
  if (!tables.islocked())
  {
    for (int c = MAX_TABLES-1; c >= 0; c--)
    {
      if (song.instr[einum].ptr[c])
        tables.settableviewfirst(c, song.instr[einum].ptr[c] - 1);
    }
  }
}

