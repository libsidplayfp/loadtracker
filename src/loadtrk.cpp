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

#define LOADTRK_C

#include "loadtrk.h"

#include "config.h"

#include "channels.h"
#include "colors.h"
#include "configfile.h"
#include "console.h"
#include "display.h"
#include "instr.h"
#include "order.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "song.h"
#include "sound.h"
#include "settings.h"
#include "table.h"
#include "timer.h"
#include "tuning.h"

#include "bme_main.h"
#include "bme_win.h"
#include "bme_snd.h"
#include "bme_io.h"

#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

constexpr int HOLDDELAY = 24;

bool monomode = true;
bool writer = false;

char loadedsongfilename[MAX_FILENAME];
char songfilename[MAX_FILENAME];
char instrfilename[MAX_FILENAME];

extern char *notename[];
const char *programname = "LoadTracker " PACKAGE_VERSION;

unsigned char hexkeytbl[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

extern unsigned char datafile[]; // from ltrkdata.cpp

const char* usage[] = {
    "Usage: loadtrk [songname] [options]",
    "Options:",
    "-Axx Set ADSR parameter for hardrestart in hex. DEFAULT=0F00",
    "-Dxx Pattern row display (0 = decimal, 1 = hex, 2 = decimal w/dots, 3 = hex w/dots) DEFAULT=2",
    "-Exx Set emulated SID model (0 = 6581 1 = 8580) DEFAULT=8580",
    "-Fxx Set custom SID clock cycles per second (0 = use PAL/NTSC default)",
    "-Gxx Set pitch of A-4 in Hz (0 = use default frequency table, close to 440Hz)",
    "-Ixx Set reSIDfp resampling mode (0 = interpolation, 1 = resampling) DEFAULT=1",
    "-Jxx Set special note names (2 chars for every note in an octave/cycle, e.g. C-DbD-EbE-F-GbG-AbA-BbB-)",
    "-Kxx Note-entry mode (0 = Protracker, 1 = DMC, 2 = Janko) DEFAULT=Protracker",
    "-Lxx SID memory location in hex. DEFAULT=D400",
    "-Mxx Set sound mixing rate DEFAULT=48000",
    "-Oxx Set pulseoptimization/skipping (0 = off, 1 = on) DEFAULT=on",
    "-Qxx Set equal divisions per octave (12 = default, 8.2019143 = Bohlen-Pierce)",
    "-Rxx Set realtime-effect optimization/skipping (0 = off, 1 = on) DEFAULT=on",
    "-Sxx Set speed multiplier (0 for 25Hz, 1 for 1x, 2 for 2x etc.)",
    "-Vxx Set finevibrato conversion (0 = off, 1 = on) DEFAULT=on",
    "-Xxx Set window type (0 = window, 1 = fullscreen) DEFAULT=window",
    "-Yxx Path to a Scala tuning file .scl",
    "-Zxx Set random SID write delay in cycles (0 = off) DEFAULT=off",
    "-bxx Set filter curve (0.0 (dark) to 1.0 (bright))",
    "-cxx Set combined waveforms strength (0 weak, 1 average, 2 strong) DEFAULT=average"
    "-xxx Use exdSID (0 = off, 1 = on)",
    "-N   Use NTSC timing",
    "-P   Use PAL timing (DEFAULT)",
    "-W   Write sound output to a file SIDAUDIO.RAW",
    "-?   Show this info again",
    "-??  Standalone online help window",
    "--dark   Use original dark colorscheme",
};

int usagelen = (sizeof usage / sizeof usage[0]);

void converthex();
void docommand();
void mousecommands();
void generalcommands();
void load();
void save();
void quit();
void clear();
void prevmultiplier();
void nextmultiplier();
void editadsr(int col);
void editbpm(int col);
void setspecialnotenames();
void switchMode();
void optimizeeverything();
void findduplicatepatterns();
void tooltips();

int main(int argc, char **argv)
{
  // Open datafile
  if (!io_openlinkeddatafile(datafile))
    return EXIT_FAILURE;

  loadconfig();

  // Init pathnames
  initpaths();

  bool dark = config.darkmode != 0;

  // Scan command line
  for (int c = 1; c < argc; c++)
  {
#ifdef _WIN32
    if ((argv[c][0] == '-') || (argv[c][0] == '/'))
#else
    if (argv[c][0] == '-')
#endif
    {
      switch (argv[c][1]) //switch (toupper(argv[c][1]))
      {
        case '-':
        if (std::strcmp(argv[c], "--dark") == 0)
        {
            dark = true;
            break;
        }
        if (std::strcmp(argv[c], "--help"))
            break;
        /* fall through */
        case '?':
        if(argv[c][2]=='?')
        {
          if (!initscreen())
            return EXIT_FAILURE;
          onlinehelp(1, 0);
          return EXIT_SUCCESS;
        }
#ifdef _WIN32
        if (!initscreen())
          return EXIT_FAILURE;
        for (int y = 0; y < usagelen; ++y)
          printtext(0, y, colors.CMESSAGE, usage[y]);
        waitkeynoupdate();
#else
        for (int y = 0; y < usagelen; ++y)
          std::printf("%s\n", usage[y]);
#endif
        return EXIT_SUCCESS;

        case 'Z':
        std::sscanf(&argv[c][2], "%u", &config.residdelay);
        break;

        case 'A':
        std::sscanf(&argv[c][2], "%x", &config.adparam);
        break;

        case 'S':
        std::sscanf(&argv[c][2], "%u", &config.multiplier);
        break;

        case 'D':
        std::sscanf(&argv[c][2], "%u", &config.patterndispmode);
        break;

        case 'E':
        std::sscanf(&argv[c][2], "%u", &config.sidmodel);
        break;

        case 'I':
        std::sscanf(&argv[c][2], "%u", &config.interpolate);
        break;

        case 'K':
        std::sscanf(&argv[c][2], "%u", &config.keypreset);
        break;

        case 'L':
        std::sscanf(&argv[c][2], "%x", &config.sidaddress);
        break;

        case 'N':
        config.ntsc = 1;
        config.customclockrate = 0;
        break;

        case 'P':
        config.ntsc = 0;
        config.customclockrate = 0;
        break;

        case 'F':
        std::sscanf(&argv[c][2], "%u", &config.customclockrate);
        break;

        case 'M':
        std::sscanf(&argv[c][2], "%u", &config.mixrate);
        break;

        case 'O':
        std::sscanf(&argv[c][2], "%u", &config.optimizepulse);
        break;

        case 'R':
        std::sscanf(&argv[c][2], "%u", &config.optimizerealtime);
        break;

        case 'V':
        std::sscanf(&argv[c][2], "%u", &config.finevibrato);
        break;

        case 'W':
        writer = true;
        break;

        case 'X':
        std::sscanf(&argv[c][2], "%d", &win_fullscreen);
        break;

        case 'G':
        std::sscanf(&argv[c][2], "%f", &config.basepitch);
        break;

        case 'b':
        std::sscanf(&argv[c][2], "%f", &config.filterbias);
        break;

        case 'c':
        std::sscanf(&argv[c][2], "%u", &config.combwaves);
        break;

        case 'Q':
        std::sscanf(&argv[c][2], "%f", &config.equaldivisionsperoctave);
        break;

        case 'J':
        std::sscanf(&argv[c][2], "%s", specialnotenames);
        break;

        case 'Y':
        std::sscanf(&argv[c][2], "%s", scalatuningfilepath);
        break;

        case 'x':
        std::sscanf(&argv[c][2], "%u", &config.exsid);
        break;
      }
    }
    else
    {
      std::strcpy(songfilename, argv[c]);
      for (int d = std::strlen(argv[c])-1; d >= 0; d--)
      {
        if ((argv[c][d] == '/') || (argv[c][d] == '\\'))
        {
          char startpath[MAX_PATHNAME];
          std::strcpy(startpath, argv[c]);
          startpath[d+1] = 0;
          chdir(startpath);
          initpaths();
          std::strcpy(songfilename, &argv[c][d+1]);
          break;
        }
      }
    }
  }

  // Init colorscheme
  colors.init(dark);

  // Validate parameters
  config.validate();

  initDisplayPositions();

  // Read Scala tuning file
  if (scalatuningfilepath[0] != '0' && scalatuningfilepath[1] != '\0')
  {
    readscalatuningfile(scalatuningfilepath, specialnotenames);
  }

  // Calculate frequencytable if necessary
  if (config.basepitch < 0.0f)
    config.basepitch = 0.0f;
  if (config.basepitch > 0.0f)
    calculatefreqtable(config);

  // Set special note names
  if (specialnotenames[1] != '\0')
  {
    setspecialnotenames();
  }

  // Set screenmode
  if (!initscreen())
    return EXIT_FAILURE;

  // Reset channels/song
  initchannels();
  clearsong(true, true, true, true, true);

  timer.setfreq(config.ntsc);
  timer.setmult(config.multiplier);

  // Init sound
  if (!sound_init(writer, config))
  {
    printtextc(MAX_ROWS/2-1, colors.CMESSAGE, "Sound init failed. Press any key to run without sound (notice that song timer won't start)");
    waitkeynoupdate();
  }

  // Load song if applicable
  if (std::strlen(songfilename)) loadsong();

  // Start editor mainloop
  printmainscreen();
  while (!exitprogram)
  {
    waitkeymouse();
    docommand();
  }

  // Shutdown sound output now
  sound_uninit();

  io_closelinkeddatafile();

  closescreen();

  saveconfig();

  // Exit
  return EXIT_SUCCESS;
}

void waitkey()
{
  for (;;)
  {
    displayupdate();
    getkey();
    if ((input.rawkey) || (input.key)) break;
    if (win_quitted) break;
  }

  converthex();
}

void waitkeymouse()
{
  for (;;)
  {
    displayupdate();
    tooltips();
    getkey();
    if ((input.rawkey) || (input.key)) break;
    if (win_quitted) break;
    if (input.mouseb || (input.wheel)) break;
  }

  converthex();
}

void waitkeymousenoupdate()
{
  for (;;)
  {
    fliptoscreen();
    getkey();
    if ((input.rawkey) || (input.key)) break;
    if (win_quitted) break;
    if (input.mouseb) break;
  }

  converthex();
}

void waitkeynoupdate()
{
  for (;;)
  {
    fliptoscreen();
    getkey();
    if ((input.rawkey) || (input.key)) break;
    if ((input.mouseb) && (!input.prevmouseb)) break;
    if (win_quitted) break;
  }
}

void converthex()
{
  hexnybble = -1;
  for (int c = 0; c < 16; c++)
  {
    if (std::tolower(input.key) == hexkeytbl[c])
    {
      if (c >= 10)
      {
        if (!input.shiftpressed) hexnybble = c;
      }
      else
      {
        hexnybble = c;
      }
    }
  }
}

void docommand()
{
  // "GUI" operation :)
  mousecommands();

  // Mode-specific commands
  switch(editmode)
  {
    case EDIT_ORDERLIST:
    orderlistcommands();
    break;

    case EDIT_INSTRUMENT:
    instrumentcommands();
    break;

    case EDIT_TABLES:
    tablecommands();
    break;

    case EDIT_PATTERN:
    patterncommands();
    break;

    case EDIT_NAMES:
    namecommands();
    break;
  }

  // General commands
  generalcommands();
}

void tooltips()
{
  settooltip("");

  // Titlebar
  if (!menu)
  {
    if (input.mousey == dpos.statusTopY)
    {
      if ((input.mousex >= dpos.statusTopFvX-3) && (input.mousex <= dpos.statusTopFvX-2) && (config.numsids == 2))
      {
        settooltip("Stereo mode");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX) && (input.mousex <= dpos.statusTopFvX+1))
      {
        settooltip("Fine vibrato");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+3) && (input.mousex <= dpos.statusTopFvX+4))
      {
        settooltip("Optimize pulse");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+6) && (input.mousex <= dpos.statusTopFvX+7))
      {
        settooltip("Optimize realtime");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+9) && (input.mousex <= dpos.statusTopFvX+12))
      {
        settooltip("Video frequency");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+14) && (input.mousex <= dpos.statusTopFvX+17))
      {
        settooltip("SID model");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+22) &&
          (input.mousex <= dpos.statusTopFvX+25))
      {
        settooltip("Hard restart ADSR");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+27) &&
          (input.mousex <= dpos.statusTopFvX+30))
      {
        settooltip("Speed multiplier");
        return;
      }
      if ((input.mousex >= dpos.statusTopFvX+31) &&
          (input.mousex <= dpos.statusTopFvX+33))
      {
        settooltip("BPM");
        return;
      }
      if ((input.mousex >= dpos.statusTopEndX-8) &&
          (input.mousex <= dpos.statusTopEndX-1))
      {
        settooltip("Online help");
        return;
      }
    }
  }

  // Instruments
  if (input.mousey == dpos.instrumentsY)
  {
    if ((input.mousex >= (dpos.instrumentsX+20)) &&
        (input.mousex <= (dpos.instrumentsX+21)))
      {
        settooltip("Attack/Decay");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+23)) &&
        (input.mousex <= (dpos.instrumentsX+24)))
      {
        settooltip("Sustain/Release");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+26)) &&
        (input.mousex <= (dpos.instrumentsX+27)))
      {
        settooltip("Wave table position");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+29)) &&
        (input.mousex <= (dpos.instrumentsX+30)))
      {
        settooltip("Pulse table position");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+32)) &&
        (input.mousex <= (dpos.instrumentsX+33)))
      {
        settooltip("Filter table position");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+35)) &&
        (input.mousex <= (dpos.instrumentsX+36)))
      {
        settooltip("Speed table position (vibrato)");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+38)) &&
        (input.mousex <= (dpos.instrumentsX+39)))
      {
        settooltip("Vibrato delay");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+41)) &&
        (input.mousex <= (dpos.instrumentsX+42)))
      {
        settooltip("Gate timer");
        return;
      }
    if ((input.mousex >= (dpos.instrumentsX+44)) &&
        (input.mousex <= (dpos.instrumentsX+45)))
      {
        settooltip("First wave");
        return;
      }
  }
  if ((input.mousey == dpos.channelsY+1) &&
      (input.mousex >= dpos.channelsX-5) &&
      (input.mousex <= dpos.channelsX-2))
  {
    settooltip("Lock/unlock tables");
    return;
  }
}

void mousecommands()
{
  int maxChns = config.getMaxChannels();

  int currentSonglen = song.len[esnum][eschn];

  if (input.wheel)
  {
    // Scroll patterns
    if ((input.mousey >= dpos.patternsY) &&
        (input.mousey <= dpos.statusBottomY - 1) &&
        (input.mousex >= dpos.patternsX) &&
        (input.mousex <= dpos.patternsX + 11 + (maxChns-1)*13))
    {
        if (input.wheel > 0)
          patternup();
        else if (input.wheel < 0)
          patterndown();
    }
    // Scroll instruments
    if ((input.mousey >= dpos.instrumentsY+1) &&
        (input.mousey <= dpos.instrumentsY+5) &&
        (input.mousex >= dpos.instrumentsX) &&
        (input.mousex <= dpos.instrumentsX+46))
    {
        if (input.wheel > 0)
          previnstr();
        else if (input.wheel < 0)
          nextinstr();
    }
    // Scroll orderlist
    if ((input.mousey >= dpos.orderlistY+1) &&
        (input.mousey <= dpos.orderlistY+1+maxChns) &&
        (input.mousex >= dpos.orderlistX) &&
        (input.mousex <= dpos.orderlistX+34+((config.numsids == 2)?13:34)))
    {
      int newchn = input.mousey - (dpos.orderlistY+1);
      if (input.wheel > 0)
      {
        if (esview[newchn] > 0)
        {
          esview[newchn]--;
          if (newchn == eschn) eseditpos--;
        }
      }
      else if (input.wheel < 0)
      {
        if ((song.len[esnum][newchn]-esview[newchn]) > config.getVisibleOrderlist()-1)
        {
          esview[newchn]++;
          if (newchn == eschn) eseditpos++;
        }
      }
    }
    // Scroll tables
    if ((input.mousey >= dpos.instrumentsY+8) &&
        (input.mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS) &&
        (input.mousex >= dpos.instrumentsX) &&
        (input.mousex <= dpos.instrumentsX+7+(MAX_TABLES-1)*12))
    {
        for (int c = 0; c < MAX_TABLES; c++)
        {
            if ((input.mousex >= dpos.instrumentsX+3+c*12) &&
                    (input.mousex <= dpos.instrumentsX+7+c*12))
            tables.m_num = c;
        }
        if (input.wheel > 0)
          tables.tableup(input.shiftpressed);
        else if (input.wheel < 0)
          tables.tabledown(input.shiftpressed);
    }
    // Scroll tempo
    if (input.mousey == dpos.statusTopY)
    {
        if ((input.mousex >= dpos.statusTopFvX+27) &&
            (input.mousex <= dpos.statusTopFvX+31))
        {
            if (input.wheel > 0)
                nextmultiplier();
            else if (input.wheel < 0)
                prevmultiplier();
        }
        else
        if ((config.multiplier == 1) &&
            (input.mousex >= dpos.statusTopFvX+31) &&
            (input.mousex <= dpos.statusTopFvX+37))
        {
            if (input.wheel > 0)
                snd_bpmtempo++;
            else if (input.wheel < 0)
                snd_bpmtempo--;
        }
    }
  }

  if (!input.mouseb) return;

  // Pattern editpos & pattern number selection
  for (int c = 0; c < maxChns; c++)
  {
    if ((input.mousey == dpos.patternsY) &&
            (input.mousex >= dpos.patternsX + 10 + c*13) &&
            (input.mousex <= dpos.patternsX + 11 + c*13))
    {
        if ((!input.prevmouseb) || (input.mouseheld > HOLDDELAY))
        {
        if (input.mouseb & MOUSEB_LEFT) 
        {
          epchn = c;
          nextpattern();
        }
        if (input.mouseb & MOUSEB_RIGHT)
        {
          epchn = c;
          prevpattern();
        }
      }
    }
    else
    {
      if ((input.mousey >= dpos.patternsY) &&
                (input.mousey <= dpos.statusBottomY - 1) &&
                (input.mousex >= dpos.patternsX + 3 + c*13) &&
                (input.mousex <= dpos.patternsX + 11 + c*13))
      {
        int x = input.mousex-(dpos.patternsX + 3)-c*13;
        int newpos = input.mousey-(dpos.patternsY+1)+epview[c];
        if (newpos < 0) newpos = 0;
        if (newpos > getPattlen(epnum[epchn])) newpos = getPattlen(epnum[epchn]);

        editmode = EDIT_PATTERN;

        if ((input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!input.prevmouseb))
        {
          if ((epmark.chn != c) || (newpos != epmark.end))
          {
            epmark.chn = c;
            epmark.start = epmark.end = newpos;
          }
        }

        if (input.mouseb & MOUSEB_LEFT)
        {
          epchn = c;
          if (x < 4) epcolumn = 0;
          if (x >= 4) epcolumn = x-3;
        }

        if (!input.prevmouseb)
        {
          if (input.mouseb & MOUSEB_LEFT)
            eppos = newpos;
        }
        else
        {
            if (input.mouseb & MOUSEB_LEFT)
            {
            if (input.mousey == dpos.patternsY) eppos--;
            if (input.mousey == dpos.statusBottomY - 1) eppos++;
          }
        }
        if (eppos < 0) eppos = 0;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);

        if (input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) epmark.end = newpos;
      }
    }
  }

  // Song editpos & songnumber selection
  if ((input.mousey >= dpos.orderlistY) &&
        (input.mousey <= dpos.orderlistY + maxChns + 2) &&
        (input.mousex >= dpos.orderlistX))
  {
    int newcolumn = (input.mousex-(dpos.orderlistX+4)) % 3;
    int newchn = input.mousey - (dpos.orderlistY+1);
    int newpos = esview[newchn] + (input.mousex-(dpos.orderlistX+4)) / 3;
    if (newcolumn < 0) newcolumn = 0;
    if (newcolumn > 1) newcolumn = 1;
    if (newpos < 0)
    {
      newpos = 0;
      newcolumn = 0;
    }
    if (newpos == currentSonglen)
    {
      newpos++;
      newcolumn = 0;
    }
    if (newpos > currentSonglen+1)
    {
      newpos = currentSonglen + 1;
      newcolumn = 1;
    }

    editmode = EDIT_ORDERLIST;

    if ((input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!input.prevmouseb) && (newpos < currentSonglen))
    {
      if ((esmark.chn != newchn) || (newpos != esmark.end))
      {
        esmark.chn = newchn;
        esmark.start = esmark.end = newpos;
      }
    }

    if (input.mouseb & MOUSEB_LEFT)
    {
      eschn = newchn;
      eseditpos = newpos;
      escolumn = newcolumn;
    }

    if ((input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (newpos < currentSonglen)) esmark.end = newpos;
  }
  if (((!input.prevmouseb) || (input.mouseheld > HOLDDELAY)) &&
        (input.mousey == dpos.orderlistY) &&
        (input.mousex >= dpos.orderlistX+23) && (input.mousex <= dpos.orderlistX+24))
  {
    if (input.mouseb & MOUSEB_LEFT) nextsong();
    if (input.mouseb & MOUSEB_RIGHT) prevsong();
  }

  // Instrument editpos
  if ((input.mousey >= dpos.instrumentsY+1) &&
        (input.mousey <= dpos.instrumentsY+5) &&
        (input.mousex >= (dpos.instrumentsX+20)) &&
        (input.mousex <= (dpos.instrumentsX+46)))
  {
    // Instr param
    eicolumn = (input.mousex-(dpos.instrumentsX+20))%3;
    if (eicolumn == 2) eicolumn--;
    eipos = (input.mousex-(dpos.instrumentsX+20))/3;
    gotoinstr(eirow+input.mousey-(dpos.instrumentsY+1));
  }
  if ((input.mousey >= dpos.instrumentsY+1) &&
        (input.mousey <= dpos.instrumentsY+5) &&
        (input.mousex >= dpos.instrumentsX+3) &&
        (input.mousex <= dpos.instrumentsX+19))
  {
    // Instr name
    editmode = EDIT_INSTRUMENT;
    eipos = 9;
    gotoinstr(eirow+input.mousey-(dpos.instrumentsY+1));
  }

  // Table editpos
  for (int c = 0; c < MAX_TABLES; c++)
  {
    if ((input.mousey >= dpos.instrumentsY+8) &&
            (input.mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS) &&
            (input.mousex >= dpos.instrumentsX+3+c*12) &&
            (input.mousex <= dpos.instrumentsX+7+c*12))
    {
      int newpos = input.mousey-(dpos.instrumentsY+8)+tables.curview();
      if (newpos < 0) newpos = 0;
      if (newpos >= MAX_TABLELEN) newpos = MAX_TABLELEN-1;

      editmode = EDIT_TABLES;

      if ((input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!input.prevmouseb))
      {
        tables.setmarkstart(c, newpos);
      }
      if (input.mouseb & MOUSEB_LEFT)
      {
        tables.setrow(c,
            newpos,
            input.mousex-(dpos.instrumentsX+3+c*12));
      }

      if (input.mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) tables.setmarkend(newpos);
    }
  }

  // Name editpos
  if ((input.mousey >= dpos.instrumentsY+8+VISIBLETABLEROWS+1) &&
        (input.mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS+3) &&
        (input.mousex >= dpos.instrumentsX+9))
  {
    editmode = EDIT_NAMES;
    enpos = input.mousey - (dpos.instrumentsY+8+VISIBLETABLEROWS+1);
  }

  // Status panel
  if ((!input.prevmouseb) &&
        (input.mousex == dpos.octaveX+7) &&
        (input.mousey == dpos.octaveY))
  {
    if (input.mouseb & (MOUSEB_LEFT))
      if (epoctave < 7) epoctave++;
    if (input.mouseb & (MOUSEB_RIGHT))
      if (epoctave > 0) epoctave--;
  }
  if ((!input.prevmouseb) && (input.mousex <= dpos.octaveX+7) && (input.mousey == dpos.octaveY+1))
  {
    recordmode = !recordmode;
  }
  if ((!input.prevmouseb) &&
      (input.mousex >= dpos.channelsX-5) &&
      (input.mousex <= dpos.channelsX-2) &&
      (input.mousey == dpos.channelsY+1))
  {
    tables.fliplock();
  }
  for (int c = 0; c < maxChns; c++)
  {
    if ((!input.prevmouseb) &&
            (input.mousey >= dpos.channelsY) &&
            (input.mousex >= dpos.channelsX + 7*c) &&
            (input.mousex <= dpos.channelsX+5 + 7*c))
      mutechannel(c);
  }
  if ((!input.prevmouseb) && (input.mousey == dpos.octaveY))
  {
    if ((input.mousex >= dpos.octaveX+20) &&
        (input.mousex <= dpos.octaveX+23))
    {
      if (isplaying())
      {
        stopsong();
      }
      else
      {
        initsong(esnum, input.shiftpressed ? PLAY_BEGINNING : PLAY_POS);
        followplay = true;
      }
    }
  }

  // Titlebar actions
  if (!menu)
  {
    if ((input.mousey == dpos.statusTopY) && (!input.prevmouseb) && (input.mouseb == MOUSEB_LEFT))
    {
      if ((input.mousex >= dpos.statusTopFvX-3) && (input.mousex <= dpos.statusTopFvX-2) && (config.numsids == 2))
      {
        monomode = !monomode;
      }
      if ((input.mousex >= dpos.statusTopFvX) && (input.mousex <= dpos.statusTopFvX+1))
      {
        config.usefinevib = !config.usefinevib;
      }
      if ((input.mousex >= dpos.statusTopFvX+3) && (input.mousex <= dpos.statusTopFvX+4))
      {
        config.optimizepulse ^= 1;
      }
      if ((input.mousex >= dpos.statusTopFvX+6) && (input.mousex <= dpos.statusTopFvX+7))
      {
        config.optimizerealtime ^= 1;
      }
      if ((input.mousex >= dpos.statusTopFvX+9) && (input.mousex <= dpos.statusTopFvX+12))
      {
        config.ntsc ^= 1;
        timer.setfreq(config.ntsc);
        sound_init(writer, config);
      }
      if ((input.mousex >= dpos.statusTopFvX+14) && (input.mousex <= dpos.statusTopFvX+17))
      {
        config.sidmodel ^= 1;
        sound_init(writer, config);
      }
      if ((input.mousex >= dpos.statusTopFvX+22) &&
          (input.mousex <= dpos.statusTopFvX+25)) editadsr(input.mousex - (dpos.statusTopFvX+22));
      if ((input.mousex >= dpos.statusTopFvX+27) &&
          (input.mousex <= dpos.statusTopFvX+28)) prevmultiplier();
      if ((input.mousex >= dpos.statusTopFvX+29) &&
          (input.mousex <= dpos.statusTopFvX+30)) nextmultiplier();
      if ((input.mousex >= dpos.statusTopFvX+31) &&
          (input.mousex <= dpos.statusTopFvX+33)) editbpm(input.mousex - (dpos.statusTopFvX+31));
      if ((input.mousex >= dpos.statusTopEndX-8) &&
          (input.mousex <= dpos.statusTopEndX-1)) onlinehelp(0,0);
    }
  }
  else
  {
    if ((!input.mousey) && (input.mouseb & MOUSEB_LEFT) && (!(input.prevmouseb & MOUSEB_LEFT)))
    {
      if ((input.mousex >= 0) && (input.mousex <= 5))
      {
        initsong(esnum, PLAY_BEGINNING);
        followplay = input.shiftpressed;
      }
      if ((input.mousex >= 7) && (input.mousex <= 15))
      {
        initsong(esnum, PLAY_POS);
        followplay = input.shiftpressed;
      }
      if ((input.mousex >= 17) && (input.mousex <= 26))
      {
        initsong(esnum, PLAY_PATTERN);
        followplay = input.shiftpressed;
      }
      if ((input.mousex >= 28) && (input.mousex <= 33))
        stopsong();
      if ((input.mousex >= 35) && (input.mousex <= 40))
        load();
      if ((input.mousex >= 42) && (input.mousex <= 47))
        save();
      if ((input.mousex >= 49) && (input.mousex <= 57))
      {
        if (config.numsids == 1)
        {
          relocator(loadedsongfilename);
        }
        else if (config.numsids == 2)
        {
          relocator_stereo(loadedsongfilename);
        }
      }
      if ((input.mousex >= 59) && (input.mousex <= 64))
        onlinehelp(0,0);
      if ((input.mousex >= 66) && (input.mousex <= 72))
        clear();
      if ((input.mousex >= 74) && (input.mousex <= 79))
        quit();
    }
  }
}

void generalcommands()
{
  int maxChns = config.getMaxChannels();
  int visibleOrderlist = config.getVisibleOrderlist();
  int currentSonglen = 0;

  switch(input.key)
  {
    case '?':
    case '-':
    if ((editmode != EDIT_NAMES) && (editmode != EDIT_ORDERLIST))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos == 9))) previnstr();
    }
    break;

    case '+':
    case '_':
    if ((editmode != EDIT_NAMES) && (editmode != EDIT_ORDERLIST))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9))) nextinstr();

    }
    break;

    case '*':
    if (editmode != EDIT_NAMES)
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave < 7) epoctave++;
      }
    }
    break;

    case '/':
    case '\'':
    if (editmode != EDIT_NAMES)
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave > 0) epoctave--;
      }
    }
    break;

    case '<':
    if (((editmode == EDIT_INSTRUMENT) && (eipos != 9)) || (editmode == EDIT_TABLES))
      previnstr();
    break;

    case '>':
    if (((editmode == EDIT_INSTRUMENT) && (eipos != 9)) || (editmode == EDIT_TABLES))
      nextinstr();
    break;

    case ';':
    for (int c = 0; c < maxChns; c++)
    {
      if (espos[c]) espos[c]--;
      if (espos[c] < esview[c])
      {
        esview[c] = espos[c];
        eseditpos = espos[c];
      }
    }
    updateviewtopos();
    //rewindsong(); // ??
    break;

    case ':':
    for (int c = 0; c < maxChns; c++)
    {
      currentSonglen = song.len[esnum][c];
      if (espos[c] < (currentSonglen-1))
        espos[c]++;
      if ((espos[c] - esview[c]) >= visibleOrderlist)
      {
        esview[c] = espos[c] - visibleOrderlist + 1;
        eseditpos = espos[c];
      }
    }
    updateviewtopos();
    //rewindsong(); // ??
    break;
  }
  if (win_quitted) exitprogram = true;
  switch(input.rawkey)
  {
    case KEY_ESC:
    if (!input.shiftpressed)
      quit();
    else
      clear();
    break;

    case KEY_KPMULTIPLY:
    if ((editmode != EDIT_NAMES) && (!input.key))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave < 7) epoctave++;
      }
    }
    break;

    case KEY_KPDIVIDE:
    if ((editmode != EDIT_NAMES) && (!input.key))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave > 0) epoctave--;
      }
    }
    break;

    case KEY_F12:
      onlinehelp(0, input.shiftpressed);
    break;

    case KEY_TAB:
    if (!input.shiftpressed) editmode++;
    else editmode--;
    if (editmode > EDIT_NAMES) editmode = EDIT_PATTERN;
    if (editmode < EDIT_PATTERN) editmode = EDIT_NAMES;
    break;

    case KEY_F1:
    initsong(esnum, PLAY_BEGINNING);
    followplay = input.shiftpressed;
    break;

    case KEY_F2:
    initsong(esnum, PLAY_POS);
    followplay = input.shiftpressed;
    break;

    case KEY_F3:
    initsong(esnum, PLAY_PATTERN);
    followplay = input.shiftpressed;
    break;

    case KEY_F4:
    if (input.shiftpressed)
      mutechannel(epchn);
    else
    {
      if (isplaying())
      {
        stopsong();
      }
      else
      {
        releasenote(epchn);
      }
    }
    break;

    case KEY_F5:
    if (!input.shiftpressed)
      editmode = EDIT_PATTERN;
    else prevmultiplier();
    break;

    case KEY_F6:
    if (!input.shiftpressed)
      editmode = EDIT_ORDERLIST;
    else nextmultiplier();
    break;

    case KEY_F7:
    if (!input.shiftpressed)
    {
      if (editmode == EDIT_INSTRUMENT)
        editmode = EDIT_TABLES;
      else
        editmode = EDIT_INSTRUMENT;
    }
    else editadsr(0);
    break;

    case KEY_F8:
    if (!input.shiftpressed)
      editmode = EDIT_NAMES;
    else
    {
      config.sidmodel ^= 1;
      sound_init(writer, config);
    }
    break;

    case KEY_F9:
    if (!input.shiftpressed)
    {
        if (config.numsids == 1)
        {
          relocator(loadedsongfilename);
        }
        else if (config.numsids == 2)
        {
          relocator_stereo(loadedsongfilename);
        }
    }
    else if (input.shiftpressed && (config.numsids == 2))
    {
        monomode = !monomode;
    }
    break;

    case KEY_F10:
    load();
    break;

    case KEY_F11:
    save();
    break;

    case KEY_M:
    if (input.altpressed)
    {
      switchMode();
    }
    break;
  }
}

void load()
{
  if ((editmode != EDIT_INSTRUMENT) && (editmode != EDIT_TABLES))
  {
    if (!input.shiftpressed)
    {
      if (fileselector(songfilename, songpath, songfilter, "LOAD SONG", 0))
        loadsong();
    }
    else
    {
      if (fileselector(songfilename, songpath, songfilter, "MERGE SONG", 0))
        mergesong();
    }
  }
  else
  {
    if (einum)
    {
      if (fileselector(instrfilename, instrpath, instrfilter, "LOAD INSTRUMENT", 0))
        loadinstrument();
    }
  }
  input.clearkeys();
}

void save()
{
  if ((editmode != EDIT_INSTRUMENT) && (editmode != EDIT_TABLES))
  {
    bool done = false;

    // Repeat until quit or save successful
    while (!done)
    {
      if (std::strlen(loadedsongfilename)) std::strcpy(songfilename, loadedsongfilename);
      if (fileselector(songfilename, songpath, songfilter, "SAVE SONG", 3))
        done = savesong();
      else done = true;
    }
  }
  else
  {
    if (einum)
    {
      bool done = false;
      int useinstrname = 0;
      char tempfilename[MAX_FILENAME];

      // Repeat until quit or save successful
      while (!done)
      {
        if ((!std::strlen(instrfilename)) && (std::strlen(song.instr[einum].name)))
        {
          useinstrname = 1;
          std::strcpy(instrfilename, song.instr[einum].name);
          std::strcat(instrfilename, ".ins");
          std::strcpy(tempfilename, instrfilename);
        }

        if (fileselector(instrfilename, instrpath, instrfilter, "SAVE INSTRUMENT", 3))
          done = saveinstrument();
        else done = true;

        if (useinstrname)
        {
          if (!std::strcmp(tempfilename, instrfilename))
            std::memset(instrfilename, 0, sizeof instrfilename);
        }
      }
    }
  }
  input.clearkeys();
}

void quit()
{
  if ((!input.shiftpressed) || (input.mouseb))
  {
    printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Really Quit (y/n)?");
    waitkey();
    printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
    if (input.iskeyyes()) exitprogram = true;
  }
  input.clearkeys();
}

void clear()
{
  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Optimize everything (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes())
  {
    optimizeeverything();
    input.clearkeys();
    return;
  }

  bool cs = false;
  bool cp = false;
  bool ci = false;
  bool ct = false;
  bool cn = false;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Clear orderlists (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes()) cs = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Clear patterns (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes()) cp = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Clear instruments (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes()) ci = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Clear tables (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes()) ct = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, colors.CMESSAGE, "Clear songname (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if (input.iskeyyes()) cn = true;

  if (cp)
  {
    bool selectdone = false;
    unsigned olddpl = config.defaultpatternlength;

    printtext(dpos.statusBottomX+20, dpos.statusBottomY, colors.CMESSAGE, "Pattern length:");
    while (!selectdone)
    {
        if (config.patterndispmode)
        {
            std::sprintf(textbuffer, "%02X ", config.defaultpatternlength);
        }
        else
        {
            std::sprintf(textbuffer, "%02d ", config.defaultpatternlength);
        }
        printtext(dpos.statusBottomX+35, dpos.statusBottomY, colors.CTITLE, textbuffer);
      waitkey();
      switch(input.rawkey)
      {
        case KEY_LEFT:
        config.defaultpatternlength -= 7;
        /* fall through */
        case KEY_DOWN:
        config.defaultpatternlength--;
        if (config.defaultpatternlength < 1) config.defaultpatternlength = 1;
        break;

        case KEY_RIGHT:
        config.defaultpatternlength += 7;
        /* fall through */
        case KEY_UP:
        config.defaultpatternlength++;
        if (config.defaultpatternlength > MAX_PATTROWS) config.defaultpatternlength = MAX_PATTROWS;
        break;

        case KEY_ESC:
        config.defaultpatternlength = olddpl;
        selectdone = true;
        break;

        case KEY_ENTER:
        selectdone = true;
        break;
      }
    }
    printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  }

  if (cs | cp | ci | ct | cn)
    std::memset(songfilename, 0, sizeof songfilename);
  clearsong(cs, cp, ci, ct, cn);

  input.clearkeys();
}

void editadsr(int col)
{
  ehmode = EditHdr::ADSR;
  eacolumn = col;

  for (;;)
  {
    waitkeymouse();

    if (win_quitted)
    {
      exitprogram = true;
      input.clearkeys();
      return;
    }

    if ((input.mousey == dpos.statusTopY) && (!input.prevmouseb) && (input.mouseb == MOUSEB_LEFT) &&
          (input.mousex >= dpos.statusTopFvX+22) &&
          (input.mousex <= dpos.statusTopFvX+25))
    {
        eacolumn = input.mousex - (dpos.statusTopFvX+22);
        continue;
    }

    if (hexnybble >= 0)
    {
      switch(eacolumn)
      {
        case 0:
        config.adparam &= 0x0fff;
        config.adparam |= hexnybble << 12;
        break;

        case 1:
        config.adparam &= 0xf0ff;
        config.adparam |= hexnybble << 8;
        break;

        case 2:
        config.adparam &= 0xff0f;
        config.adparam |= hexnybble << 4;
        break;

        case 3:
        config.adparam &= 0xfff0;
        config.adparam |= hexnybble;
        break;
      }
      eacolumn++;
    }

    switch(input.rawkey)
    {
      case KEY_F7:
      if (!input.shiftpressed) break;
      /* fall through */
      case KEY_ESC:
      case KEY_ENTER:
      case KEY_TAB:
      ehmode = EditHdr::NONE;
      input.clearkeys();
      return;

      case KEY_BACKSPACE:
      if (!eacolumn) break;
      /* fall through */
      case KEY_LEFT:
      eacolumn--;
      break;

      case KEY_RIGHT:
      eacolumn++;
    }
    eacolumn &= 3;

    if ((input.mouseb) && (!input.prevmouseb))
    {
      ehmode = EditHdr::NONE;
      return;
    }
  }
}

void editbpm(int col)
{
    ehmode = EditHdr::BPM;
    eacolumn = col;

    for (;;)
    {
        waitkeymouse();

        if (win_quitted)
        {
            exitprogram = true;
            input.clearkeys();
            return;
        }

        if ((input.mousey == dpos.statusTopY) && (!input.prevmouseb) && (input.mouseb == MOUSEB_LEFT) &&
            (input.mousex >= dpos.statusTopFvX+31) &&
            (input.mousex <= dpos.statusTopFvX+33))
        {
            eacolumn = input.mousex - (dpos.statusTopFvX+31);
            continue;
        }

        if (input.key >= 48 && input.key <= 58)
        {
            int number = input.key - 48;

            switch(eacolumn)
            {
            case 0:
                snd_bpmtempo = snd_bpmtempo - (((snd_bpmtempo / 100) % 10) * 100) + (number * 100);
                break;

            case 1:
                snd_bpmtempo = snd_bpmtempo - (((snd_bpmtempo / 10) % 10) * 10) + (number * 10);
                break;

            case 2:
                snd_bpmtempo = snd_bpmtempo - (((snd_bpmtempo / 1) % 10) * 1) + (number * 1);
                break;
            }
            eacolumn++;
        }

        switch(input.rawkey)
        {
        case KEY_F7:
            if (!input.shiftpressed) break;
            // fall through
        case KEY_ESC:
        case KEY_ENTER:
        case KEY_TAB:
            ehmode = EditHdr::NONE;
            input.clearkeys();
            return;

        case KEY_BACKSPACE:
            if (!eacolumn) break;
            // fall through
        case KEY_LEFT:
            eacolumn--;
            break;

        case KEY_RIGHT:
            eacolumn++;
        }
        eacolumn &= 3;
        if (eacolumn == 3) eacolumn = 0;

        if ((input.mouseb) && (!input.prevmouseb))
        {
            ehmode = EditHdr::NONE;
            return;
        }
    }
}

void prevmultiplier()
{
  if (config.multiplier > 0)
  {
    config.multiplier--;
    timer.setmult(config.multiplier);
    sound_init(writer, config);
  }
}

void nextmultiplier()
{
  if (config.multiplier < 16)
  {
    config.multiplier++;
    timer.setmult(config.multiplier);
    sound_init(writer, config);
  }
}

void setspecialnotenames()
{
  char octave[11];

  int i = 0;
  int oct = 0;
  while (i < 93)
  {
    for (int j = 0; j < 186; j += 2)
    {
      if (specialnotenames[j] == '\0')
        break;
      if (i < 93)
      {
        char *name = (char*)std::malloc(4); // FIXME
        std::strncpy(name, specialnotenames + j, 2);
        std::sprintf(octave, "%d", oct);
        std::strcpy(name + 2, octave);
        notename[i] = name;
        i++;
      }
    }
    oct++;
  }
}

void switchMode()
{
    char nextMode[7];
    std::strcpy(nextMode, (config.numsids == 1) ? "STEREO" : "MONO");

    char textbuffer[80];
    std::sprintf(textbuffer, "Switch to %s Mode (y/n) ?", nextMode);

    printtextcp(
        dpos.statusBottomX+29,
        dpos.statusBottomY,
        colors.CMESSAGE,
        textbuffer
    );
    settooltip("Warning: All Songdata Will Be Lost!!!");

    waitkey();

    printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
    printblank(dpos.statusBottomX, dpos.statusBottomY+1, 58);

    if (input.iskeyyes())
    {
        std::memset(songfilename, 0, sizeof songfilename);

        config.numsids ^= 3;
        clearsong(true, true, true, true, true);

        sound_init(writer, config);
        initDisplayPositions();
        printmainscreen();
    }
    input.clearkeys();
}

void optimizeeverything()
{
  stopsong();

  findduplicatepatterns();

  std::memset(instrused, 0, sizeof instrused);

  for (int c = MAX_PATT-1; c >= 0; c--)
  {
    if (pattused[c])
    {
      for (int d = 0; d < MAX_PATTROWS; d++)
      {
        if (song.pattern[c][d*4] == ENDPATT) break;
        if (song.pattern[c][d*4+1])
          instrused[song.pattern[c][d*4+1]] = 1;
      }
    }
    else deletepattern(c);
  }

  countpatternlengths();

  for (int c = MAX_INSTR-2; c >= 1; c--)
  {
    if (!instrused[c])
    {
      clearinstr(c);

      if (c < MAX_INSTR-2)
      {
        std::memmove(&song.instr[c], &song.instr[c+1], (MAX_INSTR-2-c) * sizeof(Instr));
        clearinstr(MAX_INSTR-2);
        for (int d = 0; d < MAX_PATT; d++)
        {
          for (int e = 0; e < getPattlen(d); e++)
          {
            if ((song.pattern[d][e*4+1] > c) && (song.pattern[d][e*4+1] != MAX_INSTR-1))
              song.pattern[d][e*4+1]--;
          }
        }
      }
    }
  }

  for (int c = 0; c < MAX_TABLES; c++) optimizetable(c);
}

void findduplicatepatterns()
{
  int maxChns = config.getMaxChannels();

  findusedpatterns();

  for (int c = 0; c < MAX_PATT; c++)
  {
    if (pattused[c])
    {
      for (int d = c+1; d < MAX_PATT; d++)
      {
        if (getPattlen(d) == getPattlen(c))
        {
          if (!std::memcmp(song.pattern[c], song.pattern[d], getPattlen(c)*4))
          {
            for (int f = 0; f < MAX_SONGS; f++)
            {
              if ((song.len[f][0]) &&
                  (song.len[f][1]) &&
                  (song.len[f][2]))
              {
                for (int g = 0; g < maxChns; g++)
                {
                  for (int h = 0; h < song.len[f][g]; h++)
                  {
                    if (song.order[f][g][h] == d)
                      song.order[f][g][h] = c;
                  }
                }
              }
            }
            for (int f = 0; f < maxChns; f++)
              if (epnum[f] == d) epnum[f] = c;
          }
        }
      }
    }
  }

  findusedpatterns();
}
