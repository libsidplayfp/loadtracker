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
#include "table.h"

#include "bme_main.h"
#include "bme_win.h"
#include "bme_snd.h"
#include "bme_io.h"

#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#define HOLDDELAY 24

bool menu = false;
int editmode = EDIT_PATTERN;
bool recordmode = true;
bool followplay = false;
int hexnybble = -1;
bool exitprogram = false;
int eacolumn = 0;
EditHdr ehmode = EditHdr::NONE;

bool monomode = true;
bool writer = false;

char loadedsongfilename[MAX_FILENAME];
char songfilename[MAX_FILENAME];
char songfilter[MAX_FILENAME];
char songpath[MAX_PATHNAME];
char instrfilename[MAX_FILENAME];
char instrfilter[MAX_FILENAME];
char instrpath[MAX_PATHNAME];
char packedpath[MAX_PATHNAME];

int tuningcount = 0;
double tuning[96];
char tuningname[64];

extern char *notename[];
const char *programname = "LoadTracker " PACKAGE_VERSION;

char textbuffer[MAX_PATHNAME];

unsigned char hexkeytbl[] = {'0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

extern unsigned char datafile[]; // from ltrkdata.cpp

Colors colors;

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
void readscalatuningfile();
void setspecialnotenames();
void calculatefreqtable();
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

  bool dark = darkmode != 0;

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
          printtext(0, y, 15, usage[y]);
        waitkeynoupdate();
#else
        for (int y = 0; y < usagelen; ++y)
          std::printf("%s\n", usage[y]);
#endif
        return EXIT_SUCCESS;

        case 'Z':
        std::sscanf(&argv[c][2], "%u", &residdelay);
        break;

        case 'A':
        std::sscanf(&argv[c][2], "%x", &adparam);
        break;

        case 'S':
        std::sscanf(&argv[c][2], "%u", &multiplier);
        break;

        case 'D':
        std::sscanf(&argv[c][2], "%u", &patterndispmode);
        break;

        case 'E':
        std::sscanf(&argv[c][2], "%u", &sidmodel);
        break;

        case 'I':
        std::sscanf(&argv[c][2], "%u", &interpolate);
        break;

        case 'K':
        std::sscanf(&argv[c][2], "%u", &keypreset);
        break;

        case 'L':
        std::sscanf(&argv[c][2], "%x", &sidaddress);
        break;

        case 'N':
        ntsc = 1;
        customclockrate = 0;
        break;

        case 'P':
        ntsc = 0;
        customclockrate = 0;
        break;

        case 'F':
        std::sscanf(&argv[c][2], "%u", &customclockrate);
        break;

        case 'M':
        std::sscanf(&argv[c][2], "%u", &mr);
        break;

        case 'O':
        std::sscanf(&argv[c][2], "%u", &optimizepulse);
        break;

        case 'R':
        std::sscanf(&argv[c][2], "%u", &optimizerealtime);
        break;

        case 'V':
        std::sscanf(&argv[c][2], "%u", &finevibrato);
        break;

        case 'W':
        writer = true;
        break;

        case 'X':
        std::sscanf(&argv[c][2], "%d", &win_fullscreen);
        break;

        case 'G':
        std::sscanf(&argv[c][2], "%f", &basepitch);
        break;

        case 'b':
        std::sscanf(&argv[c][2], "%f", &filterbias);
        break;

        case 'c':
        std::sscanf(&argv[c][2], "%u", &combwaves);
        break;

        case 'Q':
        std::sscanf(&argv[c][2], "%f", &equaldivisionsperoctave);
        break;

        case 'J':
        std::sscanf(&argv[c][2], "%s", specialnotenames);
        break;

        case 'Y':
        std::sscanf(&argv[c][2], "%s", scalatuningfilepath);
        break;

        case 'x':
        std::sscanf(&argv[c][2], "%u", &exsid);
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
  initcolorscheme(dark);

  // Validate parameters
  validateconfig();

  initDisplayPositions();

  // Read Scala tuning file
  if (scalatuningfilepath[0] != '0' && scalatuningfilepath[1] != '\0')
  {
    readscalatuningfile();
  }

  // Calculate frequencytable if necessary
  if (basepitch < 0.0f)
    basepitch = 0.0f;
  if (basepitch > 0.0f)
    calculatefreqtable();

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

  // Init sound
  if (!sound_init(mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves))
  {
    printtextc(MAX_ROWS/2-1,15,"Sound init failed. Press any key to run without sound (notice that song timer won't start)");
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

  win_savepos();

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
    if ((rawkey) || (key)) break;
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
    if ((rawkey) || (key)) break;
    if (win_quitted) break;
    if (mouseb || (win_mouseywheel != 0.f)) break;
  }

  converthex();
}

void waitkeymousenoupdate()
{
  for (;;)
  {
    fliptoscreen();
    getkey();
    if ((rawkey) || (key)) break;
    if (win_quitted) break;
    if (mouseb) break;
  }

  converthex();
}

void waitkeynoupdate()
{
  for (;;)
  {
    fliptoscreen();
    getkey();
    if ((rawkey) || (key)) break;
    if ((mouseb) && (!prevmouseb)) break;
    if (win_quitted) break;
  }
}

void converthex()
{
  int c;

  hexnybble = -1;
  for (c = 0; c < 16; c++)
  {
    if (std::tolower(key) == hexkeytbl[c])
    {
      if (c >= 10)
      {
        if (!shiftpressed) hexnybble = c;
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
    if (mousey == dpos.statusTopY)
    {
      if ((mousex >= dpos.statusTopFvX-3) && (mousex <= dpos.statusTopFvX-2) && (numsids == 2))
      {
        settooltip("Stereo mode");
      }
      if ((mousex >= dpos.statusTopFvX) && (mousex <= dpos.statusTopFvX+1))
      {
        settooltip("Fine vibrato");
      }
      if ((mousex >= dpos.statusTopFvX+3) && (mousex <= dpos.statusTopFvX+4))
      {
        settooltip("Optimize pulse");
      }
      if ((mousex >= dpos.statusTopFvX+6) && (mousex <= dpos.statusTopFvX+7))
      {
        settooltip("Optimize realtime");
      }
      if ((mousex >= dpos.statusTopFvX+9) && (mousex <= dpos.statusTopFvX+12))
      {
        settooltip("Video frequency");
      }
      if ((mousex >= dpos.statusTopFvX+14) && (mousex <= dpos.statusTopFvX+17))
      {
        settooltip("SID model");
      }
      if ((mousex >= dpos.statusTopFvX+22) &&
          (mousex <= dpos.statusTopFvX+25)) settooltip("Hard restart ADSR");
      if ((mousex >= dpos.statusTopFvX+27) &&
          (mousex <= dpos.statusTopFvX+30)) settooltip("Speed multiplier");
      if ((mousex >= dpos.statusTopFvX+31) &&
          (mousex <= dpos.statusTopFvX+33)) settooltip("BPM");
      if ((mousex >= dpos.statusTopEndX-8) &&
          (mousex <= dpos.statusTopEndX-1)) settooltip("Online help");
    }
  }

  // Instruments
  if (mousey == dpos.instrumentsY)
  {
    if ((mousex >= (dpos.instrumentsX+20)) &&
        (mousex <= (dpos.instrumentsX+21)))
      settooltip("Attack/Decay");
    if ((mousex >= (dpos.instrumentsX+23)) &&
        (mousex <= (dpos.instrumentsX+24)))
      settooltip("Sustain/Release");
    if ((mousex >= (dpos.instrumentsX+26)) &&
        (mousex <= (dpos.instrumentsX+27)))
      settooltip("Wave table position");
    if ((mousex >= (dpos.instrumentsX+29)) &&
        (mousex <= (dpos.instrumentsX+30)))
      settooltip("Pulse table position");
    if ((mousex >= (dpos.instrumentsX+32)) &&
        (mousex <= (dpos.instrumentsX+33)))
      settooltip("Filter table position");
    if ((mousex >= (dpos.instrumentsX+35)) &&
        (mousex <= (dpos.instrumentsX+36)))
      settooltip("Speed table position (vibrato)");
    if ((mousex >= (dpos.instrumentsX+38)) &&
        (mousex <= (dpos.instrumentsX+39)))
      settooltip("Vibrato delay");
    if ((mousex >= (dpos.instrumentsX+41)) &&
        (mousex <= (dpos.instrumentsX+42)))
      settooltip("Gate timer");
    if ((mousex >= (dpos.instrumentsX+44)) &&
        (mousex <= (dpos.instrumentsX+45)))
      settooltip("First wave");
  }
}

void mousecommands()
{
  int maxChns = getMaxChannels();

  int currentSonglen = song.len[esnum][eschn];

  if (win_mouseywheel != 0.f)
  {
    // Scroll patterns
    if ((mousey >= dpos.patternsY) &&
        (mousey <= dpos.statusBottomY - 1) &&
        (mousex >= dpos.patternsX) &&
        (mousex <= dpos.patternsX + 11 + (maxChns-1)*13))
    {
        if (win_mouseywheel > 0.f)
          patternup();
        else if (win_mouseywheel < 0.f)
          patterndown();
    }
    // Scroll instruments
    if ((mousey >= dpos.instrumentsY+1) &&
        (mousey <= dpos.instrumentsY+5) &&
        (mousex >= dpos.instrumentsX) &&
        (mousex <= dpos.instrumentsX+46))
    {
        if (win_mouseywheel > 0.f)
          previnstr();
        else if (win_mouseywheel < 0.f)
          nextinstr();
    }
    // Scroll orderlist
    if ((mousey >= dpos.orderlistY+1) &&
        (mousey <= dpos.orderlistY+1+maxChns) &&
        (mousex >= dpos.orderlistX) &&
        (mousex <= dpos.orderlistX+34+((numsids == 2)?13:34)))
    {
      int newchn = mousey - (dpos.orderlistY+1);
      if (win_mouseywheel > 0.f)
      {
        if (esview[newchn] > 0)
        {
          esview[newchn]--;
          if (newchn == eschn) eseditpos--;
        }
      }
      else if (win_mouseywheel < 0.f)
      {
        if ((song.len[esnum][newchn]-esview[newchn]) > getVisibleOrderlist()-1)
        {
          esview[newchn]++;
          if (newchn == eschn) eseditpos++;
        }
      }
    }
    // Scroll tables
    if ((mousey >= dpos.instrumentsY+8) &&
        (mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS) &&
        (mousex >= dpos.instrumentsX) &&
        (mousex <= dpos.instrumentsX+7+(MAX_TABLES-1)*12))
    {
        if (win_mouseywheel > 0.f)
          tables.tableup(shiftpressed);
        else if (win_mouseywheel < 0.f)
          tables.tabledown(shiftpressed);
    }
    // Scroll tempo
    if (mousey == dpos.statusTopY)
    {
        if ((mousex >= dpos.statusTopFvX+27) &&
            (mousex <= dpos.statusTopFvX+31))
        {
            if (win_mouseywheel > 0.f)
                nextmultiplier();
            else if (win_mouseywheel < 0.f)
                prevmultiplier();
        }
        else
        if ((multiplier == 1) &&
            (mousex >= dpos.statusTopFvX+31) &&
            (mousex <= dpos.statusTopFvX+37))
        {
            if (win_mouseywheel > 0.f)
                snd_bpmtempo++;
            else if (win_mouseywheel < 0.f)
                snd_bpmtempo--;
        }
    }
  }

  if (!mouseb) return;

  // Pattern editpos & pattern number selection
  for (int c = 0; c < maxChns; c++)
  {
    if ((mousey == dpos.patternsY) &&
            (mousex >= dpos.patternsX + 10 + c*13) &&
            (mousex <= dpos.patternsX + 11 + c*13))
    {
        if ((!prevmouseb) || (mouseheld > HOLDDELAY))
        {
        if (mouseb & MOUSEB_LEFT) 
        {
          epchn = c;
          nextpattern();
        }
        if (mouseb & MOUSEB_RIGHT)
        {
          epchn = c;
          prevpattern();
        }
      }
    }
    else
    {
      if ((mousey >= dpos.patternsY) &&
                (mousey <= dpos.statusBottomY - 1) &&
                (mousex >= dpos.patternsX + 3 + c*13) &&
                (mousex <= dpos.patternsX + 11 + c*13))
      {
        int x = mousex-(dpos.patternsX + 3)-c*13;
        int newpos = mousey-(dpos.patternsY+1)+epview[c];
        if (newpos < 0) newpos = 0;
        if (newpos > getPattlen(epnum[epchn])) newpos = getPattlen(epnum[epchn]);

        editmode = EDIT_PATTERN;

        if ((mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!prevmouseb))
        {
          if ((epmark.chn != c) || (newpos != epmark.end))
          {
            epmark.chn = c;
            epmark.start = epmark.end = newpos;
          }
        }

        if (mouseb & MOUSEB_LEFT)
        {
          epchn = c;
          if (x < 4) epcolumn = 0;
          if (x >= 4) epcolumn = x-3;
        }

        if (!prevmouseb)
        {
          if (mouseb & MOUSEB_LEFT)
            eppos = newpos;
        }
        else
        {
            if (mouseb & MOUSEB_LEFT)
            {
            if (mousey == dpos.patternsY) eppos--;
            if (mousey == dpos.statusBottomY - 1) eppos++;
          }
        }
        if (eppos < 0) eppos = 0;
        if (eppos > getPattlen(epnum[epchn])) eppos = getPattlen(epnum[epchn]);

        if (mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) epmark.end = newpos;
      }
    }
  }

  // Song editpos & songnumber selection
  if ((mousey >= dpos.orderlistY) &&
        (mousey <= dpos.orderlistY + maxChns + 2) &&
        (mousex >= dpos.orderlistX))
  {
    int newcolumn = (mousex-(dpos.orderlistX+4)) % 3;
    int newchn = mousey - (dpos.orderlistY+1);
    int newpos = esview[newchn] + (mousex-(dpos.orderlistX+4)) / 3;
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

    if ((mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!prevmouseb) && (newpos < currentSonglen))
    {
      if ((esmark.chn != newchn) || (newpos != esmark.end))
      {
        esmark.chn = newchn;
        esmark.start = esmark.end = newpos;
      }
    }

    if (mouseb & MOUSEB_LEFT)
    {
      eschn = newchn;
      eseditpos = newpos;
      escolumn = newcolumn;
    }

    if ((mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (newpos < currentSonglen)) esmark.end = newpos;
  }
  if (((!prevmouseb) || (mouseheld > HOLDDELAY)) &&
        (mousey == dpos.orderlistY) &&
        (mousex >= dpos.orderlistX+23) && (mousex <= dpos.orderlistX+24))
  {
    if (mouseb & MOUSEB_LEFT) nextsong();
    if (mouseb & MOUSEB_RIGHT) prevsong();
  }

  // Instrument editpos
  if ((mousey >= dpos.instrumentsY+1) &&
        (mousey <= dpos.instrumentsY+5) &&
        (mousex >= (dpos.instrumentsX+20)) &&
        (mousex <= (dpos.instrumentsX+46)))
  {
    // Instr param
    eicolumn = (mousex-(dpos.instrumentsX+20))%3;
    if (eicolumn == 2) eicolumn--;
    eipos = (mousex-(dpos.instrumentsX+20))/3;
    gotoinstr(eirow+mousey-(dpos.instrumentsY+1));
  }
  if ((mousey >= dpos.instrumentsY+1) &&
        (mousey <= dpos.instrumentsY+5) &&
        (mousex >= dpos.instrumentsX+3) &&
        (mousex <= dpos.instrumentsX+19))
  {
    // Instr name
    editmode = EDIT_INSTRUMENT;
    eipos = 9;
    gotoinstr(eirow+mousey-(dpos.instrumentsY+1));
  }

  // Table editpos
  for (int c = 0; c < MAX_TABLES; c++)
  {
    if ((mousey >= dpos.instrumentsY+8) &&
            (mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS) &&
            (mousex >= dpos.instrumentsX+3+c*12) &&
            (mousex <= dpos.instrumentsX+7+c*12))
    {
      int newpos = mousey-(dpos.instrumentsY+8)+tables.curview();
      if (newpos < 0) newpos = 0;
      if (newpos >= MAX_TABLELEN) newpos = MAX_TABLELEN-1;

      editmode = EDIT_TABLES;

      if ((mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) && (!prevmouseb))
      {
        tables.setmarkstart(c, newpos);
      }
      if (mouseb & MOUSEB_LEFT)
      {
        tables.setrow(c,
            newpos,
            mousex-(dpos.instrumentsX+3+c*12));
      }

      if (mouseb & (MOUSEB_RIGHT|MOUSEB_MIDDLE)) tables.setmarkend(newpos);
    }
  }

  // Name editpos
  if ((mousey >= dpos.instrumentsY+8+VISIBLETABLEROWS+1) &&
        (mousey <= dpos.instrumentsY+8+VISIBLETABLEROWS+3) &&
        (mousex >= dpos.instrumentsX+9))
  {
    editmode = EDIT_NAMES;
    enpos = mousey - (dpos.instrumentsY+8+VISIBLETABLEROWS+1);
  }

  // Status panel
  if ((!prevmouseb) &&
        (mousex == dpos.octaveX+7) &&
        (mousey == dpos.octaveY))
  {
    if (mouseb & (MOUSEB_LEFT))
      if (epoctave < 7) epoctave++;
    if (mouseb & (MOUSEB_RIGHT))
      if (epoctave > 0) epoctave--;
  }
  if ((!prevmouseb) && (mousex <= dpos.octaveX+7) && (mousey == dpos.octaveY+1))
  {
    recordmode = !recordmode;
  }
  if ((!prevmouseb) &&
      (mousex >= dpos.channelsX-5) &&
      (mousex <= dpos.channelsX-2) &&
      (mousey == dpos.channelsY+1))
  {
    tables.fliplock();
  }
  for (int c = 0; c < maxChns; c++)
  {
    if ((!prevmouseb) &&
            (mousey >= dpos.channelsY) &&
            (mousex >= dpos.channelsX + 7*c) &&
            (mousex <= dpos.channelsX+5 + 7*c))
      mutechannel(c);
  }
  if ((!prevmouseb) && (mousey == dpos.octaveY))
  {
    if ((mousex >= dpos.octaveX+20) &&
        (mousex <= dpos.octaveX+23))
    {
      if (isplaying())
      {
        stopsong();
      }
      else
      {
        initsong(esnum, shiftpressed ? PLAY_BEGINNING : PLAY_POS);
        followplay = true;
      }
    }
  }

  // Titlebar actions
  if (!menu)
  {
    if ((mousey == dpos.statusTopY) && (!prevmouseb) && (mouseb == MOUSEB_LEFT))
    {
      if ((mousex >= dpos.statusTopFvX-3) && (mousex <= dpos.statusTopFvX-2) && (numsids == 2))
      {
        monomode = !monomode;
      }
      if ((mousex >= dpos.statusTopFvX) && (mousex <= dpos.statusTopFvX+1))
      {
        usefinevib = !usefinevib;
      }
      if ((mousex >= dpos.statusTopFvX+3) && (mousex <= dpos.statusTopFvX+4))
      {
        optimizepulse ^= 1;
      }
      if ((mousex >= dpos.statusTopFvX+6) && (mousex <= dpos.statusTopFvX+7))
      {
        optimizerealtime ^= 1;
      }
      if ((mousex >= dpos.statusTopFvX+9) && (mousex <= dpos.statusTopFvX+12))
      {
        ntsc ^= 1;
        sound_init(mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves);
      }
      if ((mousex >= dpos.statusTopFvX+14) && (mousex <= dpos.statusTopFvX+17))
      {
        sidmodel ^= 1;
        sound_init(mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves);
      }
      if ((mousex >= dpos.statusTopFvX+22) &&
          (mousex <= dpos.statusTopFvX+25)) editadsr(mousex - (dpos.statusTopFvX+22));
      if ((mousex >= dpos.statusTopFvX+27) &&
          (mousex <= dpos.statusTopFvX+28)) prevmultiplier();
      if ((mousex >= dpos.statusTopFvX+29) &&
          (mousex <= dpos.statusTopFvX+30)) nextmultiplier();
      if ((mousex >= dpos.statusTopFvX+31) &&
          (mousex <= dpos.statusTopFvX+33)) editbpm(mousex - (dpos.statusTopFvX+31));
      if ((mousex >= dpos.statusTopEndX-8) &&
          (mousex <= dpos.statusTopEndX-1)) onlinehelp(0,0);
    }
  }
  else
  {
    if ((!mousey) && (mouseb & MOUSEB_LEFT) && (!(prevmouseb & MOUSEB_LEFT)))
    {
      if ((mousex >= 0) && (mousex <= 5))
      {
        initsong(esnum, PLAY_BEGINNING);
        followplay = shiftpressed;
      }
      if ((mousex >= 7) && (mousex <= 15))
      {
        initsong(esnum, PLAY_POS);
        followplay = shiftpressed;
      }
      if ((mousex >= 17) && (mousex <= 26))
      {
        initsong(esnum, PLAY_PATTERN);
        followplay = shiftpressed;
      }
      if ((mousex >= 28) && (mousex <= 33))
        stopsong();
      if ((mousex >= 35) && (mousex <= 40))
        load();
      if ((mousex >= 42) && (mousex <= 47))
        save();
      if ((mousex >= 49) && (mousex <= 57))
      {
        if (numsids == 1)
        {
          relocator();
        }
        else if (numsids == 2)
        {
          relocator_stereo();
        }
      }
      if ((mousex >= 59) && (mousex <= 64))
        onlinehelp(0,0);
      if ((mousex >= 66) && (mousex <= 72))
        clear();
      if ((mousex >= 74) && (mousex <= 79))
        quit();
    }
  }
}

void generalcommands()
{
  int maxChns = getMaxChannels();
  int visibleOrderlist = getVisibleOrderlist();
  int currentSonglen = 0;

  switch(key)
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
  switch(rawkey)
  {
    case KEY_ESC:
    if (!shiftpressed)
      quit();
    else
      clear();
    break;

    case KEY_KPMULTIPLY:
    if ((editmode != EDIT_NAMES) && (!key))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave < 7) epoctave++;
      }
    }
    break;

    case KEY_KPDIVIDE:
    if ((editmode != EDIT_NAMES) && (!key))
    {
      if (!((editmode == EDIT_INSTRUMENT) && (eipos >= 9)))
      {
        if (epoctave > 0) epoctave--;
      }
    }
    break;

    case KEY_F12:
      onlinehelp(0, shiftpressed);
    break;

    case KEY_TAB:
    if (!shiftpressed) editmode++;
    else editmode--;
    if (editmode > EDIT_NAMES) editmode = EDIT_PATTERN;
    if (editmode < EDIT_PATTERN) editmode = EDIT_NAMES;
    break;

    case KEY_F1:
    initsong(esnum, PLAY_BEGINNING);
    followplay = shiftpressed;
    break;

    case KEY_F2:
    initsong(esnum, PLAY_POS);
    followplay = shiftpressed;
    break;

    case KEY_F3:
    initsong(esnum, PLAY_PATTERN);
    followplay = shiftpressed;
    break;

    case KEY_F4:
    if (shiftpressed)
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
    if (!shiftpressed)
      editmode = EDIT_PATTERN;
    else prevmultiplier();
    break;

    case KEY_F6:
    if (!shiftpressed)
      editmode = EDIT_ORDERLIST;
    else nextmultiplier();
    break;

    case KEY_F7:
    if (!shiftpressed)
    {
      if (editmode == EDIT_INSTRUMENT)
        editmode = EDIT_TABLES;
      else
        editmode = EDIT_INSTRUMENT;
    }
    else editadsr(0);
    break;

    case KEY_F8:
    if (!shiftpressed)
      editmode = EDIT_NAMES;
    else
    {
      sidmodel ^= 1;
      sound_init( mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves);
    }
    break;

    case KEY_F9:
    if (!shiftpressed)
    {
        if (numsids == 1)
        {
          relocator();
        }
        else if (numsids == 2)
        {
          relocator_stereo();
        }
    }
    else if (shiftpressed && (numsids == 2))
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
    if (altpressed)
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
    if (!shiftpressed)
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
  key = 0;
  rawkey = 0;
}

void save()
{
  if ((editmode != EDIT_INSTRUMENT) && (editmode != EDIT_TABLES))
  {
    bool done = false;

    // Repeat until quit or save successful
    while (!done)
    {
      if (std::strlen(loadedsongfilename)) strcpy(songfilename, loadedsongfilename);
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
  key = 0;
  rawkey = 0;
}

void quit()
{
  if ((!shiftpressed) || (mouseb))
  {
    printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Really Quit (y/n)?");
    waitkey();
    printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
    if ((key == 'y') || (key == 'Y')) exitprogram = true;
  }
  key = 0;
  rawkey = 0;
}

void clear()
{
  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Optimize everything (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y'))
  {
    optimizeeverything();
    key = 0;
    rawkey = 0;
    return;
  }

  bool cs = false;
  bool cp = false;
  bool ci = false;
  bool ct = false;
  bool cn = false;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Clear orderlists (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y')) cs = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Clear patterns (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y')) cp = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Clear instruments (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y')) ci = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Clear tables (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y')) ct = true;

  printtextcp(dpos.statusBottomX+29, dpos.statusBottomY, 15, "Clear songname (y/n)?");
  waitkey();
  printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
  if ((key == 'y') || (key == 'Y')) cn = true;

  if (cp)
  {
    bool selectdone = false;
    unsigned olddpl = defaultpatternlength;

    printtext(dpos.statusBottomX+20, dpos.statusBottomY, 15, "Pattern length:");
    while (!selectdone)
    {
        if (patterndispmode)
        {
            std::sprintf(textbuffer, "%02X ", defaultpatternlength);
        }
        else
        {
            std::sprintf(textbuffer, "%02d ", defaultpatternlength);
        }
        printtext(dpos.statusBottomX+35, dpos.statusBottomY, colors.CTITLE, textbuffer);
      waitkey();
      switch(rawkey)
      {
        case KEY_LEFT:
        defaultpatternlength -= 7;
        /* fall through */
        case KEY_DOWN:
        defaultpatternlength--;
        if (defaultpatternlength < 1) defaultpatternlength = 1;
        break;

        case KEY_RIGHT:
        defaultpatternlength += 7;
        /* fall through */
        case KEY_UP:
        defaultpatternlength++;
        if (defaultpatternlength > MAX_PATTROWS) defaultpatternlength = MAX_PATTROWS;
        break;

        case KEY_ESC:
        defaultpatternlength = olddpl;
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

  key = 0;
  rawkey = 0;
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
      key = 0;
      rawkey = 0;
      return;
    }

    if ((mousey == dpos.statusTopY) && (!prevmouseb) && (mouseb == MOUSEB_LEFT) &&
          (mousex >= dpos.statusTopFvX+22) &&
          (mousex <= dpos.statusTopFvX+25))
    {
        eacolumn = mousex - (dpos.statusTopFvX+22);
        continue;
    }

    if (hexnybble >= 0)
    {
      switch(eacolumn)
      {
        case 0:
        adparam &= 0x0fff;
        adparam |= hexnybble << 12;
        break;

        case 1:
        adparam &= 0xf0ff;
        adparam |= hexnybble << 8;
        break;

        case 2:
        adparam &= 0xff0f;
        adparam |= hexnybble << 4;
        break;

        case 3:
        adparam &= 0xfff0;
        adparam |= hexnybble;
        break;
      }
      eacolumn++;
    }

    switch(rawkey)
    {
      case KEY_F7:
      if (!shiftpressed) break;
      /* fall through */
      case KEY_ESC:
      case KEY_ENTER:
      case KEY_TAB:
      ehmode = EditHdr::NONE;
      key = 0;
      rawkey = 0;
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

    if ((mouseb) && (!prevmouseb))
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
            key = 0;
            rawkey = 0;
            return;
        }

        if ((mousey == dpos.statusTopY) && (!prevmouseb) && (mouseb == MOUSEB_LEFT) &&
            (mousex >= dpos.statusTopFvX+31) &&
            (mousex <= dpos.statusTopFvX+33))
        {
            eacolumn = mousex - (dpos.statusTopFvX+31);
            continue;
        }

        if (key >= 48 && key <= 58)
        {
            int number = key - 48;

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

        switch(rawkey)
        {
        case KEY_F7:
            if (!shiftpressed) break;
            // fall through
        case KEY_ESC:
        case KEY_ENTER:
        case KEY_TAB:
            ehmode = EditHdr::NONE;
            key = 0;
            rawkey = 0;
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

        if ((mouseb) && (!prevmouseb))
        {
            ehmode = EditHdr::NONE;
            return;
        }
    }
}

void prevmultiplier()
{
  if (multiplier > 0)
  {
    multiplier--;
    sound_init(mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves);
  }
}

void nextmultiplier()
{
  if (multiplier < 16)
  {
    multiplier++;
    sound_init(mr, writer, sidmodel, ntsc, multiplier, interpolate, customclockrate, exsid, filterbias, combwaves);
  }
}

void calculatefreqtable()
{
  double basefreq = (double)basepitch * (16777216.0 / 985248.0) * std::pow(2.0, 0.25) / 32.0;
  double cyclebasefreq = basefreq;
  double freq = basefreq;

  if (tuningcount)
  {
    int c = 0;
    while (c < 96)
    {
      for (int i = 0; i < tuningcount; i++)
      {
        if (c < 96)
        {
          int intfreq = freq + 0.5;
          if (intfreq > 0xffff)
              intfreq = 0xffff;
          freqtbllo[c] = intfreq & 0xff;
          freqtblhi[c] = intfreq >> 8;
          freq = cyclebasefreq * tuning[i];
          c++;
        }
      }
      cyclebasefreq = freq;
    }
  }
  else
  {
    for (int c = 0; c < 8*12 ; c++)
    {
      double note = c;
      double freq = basefreq * std::pow(2.0, note/(double)equaldivisionsperoctave);
      int intfreq = freq + 0.5;
      if (intfreq > 0xffff)
          intfreq = 0xffff;
      freqtbllo[c] = intfreq & 0xff;
      freqtblhi[c] = intfreq >> 8;
    }
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

void readscalatuningfile()
{
  FILE *scalatuningfile = fopen(scalatuningfilepath, "rt");
  if (scalatuningfile)
  {
    char configbuf[MAX_PATHNAME];
    char *configptr;
    char strbuf[64];
    char name[3];

    // Tuning name
    for (;;)
    {
      if (feof(scalatuningfile)) return;
      std::fgets(configbuf, MAX_PATHNAME, scalatuningfile);
      if ((configbuf[0]) && (configbuf[0] != '!') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
    }
    configptr = configbuf;
    std::sscanf(configptr, "%63[^\t\n]", tuningname);

    // Tuning count
    for (;;)
    {
      if (feof(scalatuningfile)) return;
      std::fgets(configbuf, MAX_PATHNAME, scalatuningfile);
      if ((configbuf[0]) && (configbuf[0] != '!') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
    }
    configptr = configbuf;
    std::sscanf(configptr, "%d", &tuningcount);

    // Tunings 
    for (int i = 0; i < tuningcount; i++)
    {
      for (;;)
      {
        if (feof(scalatuningfile)) return;
        std::fgets(configbuf, MAX_PATHNAME, scalatuningfile);
        if ((configbuf[0]) && (configbuf[0] != '!') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
      }
      configptr = configbuf;
      name[0] = '\0';
      std::sscanf(configptr, "%63s %2s", strbuf, name);
      if (!i)
      {
        std::strcpy(specialnotenames, name);
      }
      else
      {
        if (i == tuningcount - 1)
        {
          char *tmp = strdup(specialnotenames);
          std::strcpy(specialnotenames, name);
          std::strcat(specialnotenames, tmp);
          std::free(tmp);
        }
        else
        {
          std::strcat(specialnotenames, name);
        }
      }
      if (!std::strchr(strbuf, '.'))
      {
        double numerator;
        double denominator;
        std::sscanf(strbuf, "%lf", &numerator);
        if (std::strchr(strbuf, '/'))
        {
          std::sscanf(std::strchr(strbuf, '/') + 1, "%lf", &denominator);
          tuning[i] = numerator / denominator;
        }
      }
      else
      {
        double centvalue;
        std::sscanf(configptr, "%lf", &centvalue);
        tuning[i] = std::pow(2.0, centvalue / 1200.0);
      }
    }
    std::fclose(scalatuningfile);
  }
}

void switchMode()
{
    char nextMode[7];
    std::strcpy(nextMode, (numsids == 1) ? "STEREO" : "MONO");

    char textbuffer[80];
    std::sprintf(textbuffer, "Switch to %s Mode (y/n) ?", nextMode);

    printtextcp(
        dpos.statusBottomX+29,
        dpos.statusBottomY,
        colors.CTITLE,
        textbuffer
    );
    printtextcp(
        dpos.statusBottomX+29,
        dpos.statusBottomY+1,
        colors.CEDIT,
        "!!! SONGDATA WILL BE LOST !!!"
    );

    waitkey();

    printblank(dpos.statusBottomX, dpos.statusBottomY, 58);
    printblank(dpos.statusBottomX, dpos.statusBottomY+1, 58);

    if ((key == 'y') || (key == 'Y'))
    {
        std::memset(songfilename, 0, sizeof songfilename);

        numsids ^= 3;
        clearsong(true, true, true, true, true);

        sound_init(
            mr,
            writer,
            sidmodel,
            ntsc,
            multiplier,
            interpolate,
            customclockrate, exsid, filterbias, combwaves
        );
        initDisplayPositions();
        printmainscreen();
    }
    key = 0;
    rawkey = 0;
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
  int maxChns = getMaxChannels();

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

int getMaxChannels()
{
    return (numsids == 1) ? MAX_CHN_MONO : MAX_CHN;
}

int getVisibleOrderlist()
{
    return (numsids == 1) ? 23 : 14;
}
