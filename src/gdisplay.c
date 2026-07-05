//
// GOATTRACKER v2 screen display routines
//

#define GDISPLAY_C

#include "goattrk2.h"

char *notename[] =
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

void initcolorscheme(int dark)
{
  colors.CBKGND   = dark ? CBLACK : CDBLUE;

  colors.CNORMAL  = (dark ? CGREY : CLBLUE) |(colors.CBKGND<<4);
  colors.CMUTE    = CDGREY |(colors.CBKGND<<4);
  colors.CEDIT    = CLGREEN|(colors.CBKGND<<4);
  colors.CPLAYING = CLRED  |(colors.CBKGND<<4);
  colors.CCOMMAND = CLGREY |(colors.CBKGND<<4);
  colors.CTITLE   = CWHITE |(colors.CBKGND<<4);

  colors.CHDRBG   = dark ? CDBLUE : CLBLUE;
  colors.CHDRFG   = dark ? CWHITE : CDBLUE;

  colors.CHEADER  = colors.CHDRFG|(colors.CHDRBG<<4);
}

void printmainscreen(void)
{
  clearscreen();
  printstatus();
  fliptoscreen();
}

void displayupdate(void)
{
  if (cursorflashdelay >= 6)
  {
    cursorflashdelay %= 6;
    cursorflash++;
    cursorflash &= 3;
  }
  printstatus();
  fliptoscreen();
}

void printstatus(void)
{
  int c, d, color, color2;
  int cc = cursorcolortable[cursorflash];
  int visibleOrderlist = 14;
  int maxChns = MAX_CHN;
  if (numsids == 1)
  {
    maxChns = 3;
    visibleOrderlist = VISIBLEORDERLIST;
  }
  menu = 0;

  if ((mouseb > MOUSEB_LEFT) && (mousey <= 1) && (!eamode)) menu = 1;

  printblankc(0, 0, colors.CHEADER, MAX_COLUMNS);

  if (!menu)
  {
    if (!strlen(loadedsongfilename))
      sprintf(textbuffer, "%s", programname);
    else
      sprintf(textbuffer, "%s - %s", programname, loadedsongfilename);
    textbuffer[49] = 0;
    printtext(dpos.statusTopX+1, dpos.statusTopY, colors.CHEADER, textbuffer);

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

    sprintf(textbuffer, "HR:%04X", adparam);
    printtext(dpos.statusTopFvX+19, dpos.statusTopY, colors.CHEADER, textbuffer);
    if (eamode) printbg(dpos.statusTopFvX+22+eacolumn, dpos.statusTopY, cc, 1);

    if (multiplier)
    {
      sprintf(textbuffer, "%2dX", multiplier);
      printtext(dpos.statusTopFvX+27, dpos.statusTopY, colors.CHEADER, textbuffer);
    }
    else printtext(dpos.statusTopFvX+27, dpos.statusTopY, colors.CHEADER, "25Hz");

    printtext(dpos.statusTopEndX-8, dpos.statusTopY, colors.CHEADER, "F12=HELP");
  }
  else
  {
    printtext(0, dpos.statusTopY, colors.CHEADER, " PLAY | PLAYPOS | PLAYPATT | STOP | LOAD | SAVE | PACK/RL | HELP | CLEAR | QUIT |");
  }

  if ((followplay) && (isplaying()))
  {
    for (c = 0; c < maxChns; c++)
    {
      int currentSonglen = 0;
      if (numsids == 1)
      {
        currentSonglen = songlen[esnum][c];
      }
      else if (numsids == 2)
      {
        currentSonglen = songlen_stereo[esnum][c];
      }
      int newpos = chn[c].pattptr / 4;
      if (chn[c].advance) epnum[c] = chn[c].pattnum;

      if (newpos > pattlen[epnum[c]]) newpos = pattlen[epnum[c]];

      if (c == epchn)
      {
        eppos = newpos;
        epview = newpos-VISIBLEPATTROWS/2;
      }

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

  for (c = 0; c < maxChns; c++)
  {
    sprintf(textbuffer, "CH.");
    printtext(dpos.patternsX+c*13, dpos.patternsY, colors.CTITLE, textbuffer);
    sprintf(textbuffer, "%d", c+1);
    printtext(dpos.patternsX+3+c*13, dpos.patternsY, colors.CTITLE, textbuffer);
    sprintf(textbuffer, "PATT.");
    printtext(dpos.patternsX+5+c*13, dpos.patternsY, colors.CTITLE, textbuffer);
    sprintf(textbuffer, "%02X", epnum[c]);
    printtext(dpos.patternsX+10+c*13, dpos.patternsY, colors.CTITLE, textbuffer);

    for (d = 0; d < VISIBLEPATTROWS; d++)
    {
      int p = epview+d;
      color = colors.CNORMAL;
      if ((epnum[c] == chn[c].pattnum) && (isplaying()))
      {
        int chnrow = chn[c].pattptr / 4;
        if (chnrow > pattlen[chn[c].pattnum]) chnrow = pattlen[chn[c].pattnum];
        if (chnrow == p) color = colors.CPLAYING;
      }

      if (chn[c].mute) color = colors.CMUTE;
      if (p == eppos) color = colors.CEDIT;
      if ((p < 0) || (p > pattlen[epnum[c]]))
      {
        sprintf(textbuffer, "             ");
      }
      else
      {
        if (!(patterndispmode & 1))
        {
          if (p < 100)
            sprintf(textbuffer, " %02d", p);
          else
            sprintf(textbuffer, "%03d", p);
        }
        else
          sprintf(textbuffer, " %02X", p);

        if (pattern[epnum[c]][p*4] == ENDPATT)
        {
          sprintf(&textbuffer[3], " PATT. END");
          if (color == colors.CNORMAL) color = colors.CCOMMAND;
        }
        else
        {
          sprintf(&textbuffer[3], " %s %02X%01X%02X",
            notename[pattern[epnum[c]][p*4]-FIRSTNOTE],
            pattern[epnum[c]][p*4+1],
            pattern[epnum[c]][p*4+2],
            pattern[epnum[c]][p*4+3]);
          if (patterndispmode & 2)
          {
            if (!pattern[epnum[c]][p*4+1])
              memset(&textbuffer[8], '.', 2);
            if (!pattern[epnum[c]][p*4+2])
              memset(&textbuffer[10], '.', 3);
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
            printbg(dpos.patternsX+c*13+3, dpos.patternsY+1+d, 8, 9);
        }
        else
        {
          if ((p <= epmarkstart) && (p >= epmarkend))
            printbg(dpos.patternsX+c*13+3, dpos.patternsY+1+d, 8, 9);
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

  sprintf(textbuffer, "CHN ORDERLIST (SUBTUNE ", esnum, eseditpos);
  printtext(dpos.orderlistX, dpos.orderlistY, colors.CTITLE, textbuffer);
  sprintf(textbuffer, "%02X", esnum);
  printtext(dpos.orderlistX+23, dpos.orderlistY, colors.CEDIT, textbuffer);
  sprintf(textbuffer, ", POS ");
  printtext(dpos.orderlistX+25, dpos.orderlistY, colors.CTITLE, textbuffer);
  sprintf(textbuffer, "%02X", eseditpos);
  printtext(dpos.orderlistX+31, dpos.orderlistY, colors.CEDIT, textbuffer);
  sprintf(textbuffer, ")");
  printtext(dpos.orderlistX+33, dpos.orderlistY, colors.CTITLE, textbuffer);

  for (c = 0; c < maxChns; c++)
  {
    sprintf(textbuffer, " %d ", c+1);
    printtext(dpos.orderlistX, dpos.orderlistY+1+c, colors.CTITLE, textbuffer);
    for (d = 0; d < visibleOrderlist; d++)
    {
      int p = esview+d;
      unsigned char currentSongorder = 0;
      int currentSonglen = 0;
      if (numsids == 1)
      {
        currentSongorder = songorder[esnum][c][p];
        currentSonglen = songlen[esnum][c];
      }
      else if (numsids == 2)
      {
        currentSongorder = songorder_stereo[esnum][c][p];
        currentSonglen = songlen_stereo[esnum][c];
      }
      color = colors.CNORMAL;
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
        sprintf(textbuffer, "   ");
      }
      else
      {
        if (currentSongorder < LOOPSONG)
        {
          if ((currentSongorder < REPEAT) || (p >= currentSonglen))
          {
            sprintf(textbuffer, "%02X ", currentSongorder);
            if ((p >= currentSonglen) && (color == colors.CNORMAL)) color = colors.CCOMMAND;
          }
          else
          {
            if (currentSongorder >= TRANSUP)
            {
              sprintf(textbuffer, "+%01X ", currentSongorder&0xf);
              if (color == colors.CNORMAL) color = colors.CCOMMAND;
            }
            else
            {
              if (currentSongorder >= TRANSDOWN)
              {
                sprintf(textbuffer, "-%01X ", 16-(currentSongorder & 0x0f));
                if (color == colors.CNORMAL) color = colors.CCOMMAND;
              }
              else
              {
                sprintf(textbuffer, "R%01X ", (currentSongorder+1) & 0x0f);
                if (color == colors.CNORMAL) color = colors.CCOMMAND;
              }
            }
          }
        }
        if (currentSongorder == LOOPSONG)
        {
          sprintf(textbuffer, "RST");
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
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, 1, 3);
            else
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, 1, 2);
          }
        }
        else
        {
          if ((p <= esmarkstart) && (p >= esmarkend))
          {
            if (p != esmarkstart)
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, 1, 3);
            else
              printbg(dpos.orderlistX+4+d*3, dpos.orderlistY+1+c, 1, 2);
          }
        }
      }
      if ((p == eseditpos) && (editmode == EDIT_ORDERLIST) && (eschn == c))
      {
        if (!eamode) printbg(dpos.orderlistX+4+d*3+escolumn, dpos.orderlistY+1+c, cc, 1);
      }
    }
  }

  sprintf(textbuffer, "INSTRUMENT NUM. %02X  %-16s", einum, instr[einum].name);
  printtext(dpos.instrumentsX, dpos.instrumentsY, colors.CTITLE, textbuffer);

  sprintf(textbuffer, "Attack/Decay    %02X", instr[einum].ad);
  if (eipos == 0) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX, dpos.instrumentsY+1, color, textbuffer);

  sprintf(textbuffer, "Sustain/Release %02X", instr[einum].sr);
  if (eipos == 1) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX, dpos.instrumentsY+2, color, textbuffer);

  sprintf(textbuffer, "Wavetable Pos   %02X", instr[einum].ptr[WTBL]);
  if (eipos == 2) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX, dpos.instrumentsY+3, color, textbuffer);

  sprintf(textbuffer, "Pulsetable Pos  %02X", instr[einum].ptr[PTBL]);
  if (eipos == 3) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX, dpos.instrumentsY+4, color, textbuffer);

  sprintf(textbuffer, "Filtertable Pos %02X", instr[einum].ptr[FTBL]);
  if (eipos == 4) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX, dpos.instrumentsY+5, color, textbuffer);

  sprintf(textbuffer, "Vibrato Param   %02X", instr[einum].ptr[STBL]);
  if (eipos == 5) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX+20, dpos.instrumentsY+1, color, textbuffer);

  sprintf(textbuffer, "Vibrato Delay   %02X", instr[einum].vibdelay);
  if (eipos == 6) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX+20, dpos.instrumentsY+2, color, textbuffer);

  sprintf(textbuffer, "HR/Gate Timer   %02X", instr[einum].gatetimer);
  if (eipos == 7) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX+20, dpos.instrumentsY+3, color, textbuffer);

  sprintf(textbuffer, "1stFrame Wave   %02X", instr[einum].firstwave);
  if (eipos == 8) color = colors.CEDIT; else color = colors.CNORMAL;
  printtext(dpos.instrumentsX+20, dpos.instrumentsY+4, color, textbuffer);

  if (editmode == EDIT_INSTRUMENT)
  {
    if (eipos < 9)
    {
      if (!eamode) printbg(dpos.instrumentsX+16+eicolumn+20*(eipos/5), dpos.instrumentsY+1+(eipos%5), cc, 1);
    }
    else
    {
      if (!eamode) printbg(dpos.instrumentsX+20+strlen(instr[einum].name), dpos.instrumentsY, cc, 1);
    }
  }

  sprintf(textbuffer, "WAVE TBL  PULSETBL  FILT.TBL  SPEEDTBL");
  printtext(dpos.instrumentsX, dpos.instrumentsY+7, colors.CTITLE, textbuffer);

  for (c = 0; c < MAX_TABLES; c++)
  {
    for (d = 0; d < VISIBLETABLEROWS; d++)
    {
      int p = etview[c]+d;

      color = colors.CNORMAL;
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
      sprintf(textbuffer, "%02X:%02X %02X", p+1, ltable[c][p], rtable[c][p]);
      if (patterndispmode & 2)
      {
        if (!ltable[c][p] && !rtable[c][p] && !ltable[c][p+1] && !rtable[c][p+1])
        {
          memset(&textbuffer[3], '.', 2);
          memset(&textbuffer[6], '.', 2);
        }
      }
      printtext(dpos.instrumentsX+10*c, dpos.instrumentsY+8+d, color, textbuffer);

      if (etmarknum == c)
      {
        if (etmarkstart <= etmarkend)
        {
          if ((p >= etmarkstart) && (p <= etmarkend))
            printbg(dpos.instrumentsX+10*c+3, dpos.instrumentsY+8+d, 1, 5);
        }
        else
        {
          if ((p <= etmarkstart) && (p >= etmarkend))
            printbg(dpos.instrumentsX+10*c+3, dpos.instrumentsY+8+d, 1, 5);
        }
      }
    }
  }

  if (editmode == EDIT_TABLES)
  {
    if (!eamode) printbg(dpos.instrumentsX+3+etnum*10+(etcolumn & 1)+(etcolumn/2)*3, dpos.instrumentsY+8+etpos-etview[etnum], cc, 1);
  }

  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+1, colors.CTITLE, "NAME     ");
  sprintf(textbuffer, "%-32s", songname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+1, colors.CEDIT, textbuffer);

  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+2, colors.CTITLE, "AUTHOR   ");
  sprintf(textbuffer, "%-32s", authorname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+2, colors.CEDIT, textbuffer);

  printtext(dpos.instrumentsX, dpos.instrumentsY+8+VISIBLETABLEROWS+3, colors.CTITLE, "RELEASED ");
  sprintf(textbuffer, "%-32s", copyrightname);
  printtext(dpos.instrumentsX+9, dpos.instrumentsY+8+VISIBLETABLEROWS+3, colors.CEDIT, textbuffer);

  if ((editmode == EDIT_NAMES) && (!eamode))
  {
    switch(enpos)
    {
      case 0:
      printbg(dpos.instrumentsX+9+strlen(songname), dpos.instrumentsY+8+VISIBLETABLEROWS+1, cc, 1);
      break;
      case 1:
      printbg(dpos.instrumentsX+9+strlen(authorname), dpos.instrumentsY+8+VISIBLETABLEROWS+2, cc, 1);
      break;
      case 2:
      printbg(dpos.instrumentsX+9+strlen(copyrightname), dpos.instrumentsY+8+VISIBLETABLEROWS+3, cc, 1);
      break;
    }
  }
  sprintf(textbuffer, "OCTAVE %d", epoctave);
  printtext(dpos.octaveX, dpos.octaveY, colors.CTITLE, textbuffer);

  switch(autoadvance)
  {
    case 0:
    color = 10;
    break;

    case 1:
    color = 14;
    break;

    case 2:
    color = 12;
    break;
  }

  if (recordmode) printtext(dpos.octaveX, dpos.octaveY+1, color, "EDITMODE");
  else printtext(dpos.octaveX, dpos.octaveY+1, color, "JAM MODE");

  if (isplaying()) printtext(dpos.octaveX+10, dpos.octaveY, colors.CTITLE, "PLAYING");
  else printtext(dpos.octaveX+10, dpos.octaveY, colors.CTITLE, "STOPPED");
  if (multiplier)
  {
    if (!ntsc)
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(25*multiplier) & 1], timesec);
    else
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[timeframe/(30*multiplier) & 1], timesec);
  }
  else
  {
    if (!ntsc)
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/13) & 1], timesec);
    else
      sprintf(textbuffer, " %02d%c%02d ", timemin, timechar[(timeframe/15) & 1], timesec);
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
  for (c = 0; c < maxChns; c++)
  {
    int chnpos = chn[c].songptr;
    int chnrow = chn[c].pattptr/4;
    chnpos--;
    if (chnpos < 0) chnpos = 0;
    if (chnrow > pattlen[chn[c].pattnum]) chnrow = pattlen[chn[c].pattnum];
    if (chnrow >= 100) chnrow -= 100;

    sprintf(textbuffer, "%03X/%02X",
      chnpos,chnrow);
    printtext(dpos.channelsX+7*c, dpos.channelsY+1, chn[c].mute ? colors.CMUTE : colors.CEDIT, textbuffer);
  }

  if (etlock) printtext(dpos.channelsX-2, dpos.channelsY+1, colors.CTITLE, " ");
    else printtext(dpos.channelsX-2, dpos.channelsY+1, colors.CTITLE, "U");
}


void resettime(void)
{
  timemin = 0;
  timesec = 0;
  timeframe = 0;
}

void incrementtime(void)
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

