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

#include "commands.h"

#include "common.h"
#include "console.h"
#include "display.h"
#include "instr.h"
#include "order.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "settings.h"
#include "song.h"
#include "table.h"
#include "tuning.h"

#include "bme_main.h"

#include <cstring>

unsigned char notekeytbl1[] =
{
    KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_V,
    KEY_G, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M, KEY_COMMA, KEY_L, KEY_COLON
};

unsigned char notekeytbl2[] =
{
    KEY_Q, KEY_2, KEY_W, KEY_3, KEY_E, KEY_R,
    KEY_5, KEY_T, KEY_6, KEY_Y, KEY_7, KEY_U, KEY_I, KEY_9, KEY_O, KEY_0, KEY_P
};

unsigned char dmckeytbl[] =
{
    KEY_A, KEY_W, KEY_S, KEY_E, KEY_D, KEY_F,
    KEY_T, KEY_G, KEY_Y, KEY_H, KEY_U, KEY_J, KEY_K, KEY_O, KEY_L, KEY_P
};

unsigned char jankokeytbl1[] =
{
    KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_F, KEY_V,
    KEY_G, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M, KEY_K, KEY_COMMA, KEY_L, KEY_COLON
};

unsigned char jankokeytbl2[] =
{
    KEY_Q, KEY_2, KEY_W, KEY_3, KEY_E, KEY_4, KEY_R,
    KEY_5, KEY_T, KEY_6, KEY_Y, KEY_7, KEY_U, KEY_8, KEY_I, KEY_9, KEY_O, KEY_0, KEY_P
};

unsigned char patterncopybuffer[MAX_PATTROWS*4+4];
unsigned char cmdcopybuffer[MAX_PATTROWS*4+4];
int patterncopyrows = 0;
int cmdcopyrows = 0;

unsigned char trackcopybuffer[MAX_SONGLEN+2];
int trackcopyrows = 0;
int trackcopywhole;
int trackcopyrpos;

Instr instrcopybuffer;
int cutinstr = -1;

unsigned char ltablecopybuffer[MAX_TABLELEN];
unsigned char rtablecopybuffer[MAX_TABLELEN];
int tablecopyrows = 0;

void clear(bool cs, bool cp, bool ci, bool cf, bool cn)
{
    if (ci)
        std::memset(&instrcopybuffer, 0, sizeof(Instr));
    clearsong(cs, cp, ci, cf, cn);
}

void gototable(int num, int pos)
{
  editmode = EDIT_TABLES;
  tables.settableview(num, pos);
}

void patterncommands()
{
  int maxChns = config.getMaxChannels();

  switch(input.key)
  {
    case '<':
    case '(':
    case '[':
    prevpattern();
    break;

    case '>':
    case ')':
    case ']':
    nextpattern();
    break;
  }
  {
    int newnote = -1;
    if (input.key)
    {
      switch (config.keypreset)
      {
        case KEY_TRACKER:
        for (int i = 0; i < (int)sizeof(notekeytbl1); i++)
        {
          if ((input.rawkey == notekeytbl1[i]) && (!epcolumn) && (!input.shiftpressed) && (!input.altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        for (int i = 0; i < (int)sizeof(notekeytbl2); i++)
        {
          if ((input.rawkey == notekeytbl2[i]) && (!epcolumn) && (!input.shiftpressed) && (!input.altpressed))
          {
            newnote = FIRSTNOTE+i+(epoctave+1)*12;
          }
        }
        break;

        case KEY_DMC:
        for (int i = 0; i < (int)sizeof(dmckeytbl); i++)
        {
          if ((input.rawkey == dmckeytbl[i]) && (!epcolumn) && (!input.shiftpressed) && (!input.altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        break;

        case KEY_JANKO:
        for (int i = 0; i < (int)sizeof(jankokeytbl1); i++)
        {
          if ((input.rawkey == jankokeytbl1[i]) && (!epcolumn) && (!input.shiftpressed) && (!input.altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        for (int i = 0; i < (int)sizeof(jankokeytbl2); i++)
        {
          if ((input.rawkey == jankokeytbl2[i]) && (!epcolumn) && (!input.shiftpressed) && (!input.altpressed))
          {
            newnote = FIRSTNOTE+i+(epoctave+1)*12;
          }
        }
        break;
      }
    }

    if (newnote > LASTNOTE) newnote = -1;
    if ((input.rawkey == KEY_BACKSPACE) && (!epcolumn)) newnote = REST;
    if ((input.rawkey == KEY_CAPSLOCK) && (!epcolumn)) newnote = KEYOFF;
    if (input.rawkey == KEY_ENTER)
    {
      switch(epcolumn)
      {
        case 0:
        if (input.shiftpressed)
          newnote = KEYON;
        else
          newnote = KEYOFF;
        break;

        case 1:
        case 2:
        if (song.pattern[epnum[epchn]][eppos*4+1])
        {
          gotoinstr(song.pattern[epnum[epchn]][eppos*4+1]);
          return;
        }
        break;

        default:
        switch (song.pattern[epnum[epchn]][eppos*4+2])
        {
          case CMD_SETWAVEPTR:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(WTBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.gettablelen(WTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(WTBL, pos);
              return;
            }
          }
          break;

          case CMD_SETPULSEPTR:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(PTBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.gettablelen(PTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(PTBL, pos);
              return;
            }
          }
          break;

          case CMD_SETFILTERPTR:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(FTBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.gettablelen(FTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(FTBL, pos);
              return;
            }
          }
          break;

          case CMD_FUNKTEMPO:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            if (!input.shiftpressed)
            {
              gototable(STBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(song.pattern[epnum[epchn]][eppos*4+3], MST_FUNKTEMPO, true);
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.findfreespeedtable();
              if (pos >= 0)
              {
                song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
                gototable(STBL, pos);
                return;
              }
            }
          }
          break;

          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          case CMD_TONEPORTA:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            if (!input.shiftpressed)
            {
              gototable(STBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(song.pattern[epnum[epchn]][eppos*4+3], MST_PORTAMENTO, true);
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.findfreespeedtable();
              if (pos >= 0)
              {
                song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
                gototable(STBL, pos);
                return;
              }
            }
          }
          break;

          case CMD_VIBRATO:
          if (song.pattern[epnum[epchn]][eppos*4+3])
          {
            if (!input.shiftpressed)
            {
              gototable(STBL, song.pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(song.pattern[epnum[epchn]][eppos*4+3], config.finevibrato, true);
              song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (input.shiftpressed)
            {
              int pos = song.findfreespeedtable();
              if (pos >= 0)
              {
                song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
                gototable(STBL, pos);
                return;
              }
            }
          }
          break;
        }
        break;
      }
      if ((autoadvance < 2) && (epcolumn))
      {
        eppos++;
        if (eppos > getPattlen(epnum[epchn]))
        {
          eppos = 0;
        }
      }
    }

    if (newnote >= 0) {
        insertnote(newnote);
    }
  }
  switch(input.rawkey)
  {
    case KEY_O:
    if (input.shiftpressed) shrinkpattern();
    break;

    case KEY_P:
    if (input.shiftpressed) expandpattern();
    break;

    case KEY_J:
    if (input.shiftpressed) joinpattern();
    break;

    case KEY_K:
    if (input.shiftpressed) splitpattern();
    break;

    case KEY_Z:
    if (input.shiftpressed)
    {
      autoadvance++;
      if (autoadvance > 2) autoadvance = 0;
      if (config.keypreset == KEY_TRACKER)
      {
        if (autoadvance == 1) autoadvance = 2;
      }
    }
    break;

    case KEY_E:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        int d = 0;
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          cmdcopybuffer[d*4+2] = song.pattern[epnum[epmark.chn]][c*4+2];
          cmdcopybuffer[d*4+3] = song.pattern[epnum[epmark.chn]][c*4+3];
          d++;
        }
        cmdcopyrows = d;
        epmark.chn = -1;
      }
      else
      {
        if (eppos < getPattlen(epnum[epchn]))
        {
          cmdcopybuffer[2] = song.pattern[epnum[epchn]][eppos*4+2];
          cmdcopybuffer[3] = song.pattern[epnum[epchn]][eppos*4+3];
          cmdcopyrows = 1;
        }
      }
    }
    break;

    case KEY_R:
    if (input.shiftpressed)
    {
      for (int c = 0; c < cmdcopyrows; c++)
      {
        if (eppos >= getPattlen(epnum[epchn])) break;
        song.pattern[epnum[epchn]][eppos*4+2] = cmdcopybuffer[c*4+2];
        song.pattern[epnum[epchn]][eppos*4+3] = cmdcopybuffer[c*4+3];
        eppos++;
      }
    }
    break;

    case KEY_I:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        int e = markend;
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          for (int d = 0; d < 4; d++)
          {
            std::swap(song.pattern[epnum[epmark.chn]][c*4+d], song.pattern[epnum[epmark.chn]][e*4+d]);
          }
          e--;
          if (e < c) break;
        }
      }
      else
      {
        int e = getPattlen(epnum[epchn]) - 1;
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          for (int d = 0; d < 4; d++)
          {
            std::swap(song.pattern[epnum[epchn]][c*4+d], song.pattern[epnum[epchn]][e*4+d]);
          }
          e--;
          if (e < c) break;
        }
      }
    }
    break;

    case KEY_Q:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          if ((song.pattern[epnum[epmark.chn]][c*4] < LASTNOTE) &&
              (song.pattern[epnum[epmark.chn]][c*4] >= FIRSTNOTE))
            song.pattern[epnum[epmark.chn]][c*4]++;
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((song.pattern[epnum[epchn]][c*4] < LASTNOTE) &&
              (song.pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
            song.pattern[epnum[epchn]][c*4]++;
        }
      }
    }
    break;

    case KEY_A:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          if ((song.pattern[epnum[epmark.chn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epmark.chn]][c*4] > FIRSTNOTE))
            song.pattern[epnum[epmark.chn]][c*4]--;
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((song.pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epchn]][c*4] > FIRSTNOTE))
            song.pattern[epnum[epchn]][c*4]--;
        }
      }
    }
    break;

    case KEY_W:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          if ((song.pattern[epnum[epmark.chn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epmark.chn]][c*4] >= FIRSTNOTE))
          {
            song.pattern[epnum[epmark.chn]][c*4] += 12;
            if (song.pattern[epnum[epmark.chn]][c*4] > LASTNOTE)
              song.pattern[epnum[epmark.chn]][c*4] = LASTNOTE;
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((song.pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
          {
            song.pattern[epnum[epchn]][c*4] += 12;
            if (song.pattern[epnum[epchn]][c*4] > LASTNOTE)
              song.pattern[epnum[epchn]][c*4] = LASTNOTE;
          }
        }
      }
    }
    break;

    case KEY_S:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          if ((song.pattern[epnum[epmark.chn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epmark.chn]][c*4] >= FIRSTNOTE))
          {
            song.pattern[epnum[epmark.chn]][c*4] -= 12;
            if (song.pattern[epnum[epmark.chn]][c*4] < FIRSTNOTE)
              song.pattern[epnum[epmark.chn]][c*4] = FIRSTNOTE;
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((song.pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (song.pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
          {
            song.pattern[epnum[epchn]][c*4] -= 12;
            if (song.pattern[epnum[epchn]][c*4] < FIRSTNOTE)
              song.pattern[epnum[epchn]][c*4] = FIRSTNOTE;
          }
        }
      }
    }
    break;

    case KEY_M:
    if (input.shiftpressed)
    {
      config.stepsize++;
      if (config.stepsize > MAX_PATTROWS) config.stepsize = MAX_PATTROWS;
    }
    break;

    case KEY_N:
    if (input.shiftpressed)
    {
      config.stepsize--;
      if (config.stepsize < 2) config.stepsize = 2;
    }
    break;

    case KEY_H:
    if (input.shiftpressed)
    {
      int c;
      switch (song.pattern[epnum[epchn]][eppos*4+2])
      {
        case CMD_PORTAUP:
        case CMD_PORTADOWN:
        case CMD_VIBRATO:
        case CMD_TONEPORTA:
        if (song.pattern[epnum[epchn]][eppos*4+2] == CMD_TONEPORTA)
          c = eppos-1;
        else
          c = eppos;
        for (; c >= 0; c--)
        {
          if ((song.pattern[epnum[epchn]][c*4] >= FIRSTNOTE) &&
              (song.pattern[epnum[epchn]][c*4] <= LASTNOTE))
          {
            int note = song.pattern[epnum[epchn]][c*4] - FIRSTNOTE;
            int right = song.pattern[epnum[epchn]][eppos*4+3] & 0xf;
            int left = song.pattern[epnum[epchn]][eppos*4+3] >> 4;

            if (note > MAX_NOTES-1) note--;
            int pitch1 = freqtbllo[note] | (freqtblhi[note] << 8);
            int pitch2 = freqtbllo[note+1] | (freqtblhi[note+1] << 8);
            int delta = pitch2 - pitch1;

            while (left--) delta <<= 1;
            while (right--) delta >>= 1;

            if (song.pattern[epnum[epchn]][eppos*4+2] == CMD_VIBRATO)
            {
              if (delta > 0xff) delta = 0xff;
            }
            int pos = makespeedtable(delta, MST_RAW, true);
            song.pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            break;
          }
        }
        break;
      }
    }
    break;

    case KEY_L:
    if (input.shiftpressed)
    {
      if (epmark.chn == -1)
      {
        epmark.chn = epchn;
        epmark.start = 0;
        epmark.end = getPattlen(epnum[epchn])-1;
      }
      else epmark.chn = -1;
    }
    break;

    case KEY_C:
    case KEY_X:
    if (input.shiftpressed)
    {
      if (epmark.chn != -1)
      {
        int markstart = epmark.start;
        int markend = epmark.end;
        if (markstart > markend)
        {
            std::swap(markstart, markend);
        }
        int d = 0;
        for (int c = markstart; c <= markend; c++)
        {
          if (c >= getPattlen(epnum[epmark.chn])) break;
          patterncopybuffer[d*4] = song.pattern[epnum[epmark.chn]][c*4];
          patterncopybuffer[d*4+1] = song.pattern[epnum[epmark.chn]][c*4+1];
          patterncopybuffer[d*4+2] = song.pattern[epnum[epmark.chn]][c*4+2];
          patterncopybuffer[d*4+3] = song.pattern[epnum[epmark.chn]][c*4+3];
          if (input.rawkey == KEY_X)
          {
            song.pattern[epnum[epmark.chn]][c*4] = REST;
            song.pattern[epnum[epmark.chn]][c*4+1] = 0;
            song.pattern[epnum[epmark.chn]][c*4+2] = 0;
            song.pattern[epnum[epmark.chn]][c*4+3] = 0;
          }
          d++;
        }
        patterncopyrows = d;
        epmark.chn = -1;
      }
      else
      {
        int d = 0;
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          patterncopybuffer[d*4] = song.pattern[epnum[epchn]][c*4];
          patterncopybuffer[d*4+1] = song.pattern[epnum[epchn]][c*4+1];
          patterncopybuffer[d*4+2] = song.pattern[epnum[epchn]][c*4+2];
          patterncopybuffer[d*4+3] = song.pattern[epnum[epchn]][c*4+3];
          if (input.rawkey == KEY_X)
          {
            song.pattern[epnum[epchn]][c*4] = REST;
            song.pattern[epnum[epchn]][c*4+1] = 0;
            song.pattern[epnum[epchn]][c*4+2] = 0;
            song.pattern[epnum[epchn]][c*4+3] = 0;
          }
          d++;
        }
        patterncopyrows = d;
      }
    }
    break;

    case KEY_V:
    if ((input.shiftpressed) && (patterncopyrows))
    {
      for (int c = 0; c < patterncopyrows; c++)
      {
        if (eppos >= getPattlen(epnum[epchn])) break;
        song.pattern[epnum[epchn]][eppos*4] = patterncopybuffer[c*4];
        song.pattern[epnum[epchn]][eppos*4+1] = patterncopybuffer[c*4+1];
        song.pattern[epnum[epchn]][eppos*4+2] = patterncopybuffer[c*4+2];
        song.pattern[epnum[epchn]][eppos*4+3] = patterncopybuffer[c*4+3];
        eppos++;
      }
    }
    break;

    case KEY_DEL:
    if (epmark.chn == epchn) epmark.chn = -1;
    if ((getPattlen(epnum[epchn])-eppos)*4-4 >= 0)
    {
      std::memmove(&song.pattern[epnum[epchn]][eppos*4],
        &song.pattern[epnum[epchn]][eppos*4+4],
        (getPattlen(epnum[epchn])-eppos)*4-4);
      song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-4] = REST;
      song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-3] = 0x00;
      song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-2] = 0x00;
      song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-1] = 0x00;
    }
    else
    {
      if ((eppos > 1) && (eppos == getPattlen(epnum[epchn])))
      {
        song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-4] = ENDPATT;
        song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-3] = 0x00;
        song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-2] = 0x00;
        song.pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-1] = 0x00;
        countthispattern();
        eppos = getPattlen(epnum[epchn]);
      }
    }
    break;

    case KEY_INS:
    if (epmark.chn == epchn) epmark.chn = -1;
    if ((getPattlen(epnum[epchn])-eppos)*4-4 >= 0)
    {
      std::memmove(&song.pattern[epnum[epchn]][eppos*4+4],
        &song.pattern[epnum[epchn]][eppos*4],
        (getPattlen(epnum[epchn])-eppos)*4-4);
      song.pattern[epnum[epchn]][eppos*4] = REST;
      song.pattern[epnum[epchn]][eppos*4+1] = 0x00;
      song.pattern[epnum[epchn]][eppos*4+2] = 0x00;
      song.pattern[epnum[epchn]][eppos*4+3] = 0x00;
    }
    else
    {
      if ((eppos < MAX_PATTROWS) && (eppos == getPattlen(epnum[epchn])))
      {
        song.pattern[epnum[epchn]][eppos*4] = REST;
        song.pattern[epnum[epchn]][eppos*4+1] = 0x00;
        song.pattern[epnum[epchn]][eppos*4+2] = 0x00;
        song.pattern[epnum[epchn]][eppos*4+3] = 0x00;
        song.pattern[epnum[epchn]][eppos*4+4] = ENDPATT;
        song.pattern[epnum[epchn]][eppos*4+5] = 0x00;
        song.pattern[epnum[epchn]][eppos*4+6] = 0x00;
        song.pattern[epnum[epchn]][eppos*4+7] = 0x00;
        countthispattern();
        eppos = getPattlen(epnum[epchn]);
      }
    }
    break;

    case KEY_SPACE:
    if (!input.shiftpressed)
      recordmode = !recordmode;
    else
    {
      if (getlastsonginit() != PLAY_PATTERN)
      {
        if (eseditpos != espos[eschn])
        {
          for (int c = 0; c < maxChns; c++)
          {
            if (eseditpos < song.len[esnum][c]) espos[c] = eseditpos;
            if (esend[c] <= espos[c]) esend[c] = 0;
          }
        }
        initsongpos(esnum, PLAY_POS, eppos);
      }
      else initsongpos(esnum, PLAY_PATTERN, eppos);
      followplay = false;
    }
    break;

    case KEY_RIGHT:
    if (!input.shiftpressed)
    {
      epcolumn++;
      if (epcolumn >= 6)
      {
        epcolumn = 0;
        epchn++;
        if (epchn >= maxChns) epchn = 0;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
      }
    }
    else
    {
      if (epnum[epchn] < MAX_PATT-1)
      {
        epnum[epchn]++;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
      }
      if (epchn == epmark.chn) epmark.chn = -1;
    }
    break;

    case KEY_LEFT:
    if (!input.shiftpressed)
    {
      epcolumn--;
      if (epcolumn < 0)
      {
        epcolumn = 5;
        epchn--;
        if (epchn < 0) epchn = maxChns-1;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
      }
    }
    else
    {
      if (epnum[epchn] > 0)
      {
        epnum[epchn]--;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
      }
      if (epchn == epmark.chn) epmark.chn = -1;
    }
    break;

    case KEY_HOME:
    patternup(eppos);
    break;

    case KEY_END:
    patterndown(getPattlen(epnum[epchn]) - eppos);
    break;

    case KEY_PGUP:
    patternup(PGUPDNREPEAT);
    break;

    case KEY_PGDN:
    patterndown(PGUPDNREPEAT);
    break;

    case KEY_UP:
    patternup();
    break;

    case KEY_DOWN:
    patterndown();
    break;

    case KEY_APOST2:
    if (!input.shiftpressed)
    {
      epchn++;
      if (epchn >= maxChns) epchn = 0;
      if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
    }
    else
    {
      epchn--;
      if (epchn < 0) epchn = maxChns-1;
      if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
    }
    break;

    case KEY_1:
    case KEY_2:
    case KEY_3:
    if (input.shiftpressed)
      mutechannel(input.rawkey - KEY_1);
    break;
    case KEY_4:
    case KEY_5:
    case KEY_6:
    if (input.shiftpressed && (config.numsids == 2))
    {
        mutechannel(input.rawkey - KEY_1);
    }
    break;
  }
  if ((config.keypreset == KEY_DMC) && (hexnybble >= 0) && (hexnybble <= 7) && (!epcolumn))
  {
    int oldbyte = song.pattern[epnum[epchn]][eppos*4];
    epoctave = hexnybble;
    if ((oldbyte >= FIRSTNOTE) && (oldbyte <= LASTNOTE))
    {

      if (recordmode)
      {
        int oldnote = (oldbyte - FIRSTNOTE) %12;
        int newbyte = oldnote+epoctave*12 + FIRSTNOTE;
        if (newbyte <= LASTNOTE)
        {
          song.pattern[epnum[epchn]][eppos*4] = newbyte;
        }

        if (autoadvance < 1)
        {
          eppos++;
          if (eppos > getPattlen(epnum[epchn]))
          {
            eppos = 0;
          }
        }
      }
    }
  }

  if ((hexnybble >= 0) && (epcolumn) && recordmode)
  {
    if (eppos < getPattlen(epnum[epchn]))
    {
      switch(epcolumn)
      {
        case 1:
        song.pattern[epnum[epchn]][eppos*4+1] &= 0x0f;
        song.pattern[epnum[epchn]][eppos*4+1] |= hexnybble << 4;
        song.pattern[epnum[epchn]][eppos*4+1] &= (MAX_INSTR - 1);
        break;

        case 2:
        song.pattern[epnum[epchn]][eppos*4+1] &= 0xf0;
        song.pattern[epnum[epchn]][eppos*4+1] |= hexnybble;
        song.pattern[epnum[epchn]][eppos*4+1] &= (MAX_INSTR - 1);
        break;

        case 3:
        song.pattern[epnum[epchn]][eppos*4+2] = hexnybble;
        if (!song.pattern[epnum[epchn]][eppos*4+2])
          song.pattern[epnum[epchn]][eppos*4+3] = 0;
        break;

        case 4:
        song.pattern[epnum[epchn]][eppos*4+3] &= 0x0f;
        song.pattern[epnum[epchn]][eppos*4+3] |= hexnybble << 4;
        if (!song.pattern[epnum[epchn]][eppos*4+2])
          song.pattern[epnum[epchn]][eppos*4+3] = 0;
        break;

        case 5:
        song.pattern[epnum[epchn]][eppos*4+3] &= 0xf0;
        song.pattern[epnum[epchn]][eppos*4+3] |= hexnybble;
        if (!song.pattern[epnum[epchn]][eppos*4+2])
          song.pattern[epnum[epchn]][eppos*4+3] = 0;
        break;
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
  updateview();
}

void orderlistcommands()
{
  int maxChns = config.getMaxChannels();

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

  switch(input.key)
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
  switch(input.rawkey)
  {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    if (input.shiftpressed)
    {
      int schn = eschn;
      int tchn = 0;

      esmark.chn = -1;
      switch (input.rawkey)
      {
      case KEY_1: tchn = 0; break;
      case KEY_2: tchn = 1; break;
      case KEY_3: tchn = 2; break;
      }
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
    if (input.shiftpressed)
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
    if (input.shiftpressed)
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
    if (input.shiftpressed)
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
    if (input.shiftpressed)
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
    if (!input.shiftpressed)
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
    if (!input.shiftpressed)
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
      if (!input.shiftpressed)
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
    if (input.shiftpressed) esmark.chn = -1;
    break;

    case KEY_DOWN:
    eschn++;
    if (eschn >= maxChns) eschn = 0;
    if ((eseditpos == song.len[esnum][eschn]) || (eseditpos > song.len[esnum][eschn]+1))
    {
      eseditpos = song.len[esnum][eschn]+1;
      escolumn = 0;
    }
    if (input.shiftpressed) esmark.chn = -1;
    break;
  }
  if (eseditpos - esview[eschn] < 0)
  {
    esview[eschn] = eseditpos;
  }
  int visibleOrderlist = config.getVisibleOrderlist();
  if (eseditpos - esview[eschn] >= visibleOrderlist)
  {
    esview[eschn] = eseditpos - visibleOrderlist + 1;
  }
}

void instrumentcommands()
{
  switch(input.rawkey)
  {
    case KEY_CANC:
    case KEY_DEL:
    if ((einum) && (input.shiftpressed) && (eipos < 9))
    {
      song.deleteinstrtable(einum);
      clearinstr(einum);
    }
    break;

    case KEY_X:
    if ((einum) && (input.shiftpressed) && (eipos < 9))
    {
      cutinstr = einum;
      std::memcpy(&instrcopybuffer, &song.instr[einum], sizeof(Instr));
      clearinstr(einum);
    }
    break;

    case KEY_C:
    if ((einum) && (input.shiftpressed) && (eipos < 9))
    {
      cutinstr = -1;
      std::memcpy(&instrcopybuffer, &song.instr[einum], sizeof(Instr));
    }
    break;

    case KEY_S:
    if ((einum) && (input.shiftpressed) && (eipos < 9))
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
    if ((einum) && (input.shiftpressed) && (eipos < 9))
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
    if ((eipos != 9) && (input.shiftpressed))
    {
      eipos = 9;
      return;
    }
    break;

    case KEY_U:
    if (input.shiftpressed)
    {
      tables.fliplock();
      tables.validatetableview();
    }
    break;

    case KEY_SPACE:
    if (eipos != 9)
    {
      if (!input.shiftpressed)
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
          if ((eipos == 5) && (input.shiftpressed))
          {
            song.instr[einum].ptr[STBL] = makespeedtable(song.instr[einum].ptr[STBL], config.finevibrato, true) + 1;
            break;
          }
          pos = song.instr[einum].ptr[eipos-2] - 1;
        }
        else
        {
          pos = song.gettablelen(eipos-2);
          if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
          if (input.shiftpressed) song.instr[einum].ptr[eipos-2] = pos + 1;
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
      eicolumn = 0;
      eipos++;
      if (eipos >= 9) eipos = 0;
      break;
    }
  }
  // Validate instrument parameters
  if (einum)
  {
    if (!(song.instr[einum].gatetimer & 0x3f)) song.instr[einum].gatetimer |= 1;
  }
}

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
    if (tables.inccolumn())
    {
      tables.m_pos -= tables.curview();
      tables.incnum();
      tables.m_pos += tables.curview();
    }
    if (input.shiftpressed) tables.resetmarknum();
    break;

    case KEY_LEFT:
    if (tables.deccolumn())
    {
      tables.m_pos -= tables.curview();
      tables.decnum();
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
          tables.incpos();
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
      for (c = steps; c > 1; c--) song.inserttable(tables.num(), tables.pos(), true);

      while (time)
      {
        if (std::abs(speed) < 128)
        {
          song.ltable[tables.num()][tables.pos()] = (time < 127) ? time : 127;
          song.rtable[tables.num()][tables.pos()] = speed;
          time -= song.ltable[tables.num()][tables.pos()];
        }
        else
        {
          currentpulse += speed;
          song.ltable[tables.num()][tables.pos()] = 0x80 | ((currentpulse >> 8) & 0xf);
          song.rtable[tables.num()][tables.pos()] = currentpulse & 0xff;
          time--;
        }
        tables.m_pos++;
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
      for (c = steps; c > 1; c--) song.inserttable(tables.num(), tables.pos(), true);

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
            int oldeditcolumn = tables.column();
            int pos = makespeedtable(song.rtable[tables.num()][tables.pos()], mstmode, true);
            gototable(WTBL, oldeditpos);
            tables.setcolumn(oldeditcolumn);

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
      tables.decnum();
      tables.m_pos += tables.curview();
    }
    else
    {
      tables.m_pos -= tables.curview();
      tables.incnum();
      tables.m_pos += tables.curview();
    }
  }

  if (hexnybble >= 0)
  {
    switch(tables.column())
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
    if (tables.inccolumn())
    {
      tables.incpos();
    }
  }

  tables.validatetableview();
}

void orderleft()
{
  if ((input.shiftpressed) && (eseditpos < song.len[esnum][eschn]))
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
  if ((input.shiftpressed) && (eseditpos < song.len[esnum][eschn])) esmark.end = eseditpos;
}

void orderright()
{
  if ((input.shiftpressed) && (eseditpos < song.len[esnum][eschn]))
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
  if ((input.shiftpressed) && (eseditpos < song.len[esnum][eschn])) esmark.end = eseditpos;
}

void namecommands()
{
  switch(input.rawkey)
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
