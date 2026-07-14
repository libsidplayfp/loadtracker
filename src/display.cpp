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
// screen display routines
// =============================================================================

#define DISPLAY_C

#include "display.h"

#include "configfile.h"
#include "console.h"
#include "instr.h"
#include "loadtrk.h"
#include "order.h"
#include "pattern.h"
#include "play.h"
#include "song.h"
#include "sound.h"
#include "table.h"

#include "bme_main.h"
#include "bme_snd.h"
#include "bme_win.h"

#include <cstdio>
#include <cstring>

enum
{
  CBLACK  = 0x0,
  CWHITE  = 0x1,
  CDRED   = 0x2,
  CCYAN   = 0x3,
  CPURPLE = 0x4,
  CDGREEN = 0x5,
  CDBLUE  = 0x6,
  CYELLOW = 0x7,
  CLBROWN = 0x8,
  CDBROWN = 0x9,
  CLRED   = 0xA,
  CDGREY  = 0xB,
  CGREY   = 0xC,
  CLGREEN = 0xD,
  CLBLUE  = 0xE,
  CLGREY  = 0xF
};

const char *notename[] =
 {"C-0", "C#0", "D-0", "D#0", "E-0", "F-0", "F#0", "G-0", "G#0", "A-0", "A#0", "B-0",
  "C-1", "C#1", "D-1", "D#1", "E-1", "F-1", "F#1", "G-1", "G#1", "A-1", "A#1", "B-1",
  "C-2", "C#2", "D-2", "D#2", "E-2", "F-2", "F#2", "G-2", "G#2", "A-2", "A#2", "B-2",
  "C-3", "C#3", "D-3", "D#3", "E-3", "F-3", "F#3", "G-3", "G#3", "A-3", "A#3", "B-3",
  "C-4", "C#4", "D-4", "D#4", "E-4", "F-4", "F#4", "G-4", "G#4", "A-4", "A#4", "B-4",
  "C-5", "C#5", "D-5", "D#5", "E-5", "F-5", "F#5", "G-5", "G#5", "A-5", "A#5", "B-5",
  "C-6", "C#6", "D-6", "D#6", "E-6", "F-6", "F#6", "G-6", "G#6", "A-6", "A#6", "B-6",
  "C-7", "C#7", "D-7", "D#7", "E-7", "F-7", "F#7", "G-7", "G#7", "...", "---", "+++"};

char timechar[] = {':', ' '};

int timemin = 0;
int timesec = 0;
unsigned timeframe = 0;

int cursorflash = 0;
int cursorcolortable[] = { CWHITE, CLGREY, CGREY, CLGREY };

void initcolorscheme(bool dark)
{
  colors.CBKGND   = dark ? CBLACK : CDBLUE;

  colors.CNORMAL  = (dark ? CGREY : CLBLUE)|(colors.CBKGND<<4);
  colors.CMUTE    = CDGREY |(colors.CBKGND<<4);
  colors.CEDIT    = CLGREEN|(colors.CBKGND<<4);
  colors.CPLAYING = CLRED  |(colors.CBKGND<<4);
  colors.CCOMMAND = CLGREY |(colors.CBKGND<<4);
  colors.CTITLE   = CWHITE |(colors.CBKGND<<4);

  colors.CHDRBG   = dark ? CDBLUE : CLBLUE;
  colors.CHDRFG   = dark ? CWHITE : CDBLUE;

  colors.CHEADER  = colors.CHDRFG|(colors.CHDRBG<<4);
}

void printmainscreen()
{
  clearscreen();
  printstatus();
  fliptoscreen();
}

void flashCursor()
{
    if (cursorflashdelay >= 6)
    {
      cursorflashdelay %= 6;
      cursorflash++;
      cursorflash &= 3;
    }
}

int getCursorColor()
{
    return cursorcolortable[cursorflash];
}

void displayupdate()
{
  flashCursor();
  printstatus();
  fliptoscreen();
}

void printstatus()
{
  int cc = getCursorColor();
  int visibleOrderlist = getVisibleOrderlist();
  int maxChns = getMaxChannels();

  menu = false;

  if ((mouseb > MOUSEB_LEFT) && (mousey <= 1) && (!eamode)) menu = true;

  printblankc(0, 0, colors.CHEADER, MAX_COLUMNS);

  // Header
  if (!menu)
  {
    short cblue = (colors.CHDRBG == CDBLUE) ? CLBLUE : CDBLUE;
    printtext(dpos.statusTopX+1,  dpos.statusTopY,  cblue|(colors.CHDRBG<<4), " \"\"\"");
    printtext(dpos.statusTopX+5,  dpos.statusTopY, CDGREEN|(colors.CHDRBG<<4), "\"\"\"");
    printtext(dpos.statusTopX+8,  dpos.statusTopY, CYELLOW|(colors.CHDRBG<<4), "\"\"\"");
    printtext(dpos.statusTopX+11, dpos.statusTopY, CLBROWN|(colors.CHDRBG<<4), "\"\"\"");
    printtext(dpos.statusTopX+14, dpos.statusTopY,   CDRED|(colors.CHDRBG<<4), "\"\"\" ");

    if (!std::strlen(loadedsongfilename))
      std::sprintf(textbuffer, "%s", programname);
    else
      std::sprintf(textbuffer, "%s - %s", programname, loadedsongfilename);
    textbuffer[49] = 0;
    printtext(dpos.statusTopX+19, dpos.statusTopY, colors.CHEADER, textbuffer);

    if ((numsids == 2) && monomode)
      printtext(dpos.statusTopFvX-2, dpos.statusTopY, colors.CHEADER, "M");

    if (usefinevib)
      printtext(dpos.statusTopFvX, dpos.statusTopY, colors.CHEADER, "FV");

    if (optimizepulse)
      printtext(dpos.statusTopFvX+3, dpos.statusTopY, colors.CHEADER, "PO");

    if (optimizerealtime)
      printtext(dpos.statusTopFvX+6, dpos.statusTopY, colors.CHEADER, "RO");

    if (ntsc)
      printtext(dpos.statusTopFvX+9, dpos.statusTopY, colors.CHEADER, "NTSC");
    else
      printtext(dpos.statusTopFvX+9, dpos.statusTopY, colors.CHEADER, " PAL");

    if (!sidmodel)
      printtext(dpos.statusTopFvX+14, dpos.statusTopY, colors.CHEADER, "6581");
    else
      printtext(dpos.statusTopFvX+14, dpos.statusTopY, colors.CHEADER, "8580");

    std::sprintf(textbuffer, "HR:%04X", adparam);
    printtext(dpos.statusTopFvX+19, dpos.statusTopY, colors.CHEADER, textbuffer);
    if (eamode && !ebmode) printbg(dpos.statusTopFvX+22+eacolumn, dpos.statusTopY, cc, 1);

    if (multiplier)
    {
      std::sprintf(textbuffer, "%2dX", multiplier);
      printtext(dpos.statusTopFvX+27, dpos.statusTopY, colors.CHEADER, textbuffer);
    }
    else printtext(dpos.statusTopFvX+27, dpos.statusTopY, colors.CHEADER, "25Hz");

    if (multiplier == 1)
    {
      std::sprintf(textbuffer, "%03dBPM", snd_bpmtempo);
      printtext(dpos.statusTopFvX+31, dpos.statusTopY, colors.CHEADER, textbuffer);

      if (eamode && ebmode) printbg(dpos.statusTopFvX+31+eacolumn, dpos.statusTopY, cc, 1);
    }

    printtext(dpos.statusTopEndX-8, dpos.statusTopY, colors.CHEADER, "F12=HELP");
  }
  else
  {
    printtext(0, dpos.statusTopY, colors.CHEADER, " PLAY | PLAYPOS | PLAYPATT | STOP | LOAD | SAVE | PACK/RL | HELP | CLEAR | QUIT |");
  }

  if (followplay && isplaying())
  {
    for (int c = 0; c < maxChns; c++)
    {
      int currentSonglen = songlen[esnum][c];
      int newpos = chn[c].pattptr / 4;
      if (chn[c].advance) epnum[c] = chn[c].pattnum;

      if (newpos > getPattlen(epnum[c])) newpos = getPattlen(epnum[c]);

      if (c == epchn)
      {
        eppos = newpos;
      }
      epview[c] = newpos-VISIBLEPATTROWS/2;

      newpos = chn[c].songptr;
      newpos--;
      if (newpos < 0) newpos = 0;
      if (newpos > currentSonglen) newpos = currentSonglen;

      if ((c == eschn) && (chn[c].advance))
      {
        eseditpos = newpos;
        if (newpos - esview < 0)
        {
          esview = newpos;
        }
        if (newpos - esview >= visibleOrderlist)
        {
          esview = newpos - visibleOrderlist + 1;
        }
      }
    }
  }

  // Pattern view
  for (int c = 0; c < maxChns; c++)
  {
    std::sprintf(textbuffer, "CH.%d PATT.%02X ", c+1, epnum[c]);
    printtext(dpos.patternsX+c*13, dpos.patternsY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);

    for (int d = 0; d < VISIBLEPATTROWS; d++)
    {
      int p = epview[c]+d;
      int color = colors.CNORMAL;
      int color2;
      if ((epnum[c] == chn[c].pattnum) && (isplaying()))
      {
        int chnrow = chn[c].pattptr / 4;
        if (chnrow > getPattlen(chn[c].pattnum)) chnrow = getPattlen(chn[c].pattnum);
        if (chnrow == p) color = colors.CPLAYING;
      }

      if (chn[c].mute) color = colors.CMUTE;
      if (p == eppos) color = colors.CEDIT;
      if ((p < 0) || (p > getPattlen(epnum[c])))
      {
        std::sprintf(textbuffer, "             ");
      }
      else
      {
        if (!(patterndispmode & 1))
        {
          if (p < 100)
            std::sprintf(textbuffer, " %02d", p);
          else
            std::sprintf(textbuffer, "%03d", p);
        }
        else
          std::sprintf(textbuffer, " %02X", p);

        if (pattern[epnum[c]][p*4] == ENDPATT)
        {
          std::sprintf(&textbuffer[3], " PATT. END");
          if (color == colors.CNORMAL) color = colors.CCOMMAND;
        }
        else
        {
          std::sprintf(&textbuffer[3], " %s %02X%01X%02X",
            notename[pattern[epnum[c]][p*4]-FIRSTNOTE],
            pattern[epnum[c]][p*4+1],
            pattern[epnum[c]][p*4+2],
            pattern[epnum[c]][p*4+3]);
          if (patterndispmode & 2)
          {
            if (!pattern[epnum[c]][p*4+1])
              std::memset(&textbuffer[8], '.', 2);
            if (!pattern[epnum[c]][p*4+2])
              std::memset(&textbuffer[10], '.', 3);
          }
        }
      }
      textbuffer[3] = 0;
      if (p%stepsize)
      {
        printtext(dpos.patternsX-1+c*13, dpos.patternsY+1+d, colors.CNORMAL, textbuffer);
      }
      else
      {
        printtext(dpos.patternsX-1+c*13, dpos.patternsY+1+d, colors.CCOMMAND, textbuffer);
      }
      if (color == colors.CNORMAL)
        color2 = colors.CCOMMAND;
      else
        color2 = color;
      printtext(dpos.patternsX+3+c*13, dpos.patternsY+1+d, color2, &textbuffer[4]);
      printtext(dpos.patternsX+7+c*13, dpos.patternsY+1+d, color, &textbuffer[8]);
      printtext(dpos.patternsX+9+c*13, dpos.patternsY+1+d, color2, &textbuffer[10]);
      printtext(dpos.patternsX+10+c*13, dpos.patternsY+1+d, color, &textbuffer[11]);
      if (c == epmarkchn)
      {
        if (epmarkstart <= epmarkend)
        {
          if ((p >= epmarkstart) && (p <= epmarkend))
            printbg(dpos.patternsX+c*13+3, dpos.patternsY+1+d, colors.CHDRBG, 9);
        }
        else
        {
          if ((p <= epmarkstart) && (p >= epmarkend))
            printbg(dpos.patternsX+c*13+3, dpos.patternsY+1+d, colors.CHDRBG, 9);
        }
      }
      if ((color == colors.CEDIT) && (editmode == EDIT_PATTERN) && (epchn == c))
      {
        switch(epcolumn)
        {
          case 0:
          if (!eamode) printbg(dpos.patternsX+c*13+3, dpos.patternsY+1+d, cc, 3);
          break;

          default:
          if (!eamode) printbg(dpos.patternsX+c*13+6+epcolumn, dpos.patternsY+1+d, cc, 1);
          break;
        }
      }
    }
  }

  // Orderlist view
  std::sprintf(textbuffer, "CHN ORDERLIST (SUBTUNE ");
  printtext(dpos.orderlistX, dpos.orderlistY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);
  std::sprintf(textbuffer, "%02X", esnum);
  printtext(dpos.orderlistX+23, dpos.orderlistY, colors.CEDIT|(colors.CHDRBG<<4), textbuffer);
  std::sprintf(textbuffer, ", POS ");
  printtext(dpos.orderlistX+25, dpos.orderlistY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);
  std::sprintf(textbuffer, "%02X", eseditpos);
  printtext(dpos.orderlistX+31, dpos.orderlistY, colors.CEDIT|(colors.CHDRBG<<4), textbuffer);
  std::sprintf(textbuffer, ")");
  printtext(dpos.orderlistX+33, dpos.orderlistY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);
  if (numsids == 2)
    std::sprintf(textbuffer, "            ");
  else
    std::sprintf(textbuffer, "                                      ");
  printtext(dpos.orderlistX+34, dpos.orderlistY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);

  for (int c = 0; c < maxChns; c++)
  {
    std::sprintf(textbuffer, " %d ", c+1);
    printtext(dpos.orderlistX, dpos.orderlistY+1+c, colors.CTITLE, textbuffer);
    for (int d = 0; d < visibleOrderlist; d++)
    {
      int p = esview+d;
      unsigned char currentSongorder = songorder[esnum][c][p];
      int currentSonglen = songlen[esnum][c];
      int color = colors.CNORMAL;
      if (isplaying())
      {
        int chnpos = chn[c].songptr;
        chnpos--;
        if (chnpos < 0) chnpos = 0;
        if ((p == chnpos) && (chn[c].advance)) color = colors.CPLAYING;
      }
      if (p == espos[c]) color = colors.CEDIT;
      if ((esend[c]) && (p == esend[c])) color = colors.CEDIT;

      if ((p < 0) || (p > (currentSonglen+1)) || (p > MAX_SONGLEN+1))
      {
        std::sprintf(textbuffer, "   ");
      }
      else
      {
        if (currentSongorder < LOOPSONG)
        {
          if ((currentSongorder < REPEAT) || (p >= currentSonglen))
          {
            std::sprintf(textbuffer, "%02X ", currentSongorder);
            if ((p >= currentSonglen) && (color == colors.CNORMAL)) color = colors.CCOMMAND;
          }
          else
          {
            if (currentSongorder >= TRANSUP)
            {
              std::sprintf(textbuffer, "+%01X ", currentSongorder&0xf);
              if (color == colors.CNORMAL) color = colors.CCOMMAND;
            }
            else
            {
              if (currentSongorder >= TRANSDOWN)
              {
                std::sprintf(textbuffer, "-%01X ", 16-(currentSongorder & 0x0f));
                if (color == colors.CNORMAL) color = colors.CCOMMAND;
              }
              else
              {
                std::sprintf(textbuffer, "R%01X ", (currentSongorder+1) & 0x0f);
                if (color == colors.CNORMAL) color = colors.CCOMMAND;
              }
            }
          }
        }
        if (currentSongorder == LOOPSONG)
        {
          std::sprintf(textbuffer, "RST");
          if (color == colors.CNORMAL) color = colors.CCOMMAND;
        }
      }
      if (chn[c].mute) color = colors.CMUTE;
      printtext(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, color, textbuffer);
      if (c == esmarkchn)
      {
        if (esmarkstart <= esmarkend)
        {
          if ((p >= esmarkstart) && (p <= esmarkend))
          {
            if (p != esmarkend)
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, colors.CHDRBG, 3);
            else
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, colors.CHDRBG, 2);
          }
        }
        else
        {
          if ((p <= esmarkstart) && (p >= esmarkend))
          {
            if (p != esmarkstart)
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, colors.CHDRBG, 3);
            else
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, colors.CHDRBG, 2);
          }
        }
      }
      if ((p == eseditpos) && (editmode == EDIT_ORDERLIST) && (eschn == c))
      {
        if (!eamode) printbg(dpos.orderlistX+4+d*3+escolumn, dpos.orderlistY+1+c, cc, 1);
      }
    }
  }

  // Instruments view
  std::sprintf(textbuffer, "INSTRUMENTS         AD SR WT PT FT VT VD GT FW");
  printtext(dpos.instrumentsX, dpos.instrumentsY, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);

  for (int i=0; i<5; i++)
  {
    int insnum = eirow + i;
    int color = insnum == einum ? colors.CEDIT : colors.CNORMAL;
    std::sprintf(textbuffer, "%2d %-16s %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                insnum,
                instr[insnum].name,
                instr[insnum].ad,
                instr[insnum].sr,
                instr[insnum].ptr[WTBL],
                instr[insnum].ptr[PTBL],
                instr[insnum].ptr[FTBL],
                instr[insnum].ptr[STBL],
                instr[insnum].vibdelay,
                instr[insnum].gatetimer,
                instr[insnum].firstwave
                );
    printtext(dpos.instrumentsX, dpos.instrumentsY+1+i, color, textbuffer);
  }

  int selpos = einum - eirow;
  if ((editmode == EDIT_INSTRUMENT) && (selpos >= 0) && (selpos < 5))
  {
    if (eipos < 9)
    {
      // Instr param
      if (!eamode) printbg(dpos.instrumentsX+20+3*eipos+eicolumn, dpos.instrumentsY+1+selpos, cc, 1);
    }
    else
    {
      // Instr name
      if (!eamode) printbg(dpos.instrumentsX+3+std::strlen(instr[einum].name), dpos.instrumentsY+1+selpos, cc, 1);
    }
  }

  // Tables view
  std::sprintf(textbuffer, "WAVE TBL    PULSE TBL   FILTER TBL  SPEED TBL ");
  printtext(dpos.instrumentsX, dpos.instrumentsY+7, colors.CTITLE|(colors.CHDRBG<<4), textbuffer);

  for (int c = 0; c < MAX_TABLES; c++)
  {
    for (int d = 0; d < VISIBLETABLEROWS; d++)
    {
      int p = etview[c]+d;

      int color = colors.CNORMAL;
      switch (c)
      {
        case WTBL:
        if (ltable[c][p] >= WAVECMD) color = colors.CCOMMAND;
        break;

        case PTBL:
        if (ltable[c][p] >= 0x80) color = colors.CCOMMAND;
        break;

        case FTBL:
        if ((ltable[c][p] >= 0x80) || ((!ltable[c][p]) && (rtable[c][p]))) color = colors.CCOMMAND;
        break;
      }
      if ((p == etpos) && (etnum == c)) color = colors.CEDIT;
      std::sprintf(textbuffer, "%02X:%02X %02X", p+1, ltable[c][p], rtable[c][p]);
      if (patterndispmode & 2)
      {
        if (!ltable[c][p] && !rtable[c][p] && !ltable[c][p+1] && !rtable[c][p+1])
        {
          std::memset(&textbuffer[3], '.', 2);
          std::memset(&textbuffer[6], '.', 2);
        }
      }
      printtext(dpos.instrumentsX+12*c, dpos.instrumentsY+8+d, color, textbuffer);

      if (etmarknum == c)
      {
        if (etmarkstart <= etmarkend)
        {
          if ((p >= etmarkstart) && (p <= etmarkend))
            printbg(dpos.instrumentsX+3+12*c, dpos.instrumentsY+8+d, colors.CHDRBG, 5);
        }
        else
        {
          if ((p <= etmarkstart) && (p >= etmarkend))
            printbg(dpos.instrumentsX+3+12*c, dpos.instrumentsY+8+d, colors.CHDRBG, 5);
        }
      }
    }
  }

  if (editmode == EDIT_TABLES)
  {
    if (!eamode) printbg(dpos.instrumentsX+3+etnum*12+(etcolumn & 1)+(etcolumn/2)*3, dpos.instrumentsY+8+etpos-etview[etnum], cc, 1);
  }

  // Info view
  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+1, colors.CTITLE, "NAME     ");
  std::sprintf(textbuffer, "%-32s", songname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+1, colors.CEDIT, textbuffer);

  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+2, colors.CTITLE, "AUTHOR   ");
  std::sprintf(textbuffer, "%-32s", authorname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+2, colors.CEDIT, textbuffer);

  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+3, colors.CTITLE, "RELEASED ");
  std::sprintf(textbuffer, "%-32s", copyrightname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+3, colors.CEDIT, textbuffer);

  if ((editmode == EDIT_NAMES) && (!eamode))
  {
    switch(enpos)
    {
      case 0:
      printbg(dpos.instrumentsX+9+std::strlen(songname), dpos.instrumentsY+8+VISIBLETABLEROWS+1, cc, 1);
      break;
      case 1:
      printbg(dpos.instrumentsX+9+std::strlen(authorname), dpos.instrumentsY+8+VISIBLETABLEROWS+2, cc, 1);
      break;
      case 2:
      printbg(dpos.instrumentsX+9+std::strlen(copyrightname), dpos.instrumentsY+8+VISIBLETABLEROWS+3, cc, 1);
      break;
    }
  }

  // Footer
  std::sprintf(textbuffer, "OCTAVE %d", epoctave);
  printtext(dpos.octaveX, dpos.octaveY, colors.CTITLE, textbuffer);

  int color;
  switch(autoadvance)
  {
    case 0:
    color = CLRED;
    break;

    case 1:
    color = CLBLUE;
    break;

    case 2:
    color = CGREY;
    break;
  }

  if (recordmode) printtext(dpos.octaveX, dpos.octaveY+1, color, "EDITMODE");
  else printtext(dpos.octaveX, dpos.octaveY+1, color, "JAM MODE");

  if (isplaying()) printtext(dpos.octaveX+10, dpos.octaveY, colors.CTITLE, "PLAYING");
  else printtext(dpos.octaveX+10, dpos.octaveY, colors.CTITLE, "STOPPED");
  if (multiplier)
  {
    if (!ntsc)
      std::sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(25*multiplier) & 1], timesec);
    else
      std::sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(30*multiplier) & 1], timesec);
  }
  else
  {
    if (!ntsc)
      std::sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/13) & 1], timesec);
    else
      std::sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/15) & 1], timesec);
  }

  printtext(dpos.octaveX+10, dpos.octaveY+1, colors.CEDIT, textbuffer);

  if (numsids == 1)
  {
    printtext(
        dpos.channelsX,
        dpos.channelsY,
        colors.CTITLE,
        " CHN1   CHN2   CHN3 "
    );
  }
  else if (numsids == 2)
  {
    printtext(
        dpos.channelsX,
        dpos.channelsY,
        colors.CTITLE,
        " CHN1   CHN2   CHN3   CHN4   CHN5   CHN6 "
    );
  }
  for (int c = 0; c < maxChns; c++)
  {
    int chnpos = chn[c].songptr;
    int chnrow = chn[c].pattptr/4;
    chnpos--;
    if (chnpos < 0) chnpos = 0;
    if (chnrow > getPattlen(chn[c].pattnum)) chnrow = getPattlen(chn[c].pattnum);
    if (chnrow >= 100) chnrow -= 100;

    std::sprintf(textbuffer, "%03X/%02X",
      chnpos,chnrow);
    printtext(dpos.channelsX+7*c, dpos.channelsY+1, chn[c].mute ? colors.CMUTE : colors.CEDIT, textbuffer);
  }

  if (etlock) printtext(dpos.channelsX-2, dpos.channelsY+1, colors.CTITLE, " ");
    else printtext(dpos.channelsX-2, dpos.channelsY+1, colors.CTITLE, "U");
}


void resettime()
{
  timemin = 0;
  timesec = 0;
  timeframe = 0;
}

void incrementtime()
{
  {
    timeframe++;
    if (!ntsc)
    {
      if (((multiplier) && (timeframe >= PALFRAMERATE*multiplier))
          || ((!multiplier) && (timeframe >= PALFRAMERATE/2)))
      {
        timeframe = 0;
        timesec++;
      }
    }
    else
    {
      if (((multiplier) && (timeframe >= NTSCFRAMERATE*multiplier))
          || ((!multiplier) && (timeframe >= NTSCFRAMERATE/2)))
      {
        timeframe = 0;
        timesec++;
      }
    }
    if (timesec == 60)
    {
      timesec = 0;
      timemin++;
      timemin %= 60;
    }
  }
}

