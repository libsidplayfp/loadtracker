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
// table editor
// =============================================================================

#define TABLE_C

#include "configfile.h"
#include "console.h"
#include "instr.h"
#include "loadtrk.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "song.h"
#include "table.h"

#include "bme_main.h"

#include <utility>

#include <cmath>
#include <cstring>

unsigned char ltablecopybuffer[MAX_TABLELEN];
unsigned char rtablecopybuffer[MAX_TABLELEN];
int tablecopyrows = 0;

Tables tables;

void inserttable(int num, int pos, int mode);
void deletetable(int num, int pos);

void tablecommands()
{
  switch(rawkey)
  {
    case KEY_Q:
    if ((shiftpressed) && (tables.num() == STBL))
    {
      int speed = (ltable[tables.num()][tables.pos()] << 8) | rtable[tables.num()][tables.pos()];
      speed *= 34716;
      speed /= 32768;
      if (speed > 65535) speed = 65535;

      ltable[tables.num()][tables.pos()] = speed >> 8;
      rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    break;

    case KEY_A:
    if ((shiftpressed) && (tables.num() == STBL))
    {
      int speed = (ltable[tables.num()][tables.pos()] << 8) | rtable[tables.num()][tables.pos()];
      speed *= 30929;
      speed /= 32768;

      ltable[tables.num()][tables.pos()] = speed >> 8;
      rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    break;

    case KEY_W:
    if ((shiftpressed) && (tables.num() == STBL))
    {
      int speed = (ltable[tables.num()][tables.pos()] << 8) | rtable[tables.num()][tables.pos()];
      speed *= 2;
      if (speed > 65535) speed = 65535;

      ltable[tables.num()][tables.pos()] = speed >> 8;
      rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    if ((shiftpressed) && ((tables.num() == PTBL) || (tables.num() == FTBL)) && (ltable[tables.num()][tables.pos()] < 0x80))
    {
      int speed = (signed char)(rtable[tables.num()][tables.pos()]);
      speed *= 2;

      if (speed > 127) speed = 127;
      if (speed < -128) speed = -128;
      rtable[tables.num()][tables.pos()] = speed;
    }
    break;

    case KEY_S:
    if ((shiftpressed) && (tables.num() == STBL))
    {
      int speed = (ltable[tables.num()][tables.pos()] << 8) | rtable[tables.num()][tables.pos()];
      speed /= 2;

      ltable[tables.num()][tables.pos()] = speed >> 8;
      rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    if ((shiftpressed) && ((tables.num() == PTBL) || (tables.num() == FTBL)) && (ltable[tables.num()][tables.pos()] < 0x80))
    {
      int speed = (signed char)(rtable[tables.num()][tables.pos()]);
      speed /= 2;

      rtable[tables.num()][tables.pos()] = speed;
    }
    break;

    case KEY_SPACE:
    if (!shiftpressed)
      playtestnote(FIRSTNOTE + epoctave * 12, einum, epchn);
    else
      releasenote(epchn);
    break;

    case KEY_RIGHT:
    tables.m_column++;
    if (tables.m_column > 3)
    {
      tables.m_pos -= tables.curview();
      tables.m_column = 0;
      tables.m_num++;
      if (tables.num() >= MAX_TABLES) tables.m_num = 0;
      tables.m_pos += tables.curview();
    }
    if (shiftpressed) tables.resetmarknum();
    break;

    case KEY_LEFT:
    tables.m_column--;
    if (tables.m_column < 0)
    {
      tables.m_pos -= tables.curview();
      tables.m_column = 3;
      tables.m_num--;
      if (tables.num() < 0) tables.m_num = MAX_TABLES - 1;
      tables.m_pos += tables.curview();
    }
    if (shiftpressed) tables.resetmarknum();
    break;

    case KEY_HOME:
    while (tables.pos() != 0) tables.tableup(shiftpressed);
    break;

    case KEY_END:
    while (tables.pos() != MAX_TABLELEN-1) tables.tabledown(shiftpressed);
    break;

    case KEY_PGUP:
    for (int c = 0; c < PGUPDNREPEAT; c++) tables.tableup(shiftpressed);
    break;

    case KEY_PGDN:
    for (int c = 0; c < PGUPDNREPEAT; c++) tables.tabledown(shiftpressed);
    break;

    case KEY_UP:
    tables.tableup(shiftpressed);
    break;

    case KEY_DOWN:
    tables.tabledown(shiftpressed);
    break;

    case KEY_X:
    case KEY_C:
    if (shiftpressed)
    {
      if (tables.marknum() != -1)
      {
        int markstart = tables.markstart();
        int markend = tables.markend();
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        int d = 0;
        for (int c = markstart; c <= markend; c++)
        {
          ltablecopybuffer[d] = ltable[tables.marknum()][c];
          rtablecopybuffer[d] = rtable[tables.marknum()][c];
          if (rawkey == KEY_X)
          {
            ltable[tables.marknum()][c] = 0;
            rtable[tables.marknum()][c] = 0;
          }
          d++;
        }
        tablecopyrows = d;
      }
      tables.resetmarknum();
    }
    break;

    case KEY_V:
    if (shiftpressed)
    {
      if (tablecopyrows)
      {
        for (int c = 0; c < tablecopyrows; c++)
        {
          ltable[tables.num()][tables.pos()] = ltablecopybuffer[c];
          rtable[tables.num()][tables.pos()] = rtablecopybuffer[c];
          tables.m_pos++;
          if (tables.pos() >= MAX_TABLELEN) tables.m_pos = MAX_TABLELEN-1;
        }
      }
    }
    break;

    case KEY_O:
    if (shiftpressed) optimizetable(tables.num());
    break;

    case KEY_U:
    if (shiftpressed)
    {
      tables.fliplock();
      tables.validatetableview();
    }
    break;

    case KEY_R:
    if (tables.num() == WTBL)
    {
      if (ltable[tables.num()][tables.pos()] != 0xff)
      {
        // Convert absolute pitch to relative pitch or vice versa
        int basenote = epoctave * 12;
        int note = rtable[tables.num()][tables.pos()];

        if (note >= 0x80)
        {
          note -= basenote;
          note &= 0x7f;
        }
        else
        {
          note += basenote;
          note |= 0x80;
        }

        rtable[tables.num()][tables.pos()] = note;
      }
    }
    /* fall through */
    case KEY_L:
    if (tables.num() == PTBL)
    {
      int currentpulse = -1;
      int targetpulse = ltable[tables.num()][tables.pos()] << 4;
      int speed = rtable[tables.num()][tables.pos()];

      if (!speed) break;

      int c;
      // Follow the chain of pulse commands backwards to the nearest set command so we know what current pulse is
      for (c = tables.pos()-1; c >= 0; c--)
      {
        if (ltable[tables.num()][c] == 0xff) break;
        if (ltable[tables.num()][c] >= 0x80)
        {
          currentpulse = (ltable[tables.num()][c] << 8) | rtable[tables.num()][c];
          currentpulse &= 0xfff;
          break;
        }
      }
      if (currentpulse == -1) break;

      // Then follow the chain of modulation steps
      for (; c < tables.pos(); c++)
      {
        if (ltable[tables.num()][c] < 0x80)
        {
          currentpulse += ltable[tables.num()][c] * (rtable[tables.num()][c] & 0xff);
          if (rtable[tables.num()][c] >= 0x80) currentpulse -= 256 * ltable[tables.num()][c];
          currentpulse &= 0xfff;
        }
      }

      int time = std::abs(targetpulse - currentpulse) / speed;
      int steps = (speed < 128) ? (time + 126) / 127 : time;

      if (!steps) break;
      if (tables.pos() + steps > MAX_TABLELEN) break;
      if (targetpulse < currentpulse) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) inserttable(tables.num(), tables.pos(), 1);

      while (time)
      {
        if (std::abs(speed) < 128)
        {
          ltable[tables.num()][tables.pos()] = (time < 127) ? time : 127;
          rtable[tables.num()][tables.pos()] = speed;
          time -= ltable[tables.num()][tables.pos()];
          tables.m_pos++;
        }
        else
        {
          currentpulse += speed;
          ltable[tables.num()][tables.pos()] = 0x80 | ((currentpulse >> 8) & 0xf);
          rtable[tables.num()][tables.pos()] = currentpulse & 0xff;
          time--;
          tables.m_pos++;
        }
      }
    }
    if (tables.num() == FTBL)
    {
      int currentfilter = -1;
      int targetfilter = ltable[tables.num()][tables.pos()];
      int speed = rtable[tables.num()][tables.pos()] & 0x7f;

      if (!speed) break;

      int c;
      // Follow the chain of filter commands backwards to the nearest set command so we know what current pulse is
      for (c = tables.pos()-1; c >= 0; c--)
      {
        if (ltable[tables.num()][c] == 0xff) break;
        if (ltable[tables.num()][c] == 0x00)
        {
          currentfilter = rtable[tables.num()][c];
          break;
        }
      }
      if (currentfilter == -1) break;

      // Then follow the chain of modulation steps
      for (; c < tables.pos(); c++)
      {
        if (ltable[tables.num()][c] < 0x80)
        {
          currentfilter += ltable[tables.num()][c] * rtable[tables.num()][c];
          currentfilter &= 0xff;
        }
      }

      int time = std::abs(targetfilter - currentfilter) / speed;
      int steps = (time + 126) / 127;
      if (!steps) break;
      if (tables.pos() + steps > MAX_TABLELEN) break;
      if (targetfilter < currentfilter) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) inserttable(tables.num(), tables.pos(), 1);

      while (time)
      {
        ltable[tables.num()][tables.pos()] = (time < 127) ? time : 127;
        rtable[tables.num()][tables.pos()] = speed;
        time -= ltable[tables.num()][tables.pos()];
        tables.m_pos++;
      }
    }
    break;

    case KEY_N:
    if (shiftpressed)
    {
      switch (tables.num())
      {
        // Negate pulse or filter speed
        case FTBL:
        if (!ltable[tables.num()][tables.pos()]) break;
        /* fall through */
        case PTBL:
        if (ltable[tables.num()][tables.pos()] < 0x80)
          rtable[tables.num()][tables.pos()] = (rtable[tables.num()][tables.pos()] ^ 0xff) + 1;
        break;

        // Negate relative note
        case WTBL:
        if ((ltable[tables.num()][tables.pos()] != 0xff) && (rtable[tables.num()][tables.pos()] < 0x80))
          rtable[tables.num()][tables.pos()] = (0x80 - rtable[tables.num()][tables.pos()]) & 0x7f;
        break;
      }
    }
    break;

    case KEY_DEL:
    deletetable(tables.num(), tables.pos());
    break;

    case KEY_INS:
    inserttable(tables.num(), tables.pos(), shiftpressed);
    break;

    case KEY_ENTER:
    if (tables.num() == WTBL)
    {
      int table = -1;
      int mstmode = MST_PORTAMENTO;

      switch (ltable[tables.num()][tables.pos()])
      {
        case WAVECMD + CMD_PORTAUP:
        case WAVECMD + CMD_PORTADOWN:
        case WAVECMD + CMD_TONEPORTA:
        table = STBL;
        break;

        case WAVECMD + CMD_VIBRATO:
        table = STBL;
        mstmode = finevibrato;
        break;

        case WAVECMD + CMD_FUNKTEMPO:
        table = STBL;
        mstmode = MST_FUNKTEMPO;
        break;

        case WAVECMD + CMD_SETPULSEPTR:
        table = PTBL;
        break;

        case WAVECMD + CMD_SETFILTERPTR:
        table = FTBL;
        break;
      }
      switch (table)
      {
        default:
        editmode = EDIT_INSTRUMENT;
        eipos = tables.num() + 2;
        return;

        case STBL:
        if (rtable[tables.num()][tables.pos()])
        {
          if (!shiftpressed)
          {
            gototable(STBL, rtable[tables.num()][tables.pos()] - 1);
            return;
          }
          else
          {
            int oldeditpos = tables.pos();
            int oldeditcolumn = tables.m_column;
            int pos = makespeedtable(rtable[tables.num()][tables.pos()], mstmode, true);
            gototable(WTBL, oldeditpos);
            tables.m_column = oldeditcolumn;

            rtable[tables.num()][tables.pos()] = pos + 1;
            return;
          }
        }
        else
        {
          int pos = findfreespeedtable();
          if (pos >= 0)
          {
            rtable[tables.num()][tables.pos()] = pos + 1;
            gototable(STBL, pos);
            return;
          }
        }
        break;

        case PTBL:
        case FTBL:
        if (rtable[tables.num()][tables.pos()])
        {
          gototable(table, rtable[tables.num()][tables.pos()] - 1);
          return;
        }
        else
        {
          if (shiftpressed)
          {
            int pos = gettablelen(table);
            if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
            rtable[tables.num()][tables.pos()] = pos + 1;
            gototable(table, pos);
            return;
          }
        }
      }
    }
    else
    {
      editmode = EDIT_INSTRUMENT;
      eipos = tables.num() + 2;
      return;
    }
    break;

    case KEY_APOST2:
    if (shiftpressed)
    {
      tables.m_pos -= tables.curview();
      tables.m_num--;
      if (tables.num() < 0) tables.m_num = MAX_TABLES-1;
      tables.m_pos += tables.curview();
    }
    else
    {
      tables.m_pos -= tables.curview();
      tables.m_num++;
      if (tables.num() >= MAX_TABLES) tables.m_num = 0;
      tables.m_pos += tables.curview();
    }
  }

  if (hexnybble >= 0)
  {
    switch(tables.m_column)
    {
      case 0:
      ltable[tables.num()][tables.pos()] &= 0x0f;
      ltable[tables.num()][tables.pos()] |= hexnybble << 4;
      break;
      case 1:
      ltable[tables.num()][tables.pos()] &= 0xf0;
      ltable[tables.num()][tables.pos()] |= hexnybble;
      break;
      case 2:
      rtable[tables.num()][tables.pos()] &= 0x0f;
      rtable[tables.num()][tables.pos()] |= hexnybble << 4;
      break;
      case 3:
      rtable[tables.num()][tables.pos()] &= 0xf0;
      rtable[tables.num()][tables.pos()] |= hexnybble;
      break;
    }
    tables.m_column++;
    if (tables.m_column > 3)
    {
      tables.m_column = 0;
      tables.m_pos++;
      if (tables.pos() >= MAX_TABLELEN) tables.m_pos = MAX_TABLELEN-1;
    }
  }

  tables.validatetableview();
}

void deletetable(int num, int pos)
{
  // Shift tablepointers in instruments
  for (int c = 1; c < MAX_INSTR; c++)
  {
    if ((instr[c].ptr[num]-1) > pos) instr[c].ptr[num]--;
  }

  // Shift tablepointers in wavetable commands
  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
    {
      int cmd = ltable[WTBL][c] & 0xf;

      if (num < STBL)
      {
        if (cmd == CMD_SETWAVEPTR+num)
        {
          if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]--;
        }
      }
      else
      {
        if ((cmd == CMD_FUNKTEMPO) || ((cmd >= CMD_PORTAUP) && (cmd <= CMD_VIBRATO)))
        {
          if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]--;
        }
      }
    }
  }

  // Shift tablepointers in patterns
  for (int c = 0; c < MAX_PATT; c++)
  {
    for (int d = 0; d <= MAX_PATTROWS; d++)
    {
      if (num < STBL)
      {
        if (pattern[c][d*4+2] == CMD_SETWAVEPTR+num)
        {
          if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]--;
        }
      }
      else
      {
        if ((pattern[c][d*4+2] == CMD_FUNKTEMPO) ||
           ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO)))
        {
          if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]--;
        }
      }
    }
  }

  // Shift jumppointers in the table itself
  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if (num != STBL)
    {
      if (ltable[num][c] == 0xff)
        if ((rtable[num][c]-1) > pos) rtable[num][c]--;
    }
  }

  for (int c = pos; c < MAX_TABLELEN; c++)
  {
    if (c+1 < MAX_TABLELEN)
    {
      ltable[num][c] = ltable[num][c+1];
      rtable[num][c] = rtable[num][c+1];
    }
    else
    {
      ltable[num][c] = 0;
      rtable[num][c] = 0;
    }
  }
}

void inserttable(int num, int pos, int mode)
{
  // Shift tablepointers in instruments
  for (int c = 1; c < MAX_INSTR; c++)
  {
    if (!mode)
    {
      if ((instr[c].ptr[num]-1) >= pos) instr[c].ptr[num]++;
    }
    else
    {
      if ((instr[c].ptr[num]-1) > pos) instr[c].ptr[num]++;
    }
  }

  // Shift tablepointers in wavetable commands
  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
    {
      int cmd = ltable[WTBL][c] & 0xf;

      if (num < STBL)
      {
        if (cmd == CMD_SETWAVEPTR+num)
        {
          if (!mode)
          {
            if ((rtable[WTBL][c]-1) >= pos) rtable[WTBL][c]++;
          }
          else
          {
            if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]++;
          }
        }
      }
      else
      {
        if ((cmd == CMD_FUNKTEMPO) || ((cmd >= CMD_PORTAUP) && (cmd <= CMD_VIBRATO)))
        {
          if (!mode)
          {
            if ((rtable[WTBL][c]-1) >= pos) rtable[WTBL][c]++;
          }
          else
          {
            if ((rtable[WTBL][c]-1) > pos) rtable[WTBL][c]++;
          }
        }
      }
    }
  }


  // Shift tablepointers in patterns
  for (int c = 0; c < MAX_PATT; c++)
  {
    for (int d = 0; d <= MAX_PATTROWS; d++)
    {
      if (num < STBL)
      {
        if (pattern[c][d*4+2] == CMD_SETWAVEPTR+num)
        {
          if (!mode)
          {
            if ((pattern[c][d*4+3]-1) >= pos) pattern[c][d*4+3]++;
          }
          else
          {
            if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]++;
          }
        }
      }
      else
      {
        if ((pattern[c][d*4+2] == CMD_FUNKTEMPO) ||
           ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO)))
        {
          if (!mode)
          {
            if ((pattern[c][d*4+3]-1) >= pos) pattern[c][d*4+3]++;
          }
          else
          {
            if ((pattern[c][d*4+3]-1) > pos) pattern[c][d*4+3]++;
          }
        }
      }
    }
  }

  // Shift jumppointers in the table itself
  if (num != STBL)
  {
    for (int c = 0; c < MAX_TABLELEN; c++)
    {
      if (ltable[num][c] == 0xff)
      {
        if (!mode)
        {
          if ((rtable[num][c]-1) >= pos) rtable[num][c]++;
        }
        else
        {
          if ((rtable[num][c]-1) > pos) rtable[num][c]++;
        }
      }
    }
  }

  for (int c = MAX_TABLELEN-1; c >= pos; c--)
  {
    if (c > pos)
    {
      ltable[num][c] = ltable[num][c-1];
      rtable[num][c] = rtable[num][c-1];
    }
    else
    {
      if ((num == WTBL) && (mode == 1))
      {
        ltable[num][c] = 0xe9;
        rtable[num][c] = 0;
      }
      else
      {
        ltable[num][c] = 0;
        rtable[num][c] = 0;
      }
    }
  }
}

int gettablelen(int num)
{
  int c;
  for (c = MAX_TABLELEN-1; c >= 0; c--)
  {
    if (ltable[num][c] | rtable[num][c]) break;
  }
  return c+1;
}

int gettablepartlen(int num, int pos)
{
  if (pos < 0) return 0;
  if (num == STBL) return 1;

  int c;
  for (c = pos; c < MAX_TABLELEN; c++)
  {
    if (ltable[num][c] == 0xff)
    {
      c++;
      break;
    }
  }
  return c-pos;
}

void optimizetable(int num)
{
  std::memset(tableused, 0, sizeof tableused);

  for (int c = 0; c < MAX_PATT; c++)
  {
    for (int d = 0; d < getPattlen(c); d++)
    {
      if ((pattern[c][d*4+2] >= CMD_SETWAVEPTR) && (pattern[c][d*4+2] <= CMD_SETFILTERPTR))
        exectable(pattern[c][d*4+2] - CMD_SETWAVEPTR, pattern[c][d*4+3]);
      if ((pattern[c][d*4+2] >= CMD_PORTAUP) && (pattern[c][d*4+2] <= CMD_VIBRATO))
        exectable(STBL, pattern[c][d*4+3]);
      if (pattern[c][d*4+2] == CMD_FUNKTEMPO)
        exectable(STBL, pattern[c][d*4+3]);
    }
  }

  for (int c = 0; c < MAX_INSTR; c++)
  {
    for (int d = 0; d < MAX_TABLES; d++)
    {
      exectable(d, instr[c].ptr[d]);
    }
  }

  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if (tableused[WTBL][c+1])
    {
      if ((ltable[WTBL][c] >= WAVECMD) && (ltable[WTBL][c] <= WAVELASTCMD))
      {
        int d = -1;

        switch(ltable[WTBL][c] - WAVECMD)
        {
          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          case CMD_TONEPORTA:
          case CMD_VIBRATO:
          d = STBL;
          break;

          case CMD_SETPULSEPTR:
          d = PTBL;
           break;

           case CMD_SETFILTERPTR:
           d = FTBL;
          break;
        }

        if (d != -1) exectable(d, rtable[WTBL][c]);
      }
    }
  }

  int c;
  for (c = MAX_TABLELEN-1; c >= 0; c--)
  {
    if ((ltable[num][c]) || (rtable[num][c])) break;
  }
  for (; c >= 0; c--)
  {
    if (!tableused[num][c+1]) deletetable(num, c);
  }
}

int makespeedtable(unsigned data, int mode, bool makenew)
{
  if (!data) return -1;

  unsigned char l = 0, r = 0;

  switch (mode)
  {
    case MST_NOFINEVIB:
    l = (data & 0xf0) >> 4;
    r = (data & 0x0f) << 4;
    break;

    case MST_FINEVIB:
    l = (data & 0x70) >> 4;
    r = ((data & 0x0f) << 4) | ((data & 0x80) >> 4);
    break;

    case MST_FUNKTEMPO:
    l = (data & 0xf0) >> 4;
    r = data & 0x0f;
    break;

    case MST_PORTAMENTO:
    l = (data << 2) >> 8;
    r = (data << 2) & 0xff;
    break;

    case MST_RAW:
    r = data & 0xff;
    l = data >> 8;
    break;
  }

  if (!makenew)
  {
    for (int c = 0; c < MAX_TABLELEN; c++)
    {
      if ((ltable[STBL][c] == l) && (rtable[STBL][c] == r))
        return c;
    }
  }

  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      ltable[STBL][c] = l;
      rtable[STBL][c] = r;

      tables.settableview(STBL, c);
      return c;
    }
  }
  return -1;
}

void deleteinstrtable(int i)
{
  bool eraseok = true;

  for (int c = 0; c < MAX_TABLES; c++)
  {
    if (instr[i].ptr[c])
    {
      int pos = instr[i].ptr[c]-1;
      int len = gettablepartlen(c, pos);

      // Check that this table area isn't used by another instrument
      for (int d = 1; d < MAX_INSTR; d++)
      {
        if ((d != i) && (instr[d].ptr[c]))
        {
          int cmppos = instr[d].ptr[c]-1;
          if ((cmppos >= pos) && (cmppos < pos+len)) eraseok = false;
        }
      }
      if (eraseok)
        while (len--) deletetable(c, pos);
    }
  }
}

void exectable(int num, int ptr)
{
  // Jump error check
  if ((num != STBL) && (ptr) && (ptr <= MAX_TABLELEN))
  {
    if (ltable[num][ptr-1] == 0xff)
    {
      tableerror = TYPE_JUMP;
      return;
    }
  }

  for (;;)
  {
    // Exit when table stopped
    if (!ptr) break;
    // Overflow check
    if ((num != STBL) && (ptr > MAX_TABLELEN))
    {
      tableerror = TYPE_OVERFLOW;
      break;
    }
    // If were already here, exit
    if (tableused[num][ptr]) break;
    // Mark current position used
    tableused[num][ptr] = 1;
    // Go to next ptr.
    if (num != STBL)
    {
      if (ltable[num][ptr-1] == 0xff)
      {
        ptr = rtable[num][ptr-1];
      }
      else ptr++;
    }
    else break;
  }
}

int findfreespeedtable()
{
  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!ltable[STBL][c]) && (!rtable[STBL][c]))
    {
      return c;
    }
  }
  return -1;
}

void gototable(int num, int pos)
{
  editmode = EDIT_TABLES;
  tables.settableview(num, pos);
}

void Tables::settableview(int num, int pos)
{
  m_num = num;
  m_column = 0;
  m_pos = pos;

  validatetableview();
}

void Tables::settableviewfirst(int num, int pos)
{
  m_view[num] = pos;
  settableview(num, pos);
}

void Tables::validatetableview()
{
  if (m_pos - m_view[m_num] < 0)
    m_view[m_num] = m_pos;
  if (m_pos - m_view[m_num] >= VISIBLETABLEROWS)
    m_view[m_num] = m_pos - VISIBLETABLEROWS + 1;

  // Table view lock?
  if (m_lock)
  {
    for (int c = 0; c < MAX_TABLES; c++) m_view[c] = m_view[m_num];
  }
}

void Tables::tableup(bool shiftpressed)
{
  if (shiftpressed)
  {
    if ((m_marknum != m_num) || (m_pos != m_markend))
    {
      m_marknum = m_num;
      m_markstart = m_pos;
      m_markend = m_pos;
    }
  }
  m_pos--;
  if (m_pos < 0) m_pos = 0;
  if (shiftpressed) m_markend = m_pos;

  validatetableview();
}

void Tables::tabledown(bool shiftpressed)
{
  if (shiftpressed)
  {
    if ((m_marknum != m_num) || (m_pos != m_markend))
    {
      m_marknum = m_num;
      m_markstart = m_pos;
      m_markend = m_pos;
    }
  }
  m_pos++;
  if (m_pos >= MAX_TABLELEN) m_pos = MAX_TABLELEN-1;
  if (shiftpressed) m_markend = m_pos;

  validatetableview();
}

void Tables::setrow(int num, int pos, int column)
{
    m_num = num;
    m_pos = pos;
    if (m_pos < 0) m_pos = 0;
    if (m_pos > MAX_TABLELEN-1) m_pos = MAX_TABLELEN-1;
    m_column = column;
    if (m_column >= 2) m_column--;

    validatetableview();
}

void Tables::resetmarknum()
{
    m_marknum = -1;
}

void Tables::setmarkstart(int num, int markstart)
{
    if ((m_marknum != m_num) || (markstart != m_markend))
    {
        m_marknum = num;
        m_markstart = markstart;
        m_markend = markstart;
    }
}

void Tables::setmarkend(int markend)
{
    m_markend = markend;
}

void Tables::fliplock()
{
    m_lock = !m_lock;
}

void Tables::clear()
{
  m_marknum = -1;
}
