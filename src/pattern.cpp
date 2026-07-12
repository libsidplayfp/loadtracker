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

#include "configfile.h"
#include "console.h"
#include "instr.h"
#include "loadtrk.h"
#include "order.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "song.h"
#include "table.h"

#include "bme_main.h"

#include <cstring>

unsigned char notekeytbl1[] = {KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_V,
  KEY_G, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M, KEY_COMMA, KEY_L, KEY_COLON};

unsigned char notekeytbl2[] = {KEY_Q, KEY_2, KEY_W, KEY_3, KEY_E, KEY_R,
  KEY_5, KEY_T, KEY_6, KEY_Y, KEY_7, KEY_U, KEY_I, KEY_9, KEY_O, KEY_0, KEY_P};

unsigned char dmckeytbl[] = {KEY_A, KEY_W, KEY_S, KEY_E, KEY_D, KEY_F,
  KEY_T, KEY_G, KEY_Y, KEY_H, KEY_U, KEY_J, KEY_K, KEY_O, KEY_L, KEY_P};

unsigned char jankokeytbl1[] = {KEY_Z, KEY_S, KEY_X, KEY_D, KEY_C, KEY_F, KEY_V,
  KEY_G, KEY_B, KEY_H, KEY_N, KEY_J, KEY_M, KEY_K, KEY_COMMA, KEY_L, KEY_COLON};

unsigned char jankokeytbl2[] = {KEY_Q, KEY_2, KEY_W, KEY_3, KEY_E, KEY_4, KEY_R,
  KEY_5, KEY_T, KEY_6, KEY_Y, KEY_7, KEY_U, KEY_8, KEY_I, KEY_9, KEY_O, KEY_0, KEY_P};

unsigned char patterncopybuffer[MAX_PATTROWS*4+4];
unsigned char cmdcopybuffer[MAX_PATTROWS*4+4];
int patterncopyrows = 0;
int cmdcopyrows = 0;

int epnum[MAX_CHN];
int eppos;
int epview;
int epcolumn;
int epchn;
int epoctave = 2;
int epmarkchn = -1;
int epmarkstart;
int epmarkend;
int autoadvance = 0;

void shrinkpattern();
void expandpattern();
void splitpattern();
void joinpattern();

void insertnote(int newnote)
{
    if (recordmode && (eppos < getPattlen(epnum[epchn])))
    {
        pattern[epnum[epchn]][eppos*4] = newnote;
        if (newnote < REST)
        {
            pattern[epnum[epchn]][eppos*4+1] = einum;
        }
        else
        {
            pattern[epnum[epchn]][eppos*4+1] = 0;
        }
        if ((shiftpressed) && (newnote == REST))
        {
            pattern[epnum[epchn]][eppos*4+2] = 0;
            pattern[epnum[epchn]][eppos*4+3] = 0;
        }
    }

    if (recordmode)
    {
        if (autoadvance < 2)
        {
            eppos++;
            if (eppos > getPattlen(epnum[epchn]))
            {
                eppos = 0;
            }
        }
    }
    playtestnote(newnote, einum, epchn);
}

void patterncommands()
{
  int maxChns = getMaxChannels();

  switch(key)
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
    if (key)
    {
      switch (keypreset)
      {
        case KEY_TRACKER:
        for (size_t i = 0; i < sizeof(notekeytbl1); i++)
        {
          if ((rawkey == notekeytbl1[i]) && (!epcolumn) && (!shiftpressed) && (!altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        for (size_t i = 0; i < sizeof(notekeytbl2); i++)
        {
          if ((rawkey == notekeytbl2[i]) && (!epcolumn) && (!shiftpressed) && (!altpressed))
          {
            newnote = FIRSTNOTE+i+(epoctave+1)*12;
          }
        }
        break;

        case KEY_DMC:
        for (size_t i = 0; i < sizeof(dmckeytbl); i++)
        {
          if ((rawkey == dmckeytbl[i]) && (!epcolumn) && (!shiftpressed) && (!altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        break;

        case KEY_JANKO:
        for (size_t i = 0; i < sizeof(jankokeytbl1); i++)
        {
          if ((rawkey == jankokeytbl1[i]) && (!epcolumn) && (!shiftpressed) && (!altpressed))
          {
            newnote = FIRSTNOTE+i+epoctave*12;
          }
        }
        for (size_t i = 0; i < sizeof(jankokeytbl2); i++)
        {
          if ((rawkey == jankokeytbl2[i]) && (!epcolumn) && (!shiftpressed) && (!altpressed))
          {
            newnote = FIRSTNOTE+i+(epoctave+1)*12;
          }
        }
        break;
      }
    }

    if (newnote > LASTNOTE) newnote = -1;
    if ((rawkey == KEY_BACKSPACE) && (!epcolumn)) newnote = REST;
    if ((rawkey == KEY_CAPSLOCK) && (!epcolumn)) newnote = KEYOFF;
    if (rawkey == KEY_ENTER)
    {
      switch(epcolumn)
      {
        case 0:
        if (shiftpressed)
          newnote = KEYON;
        else
          newnote = KEYOFF;
        break;

        case 1:
        case 2:
        if (pattern[epnum[epchn]][eppos*4+1])
        {
          gotoinstr(pattern[epnum[epchn]][eppos*4+1]);
          return;
        }
        break;

        default:
        switch (pattern[epnum[epchn]][eppos*4+2])
        {
          case CMD_SETWAVEPTR:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(WTBL, pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (shiftpressed)
            {
              int pos = gettablelen(WTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(WTBL, pos);
              return;
            }
          }
          break;

          case CMD_SETPULSEPTR:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(PTBL, pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (shiftpressed)
            {
              int pos = gettablelen(PTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(PTBL, pos);
              return;
            }
          }
          break;

          case CMD_SETFILTERPTR:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            gototable(FTBL, pattern[epnum[epchn]][eppos*4+3] - 1);
            return;
          }
          else
          {
            if (shiftpressed)
            {
              int pos = gettablelen(FTBL);
              if (pos >= MAX_TABLELEN-1) pos = MAX_TABLELEN - 1;
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
              gototable(FTBL, pos);
              return;
            }
          }
          break;

          case CMD_FUNKTEMPO:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            if (!shiftpressed)
            {
              gototable(STBL, pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(pattern[epnum[epchn]][eppos*4+3], MST_FUNKTEMPO, true);
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (shiftpressed)
            {
              int pos = findfreespeedtable();
              if (pos >= 0)
              {
                pattern[epnum[epchn]][eppos*4+3] = pos + 1;
                gototable(STBL, pos);
                return;
              }
            }
          }
          break;

          case CMD_PORTAUP:
          case CMD_PORTADOWN:
          case CMD_TONEPORTA:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            if (!shiftpressed)
            {
              gototable(STBL, pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(pattern[epnum[epchn]][eppos*4+3], MST_PORTAMENTO, true);
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (shiftpressed)
            {
              int pos = findfreespeedtable();
              if (pos >= 0)
              {
                pattern[epnum[epchn]][eppos*4+3] = pos + 1;
                gototable(STBL, pos);
                return;
              }
            }
          }
          break;

          case CMD_VIBRATO:
          if (pattern[epnum[epchn]][eppos*4+3])
          {
            if (!shiftpressed)
            {
              gototable(STBL, pattern[epnum[epchn]][eppos*4+3] - 1);
              return;
            }
            else
            {
              int pos = makespeedtable(pattern[epnum[epchn]][eppos*4+3], finevibrato, true);
              pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            }
          }
          else
          {
            if (shiftpressed)
            {
              int pos = findfreespeedtable();
              if (pos >= 0)
              {
                pattern[epnum[epchn]][eppos*4+3] = pos + 1;
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
  switch(rawkey)
  {
    case KEY_O:
    if (shiftpressed) shrinkpattern();
    break;

    case KEY_P:
    if (shiftpressed) expandpattern();
    break;

    case KEY_J:
    if (shiftpressed) joinpattern();
    break;

    case KEY_K:
    if (shiftpressed) splitpattern();
    break;

    case KEY_Z:
    if (shiftpressed)
    {
      autoadvance++;
      if (autoadvance > 2) autoadvance = 0;
      if (keypreset == KEY_TRACKER)
      {
        if (autoadvance == 1) autoadvance = 2;
      }
    }
    break;

    case KEY_E:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart < epmarkend)
        {
          int d = 0;
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            cmdcopybuffer[d*4+2] = pattern[epnum[epmarkchn]][c*4+2];
            cmdcopybuffer[d*4+3] = pattern[epnum[epmarkchn]][c*4+3];
            d++;
          }
          cmdcopyrows = d;
        }
        else
        {
          int d = 0;
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            cmdcopybuffer[d*4+2] = pattern[epnum[epmarkchn]][c*4+2];
            cmdcopybuffer[d*4+3] = pattern[epnum[epmarkchn]][c*4+3];
            d++;
          }
          cmdcopyrows = d;
        }
        epmarkchn = -1;
      }
      else
      {
        if (eppos < getPattlen(epnum[epchn]))
        {
          cmdcopybuffer[2] = pattern[epnum[epchn]][eppos*4+2];
          cmdcopybuffer[3] = pattern[epnum[epchn]][eppos*4+3];
          cmdcopyrows = 1;
        }
      }
    }
    break;

    case KEY_R:
    if (shiftpressed)
    {
      for (int c = 0; c < cmdcopyrows; c++)
      {
        if (eppos >= getPattlen(epnum[epchn])) break;
        pattern[epnum[epchn]][eppos*4+2] = cmdcopybuffer[c*4+2];
        pattern[epnum[epchn]][eppos*4+3] = cmdcopybuffer[c*4+3];
        eppos++;
      }
    }
    break;

    case KEY_I:
    if (shiftpressed)
    {
      char temp;
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          int e = epmarkend;
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            for (int d = 0; d < 4; d++)
            {
              temp = pattern[epnum[epmarkchn]][c*4+d];
              pattern[epnum[epmarkchn]][c*4+d] = pattern[epnum[epmarkchn]][e*4+d];
              pattern[epnum[epmarkchn]][e*4+d] = temp;
            }
            e--;
            if (e < c) break;
          }
        }
        else
        {
          int e = epmarkstart;
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            for (int d = 0; d < 4; d++)
            {
              temp = pattern[epnum[epmarkchn]][c*4+d];
              pattern[epnum[epmarkchn]][c*4+d] = pattern[epnum[epmarkchn]][e*4+d];
              pattern[epnum[epmarkchn]][e*4+d] = temp;
            }
            e--;
            if (e < c) break;
          }
        }
      }
      else
      {
        int e = getPattlen(epnum[epchn]) - 1;
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          for (int d = 0; d < 4; d++)
          {
            temp = pattern[epnum[epchn]][c*4+d];
            pattern[epnum[epchn]][c*4+d] = pattern[epnum[epchn]][e*4+d];
            pattern[epnum[epchn]][e*4+d] = temp;
          }
          e--;
          if (e < c) break;
        }
      }
    }
    break;

    case KEY_Q:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] < LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
              pattern[epnum[epmarkchn]][c*4]++;
          }
        }
        else
        {
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] < LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
              pattern[epnum[epmarkchn]][c*4]++;
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((pattern[epnum[epchn]][c*4] < LASTNOTE) &&
              (pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
            pattern[epnum[epchn]][c*4]++;
        }
      }
    }
    break;

    case KEY_A:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] > FIRSTNOTE))
              pattern[epnum[epmarkchn]][c*4]--;
          }
        }
        else
        {
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] > FIRSTNOTE))
              pattern[epnum[epmarkchn]][c*4]--;
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (pattern[epnum[epchn]][c*4] > FIRSTNOTE))
            pattern[epnum[epchn]][c*4]--;
        }
      }
    }
    break;

    case KEY_W:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
            {
              pattern[epnum[epmarkchn]][c*4] += 12;
              if (pattern[epnum[epmarkchn]][c*4] > LASTNOTE)
                pattern[epnum[epmarkchn]][c*4] = LASTNOTE;
            }
          }
        }
        else
        {
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
            {
              pattern[epnum[epmarkchn]][c*4] += 12;
              if (pattern[epnum[epmarkchn]][c*4] > LASTNOTE)
                pattern[epnum[epmarkchn]][c*4] = LASTNOTE;
            }
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
          {
            pattern[epnum[epchn]][c*4] += 12;
            if (pattern[epnum[epchn]][c*4] > LASTNOTE)
              pattern[epnum[epchn]][c*4] = LASTNOTE;
          }
        }
      }
    }
    break;

    case KEY_S:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
            {
              pattern[epnum[epmarkchn]][c*4] -= 12;
              if (pattern[epnum[epmarkchn]][c*4] < FIRSTNOTE)
                pattern[epnum[epmarkchn]][c*4] = FIRSTNOTE;
            }
          }
        }
        else
        {
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            if ((pattern[epnum[epmarkchn]][c*4] <= LASTNOTE) &&
                (pattern[epnum[epmarkchn]][c*4] >= FIRSTNOTE))
            {
              pattern[epnum[epmarkchn]][c*4] -= 12;
              if (pattern[epnum[epmarkchn]][c*4] < FIRSTNOTE)
                pattern[epnum[epmarkchn]][c*4] = FIRSTNOTE;
            }
          }
        }
      }
      else
      {
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          if ((pattern[epnum[epchn]][c*4] <= LASTNOTE) &&
              (pattern[epnum[epchn]][c*4] >= FIRSTNOTE))
          {
            pattern[epnum[epchn]][c*4] -= 12;
            if (pattern[epnum[epchn]][c*4] < FIRSTNOTE)
              pattern[epnum[epchn]][c*4] = FIRSTNOTE;
          }
        }
      }
    }
    break;

    case KEY_M:
    if (shiftpressed)
    {
      stepsize++;
      if (stepsize > MAX_PATTROWS) stepsize = MAX_PATTROWS;
    }
    break;

    case KEY_N:
    if (shiftpressed)
    {
      stepsize--;
      if (stepsize < 2) stepsize = 2;
    }
    break;

    case KEY_H:
    if (shiftpressed)
    {
      int c;
      switch (pattern[epnum[epchn]][eppos*4+2])
      {
        case CMD_PORTAUP:
        case CMD_PORTADOWN:
        case CMD_VIBRATO:
        case CMD_TONEPORTA:
        if (pattern[epnum[epchn]][eppos*4+2] == CMD_TONEPORTA)
          c = eppos-1;
        else
          c = eppos;
        for (; c >= 0; c--)
        {
          if ((pattern[epnum[epchn]][c*4] >= FIRSTNOTE) &&
              (pattern[epnum[epchn]][c*4] <= LASTNOTE))
          {
            int note = pattern[epnum[epchn]][c*4] - FIRSTNOTE;
            int right = pattern[epnum[epchn]][eppos*4+3] & 0xf;
            int left = pattern[epnum[epchn]][eppos*4+3] >> 4;

            if (note > MAX_NOTES-1) note--;
            int pitch1 = freqtbllo[note] | (freqtblhi[note] << 8);
            int pitch2 = freqtbllo[note+1] | (freqtblhi[note+1] << 8);
            int delta = pitch2 - pitch1;

            while (left--) delta <<= 1;
            while (right--) delta >>= 1;

            if (pattern[epnum[epchn]][eppos*4+2] == CMD_VIBRATO)
            {
              if (delta > 0xff) delta = 0xff;
            }
            int pos = makespeedtable(delta, MST_RAW, true);
            pattern[epnum[epchn]][eppos*4+3] = pos + 1;
            break;
          }
        }
        break;
      }
    }
    break;

    case KEY_L:
    if (shiftpressed)
    {
      if (epmarkchn == -1)
      {
        epmarkchn = epchn;
        epmarkstart = 0;
        epmarkend = getPattlen(epnum[epchn])-1;
      }
      else epmarkchn = -1;
    }
    break;

    case KEY_C:
    case KEY_X:
    if (shiftpressed)
    {
      if (epmarkchn != -1)
      {
        if (epmarkstart <= epmarkend)
        {
          int d = 0;
          for (int c = epmarkstart; c <= epmarkend; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            patterncopybuffer[d*4] = pattern[epnum[epmarkchn]][c*4];
            patterncopybuffer[d*4+1] = pattern[epnum[epmarkchn]][c*4+1];
            patterncopybuffer[d*4+2] = pattern[epnum[epmarkchn]][c*4+2];
            patterncopybuffer[d*4+3] = pattern[epnum[epmarkchn]][c*4+3];
            if (rawkey == KEY_X)
            {
              pattern[epnum[epmarkchn]][c*4] = REST;
              pattern[epnum[epmarkchn]][c*4+1] = 0;
              pattern[epnum[epmarkchn]][c*4+2] = 0;
              pattern[epnum[epmarkchn]][c*4+3] = 0;
            }
            d++;
          }
          patterncopyrows = d;
        }
        else
        {
          int d = 0;
          for (int c = epmarkend; c <= epmarkstart; c++)
          {
            if (c >= getPattlen(epnum[epmarkchn])) break;
            patterncopybuffer[d*4] = pattern[epnum[epmarkchn]][c*4];
            patterncopybuffer[d*4+1] = pattern[epnum[epmarkchn]][c*4+1];
            patterncopybuffer[d*4+2] = pattern[epnum[epmarkchn]][c*4+2];
            patterncopybuffer[d*4+3] = pattern[epnum[epmarkchn]][c*4+3];
            if (rawkey == KEY_X)
            {
              pattern[epnum[epmarkchn]][c*4] = REST;
              pattern[epnum[epmarkchn]][c*4+1] = 0;
              pattern[epnum[epmarkchn]][c*4+2] = 0;
              pattern[epnum[epmarkchn]][c*4+3] = 0;
            }
            d++;
          }
          patterncopyrows = d;
        }
        epmarkchn = -1;
      }
      else
      {
        int d = 0;
        for (int c = 0; c < getPattlen(epnum[epchn]); c++)
        {
          patterncopybuffer[d*4] = pattern[epnum[epchn]][c*4];
          patterncopybuffer[d*4+1] = pattern[epnum[epchn]][c*4+1];
          patterncopybuffer[d*4+2] = pattern[epnum[epchn]][c*4+2];
          patterncopybuffer[d*4+3] = pattern[epnum[epchn]][c*4+3];
          if (rawkey == KEY_X)
          {
            pattern[epnum[epchn]][c*4] = REST;
            pattern[epnum[epchn]][c*4+1] = 0;
            pattern[epnum[epchn]][c*4+2] = 0;
            pattern[epnum[epchn]][c*4+3] = 0;
          }
          d++;
        }
        patterncopyrows = d;
      }
    }
    break;

    case KEY_V:
    if ((shiftpressed) && (patterncopyrows))
    {
      for (int c = 0; c < patterncopyrows; c++)
      {
        if (eppos >= getPattlen(epnum[epchn])) break;
        pattern[epnum[epchn]][eppos*4] = patterncopybuffer[c*4];
        pattern[epnum[epchn]][eppos*4+1] = patterncopybuffer[c*4+1];
        pattern[epnum[epchn]][eppos*4+2] = patterncopybuffer[c*4+2];
        pattern[epnum[epchn]][eppos*4+3] = patterncopybuffer[c*4+3];
        eppos++;
      }
    }
    break;

    case KEY_DEL:
    if (epmarkchn == epchn) epmarkchn = -1;
    if ((getPattlen(epnum[epchn])-eppos)*4-4 >= 0)
    {
      std::memmove(&pattern[epnum[epchn]][eppos*4],
        &pattern[epnum[epchn]][eppos*4+4],
        (getPattlen(epnum[epchn])-eppos)*4-4);
      pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-4] = REST;
      pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-3] = 0x00;
      pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-2] = 0x00;
      pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-1] = 0x00;
    }
    else
    {
      if (eppos == getPattlen(epnum[epchn]))
      {
        if (getPattlen(epnum[epchn]) > 1)
        {
          pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-4] = ENDPATT;
          pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-3] = 0x00;
          pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-2] = 0x00;
          pattern[epnum[epchn]][getPattlen(epnum[epchn])*4-1] = 0x00;
          countthispattern();
          eppos = getPattlen(epnum[epchn]);
        }
      }
    }
    break;

    case KEY_INS:
    if (epmarkchn == epchn) epmarkchn = -1;
    if ((getPattlen(epnum[epchn])-eppos)*4-4 >= 0)
    {
      std::memmove(&pattern[epnum[epchn]][eppos*4+4],
        &pattern[epnum[epchn]][eppos*4],
        (getPattlen(epnum[epchn])-eppos)*4-4);
      pattern[epnum[epchn]][eppos*4] = REST;
      pattern[epnum[epchn]][eppos*4+1] = 0x00;
      pattern[epnum[epchn]][eppos*4+2] = 0x00;
      pattern[epnum[epchn]][eppos*4+3] = 0x00;
    }
    else
    {
      if (eppos == getPattlen(epnum[epchn]))
      {
        if (getPattlen(epnum[epchn]) < MAX_PATTROWS)
        {
          pattern[epnum[epchn]][eppos*4] = REST;
          pattern[epnum[epchn]][eppos*4+1] = 0x00;
          pattern[epnum[epchn]][eppos*4+2] = 0x00;
          pattern[epnum[epchn]][eppos*4+3] = 0x00;
          pattern[epnum[epchn]][eppos*4+4] = ENDPATT;
          pattern[epnum[epchn]][eppos*4+5] = 0x00;
          pattern[epnum[epchn]][eppos*4+6] = 0x00;
          pattern[epnum[epchn]][eppos*4+7] = 0x00;
          countthispattern();
          eppos = getPattlen(epnum[epchn]);
        }
      }
    }
    break;

    case KEY_SPACE:
    if (!shiftpressed)
      recordmode = !recordmode;
    else
    {
      if (lastsonginit != PLAY_PATTERN)
      {
        if (eseditpos != espos[eschn])
        {
          for (int c = 0; c < maxChns; c++)
          {
            if (eseditpos < songlen[esnum][c]) espos[c] = eseditpos;
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
    if (!shiftpressed)
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
      if (epchn == epmarkchn) epmarkchn = -1;
    }
    break;

    case KEY_LEFT:
    if (!shiftpressed)
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
      if (epchn == epmarkchn) epmarkchn = -1;
    }
    break;

    case KEY_HOME:
    while (eppos != 0) patternup();
    break;

    case KEY_END:
    while (eppos != getPattlen(epnum[epchn])) patterndown();
    break;

    case KEY_PGUP:
    for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      patternup();
    break;

    case KEY_PGDN:
    for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      patterndown();
    break;

    case KEY_UP:
    patternup();
    break;

    case KEY_DOWN:
    patterndown();
    break;

    case KEY_APOST2:
    if (!shiftpressed)
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
    if (shiftpressed)
      mutechannel(rawkey - KEY_1);
    break;
    case KEY_4:
    case KEY_5:
    case KEY_6:
    if (shiftpressed && (numsids == 2))
    {
        mutechannel(rawkey - KEY_1);
    }
    break;
  }
  if ((keypreset == KEY_DMC) && (hexnybble >= 0) && (hexnybble <= 7) && (!epcolumn))
  {
    int oldbyte = pattern[epnum[epchn]][eppos*4];
    epoctave = hexnybble;
    if ((oldbyte >= FIRSTNOTE) && (oldbyte <= LASTNOTE))
    {
      int newbyte;
      int oldnote = (oldbyte - FIRSTNOTE) %12;

      if (recordmode)
      {
        newbyte = oldnote+epoctave*12 + FIRSTNOTE;
        if (newbyte <= LASTNOTE)
        {
          pattern[epnum[epchn]][eppos*4] = newbyte;
        }
      }
      if (recordmode && (autoadvance < 1))
      {
        eppos++;
        if (eppos > getPattlen(epnum[epchn]))
        {
          eppos = 0;
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
        pattern[epnum[epchn]][eppos*4+1] &= 0x0f;
        pattern[epnum[epchn]][eppos*4+1] |= hexnybble << 4;
        pattern[epnum[epchn]][eppos*4+1] &= (MAX_INSTR - 1);
        break;

        case 2:
        pattern[epnum[epchn]][eppos*4+1] &= 0xf0;
        pattern[epnum[epchn]][eppos*4+1] |= hexnybble;
        pattern[epnum[epchn]][eppos*4+1] &= (MAX_INSTR - 1);
        break;

        case 3:
        pattern[epnum[epchn]][eppos*4+2] = hexnybble;
        if (!pattern[epnum[epchn]][eppos*4+2])
          pattern[epnum[epchn]][eppos*4+3] = 0;
        break;

        case 4:
        pattern[epnum[epchn]][eppos*4+3] &= 0x0f;
        pattern[epnum[epchn]][eppos*4+3] |= hexnybble << 4;
        if (!pattern[epnum[epchn]][eppos*4+2])
          pattern[epnum[epchn]][eppos*4+3] = 0;
        break;

        case 5:
        pattern[epnum[epchn]][eppos*4+3] &= 0xf0;
        pattern[epnum[epchn]][eppos*4+3] |= hexnybble;
        if (!pattern[epnum[epchn]][eppos*4+2])
          pattern[epnum[epchn]][eppos*4+3] = 0;
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
  epview = eppos-VISIBLEPATTROWS/2;
}


void patterndown()
{
  if (shiftpressed)
  {
    if ((epmarkchn != epchn) || (eppos != epmarkend))
    {
      epmarkchn = epchn;
      epmarkstart = epmarkend = eppos;
    }
  }
  eppos++;
  if (eppos > getPattlen(epnum[epchn]))
  {
    eppos = 0;
  }
  if (shiftpressed) epmarkend = eppos;
}

void patternup()
{
  if (shiftpressed)
  {
    if ((epmarkchn != epchn) || (eppos != epmarkend))
    {
      epmarkchn = epchn;
      epmarkstart = epmarkend = eppos;
    }
  }
  eppos--;
  if (eppos < 0)
  {
    eppos = getPattlen(epnum[epchn]);
  }
  if (shiftpressed) epmarkend = eppos;
}

void prevpattern()
{
  if (epnum[epchn] > 0)
  {
    epnum[epchn]--;
    if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
  }
  if (epchn == epmarkchn) epmarkchn = -1;
}

void nextpattern()
{
  if (epnum[epchn] < MAX_PATT-1)
  {
    epnum[epchn]++;
    if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);
  }
  if (epchn == epmarkchn) epmarkchn = -1;
}

void shrinkpattern()
{
  int c = epnum[epchn];

  if (getPattlen(c) < 2) return;

  stopsong();

  int l = getPattlen(c);
  int nl = l/2;

  for (int d = 0; d < nl; d++)
  {
    pattern[c][d*4] = pattern[c][d*2*4];
    pattern[c][d*4+1] = pattern[c][d*2*4+1];
    pattern[c][d*4+2] = pattern[c][d*2*4+2];
    pattern[c][d*4+3] = pattern[c][d*2*4+3];
  }

  pattern[c][nl*4] = ENDPATT;
  pattern[c][nl*4+1] = 0;
  pattern[c][nl*4+2] = 0;
  pattern[c][nl*4+3] = 0;

  eppos /= 2;

  countthispattern();
}

void expandpattern()
{
  int c = epnum[epchn];
  int l = getPattlen(c);
  int nl = l*2;
  if (nl > MAX_PATTROWS) return;

  stopsong();

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
      temp[d*4] = pattern[c][d*2];
      temp[d*4+1] = pattern[c][d*2+1];
      temp[d*4+2] = pattern[c][d*2+2];
      temp[d*4+3] = pattern[c][d*2+3];
    }
  }

  std::memcpy(pattern[c], temp, (nl+1)*4);

  eppos *= 2;

  countthispattern();
}

void splitpattern()
{
  int c = epnum[epchn];
  int l = getPattlen(c);
  int maxChns = getMaxChannels();

  if ((eppos == 0) || (eppos == l)) return;

  stopsong();

  if (insertpattern(c))
  {
    int oldesnum = esnum;
    int oldeschn = eschn;
    int oldeseditpos = eseditpos;

    for (int d = eppos; d <= l; d++)
    {
      pattern[c+1][(d-eppos)*4] = pattern[c][d*4];
      pattern[c+1][(d-eppos)*4+1] = pattern[c][d*4+1];
      pattern[c+1][(d-eppos)*4+2] = pattern[c][d*4+2];
      pattern[c+1][(d-eppos)*4+3] = pattern[c][d*4+3];
    }
    pattern[c][eppos*4] = ENDPATT;
    pattern[c][eppos*4+1] = 0;
    pattern[c][eppos*4+2] = 0;
    pattern[c][eppos*4+3] = 0;

    countpatternlengths();

    for (esnum = 0; esnum < MAX_SONGS; esnum++)
    {
      for (eschn = 0; eschn < maxChns; eschn++)
      {
        for (eseditpos = 0; eseditpos < songlen[esnum][eschn]; eseditpos++)
        {
          if (songorder[esnum][eschn][eseditpos] == c)
          {
            songorder[esnum][eschn][eseditpos] = c+1;
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
  int maxChns = getMaxChannels();

  if (eschn != epchn) return;
  if (songorder[esnum][epchn][eseditpos] != c) return;

  int d = songorder[esnum][epchn][eseditpos + 1];
  if (d >= MAX_PATT) return;
  if (getPattlen(c) + getPattlen(d) > MAX_PATTROWS) return;

  stopsong();

  if (insertpattern(c))
  {
    int oldesnum = esnum;
    int oldeschn = eschn;
    int oldeseditpos = eseditpos;
    d++;

    int e;
    for (e = 0; e < getPattlen(c); e++)
    {
      pattern[c+1][e*4] = pattern[c][e*4];
      pattern[c+1][e*4+1] = pattern[c][e*4+1];
      pattern[c+1][e*4+2] = pattern[c][e*4+2];
      pattern[c+1][e*4+3] = pattern[c][e*4+3];
    }
    for (int f = 0; f < getPattlen(d); f++)
    {
      pattern[c+1][e*4] = pattern[d][f*4];
      pattern[c+1][e*4+1] = pattern[d][f*4+1];
       pattern[c+1][e*4+2] = pattern[d][f*4+2];
       pattern[c+1][e*4+3] = pattern[d][f*4+3];
       e++;
    }
    pattern[c+1][e*4] = ENDPATT;
    pattern[c+1][e*4+1] = 0;
    pattern[c+1][e*4+2] = 0;
    pattern[c+1][e*4+3] = 0;

    countpatternlengths();

    for (esnum = 0; esnum < MAX_SONGS; esnum++)
    {
      for (eschn = 0; eschn < maxChns; eschn++)
      {
        for (eseditpos = 0; eseditpos < songlen[esnum][eschn]; eseditpos++)
        {
          if ((songorder[esnum][eschn][eseditpos] == c) && (songorder[esnum][eschn][eseditpos+1] == d))
          {
            deleteorder();
            songorder[esnum][eschn][eseditpos] = c+1;
          }
        }
      }
    }
    eschn = oldeschn;
    eseditpos = oldeseditpos;
    esnum = oldesnum;

    findusedpatterns();
    {
      int del1 = pattused[c];
      int del2 = pattused[d];

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


