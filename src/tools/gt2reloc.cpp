//
// GOATTRACKER v2.73 - gt2reloc (commandline relocator/packer)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#define GT2RELOC_C

#include "gt2reloc.h"

#include "console.h"
#include "pattern.h"
#include "reloc.h"
#include "sound.h"
#include "file.h"
#include "play.h"
#include "display.h"
#include "song.h"

#include "bme_io.h"
#include "bme_win.h"

#ifdef _WIN32
#  include <windows.h>
#endif

#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

enum class EditHdr
{
  NONE    = 0,
  ADSR    = 1,
  BPM     = 2
};

enum
{
  EDIT_PATTERN      = 0,
  EDIT_ORDERLIST    = 1,
  EDIT_INSTRUMENT   = 2,
  EDIT_TABLES       = 3,
  EDIT_NAMES        = 4
};

bool menu = false;
int editmode = EDIT_PATTERN;
bool recordmode = true;
bool followplay = false;
int hexnybble = -1;
int stepsize = 4;
int defaultpatternlength = 64;
bool exitprogram = false;
int eacolumn = 0;
int eamode = 0;
EditHdr ehmode = EditHdr::NONE;

unsigned keypreset = KEY_TRACKER;
unsigned playerversion = 0;
int fileformat = FORMAT_PRG;
int zeropageadr = 0xfc;
int playeradr = 0x1000;
unsigned sidmodel = 0;
unsigned multiplier = 1;
unsigned adparam = 0x0f00;
unsigned ntsc = 0;
unsigned patterndispmode = 0;
unsigned sidaddress = 0xd400;
unsigned sid2address = 0xd500;
unsigned finevibrato = 1;
unsigned optimizepulse = 1;
unsigned optimizerealtime = 1;
unsigned customclockrate = 0;
bool usefinevib = false;
unsigned mr = DEFAULTMIXRATE;
unsigned writer = 0;
unsigned interpolate = 0;
unsigned residdelay = 0;
float basepitch = 0.0f;
unsigned numsids = 1;
bool monomode = true;
int snd_bpmtempo = 125;

#define NUMSIDREGS 0x19
unsigned char sidreg[NUMSIDREGS];
unsigned char sidreg2[NUMSIDREGS];

char configbuf[MAX_PATHNAME];
char loadedsongfilename[MAX_FILENAME];
char songfilename[MAX_FILENAME];
char songfilter[MAX_FILENAME];
char songpath[MAX_PATHNAME];
char instrfilename[MAX_FILENAME];
char instrfilter[MAX_FILENAME];
char instrpath[MAX_PATHNAME];
char packedpath[MAX_PATHNAME];
char packedsongname[MAX_PATHNAME];

const char *programname = "GoatTracker v2.77";

char textbuffer[MAX_PATHNAME];

extern unsigned char datafile[];
Colors colors;

void calculatefreqtable();
int getMaxChannels();
int getVisibleOrderlist();
void waitkeynoupdate();

#ifdef _WIN32
FILE *STDOUT, *STDERR;
#else
#  define STDOUT stdout
#  define STDERR stderr
#endif

void usage(void)
{
    std::fprintf(STDOUT, "Usage: GT2RELOC <songname> <outfile> [options]\n");
    std::fprintf(STDOUT, "Options:\n");
    std::fprintf(STDOUT, "-Axx Set ADSR parameter for hardrestart in hex. DEFAULT=0F00\n");
    std::fprintf(STDOUT, "-Bx  enable/disable buffered SID writes. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Cx  enable/disable zeropage ghost registers. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Dx  enable/disable sound effect support. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Ex  enable/disable volume change support. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Fxx Set custom SID clock cycles per second (0 = use PAL/NTSC default)\n");
    std::fprintf(STDOUT, "-Gxx Set pitch of A-4 in Hz (0 = use default frequencytable, close to 440Hz)\n");
    std::fprintf(STDOUT, "-Hx  enable/disable storing of author info. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Ix  enable/disable optimizations. DEFAULT=enabled\n");
    std::fprintf(STDOUT, "-Jx  enable/disable full buffering. DEFAULT=disabled\n");
    std::fprintf(STDOUT, "-Lxx SID memory location in hex. DEFAULT=D400\n");
    std::fprintf(STDOUT, "-N   Use NTSC timing\n");
    std::fprintf(STDOUT, "-Oxx Set pulseoptimization/skipping (0 = off, 1 = on) DEFAULT=on\n");
    std::fprintf(STDOUT, "-P   Use PAL timing (DEFAULT)\n");
    std::fprintf(STDOUT, "-Rxx Set realtime-effect optimization/skipping (0 = off, 1 = on) DEFAULT=on\n");
    std::fprintf(STDOUT, "-Sxx Set speed multiplier (0 for 25Hz, 1 for 1x, 2 for 2x etc.) DEFAULT=1\n");
    std::fprintf(STDOUT, "-Vxx Set finevibrato conversion (0 = off, 1 = on) DEFAULT=on\n");
    std::fprintf(STDOUT, "-Wxx player memory location highbyte in hex. DEFAULT=1000\n");
    std::fprintf(STDOUT, "-Zxx zeropage memory location in hex. DEFAULT=FC\n");
    std::fprintf(STDOUT, "-?   Show options\n");
}

int main(int argc, char **argv)
{
#ifdef _WIN32
  /*
    SDL_Init() reroutes stdout and stderr, either to stdout.txt and stderr.txt
    or to nirwana. simply reopening these handles does, other than suggested on
    some web pages, not work reliably - opening new files on CON using different
    handles however does.
  */
  STDOUT = std::fopen("CON", "w");
  STDERR = std::fopen("CON", "w");
#endif

  // Open datafile
  io_openlinkeddatafile(datafile);

  // Reset channels/song
  initchannels();
  clearsong(true,true,true,true,true);

  // get input- and output file names
  if (argc >= 3) {
      std::strcpy(songfilename, argv[1]);
      std::strcpy(packedsongname, argv[2]);
  } else {
      usage();
      std::exit(EXIT_FAILURE);
  }

  // Load song
  if (std::strlen(songfilename)) {
      loadsong();
  } else {
      std::fprintf(STDERR, "error: no song filename given.\n");
      std::exit(EXIT_FAILURE);
  }

  int c = std::strlen(packedsongname);
  if (std::strlen(packedsongname) <= 0) {
      std::fprintf(STDERR, "error: no output filename given.\n");
      std::exit(EXIT_FAILURE);
  }

  // determine output format from file extension of the output filename
  c--;
  while ((c > 0) && (packedsongname[c] != '.')) c--;
  if (packedsongname[c] == '.') c++;

  if (!std::strcmp(&packedsongname[c], "sid")) {
      fileformat = FORMAT_SID;
  } else if (!std::strcmp(&packedsongname[c], "prg")) {
      fileformat = FORMAT_PRG;
  } else if (!std::strcmp(&packedsongname[c], "bin")) {
      fileformat = FORMAT_BIN;
  } else {
      fileformat = FORMAT_PRG;
  }

  std::fprintf(STDOUT, "%s Packer/Relocator\n", programname);
  std::fprintf(STDOUT, "song file:       %s\n", loadedsongfilename);
  std::fprintf(STDOUT, "output file:     %s\n", packedsongname);
  std::fprintf(STDOUT, "output format:   ");
  if (fileformat == FORMAT_SID) {
      std::fprintf(STDOUT, "sid\n");
  } else if (fileformat == FORMAT_BIN) {
      std::fprintf(STDOUT, "bin\n");
  } else {
      std::fprintf(STDOUT, "prg\n");
  }

  // Scan command line
  for (c = 3; c < argc; c++)
  {
#ifdef _WIN32
    if ((argv[c][0] == '-') || (argv[c][0] == '/'))
#else
    if (argv[c][0] == '-')
#endif
    {
      switch(std::toupper(argv[c][1]))
      {
        case '?':
        return 0;

        case 'A':
        std::sscanf(&argv[c][2], "%x", &adparam);
        break;

        case 'G':
        std::sscanf(&argv[c][2], "%f", &basepitch);
        break;

        case 'L':
        std::sscanf(&argv[c][2], "%x", &sidaddress);
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

        case 'S':
        std::sscanf(&argv[c][2], "%u", &multiplier);
        break;

        // NTSC timing
        case 'N':
        ntsc = 1;
        customclockrate = 0;
        break;
        // PAL timing
        case 'P':
        ntsc = 0;
        customclockrate = 0;
        break;
        // custom clock rate
        case 'F':
        std::sscanf(&argv[c][2], "%u", &customclockrate);
        break;

        // player options (first menu)
        // 0: Buffered SID-writes
        case 'B':
            if (argv[c][2] == '1') {
                playerversion |= PLAYER_BUFFERED;
            } else {
                playerversion &= ~PLAYER_BUFFERED;
            }
        break;
        // 1: Sound effect support
        case 'D':
            if (argv[c][2] == '1') {
                playerversion |= PLAYER_SOUNDEFFECTS;
            } else {
                playerversion &= ~PLAYER_SOUNDEFFECTS;
            }
        break;
        // 2: Volume change support
        case 'E':
            if (argv[c][2] == '1') {
                playerversion |= PLAYER_VOLUME;
            } else {
                playerversion &= ~PLAYER_VOLUME;
            }
        break;
        // 3: Store author-info
        case 'H':
            if (argv[c][2] == '1') {
                playerversion |= PLAYER_AUTHORINFO;
            } else {
                playerversion &= ~PLAYER_AUTHORINFO;
            }
        break;
        // 4: Use zeropage ghostregs
        case 'C':
            if (argv[c][2] == '1') {
                playerversion |= PLAYER_ZPGHOSTREGS;
            } else {
                playerversion &= ~PLAYER_ZPGHOSTREGS;
            }
        break;
        // 5: Disable optimization
        case 'I':
            if (argv[c][2] == '1') {
                playerversion &= ~PLAYER_NOOPTIMIZATION;
            } else {
                playerversion |= PLAYER_NOOPTIMIZATION;
            }
        break;
        // 6: Full buffering
        case 'J':
            if (argv[c][2] == '1') {
                playerversion &= ~PLAYER_FULLBUFFERED;
            } else {
                playerversion |= PLAYER_FULLBUFFERED;
            }
        break;

        // start address (second menu)
        case 'W':
        std::sscanf(&argv[c][2], "%x", (unsigned *)&playeradr);
        playeradr<<=8;
        break;

        // zeropage address (third menu)
        case 'Z':
        std::sscanf(&argv[c][2], "%x", (unsigned *)&zeropageadr);
        break;
      }
    }
    else
    {
      std::fprintf(STDERR, "error: unknown option\n");
      usage();
      std::exit(EXIT_FAILURE);
    }
  }

  // Init colorscheme
  initcolorscheme(1);

  // Validate parameters
  sidmodel &= 1;
  adparam &= 0xffff;
  zeropageadr &= 0xff;
  playeradr &= 0xff00;
  sidaddress &= 0xffff;

  if (multiplier > 16) multiplier = 16;
  if ((finevibrato == 1) && (multiplier < 2)) usefinevib = true;
  if (finevibrato > 1) usefinevib = true;
  if (optimizepulse > 1) optimizepulse = 1;
  if (optimizerealtime > 1) optimizerealtime = 1;
  if (customclockrate < 100) customclockrate = 0;

  // Calculate frequencytable if necessary
  if (basepitch < 0.0f)
    basepitch = 0.0f;
  if (basepitch > 0.0f)
    calculatefreqtable();

  // perform relocation
  relocator();

  // Exit
  return 0;
}

void waitkeymousenoupdate()
{
}

void getparam(FILE *handle, unsigned *value)
{
  char *configptr;

  for (;;)
  {
    if (std::feof(handle)) return;
    std::fgets(configbuf, MAX_PATHNAME, handle);
    if ((configbuf[0]) && (configbuf[0] != ';') && (configbuf[0] != ' ') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
  }

  configptr = configbuf;
  if (*configptr == '$')
  {
    *value = 0;
    configptr++;
    for (;;)
    {
      char c = tolower(*configptr++);
      int h = -1;

      if ((c >= 'a') && (c <= 'f')) h = c - 'a' + 10;
      if ((c >= '0') && (c <= '9')) h = c - '0';

      if (h >= 0)
      {
        *value *= 16;
        *value += h;
      }
      else break;
    }
  }
  else
  {
    *value = 0;
    for (;;)
    {
      char c = tolower(*configptr++);
      int d = -1;

      if ((c >= '0') && (c <= '9')) d = c - '0';

      if (d >= 0)
      {
        *value *= 10;
        *value += d;
      }
      else break;
    }
  }
}

void getfloatparam(FILE *handle, float *value)
{
  char *configptr;

  for (;;)
  {
    if (std::feof(handle)) return;
    std::fgets(configbuf, MAX_PATHNAME, handle);
    if ((configbuf[0]) && (configbuf[0] != ';') && (configbuf[0] != ' ') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
  }

  configptr = configbuf;
  *value = 0.0f;
  std::sscanf(configptr, "%f", value);
}

void calculatefreqtable()
{
  double basefreq = (double)basepitch * (16777216.0 / 985248.0) * std::pow(2.0, 0.25) / 32.0;
  int c;

  for (c = 0; c < 8*12 ; c++)
  {
    double note = c;
    double freq = basefreq * std::pow(2.0, note/12.0);
    int intfreq = freq + 0.5;
    if (intfreq > 0xffff)
        intfreq = 0xffff;
    freqtbllo[c] = intfreq & 0xff;
    freqtblhi[c] = intfreq >> 8;
  }
}

int getMaxChannels()
{
    return (numsids == 1) ? MAX_CHN_MONO : MAX_CHN;
}

int getVisibleOrderlist()
{
    return (numsids == 1) ? 23 : 14;
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
