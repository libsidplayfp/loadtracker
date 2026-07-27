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

#include "console.h"
#include "display.h"
#include "instr.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "settings.h"
#include "song.h"
#include "table.h"

#include "bme_main.h"

#include <utility>

#include <cmath>
#include <cstring>

#ifdef OVERFLOW
// might be defined in math.h
#  undef OVERFLOW
#endif

unsigned char ltablecopybuffer[MAX_TABLELEN];
unsigned char rtablecopybuffer[MAX_TABLELEN];
int tablecopyrows = 0;

Tables tables;

void tablecommands()
{
  switch(input.rawkey)
  {
    case KEY_Q:
    if ((input.shiftpressed) && (tables.num() == STBL))
    {
      int speed = (song.ltable[tables.num()][tables.pos()] << 8) | song.rtable[tables.num()][tables.pos()];
      speed *= 34716;
      speed /= 32768;
      if (speed > 65535) speed = 65535;

      song.ltable[tables.num()][tables.pos()] = speed >> 8;
      song.rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    break;

    case KEY_A:
    if ((input.shiftpressed) && (tables.num() == STBL))
    {
      int speed = (song.ltable[tables.num()][tables.pos()] << 8) | song.rtable[tables.num()][tables.pos()];
      speed *= 30929;
      speed /= 32768;

      song.ltable[tables.num()][tables.pos()] = speed >> 8;
      song.rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    break;

    case KEY_W:
    if ((input.shiftpressed) && (tables.num() == STBL))
    {
      int speed = (song.ltable[tables.num()][tables.pos()] << 8) | song.rtable[tables.num()][tables.pos()];
      speed *= 2;
      if (speed > 65535) speed = 65535;

      song.ltable[tables.num()][tables.pos()] = speed >> 8;
      song.rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    if ((input.shiftpressed) && ((tables.num() == PTBL) || (tables.num() == FTBL)) && (song.ltable[tables.num()][tables.pos()] < 0x80))
    {
      int speed = (signed char)(song.rtable[tables.num()][tables.pos()]);
      speed *= 2;

      if (speed > 127) speed = 127;
      if (speed < -128) speed = -128;
      song.rtable[tables.num()][tables.pos()] = speed;
    }
    break;

    case KEY_S:
    if ((input.shiftpressed) && (tables.num() == STBL))
    {
      int speed = (song.ltable[tables.num()][tables.pos()] << 8) | song.rtable[tables.num()][tables.pos()];
      speed /= 2;

      song.ltable[tables.num()][tables.pos()] = speed >> 8;
      song.rtable[tables.num()][tables.pos()] = speed & 0xff;
    }
    if ((input.shiftpressed) && ((tables.num() == PTBL) || (tables.num() == FTBL)) && (song.ltable[tables.num()][tables.pos()] < 0x80))
    {
      int speed = (signed char)(song.rtable[tables.num()][tables.pos()]);
      speed /= 2;

      song.rtable[tables.num()][tables.pos()] = speed;
    }
    break;

    case KEY_SPACE:
    if (!input.shiftpressed)
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
    if (input.shiftpressed) tables.resetmarknum();
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
    if (input.shiftpressed) tables.resetmarknum();
    break;

    case KEY_HOME:
    tables.tableup(input.shiftpressed, tables.pos());
    break;

    case KEY_END:
    tables.tabledown(input.shiftpressed, MAX_TABLELEN-1-tables.pos());
    break;

    case KEY_PGUP:
    tables.tableup(input.shiftpressed, PGUPDNREPEAT);
    break;

    case KEY_PGDN:
    tables.tabledown(input.shiftpressed, PGUPDNREPEAT);
    break;

    case KEY_UP:
    tables.tableup(input.shiftpressed);
    break;

    case KEY_DOWN:
    tables.tabledown(input.shiftpressed);
    break;

    case KEY_X:
    case KEY_C:
    if (input.shiftpressed)
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
          ltablecopybuffer[d] = song.ltable[tables.marknum()][c];
          rtablecopybuffer[d] = song.rtable[tables.marknum()][c];
          if (input.rawkey == KEY_X)
          {
            song.ltable[tables.marknum()][c] = 0;
            song.rtable[tables.marknum()][c] = 0;
          }
          d++;
        }
        tablecopyrows = d;
      }
      tables.resetmarknum();
    }
    break;

    case KEY_V:
    if (input.shiftpressed)
    {
      if (tablecopyrows)
      {
        for (int c = 0; c < tablecopyrows; c++)
        {
          song.ltable[tables.num()][tables.pos()] = ltablecopybuffer[c];
          song.rtable[tables.num()][tables.pos()] = rtablecopybuffer[c];
          tables.m_pos++;
          if (tables.pos() >= MAX_TABLELEN) tables.m_pos = MAX_TABLELEN-1;
        }
      }
    }
    break;

    case KEY_O:
    if (input.shiftpressed) optimizetable(tables.num());
    break;

    case KEY_U:
    if (input.shiftpressed)
    {
      tables.fliplock();
      tables.validatetableview();
    }
    break;

    case KEY_R:
    if (tables.num() == WTBL)
    {
      if (song.ltable[tables.num()][tables.pos()] != 0xff)
      {
        // Convert absolute pitch to relative pitch or vice versa
        int basenote = epoctave * 12;
        int note = song.rtable[tables.num()][tables.pos()];

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

        song.rtable[tables.num()][tables.pos()] = note;
      }
    }
    /* fall through */
    case KEY_L:
    if (tables.num() == PTBL)
    {
      int currentpulse = -1;
      int targetpulse = song.ltable[tables.num()][tables.pos()] << 4;
      int speed = song.rtable[tables.num()][tables.pos()];

      if (!speed) break;

      int c;
      // Follow the chain of pulse commands backwards to the nearest set command so we know what current pulse is
      for (c = tables.pos()-1; c >= 0; c--)
      {
        if (song.ltable[tables.num()][c] == 0xff) break;
        if (song.ltable[tables.num()][c] >= 0x80)
        {
          currentpulse = (song.ltable[tables.num()][c] << 8) | song.rtable[tables.num()][c];
          currentpulse &= 0xfff;
          break;
        }
      }
      if (currentpulse == -1) break;

      // Then follow the chain of modulation steps
      for (; c < tables.pos(); c++)
      {
        if (song.ltable[tables.num()][c] < 0x80)
        {
          currentpulse += song.ltable[tables.num()][c] * (song.rtable[tables.num()][c] & 0xff);
          if (song.rtable[tables.num()][c] >= 0x80) currentpulse -= 256 * song.ltable[tables.num()][c];
          currentpulse &= 0xfff;
        }
      }

      int time = std::abs(targetpulse - currentpulse) / speed;
      int steps = (speed < 128) ? (time + 126) / 127 : time;

      if (!steps) break;
      if (tables.pos() + steps > MAX_TABLELEN) break;
      if (targetpulse < currentpulse) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) song.inserttable(tables.num(), tables.pos(), 1);

      while (time)
      {
        if (std::abs(speed) < 128)
        {
          song.ltable[tables.num()][tables.pos()] = (time < 127) ? time : 127;
          song.rtable[tables.num()][tables.pos()] = speed;
          time -= song.ltable[tables.num()][tables.pos()];
          tables.m_pos++;
        }
        else
        {
          currentpulse += speed;
          song.ltable[tables.num()][tables.pos()] = 0x80 | ((currentpulse >> 8) & 0xf);
          song.rtable[tables.num()][tables.pos()] = currentpulse & 0xff;
          time--;
          tables.m_pos++;
        }
      }
    }
    if (tables.num() == FTBL)
    {
      int currentfilter = -1;
      int targetfilter = song.ltable[tables.num()][tables.pos()];
      int speed = song.rtable[tables.num()][tables.pos()] & 0x7f;

      if (!speed) break;

      int c;
      // Follow the chain of filter commands backwards to the nearest set command so we know what current pulse is
      for (c = tables.pos()-1; c >= 0; c--)
      {
        if (song.ltable[tables.num()][c] == 0xff) break;
        if (song.ltable[tables.num()][c] == 0x00)
        {
          currentfilter = song.rtable[tables.num()][c];
          break;
        }
      }
      if (currentfilter == -1) break;

      // Then follow the chain of modulation steps
      for (; c < tables.pos(); c++)
      {
        if (song.ltable[tables.num()][c] < 0x80)
        {
          currentfilter += song.ltable[tables.num()][c] * song.rtable[tables.num()][c];
          currentfilter &= 0xff;
        }
      }

      int time = std::abs(targetfilter - currentfilter) / speed;
      int steps = (time + 126) / 127;
      if (!steps) break;
      if (tables.pos() + steps > MAX_TABLELEN) break;
      if (targetfilter < currentfilter) speed = -speed;

      // Make room in the table
      for (c = steps; c > 1; c--) song.inserttable(tables.num(), tables.pos(), 1);

      while (time)
      {
        song.ltable[tables.num()][tables.pos()] = (time < 127) ? time : 127;
        song.rtable[tables.num()][tables.pos()] = speed;
        time -= song.ltable[tables.num()][tables.pos()];
        tables.m_pos++;
      }
    }
    break;

    case KEY_N:
    if (input.shiftpressed)
    {
      switch (tables.num())
      {
        // Negate pulse or filter speed
        case FTBL:
        if (!song.ltable[tables.num()][tables.pos()]) break;
        /* fall through */
        case PTBL:
        if (song.ltable[tables.num()][tables.pos()] < 0x80)
          song.rtable[tables.num()][tables.pos()] = (song.rtable[tables.num()][tables.pos()] ^ 0xff) + 1;
        break;

        // Negate relative note
        case WTBL:
        if ((song.ltable[tables.num()][tables.pos()] != 0xff) && (song.rtable[tables.num()][tables.pos()] < 0x80))
          song.rtable[tables.num()][tables.pos()] = (0x80 - song.rtable[tables.num()][tables.pos()]) & 0x7f;
        break;
      }
    }
    break;

    case KEY_DEL:
    song.deletetable(tables.num(), tables.pos());
    break;

    case KEY_INS:
    song.inserttable(tables.num(), tables.pos(), input.shiftpressed);
    break;

    case KEY_ENTER:
    if (tables.num() == WTBL)
    {
      int table = -1;
      int mstmode = MST_PORTAMENTO;

      switch (song.ltable[tables.num()][tables.pos()])
      {
        case WAVECMD + CMD_PORTAUP:
        case WAVECMD + CMD_PORTADOWN:
        case WAVECMD + CMD_TONEPORTA:
        table = STBL;
        break;

        case WAVECMD + CMD_VIBRATO:
        table = STBL;
        mstmode = config.finevibrato;
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
        if (song.rtable[tables.num()][tables.pos()])
        {
          if (!input.shiftpressed)
          {
            gototable(STBL, song.rtable[tables.num()][tables.pos()] - 1);
            return;
          }
          else
          {
            int oldeditpos = tables.pos();
            int oldeditcolumn = tables.m_column;
            int pos = makespeedtable(song.rtable[tables.num()][tables.pos()], mstmode, true);
            gototable(WTBL, oldeditpos);
            tables.m_column = oldeditcolumn;

            song.rtable[tables.num()][tables.pos()] = pos + 1;
            return;
          }
        }
        else
        {
          int pos = song.findfreespeedtable();
          if (pos >= 0)
          {
            song.rtable[tables.num()][tables.pos()] = pos + 1;
            gototable(STBL, pos);
            return;
          }
        }
        break;

        case PTBL:
        case FTBL:
        if (song.rtable[tables.num()][tables.pos()])
        {
          gototable(table, song.rtable[tables.num()][tables.pos()] - 1);
          return;
        }
        else
        {
          if (input.shiftpressed)
          {
            int pos = song.gettablelen(table);
            if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
            song.rtable[tables.num()][tables.pos()] = pos + 1;
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
    if (input.shiftpressed)
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
      song.ltable[tables.num()][tables.pos()] &= 0x0f;
      song.ltable[tables.num()][tables.pos()] |= hexnybble << 4;
      break;
      case 1:
      song.ltable[tables.num()][tables.pos()] &= 0xf0;
      song.ltable[tables.num()][tables.pos()] |= hexnybble;
      break;
      case 2:
      song.rtable[tables.num()][tables.pos()] &= 0x0f;
      song.rtable[tables.num()][tables.pos()] |= hexnybble << 4;
      break;
      case 3:
      song.rtable[tables.num()][tables.pos()] &= 0xf0;
      song.rtable[tables.num()][tables.pos()] |= hexnybble;
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
      if ((song.ltable[STBL][c] == l) && (song.rtable[STBL][c] == r))
        return c;
    }
  }

  for (int c = 0; c < MAX_TABLELEN; c++)
  {
    if ((!song.ltable[STBL][c]) && (!song.rtable[STBL][c]))
    {
      song.ltable[STBL][c] = l;
      song.rtable[STBL][c] = r;

      tables.settableview(STBL, c);
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

void Tables::tableup(bool shiftpressed, int n)
{
  if (shiftpressed)
  {
    if ((m_mark.chn != m_num) || (m_pos != m_mark.end))
    {
      m_mark.chn = m_num;
      m_mark.start = m_pos;
      m_mark.end = m_pos;
    }
  }
  m_pos-=n;
  if (m_pos < 0) m_pos = 0;
  if (shiftpressed) m_mark.end = m_pos;

  validatetableview();
}

void Tables::tabledown(bool shiftpressed, int n)
{
  if (shiftpressed)
  {
    if ((m_mark.chn != m_num) || (m_pos != m_mark.end))
    {
      m_mark.chn = m_num;
      m_mark.start = m_pos;
      m_mark.end = m_pos;
    }
  }
  m_pos+=n;
  if (m_pos >= MAX_TABLELEN) m_pos = MAX_TABLELEN-1;
  if (shiftpressed) m_mark.end = m_pos;

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
    m_mark.chn = -1;
}

void Tables::setmarkstart(int num, int markstart)
{
    if ((m_mark.chn != m_num) || (markstart != m_mark.end))
    {
        m_mark.chn = num;
        m_mark.start = markstart;
        m_mark.end = markstart;
    }
}

void Tables::setmarkend(int markend)
{
    m_mark.end = markend;
}

void Tables::fliplock()
{
    m_lock = !m_lock;
}

void Tables::clear()
{
  m_mark.chn = -1;
}
