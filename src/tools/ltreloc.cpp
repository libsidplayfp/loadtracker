/*
 * LTReloc
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
// commandline relocator/packer
// =============================================================================

#define LTRELOC_C

#include "ltreloc.h"

#include "config.h"

#include "channels.h"
#include "colors.h"
#include "console.h"
#include "pattern.h"
#include "reloc.h"
#include "settings.h"
#include "song.h"
#include "tuning.h"

#include "bme_io.h"
#include "bme_win.h"

#ifdef _WIN32
#  include <windows.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#if 1
#define NUMSIDREGS 0x19
unsigned char sidreg[NUMSIDREGS];
unsigned char sidreg2[NUMSIDREGS];
#endif

bool monomode = true;
int snd_bpmtempo = 125;

char loadedsongfilename[MAX_FILENAME];
char songfilename[MAX_FILENAME];
char instrfilename[MAX_FILENAME];

const char *programname = "LTReloc v" PACKAGE_VERSION;

extern unsigned char datafile[];

#ifdef _WIN32
FILE *STDOUT, *STDERR;
#else
#  define STDOUT stdout
#  define STDERR stderr
#endif

void usage()
{
    std::fprintf(STDOUT, "Usage: ltreloc <songname> <outfile> [options]\n");
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
    std::fprintf(STDOUT, "-Sxx Set speed config.multiplier (0 for 25Hz, 1 for 1x, 2 for 2x etc.) DEFAULT=1\n");
    std::fprintf(STDOUT, "-Vxx Set config.finevibrato conversion (0 = off, 1 = on) DEFAULT=on\n");
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

  char packedsongname[MAX_PATHNAME];
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
      if (std::strlen(loadedsongfilename) == 0)
      {
        std::fprintf(STDERR, "error: file not found.\n");
        std::exit(EXIT_FAILURE);
      }
  } else {
      std::fprintf(STDERR, "error: no song filename given.\n");
      std::exit(EXIT_FAILURE);
  }

  int c = (int)std::strlen(packedsongname);
  if (c <= 0) {
      std::fprintf(STDERR, "error: no output filename given.\n");
      std::exit(EXIT_FAILURE);
  }

  // determine output format from file extension of the output filename
  c--;
  while ((c > 0) && (packedsongname[c] != '.')) c--;
  if (packedsongname[c] == '.') c++;

  if (!std::strcmp(&packedsongname[c], "sid")) {
      config.fileformat = FORMAT_SID;
  } else if (!std::strcmp(&packedsongname[c], "prg")) {
      config.fileformat = FORMAT_PRG;
  } else if (!std::strcmp(&packedsongname[c], "bin")) {
      config.fileformat = FORMAT_BIN;
  } else {
      config.fileformat = FORMAT_PRG;
  }

  std::fprintf(STDOUT, "%s Packer/Relocator\n", programname);
  std::fprintf(STDOUT, "song file:       %s\n", loadedsongfilename);
  std::fprintf(STDOUT, "output file:     %s\n", packedsongname);
  std::fprintf(STDOUT, "output format:   ");
  if (config.fileformat == FORMAT_SID) {
      std::fprintf(STDOUT, "sid\n");
  } else if (config.fileformat == FORMAT_BIN) {
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
        std::sscanf(&argv[c][2], "%x", &config.adparam);
        break;

        case 'G':
        std::sscanf(&argv[c][2], "%f", &config.basepitch);
        break;

        case 'L':
        std::sscanf(&argv[c][2], "%x", &config.sidaddress);
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

        case 'S':
        std::sscanf(&argv[c][2], "%u", &config.multiplier);
        break;

        // NTSC timing
        case 'N':
        config.ntsc = 1;
        config.customclockrate = 0;
        break;
        // PAL timing
        case 'P':
        config.ntsc = 0;
        config.customclockrate = 0;
        break;
        // custom clock rate
        case 'F':
        std::sscanf(&argv[c][2], "%u", &config.customclockrate);
        break;

        // player options (first menu)
        // 0: Buffered SID-writes
        case 'B':
            if (argv[c][2] == '1') {
                config.playerversion |= PLAYER_BUFFERED;
            } else {
                config.playerversion &= ~PLAYER_BUFFERED;
            }
        break;
        // 1: Sound effect support
        case 'D':
            if (argv[c][2] == '1') {
                config.playerversion |= PLAYER_SOUNDEFFECTS;
            } else {
                config.playerversion &= ~PLAYER_SOUNDEFFECTS;
            }
        break;
        // 2: Volume change support
        case 'E':
            if (argv[c][2] == '1') {
                config.playerversion |= PLAYER_VOLUME;
            } else {
                config.playerversion &= ~PLAYER_VOLUME;
            }
        break;
        // 3: Store author-info
        case 'H':
            if (argv[c][2] == '1') {
                config.playerversion |= PLAYER_AUTHORINFO;
            } else {
                config.playerversion &= ~PLAYER_AUTHORINFO;
            }
        break;
        // 4: Use zeropage ghostregs
        case 'C':
            if (argv[c][2] == '1') {
                config.playerversion |= PLAYER_ZPGHOSTREGS;
            } else {
                config.playerversion &= ~PLAYER_ZPGHOSTREGS;
            }
        break;
        // 5: Disable optimization
        case 'I':
            if (argv[c][2] == '1') {
                config.playerversion &= ~PLAYER_NOOPTIMIZATION;
            } else {
                config.playerversion |= PLAYER_NOOPTIMIZATION;
            }
        break;
        // 6: Full buffering
        case 'J':
            if (argv[c][2] == '1') {
                config.playerversion &= ~PLAYER_FULLBUFFERED;
            } else {
                config.playerversion |= PLAYER_FULLBUFFERED;
            }
        break;

        // start address (second menu)
        case 'W':
        std::sscanf(&argv[c][2], "%x", (unsigned *)&config.playeradr);
        config.playeradr<<=8;
        break;

        // zeropage address (third menu)
        case 'Z':
        std::sscanf(&argv[c][2], "%x", (unsigned *)&config.zeropageadr);
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
  colors.init(true);

  // Validate parameters
  config.validate();

  // Calculate frequencytable if necessary
  if (config.basepitch < 0.0f)
    config.basepitch = 0.0f;
  if (config.basepitch > 0.0f)
    calculatefreqtable(config.basepitch, 12.);

  // perform relocation
  relocator(packedsongname);
  // FIXME relocator_stereo

  // Exit
  return 0;
}

void waitkeymousenoupdate()
{
}

void waitkeynoupdate()
{
}
