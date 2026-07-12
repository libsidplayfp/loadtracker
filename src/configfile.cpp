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

#define CONFIGFILE_C

#include "configfile.h"

#include "common.h"
#include "file.h"
#include "sound.h"
#include "pattern.h"
#include "reloc.h"

#include "bme_win.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

// Increase if configuration has incompatible changes
#define CFG_VERSION 2

// config
unsigned mr = DEFAULTMIXRATE;
unsigned sidmodel = 1;
unsigned numsids = 1;
unsigned ntsc = 0;
int fileformat = FORMAT_PRG;
int playeradr = 0x1000;
int zeropageadr = 0xfc;
unsigned playerversion = 0;
unsigned keypreset = KEY_TRACKER;
unsigned defaultpatternlength = 64;
int stepsize = 4;
unsigned multiplier = 1;
unsigned adparam = 0x0f00;
unsigned interpolate = 1;
unsigned patterndispmode = 2;
unsigned sidaddress = 0xd400;
unsigned sid2address = 0xd500;
float panning = 1.0f;
unsigned finevibrato = 1;
unsigned optimizepulse = 1;
unsigned optimizerealtime = 1;
unsigned residdelay = 0;
unsigned customclockrate = 0;
float basepitch = 0.0f;
float filterbias = 0.5f;
unsigned combwaves = 1;
float equaldivisionsperoctave = 12.0f;
char specialnotenames[186];
char scalatuningfilepath[MAX_PATHNAME];
unsigned exsid = 0;
unsigned darkmode = 0;

void getparam(FILE *handle, unsigned *value);
void getfloatparam(FILE *handle, float *value);
void getstringparam(FILE *handle, char *value);

void getFilename(char filename[MAX_PATHNAME], bool createdir)
{
#ifdef __WIN32__
  GetModuleFileName(NULL, filename, MAX_PATHNAME);
  filename[strlen(filename)-3] = 'c';
  filename[strlen(filename)-2] = 'f';
  filename[strlen(filename)-1] = 'g';
#elif __amigaos__
  std::strcpy(filename, "PROGDIR:loadtrk.cfg");
#else
  char* xdg_home = std::getenv("XDG_CONFIG_HOME");
  if (xdg_home)
  {
    std::strcpy(filename, xdg_home);
    std::strcat(filename, "/loadtrk");
  }
  else
  {
    std::strcpy(filename, getenv("HOME"));
    std::strcat(filename, "/.config/loadtrk");
  }
  if (createdir)
    mkdir(filename, S_IRUSR | S_IWUSR | S_IXUSR);
  std::strcat(filename, "/loadtrk.cfg");
#endif
}

void loadconfig()
{
  char filename[MAX_PATHNAME];

  getFilename(filename, false);

  specialnotenames[0] = 0;
  scalatuningfilepath[0] = 0;
  FILE *configfile = fopen(filename, "rt");
  if (configfile)
  {
    unsigned cfg_version;
    getparam(configfile, &cfg_version);
    if (cfg_version == CFG_VERSION)
    {
        getparam(configfile, &mr);
        getparam(configfile, &sidmodel);
        getparam(configfile, &numsids);
        getparam(configfile, &ntsc);
        getparam(configfile, (unsigned *)&fileformat);
        getparam(configfile, (unsigned *)&playeradr);
        getparam(configfile, (unsigned *)&zeropageadr);
        getparam(configfile, &playerversion);
        getparam(configfile, &keypreset);
        getparam(configfile, &defaultpatternlength);
        getparam(configfile, (unsigned *)&stepsize);
        getparam(configfile, &multiplier);
        getparam(configfile, &adparam);
        getparam(configfile, &interpolate);
        getparam(configfile, &patterndispmode);
        getparam(configfile, &sidaddress);
        getparam(configfile, &sid2address);
        getfloatparam(configfile, &panning);
        getparam(configfile, &finevibrato);
        getparam(configfile, &optimizepulse);
        getparam(configfile, &optimizerealtime);
        getparam(configfile, &residdelay);
        getparam(configfile, &customclockrate);
        getparam(configfile, (unsigned*)&win_fullscreen);
        getfloatparam(configfile, &basepitch);
        getfloatparam(configfile, &filterbias);
        getparam(configfile, &combwaves);
        getfloatparam(configfile, &equaldivisionsperoctave);
        getstringparam(configfile, specialnotenames);
        getstringparam(configfile, scalatuningfilepath);
        getparam(configfile, &exsid);
        getparam(configfile, &darkmode);
        getparam(configfile, &xpos);
        getparam(configfile, &ypos);
        getparam(configfile, &xsize);
        getparam(configfile, &ysize);
    }
    std::fclose(configfile);
  }
}

void saveconfig()
{
  char filename[MAX_PATHNAME];

  getFilename(filename, true);

  FILE *configfile = std::fopen(filename, "wt");
  if (configfile)
  {
    std::fprintf(configfile,
                 ";------------------------------------------------------------------------------\n"
                 ";LoadTracker config file. Rows starting with ; are comments.                   \n"
                 ";Hex parameters are to be preceded with $ and decimal parameters with nothing. \n"
                 ";------------------------------------------------------------------------------\n"
                 "\n"
                 ";config version\n%d\n\n"
                 ";reSIDfp mixing rate (in Hz)\n%d\n\n"
                 ";reSIDfp model (0 = 6581, 1 = 8580)\n%d\n\n"
                 ";Number of SIDs (1, 2, default 1)\n%d\n\n"
                 ";Timing mode (0 = PAL, 1 = NTSC)\n%d\n\n"
                 ";Packer/relocator fileformat (0 = SID, 1 = PRG, 2 = BIN)\n%d\n\n"
                 ";Packer/relocator player address\n$%04x\n\n"
                 ";Packer/relocator zeropage baseaddress\n$%02x\n\n"
                 ";Packer/relocator player type (0 = standard ... 3 = minimal)\n%d\n\n"
                 ";Key entry mode (0 = Protracker, 1 = DMC, 2 = Janko)\n%d\n\n"
                 ";Pattern default size (default = 32 / $20)\n%d\n\n"
                 ";Pattern highlight step size\n%d\n\n"
                 ";Speed multiplier (0 = 25Hz, 1 = 1X, 2 = 2X etc.)\n%d\n\n"
                 ";Hardrestart ADSR parameter\n$%04x\n\n"
                 ";reSIDfp resampling mode (0 = interpolation, 1 = resampling)\n%d\n\n"
                 ";Pattern display mode (0 = decimal, 1 = hex, 2 = decimal w/dots, 3 = hex w/dots)\n%d\n\n"
                 ";SID baseaddress\n$%04x\n\n"
                 ";2nd SID baseaddress\n$%04x\n\n"
                 ";Panning in stereo mode (0.00 - 1.00; 0 = channels reversed, 0.5 = mono, 1 = seperated)\n%.2f\n\n"
                 ";Finevibrato mode (0 = off, 1 = on)\n%d\n\n"
                 ";Pulseskipping (0 = off, 1 = on)\n%d\n\n"
                 ";Realtime effect skipping (0 = off, 1 = on)\n%d\n\n"
                 ";Random reSIDfp write delay in cycles (0 = off)\n%d\n\n"
                 ";Custom SID clock cycles per second (0 = use PAL/NTSC default)\n%d\n\n"
                 ";Window type (0 = window, 1 = fullscreen)\n%d\n\n"
                 ";Base pitch of A-4 in Hz (0 = use default frequencytable)\n%f\n\n"
                 ";Filter curve (0.0 (dark) to 1.0 (bright))\n%f\n\n"
                 ";Combined waveforms strength (0 weak, 1 average, 2 strong)\n%d\n\n"
                 ";Equal divisions per octave (12 = default, 8.2019143 = Bohlen-Pierce)\n%f\n\n"
                 ";Special note names (2 chars for every note in an octave/cycle)\n\"%s\"\n\n"
                 ";Path to a Scala tuning file .scl\n\"%s\"\n\n"
                 ";Use exSID (0 = off, 1 = on)\n%d\n\n"
                 ";Use dark mode (0 = off, 1 = on)\n%d\n\n"
                 ";Window X position\n%d\n\n"
                 ";Window Y position\n%d\n\n"
                 ";Window X size\n%d\n\n"
                 ";Window Y size\n%d\n\n",
        CFG_VERSION,
        mr,
        sidmodel,
        numsids,
        ntsc,
        fileformat,
        playeradr,
        zeropageadr,
        playerversion,
        keypreset,
        defaultpatternlength,
        stepsize,
        multiplier,
        adparam,
        interpolate,
        patterndispmode,
        sidaddress,
        sid2address,
        panning,
        finevibrato,
        optimizepulse,
        optimizerealtime,
        residdelay,
        customclockrate,
        win_fullscreen,
        basepitch,
        filterbias,
        combwaves,
        equaldivisionsperoctave,
        specialnotenames,
        scalatuningfilepath,
        exsid,
        darkmode,
        xpos,
        ypos,
        xsize,
        ysize);
    std::fclose(configfile);
  }
}

void getparam(FILE *handle, unsigned *value)
{
  char configbuf[MAX_PATHNAME];
  for (;;)
  {
    if (std::feof(handle)) return;
    std::fgets(configbuf, MAX_PATHNAME, handle);
    if ((configbuf[0]) && (configbuf[0] != ';') && (configbuf[0] != ' ') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
  }

  char *configptr = configbuf;
  if (*configptr == '$')
  {
    *value = 0;
    configptr++;
    for (;;)
    {
      char c = std::tolower(*configptr++);
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
      char c = std::tolower(*configptr++);
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
  char configbuf[MAX_PATHNAME];
  for (;;)
  {
    if (std::feof(handle)) return;
    std::fgets(configbuf, MAX_PATHNAME, handle);
    if ((configbuf[0]) && (configbuf[0] != ';') && (configbuf[0] != ' ') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
  }

  char *configptr = configbuf;
  *value = 0.0f;
  std::sscanf(configptr, "%f", value);
}

void getstringparam(FILE *handle, char *value)
{
  char configbuf[MAX_PATHNAME];
  for (;;)
  {
    if (std::feof(handle)) return;
    std::fgets(configbuf, MAX_PATHNAME, handle);
    if ((configbuf[0]) && (configbuf[0] != ';') && (configbuf[0] != ' ') && (configbuf[0] != 13) && (configbuf[0] != 10)) break;
  }

  char *configptr = configbuf;

    // FIXME prevent overflows
  char tmpbuf[MAX_PATHNAME];
  std::sscanf(configptr, "%s", tmpbuf);
  // Strip quotes
  size_t len = std::strlen(tmpbuf);
  std::memcpy(value, tmpbuf+1, len-1);
  value[len-2] = '\0';
}

// TODO getboolparam
