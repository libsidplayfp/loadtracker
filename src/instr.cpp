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

#include "console.h"
#include "loadtrk.h"
#include "pattern.h"
#include "play.h"
#include "song.h"
#include "table.h"

#include "bme_main.h"

#include <cstring>

INSTR instrcopybuffer;
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
      std::memcpy(&instrcopybuffer, &instr[einum], sizeof(INSTR));
      clearinstr(einum);
    }
    break;

    case KEY_C:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      cutinstr = -1;
      std::memcpy(&instrcopybuffer, &instr[einum], sizeof(INSTR));
    }
    break;

    case KEY_S:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      std::memcpy(&instr[einum], &instrcopybuffer, sizeof(INSTR));
      if (cutinstr != -1)
      {
        for (int c = 0; c < MAX_PATT; c++)
        {
          for (int d = 0; d < getPattlen(c); d++)
            if (pattern[c][d*4+1] == cutinstr) pattern[c][d*4+1] = einum;
        }
      }
    }
    break;

    case KEY_V:
    if ((einum) && (shiftpressed) && (eipos < 9))
    {
      std::memcpy(&instr[einum], &instrcopybuffer, sizeof(INSTR));
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
    for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      previnstr();
    break;

    case KEY_PGDN:
    for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      nextinstr();
    break;

    case KEY_HOME:
    while (einum != 0) previnstr();
    break;

    case KEY_END:
    while (einum != MAX_INSTR-1) nextinstr();
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
      etlock ^= 1;
      validatetableview();
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

        if (instr[einum].ptr[eipos-2])
        {
          if ((eipos == 5) && (shiftpressed))
          {
            instr[einum].ptr[STBL] = makespeedtable(instr[einum].ptr[STBL], finevibrato, true) + 1;
            break;
          }
          pos = instr[einum].ptr[eipos-2] - 1;
        }
        else
        {
          pos = gettablelen(eipos-2);
          if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
          if (shiftpressed) instr[einum].ptr[eipos-2] = pos + 1;
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
  if ((eipos == 9) && (einum)) editstring(instr[einum].name, MAX_INSTRNAMELEN);
  if ((hexnybble >= 0) && (eipos < 9) && (einum))
  {
    unsigned char *ptr = &instr[einum].ad;
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
    if (!(instr[einum].gatetimer & 0x3f)) instr[einum].gatetimer |= 1;
  }
}


void clearinstr(int num)
{
  std::memset(&instr[num], 0, sizeof(INSTR));
  if (num)
  {
    if (multiplier)
      instr[num].gatetimer = 2 * multiplier;
    else
      instr[num].gatetimer = 1;

    instr[num].firstwave = 0x9;
  }
}

void gotoinstr(int i)
{
  if ((i < 0) || (i >= MAX_INSTR)) return;

  einum = i;
  showinstrtable();

  editmode = EDIT_INSTRUMENT;
}

void nextinstr()
{
  einum++;
  if (einum >= MAX_INSTR) einum = MAX_INSTR - 1;
  if ((einum - eirow) >= 5) eirow++;
  showinstrtable();
}

void previnstr()
{
  einum--;
  if (einum < 0) einum = 0;
  if ((einum - eirow) < 0) eirow--;
  showinstrtable();
}

void showinstrtable()
{
  if (!etlock)
  {
    for (int c = MAX_TABLES-1; c >= 0; c--)
    {
      if (instr[einum].ptr[c])
        settableviewfirst(c, instr[einum].ptr[c] - 1);
    }
  }
}

