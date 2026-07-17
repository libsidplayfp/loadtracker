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
// song data model, loading/saving/conversion
// =============================================================================

#define SONG_C

#include "song.h"

#include "common.h"
#include "configfile.h"
#include "instr.h"
#include "loadtrk.h"
#include "order.h"
#include "pattern.h"
#include "play.h"
#include "reloc.h"
#include "table.h"

#include "bme_end.h"

#include <cstring>
#include <cstdio>

Song song;

int pattlen[MAX_PATT];
int highestusedpattern;
int highestusedinstr;

int determinechannels(FILE* handle);

bool savesong()
{
  const char ident[] = {'G', 'T', 'S', '5'};

  int maxChns = getMaxChannels();

  if (std::strlen(songfilename) < MAX_FILENAME-4)
  {
    int extfound = 0;
    for (int c = std::strlen(songfilename)-1; c >= 0; c--)
    {
      if (songfilename[c] == '.') extfound = 1;
    }
    if (!extfound) std::strcat(songfilename, ".sng");
  }
  FILE *handle = std::fopen(songfilename, "wb");
  if (handle)
  {
    std::fwrite(ident, 4, 1, handle);

    // Determine amount of patterns & instruments
    countpatternlengths();
    for (int c = 1; c < MAX_INSTR; c++)
    {
      if ((song.instr[c].ad) || (song.instr[c].sr) || (song.instr[c].ptr[0]) || (song.instr[c].ptr[1]) ||
          (song.instr[c].ptr[2]) || (song.instr[c].vibdelay) || (song.instr[c].ptr[3]))
      {
        if (c > highestusedinstr) highestusedinstr = c;
      }
    }

    // Write infotexts
    std::fwrite(song.title, sizeof song.title, 1, handle);
    std::fwrite(song.author, sizeof song.author, 1, handle);
    std::fwrite(song.released, sizeof song.released, 1, handle);

    // Determine amount of songs to be saved
    int c = MAX_SONGS - 1;
    for (;;)
    {
     if ((song.len[c][0]) &&
         (song.len[c][1]) &&
         (song.len[c][2])) break;
     if (c == 0) break;
     c--;
    }
    int amount = c + 1;

    fwrite8(handle, amount);
    // Write song.orderlists
    for (int d = 0; d < amount; d++)
    {
      for (int c = 0; c < maxChns; c++)
      {
         int length = song.len[d][c]+1;
         fwrite8(handle, length);
         int writebytes = length;
         writebytes++;
         std::fwrite(song.order[d][c], writebytes, 1, handle);
      }
    }
    // Write amount of instruments
    fwrite8(handle, highestusedinstr);
    // Write instruments
    for (int c = 1; c <= highestusedinstr; c++)
    {
      fwrite8(handle, song.instr[c].ad);
      fwrite8(handle, song.instr[c].sr);
      fwrite8(handle, song.instr[c].ptr[WTBL]);
      fwrite8(handle, song.instr[c].ptr[PTBL]);
      fwrite8(handle, song.instr[c].ptr[FTBL]);
      fwrite8(handle, song.instr[c].ptr[STBL]);
      fwrite8(handle, song.instr[c].vibdelay);
      fwrite8(handle, song.instr[c].gatetimer);
      fwrite8(handle, song.instr[c].firstwave);
      std::fwrite(&song.instr[c].name, MAX_INSTRNAMELEN, 1, handle);
    }
    // Write tables
    for (int c = 0; c < MAX_TABLES; c++)
    {
      int writebytes = gettablelen(c);
      fwrite8(handle, writebytes);
      std::fwrite(song.ltable[c], writebytes, 1, handle);
      std::fwrite(song.rtable[c], writebytes, 1, handle);
    }
    // Write patterns
    amount = highestusedpattern + 1;
    fwrite8(handle, amount);
    for (c = 0; c < amount; c++)
    {
      int length = pattlen[c]+1;
      fwrite8(handle, length);
      fwrite(song.pattern[c], length * 4, 1, handle);
    }
    std::fclose(handle);
    std::strcpy(loadedsongfilename, songfilename);
    return true;
  }
  return false;
}

bool saveinstrument()
{
  const char ident[] = {'G', 'T', 'I', '5'};

  if (std::strlen(instrfilename) < MAX_FILENAME-4)
  {
    int extfound = 0;
    for (int c = std::strlen(instrfilename)-1; c >= 0; c--)
    {
      if (instrfilename[c] == '.') extfound = 1;
    }
    if (!extfound) std::strcat(instrfilename, ".ins");
  }

  FILE *handle = std::fopen(instrfilename, "wb");
  if (handle)
  {
    std::fwrite(ident, 4, 1, handle);

    // Write instrument
    fwrite8(handle, song.instr[einum].ad);
    fwrite8(handle, song.instr[einum].sr);
    fwrite8(handle, song.instr[einum].ptr[WTBL]);
    fwrite8(handle, song.instr[einum].ptr[PTBL]);
    fwrite8(handle, song.instr[einum].ptr[FTBL]);
    fwrite8(handle, song.instr[einum].ptr[STBL]);
    fwrite8(handle, song.instr[einum].vibdelay);
    fwrite8(handle, song.instr[einum].gatetimer);
    fwrite8(handle, song.instr[einum].firstwave);
    fwrite(&song.instr[einum].name, MAX_INSTRNAMELEN, 1, handle);
    for (int c = 0; c < MAX_TABLES; c++)
    {
      if (song.instr[einum].ptr[c])
      {
        int pos = song.instr[einum].ptr[c] - 1;
        int len = gettablepartlen(c, pos);
        fwrite8(handle, len);
        std::fwrite(&song.ltable[c][pos], len, 1, handle);
        std::fwrite(&song.rtable[c][pos], len, 1, handle);
      }
      else fwrite8(handle, 0);
    }
    std::fclose(handle);
    return true;
  }
  return false;
}

void loadsong()
{
  int maxChns = getMaxChannels();
  int channelstoload = maxChns;

  FILE *handle = std::fopen(songfilename, "rb");

  int ok = 0;
  char ident[4];

  if (handle)
  {
    std::fread(ident, 4, 1, handle);
    if ((!std::memcmp(ident, "GTS3", 4)) || (!std::memcmp(ident, "GTS4", 4)) || (!std::memcmp(ident, "GTS5", 4)))
    {
      int length;
      int amount;
      int loadsize;
      clearsong(true, true, true, true, true);
      ok = 1;

      // Read infotexts
      std::fread(song.title, sizeof song.title, 1, handle);
      std::fread(song.author, sizeof song.author, 1, handle);
      std::fread(song.released, sizeof song.released, 1, handle);

      // Read song.orderlists
      channelstoload = determinechannels(handle);
      amount = fread8(handle);
      for (int d = 0; d < amount; d++)
      {
        for (int c = 0; c < channelstoload; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          std::fread(song.order[d][c], loadsize, 1, handle);
        }
      }
      // Read instruments
      amount = fread8(handle);
      for (int c = 1; c <= amount; c++)
      {
        song.instr[c].ad = fread8(handle);
        song.instr[c].sr = fread8(handle);
        song.instr[c].ptr[WTBL] = fread8(handle);
        song.instr[c].ptr[PTBL] = fread8(handle);
        song.instr[c].ptr[FTBL] = fread8(handle);
        song.instr[c].ptr[STBL] = fread8(handle);
        song.instr[c].vibdelay = fread8(handle);
        song.instr[c].gatetimer = fread8(handle);
        song.instr[c].firstwave = fread8(handle);
        std::fread(&song.instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (int c = 0; c < MAX_TABLES; c++)
      {
        loadsize = fread8(handle);
        std::fread(song.ltable[c], loadsize, 1, handle);
        std::fread(song.rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (int c = 0; c < amount; c++)
      {
        length = fread8(handle) * 4;
        std::fread(song.pattern[c], length, 1, handle);
      }
      countpatternlengths();
      songchange();
    }

    // Goattracker v2.xx (3-table) import
    if (!std::memcmp(ident, "GTS2", 4))
    {
      int length;
      int amount;
      int loadsize;
      clearsong(true, true, true, true, true);
      ok = 1;

      // Read infotexts
      std::fread(song.title, sizeof song.title, 1, handle);
      std::fread(song.author, sizeof song.author, 1, handle);
      std::fread(song.released, sizeof song.released, 1, handle);

      // Read song.orderlists
      channelstoload = determinechannels(handle);
      amount = fread8(handle);
      for (int d = 0; d < amount; d++)
      {
        for (int c = 0; c < channelstoload; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          std::fread(song.order[d][c], loadsize, 1, handle);
        }
      }
      // Read instruments
      amount = fread8(handle);
      for (int c = 1; c <= amount; c++)
      {
        song.instr[c].ad = fread8(handle);
        song.instr[c].sr = fread8(handle);
        song.instr[c].ptr[WTBL] = fread8(handle);
        song.instr[c].ptr[PTBL] = fread8(handle);
        song.instr[c].ptr[FTBL] = fread8(handle);
        song.instr[c].vibdelay = fread8(handle);
        song.instr[c].ptr[STBL] = makespeedtable(fread8(handle), finevibrato, false) + 1;
        song.instr[c].gatetimer = fread8(handle);
        song.instr[c].firstwave = fread8(handle);
        std::fread(&song.instr[c].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (int c = 0; c < MAX_TABLES-1; c++)
      {
        loadsize = fread8(handle);
        std::fread(song.ltable[c], loadsize, 1, handle);
        std::fread(song.rtable[c], loadsize, 1, handle);
      }
      // Read patterns
      amount = fread8(handle);
      for (int c = 0; c < amount; c++)
      {
        length = fread8(handle) * 4;
        std::fread(song.pattern[c], length, 1, handle);

        // Convert speedtable-requiring commands
        for (int d = 0; d < length; d++)
        {
          switch (song.pattern[c][d*4+2])
          {
            case CMD_FUNKTEMPO:
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_FUNKTEMPO, false) + 1;
            break;

            case CMD_PORTAUP:
            case CMD_PORTADOWN:
            case CMD_TONEPORTA:
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_PORTAMENTO, false) + 1;
            break;

            case CMD_VIBRATO:
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], finevibrato, false) + 1;
            break;
          }
        }
      }
      countpatternlengths();
      songchange();
    }
    // Goattracker 1.xx import
    if (!std::memcmp(ident, "GTS!", 4))
    {
      int length;
      int amount;
      int loadsize;
      int fw = 0;
      int fp = 0;
      int ff = 0;
      int fi = 0;
      int numfilter = 0;
      unsigned char filtertable[256];
      unsigned char filtermap[64];
      int arpmap[32][256];
      unsigned char pulse[32], pulseadd[32], pulselimitlow[32], pulselimithigh[32];
      int filterjumppos[64];

      clearsong(true, true, true, true, true);
      ok = 1;

      // Read infotexts
      std::fread(song.title, sizeof song.title, 1, handle);
      std::fread(song.author, sizeof song.author, 1, handle);
      std::fread(song.released, sizeof song.released, 1, handle);

      // Read song.orderlists
      channelstoload = determinechannels(handle);
      amount = fread8(handle);
      for (int d = 0; d < amount; d++)
      {
        for (int c = 0; c < channelstoload; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          std::fread(song.order[d][c], loadsize, 1, handle);
        }
      }

      // Convert instruments
      for (int c = 1; c < 32; c++)
      {
        unsigned char wavelen;

        song.instr[c].ad = fread8(handle);
        song.instr[c].sr = fread8(handle);
        pulse[c] = fread8(handle);
        pulseadd[c] = fread8(handle);
        pulselimitlow[c] = fread8(handle);
        pulselimithigh[c] = fread8(handle);
        song.instr[c].ptr[FTBL] = fread8(handle); // Will be converted later
        if (song.instr[c].ptr[FTBL] > numfilter) numfilter = song.instr[c].ptr[FTBL];
        if (pulse[c] & 1) song.instr[c].gatetimer |= 0x80; // "No hardrestart" flag
        pulse[c] &= 0xfe;
        wavelen = fread8(handle)/2;
        std::fread(&song.instr[c].name, MAX_INSTRNAMELEN, 1, handle);
        song.instr[c].ptr[WTBL] = fw+1;

        // Convert wavetable
        for (int d = 0; d < wavelen; d++)
        {
          if (fw < MAX_TABLELEN)
          {
            song.ltable[WTBL][fw] = fread8(handle);
            song.rtable[WTBL][fw] = fread8(handle);
            if (song.ltable[WTBL][fw] == 0xff)
              if (song.rtable[WTBL][fw]) song.rtable[WTBL][fw] += song.instr[c].ptr[WTBL]-1;
            if ((song.ltable[WTBL][fw] >= 0x8) && (song.ltable[WTBL][fw] <= 0xf))
              song.ltable[WTBL][fw] |= 0xe0;
            fw++;
          }
          else
          {
            fread8(handle);
            fread8(handle);
          }
        }

        // Remove empty wavetable afterwards
        if ((wavelen == 2) && (!song.ltable[WTBL][fw-2]) && (!song.rtable[WTBL][fw-2]))
        {
          song.instr[c].ptr[WTBL] = 0;
          fw -= 2;
          song.ltable[WTBL][fw] = 0;
          song.rtable[WTBL][fw] = 0;
          song.ltable[WTBL][fw+1] = 0;
          song.rtable[WTBL][fw+1] = 0;
        }

        // Convert pulsetable
        if (pulse[c])
        {
          int pulsetime, pulsedist, hlpos;

          // Check for duplicate pulse settings
          for (int d = 1; d < c; d++)
          {
            if ((pulse[d] == pulse[c]) && (pulseadd[d] == pulseadd[c]) && (pulselimitlow[d] == pulselimitlow[c]) &&
                (pulselimithigh[d] == pulselimithigh[c]))
            {
              song.instr[c].ptr[PTBL] = song.instr[d].ptr[PTBL];
              goto PULSEDONE;
            }
          }

          // Initial pulse setting
          if (fp >= MAX_TABLELEN) goto PULSEDONE;
          song.instr[c].ptr[PTBL] = fp+1;
          song.ltable[PTBL][fp] = 0x80 | (pulse[c] >> 4);
          song.rtable[PTBL][fp] = pulse[c] << 4;
          fp++;

          // Pulse modulation
          if (pulseadd[c])
          {
            int startpulse = pulse[c]*16;
            int currentpulse = pulse[c]*16;
            // Phase 1: From startpos to high limit
            pulsedist = pulselimithigh[c]*16 - currentpulse;
            if (pulsedist > 0)
            {
              pulsetime = pulsedist/pulseadd[c];
              currentpulse += pulsetime*pulseadd[c];
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                song.ltable[PTBL][fp] = acttime;
                song.rtable[PTBL][fp] = pulseadd[c] / 2;
                fp++;
                pulsetime -= acttime;
              }
            }

            hlpos = fp;
            // Phase 2: from high limit to low limit
            pulsedist = currentpulse - pulselimitlow[c]*16;
            if (pulsedist > 0)
            {
              pulsetime = pulsedist/pulseadd[c];
              currentpulse -= pulsetime*pulseadd[c];
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                song.ltable[PTBL][fp] = acttime;
                song.rtable[PTBL][fp] = -(pulseadd[c] / 2);
                fp++;
                pulsetime -= acttime;
              }
            }

            // Phase 3: from low limit back to startpos/high limit
            if ((startpulse < pulselimithigh[c]*16) && (startpulse > currentpulse))
            {
              pulsedist = startpulse - currentpulse;
              if (pulsedist > 0)
              {
                pulsetime = pulsedist/pulseadd[c];
                while (pulsetime)
                {
                  int acttime = pulsetime;
                  if (acttime > 127) acttime = 127;
                  if (fp >= MAX_TABLELEN) goto PULSEDONE;
                  song.ltable[PTBL][fp] = acttime;
                  song.rtable[PTBL][fp] = pulseadd[c] / 2;
                  fp++;
                  pulsetime -= acttime;
                }
              }
              // Pulse jump back to beginning
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              song.ltable[PTBL][fp] = 0xff;
              song.rtable[PTBL][fp] = song.instr[c].ptr[PTBL] + 1;
              fp++;
            }
            else
            {
              pulsedist = pulselimithigh[c]*16 - currentpulse;
              if (pulsedist > 0)
              {
                pulsetime = pulsedist/pulseadd[c];
                while (pulsetime)
                {
                  int acttime = pulsetime;
                  if (acttime > 127) acttime = 127;
                  if (fp >= MAX_TABLELEN) goto PULSEDONE;
                  song.ltable[PTBL][fp] = acttime;
                  song.rtable[PTBL][fp] = pulseadd[c] / 2;
                  fp++;
                  pulsetime -= acttime;
                }
              }
              // Pulse jump back to beginning
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              song.ltable[PTBL][fp] = 0xff;
              song.rtable[PTBL][fp] = hlpos + 1;
              fp++;
            }
          }
          else
          {
            // Pulse stopped
            if (fp >= MAX_TABLELEN) goto PULSEDONE;
            song.ltable[PTBL][fp] = 0xff;
            song.rtable[PTBL][fp] = 0;
            fp++;
          }
          PULSEDONE: {}
        }
      }
      // Convert patterns
      amount = fread8(handle);
      for (int c = 0; c < amount; c++)
      {
        length = fread8(handle);
        for (int d = 0; d < length/3; d++)
        {
          unsigned char note, cmd, data, instr;
          note = fread8(handle);
          cmd = fread8(handle);
          data = fread8(handle);
          instr = cmd >> 3;
          cmd &= 7;

          switch(note)
          {
            default:
            note += FIRSTNOTE;
            if (note > LASTNOTE) note = REST;
            break;

            case OLDKEYOFF:
            note = KEYOFF;
            break;

            case OLDREST:
            note = REST;
            break;

            case ENDPATT:
            break;
          }
          switch(cmd)
          {
            case 5:
            cmd = CMD_SETFILTERPTR;
            if (data > numfilter) numfilter = data;
            break;

            case 7:
            if (data < 0xf0)
              cmd = CMD_SETTEMPO;
            else
            {
              cmd = CMD_SETMASTERVOL;
              data &= 0x0f;
            }
            break;
          }
          song.pattern[c][d*4] = note;
          song.pattern[c][d*4+1] = instr;
          song.pattern[c][d*4+2] = cmd;
          song.pattern[c][d*4+3] = data;
        }
      }
      countpatternlengths();
      fi = highestusedinstr + 1;
      songchange();

      // Read filtertable
      std::fread(filtertable, 256, 1, handle);

      // Convert filtertable
      for (int c = 0; c < 64; c++)
      {
        filterjumppos[c] = -1;
        filtermap[c] = 0;
        if (filtertable[c*4+3] > numfilter) numfilter = filtertable[c*4+3];
      }

      if (numfilter > 63) numfilter = 63;

      for (int c = 1; c <= numfilter; c++)
      {
        filtermap[c] = ff+1;

        if (filtertable[c*4]|filtertable[c*4+1]|filtertable[c*4+2]|filtertable[c*4+3])
        {
          // Filter set
          if (filtertable[c*4])
          {
            song.ltable[FTBL][ff] = 0x80 + (filtertable[c*4+1] & 0x70);
            song.rtable[FTBL][ff] = filtertable[c*4];
            ff++;
            if (filtertable[c*4+2])
            {
              song.ltable[FTBL][ff] = 0x00;
              song.rtable[FTBL][ff] = filtertable[c*4+2];
              ff++;
            }
          }
          else
          {
            // Filter modulation
            int time = filtertable[c*4+1];

            while (time)
            {
              int acttime = time;
              if (acttime > 127) acttime = 127;
              song.ltable[FTBL][ff] = acttime;
              song.rtable[FTBL][ff] = filtertable[c*4+2];
              ff++;
              time -= acttime;
            }
          }

          // Jump to next step: unnecessary if follows directly
          if (filtertable[c*4+3] != c+1)
          {
            filterjumppos[c] = ff;
            song.ltable[FTBL][ff] = 0xff;
            song.rtable[FTBL][ff] = filtertable[c*4+3]; // Fix the jump later
            ff++;
          }
        }
      }

      // Now fix jumps as the filterstep mapping is known
      for (int c = 1; c <= numfilter; c++)
      {
        if (filterjumppos[c] != -1)
          song.rtable[FTBL][filterjumppos[c]] = filtermap[song.rtable[FTBL][filterjumppos[c]]];
      }

      // Fix filterpointers in instruments
      for (int c = 1; c < 32; c++)
        song.instr[c].ptr[FTBL] = filtermap[song.instr[c].ptr[FTBL]];

      // Now fix pattern commands
      std::memset(arpmap, 0, sizeof arpmap);
      for (int c = 0; c < MAX_PATT; c++)
      {
        unsigned char i = 0;
        for (int d = 0; d <= MAX_PATTROWS; d++)
        {
          if (song.pattern[c][d*4+1]) i = song.pattern[c][d*4+1];

          // Convert portamento & vibrato
          if (song.pattern[c][d*4+2] == CMD_PORTAUP)
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_PORTAMENTO, false) + 1;
          if (song.pattern[c][d*4+2] == CMD_PORTADOWN)
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_PORTAMENTO, false) + 1;
          if (song.pattern[c][d*4+2] == CMD_TONEPORTA)
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_PORTAMENTO, false) + 1;
          if (song.pattern[c][d*4+2] == CMD_VIBRATO)
            song.pattern[c][d*4+3] = makespeedtable(song.pattern[c][d*4+3], MST_NOFINEVIB, false) + 1;

          // Convert filterjump
          if (song.pattern[c][d*4+2] == CMD_SETFILTERPTR)
            song.pattern[c][d*4+3] = filtermap[song.pattern[c][d*4+3]];

          // Convert funktempo
          if ((song.pattern[c][d*4+2] == CMD_SETTEMPO) && (!song.pattern[c][d*4+3]))
          {
            song.pattern[c][d*4+2] = CMD_FUNKTEMPO;
            song.pattern[c][d*4+3] = makespeedtable((filtertable[2] << 4) | (filtertable[3] & 0x0f), MST_FUNKTEMPO, false) + 1;
          }
          // Convert arpeggio
          if ((song.pattern[c][d*4+2] == CMD_DONOTHING) && (song.pattern[c][d*4+3]))
          {
            // Must be in conjunction with a note
            if ((song.pattern[c][d*4] >= FIRSTNOTE) && (song.pattern[c][d*4] <= LASTNOTE))
            {
              unsigned char param = song.pattern[c][d*4+3];
              if (i)
              {
                // Old arpeggio
                if (arpmap[i][param])
                {
                  // As command, or as instrument?
                  if (arpmap[i][param] < 256)
                  {
                    song.pattern[c][d*4+2] = CMD_SETWAVEPTR;
                    song.pattern[c][d*4+3] = arpmap[i][param];
                  }
                  else
                  {
                    song.pattern[c][d*4+1] = arpmap[i][param] - 256;
                    song.pattern[c][d*4+3] = 0;
                  }
                }
                else
                {
                  int e;
                  unsigned char arpstart;
                  unsigned char arploop;

                  // New arpeggio
                  // Copy first the instrument's wavetable up to loop/end point
                  arpstart = fw + 1;
                  if (song.instr[i].ptr[WTBL])
                  {
                    for (e = song.instr[i].ptr[WTBL]-1;; e++)
                    {
                      if (song.ltable[WTBL][e] == 0xff) break;
                      if (fw < MAX_TABLELEN)
                      {
                        song.ltable[WTBL][fw] = song.ltable[WTBL][e];
                        fw++;
                      }
                    }
                  }
                  // Then make the arpeggio
                  arploop = fw + 1;
                  if (fw < MAX_TABLELEN-3)
                  {
                    song.ltable[WTBL][fw] = (param & 0x80) >> 7;
                    song.rtable[WTBL][fw] = (param  & 0x70) >> 4;
                    fw++;
                    song.ltable[WTBL][fw] = (param & 0x80) >> 7;
                    song.rtable[WTBL][fw] = (param & 0xf);
                    fw++;
                    song.ltable[WTBL][fw] = (param & 0x80) >> 7;
                    song.rtable[WTBL][fw] = 0;
                    fw++;
                    song.ltable[WTBL][fw] = 0xff;
                    song.rtable[WTBL][fw] = arploop;
                    fw++;

                    // Create new instrument if possible
                    if (fi < MAX_INSTR)
                    {
                      arpmap[i][param] = fi + 256;
                      song.instr[fi] = song.instr[i];
                      song.instr[fi].ptr[WTBL] = arpstart;
                      // Add arpeggio parameter to new instrument name
                      if (strlen(song.instr[fi].name) < MAX_INSTRNAMELEN-3)
                      {
                        char arpname[8];
                        sprintf(arpname, "0%02X", param&0x7f);
                        strcat(song.instr[fi].name, arpname);
                      }
                      fi++;
                    }
                    else
                    {
                      arpmap[i][param] = arpstart;
                    }
                  }

                  if (arpmap[i][param])
                  {
                    // As command, or as instrument?
                    if (arpmap[i][param] < 256)
                    {
                      song.pattern[c][d*4+2] = CMD_SETWAVEPTR;
                      song.pattern[c][d*4+3] = arpmap[i][param];
                    }
                    else
                    {
                      song.pattern[c][d*4+1] = arpmap[i][param] - 256;
                      song.pattern[c][d*4+3] = 0;
                    }
                  }
                }
              }
            }
            // If arpeggio could not be converted, databyte zero
            if (!song.pattern[c][d*4+2])
              song.pattern[c][d*4+3] = 0;
          }
        }
      }
    }
    std::fclose(handle);
  }
  if (ok)
  {
    std::strcpy(loadedsongfilename, songfilename);

    // Reset table views
    for (int c = 0; c < MAX_TABLES; c++) tables.settableview(c, 0);

    // Convert pulsemodulation speed of < v2.4 songs
    if (ident[3] < '4')
    {
      for (int c = 0; c < MAX_TABLELEN; c++)
      {
        if ((song.ltable[PTBL][c] < 0x80) && (song.rtable[PTBL][c]))
        {
          int speed = ((signed char)song.rtable[PTBL][c]);
          speed <<= 1;
          if (speed > 127) speed = 127;
          if (speed < -128) speed = -128;
          song.rtable[PTBL][c] = speed;
        }
      }
    }

    // Convert old legato/nohr parameters
    if (ident[3] < '5')
    {
        for (int c = 1; c < MAX_INSTR; c++)
        {
            if (song.instr[c].firstwave >= 0x80)
            {
                song.instr[c].gatetimer |= 0x80;
                song.instr[c].firstwave &= 0x7f;
            }
            if (!song.instr[c].firstwave) song.instr[c].gatetimer |= 0x40;
        }
    }

    // If was a mono song, create empty orderlists for channels 4-6
    if (channelstoload < MAX_CHN)
    {
      int emptypatt = MAX_PATT-1;

      findusedpatterns();
      for (int c = 0; c < MAX_PATT; c++)
      {
        if (!pattused[c])
        {
          int d;
          int ok = 1;
          for (d = 0; d < pattlen[c]; d++)
          {
            if ((song.pattern[c][d*4] != REST) || (song.pattern[c][d*4+1] != 0x00) ||
                (song.pattern[c][d*4+2] != 0x00) || (song.pattern[c][d*4+3] != 0x00))
              ok = 0;
          }

          if (ok)
          {
            emptypatt = c;
            break;
          }
        }
      }

      for (int c = 0; c < MAX_SONGS; c++)
      {
        if (song.len[c][0])
        {
           for (int d = channelstoload; d < MAX_CHN; d++)
           {
             song.order[c][d][0] = emptypatt;
             song.order[c][d][1] = 0xff;
             song.order[c][d][2] = 0x00;
             song.len[c][d] = 1;
           }
        }
      }
      songchange();
    }
  }
}

void loadinstrument()
{
  FILE *handle = std::fopen(instrfilename, "rb");
  if (handle)
  {
    char ident[4];
    int pulsestart = -1;
    int pulseend = -1;

    stopsong();
    std::fread(ident, 4, 1, handle);

    if ((!std::memcmp(ident, "GTI3", 4)) || (!std::memcmp(ident, "GTI4", 4)) || (!std::memcmp(ident, "GTI5", 4)))
    {
      unsigned char optr[4];

      song.instr[einum].ad = fread8(handle);
      song.instr[einum].sr = fread8(handle);
      optr[0] = fread8(handle);
      optr[1] = fread8(handle);
      optr[2] = fread8(handle);
      optr[3] = fread8(handle);
      song.instr[einum].vibdelay = fread8(handle);
      song.instr[einum].gatetimer = fread8(handle);
      song.instr[einum].firstwave = fread8(handle);
      std::fread(&song.instr[einum].name, MAX_INSTRNAMELEN, 1, handle);

      // Erase old tabledata
      deleteinstrtable(einum);

      // Load new tabledata
      for (int c = 0; c < MAX_TABLES; c++)
      {
        int start = gettablelen(c);
        int len = fread8(handle);

        if (len)
        {
          int d;
          for (d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
            song.ltable[c][d] = fread8(handle);
          while (d < start+len)
          {
            fread8(handle);
            d++;
          }
          for (d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
            song.rtable[c][d] = fread8(handle);
          while (d < start+len)
          {
            fread8(handle);
            d++;
          }
          if (c != STBL)
          {
            for (int d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
            {
              if (song.ltable[c][d] == 0xff)
              {
                if (song.rtable[c][d])
                  song.rtable[c][d] = song.rtable[c][d] - optr[c] + start + 1;
              }
            }
          }
          if (c == PTBL)
          {
            pulsestart = start;
            pulseend = start + len;
          }
          song.instr[einum].ptr[c] = start + 1;
        }
        else song.instr[einum].ptr[c] = 0;
      }
    }

    // Goattracker v2.xx (3-table) import
    if (!std::memcmp(ident, "GTI2", 4))
    {
      unsigned char optr[3];

      song.instr[einum].ad = fread8(handle);
      song.instr[einum].sr = fread8(handle);
      optr[0] = fread8(handle);
      optr[1] = fread8(handle);
      optr[2] = fread8(handle);
      song.instr[einum].vibdelay = fread8(handle);
      song.instr[einum].ptr[STBL] = makespeedtable(fread8(handle), finevibrato, false) + 1;
      song.instr[einum].gatetimer = fread8(handle);
      song.instr[einum].firstwave = fread8(handle);
      std::fread(&song.instr[einum].name, MAX_INSTRNAMELEN, 1, handle);

      // Erase old tabledata
      deleteinstrtable(einum);

      // Load new tabledata
      for (int c = 0; c < MAX_TABLES-1; c++)
      {
        int start = gettablelen(c);
        int len = fread8(handle);

        if (len)
        {
          int d;
          for (d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
            song.ltable[c][d] = fread8(handle);
          while (d < start+len)
          {
            fread8(handle);
            d++;
          }
          for (d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
            song.rtable[c][d] = fread8(handle);
          while (d < start+len)
          {
            fread8(handle);
            d++;
          }
          for (int d = start; (d < start+len) && (d < MAX_TABLELEN); d++)
          {
            if (song.ltable[c][d] == 0xff)
            {
              if (song.rtable[c][d])
                song.rtable[c][d] = song.rtable[c][d] - optr[c] + start + 1;
            }
          }
          if (c == PTBL)
          {
            pulsestart = start;
            pulseend = start + len;
          }
          song.instr[einum].ptr[c] = start + 1;
        }
        else song.instr[einum].ptr[c] = 0;
      }
    }
    // Goattracker 1.xx import
    if (!std::memcmp(ident, "GTI!", 4))
    {

      unsigned char pulse, pulseadd, pulselimitlow, pulselimithigh, wavelen;
      unsigned char filtertemp[4];
      int fw, fp, ff;

      // Erase old tabledata
      deleteinstrtable(einum);

      fw = gettablelen(WTBL);
      fp = gettablelen(PTBL);
      ff = gettablelen(FTBL);

      song.instr[einum].ad = fread8(handle);
      song.instr[einum].sr = fread8(handle);
      if (multiplier)
        song.instr[einum].gatetimer = 2 * multiplier;
      else
        song.instr[einum].gatetimer = 1;
      song.instr[einum].firstwave = 0x9;
      pulse = fread8(handle);
      pulseadd = fread8(handle);
      pulselimitlow = fread8(handle);
      pulselimithigh = fread8(handle);
      song.instr[einum].ptr[FTBL] = fread8(handle) ? ff+1 : 0;
      if (pulse & 1) song.instr[einum].gatetimer |= 0x80; // "No hardrestart" flag
        wavelen = fread8(handle)/2;
      std::fread(&song.instr[einum].name, MAX_INSTRNAMELEN, 1, handle);
      song.instr[einum].ptr[WTBL] = fw+1;

      // Convert wavetable
      for (int d = 0; d < wavelen; d++)
      {
        if (fw < MAX_TABLELEN)
        {
          song.ltable[WTBL][fw] = fread8(handle);
          song.rtable[WTBL][fw] = fread8(handle);
          if (song.ltable[WTBL][fw] == 0xff)
            if (song.rtable[WTBL][fw]) song.rtable[WTBL][fw] += song.instr[einum].ptr[WTBL]-1;
          fw++;
        }
        else
        {
          fread8(handle);
          fread8(handle);
        }
      }

      // Remove empty wavetable afterwards
      if ((wavelen == 2) && (!song.ltable[WTBL][fw-2]) && (!song.rtable[WTBL][fw-2]))
      {
        song.instr[einum].ptr[WTBL] = 0;
        fw -= 2;
        song.ltable[WTBL][fw] = 0;
        song.rtable[WTBL][fw] = 0;
        song.ltable[WTBL][fw+1] = 0;
        song.rtable[WTBL][fw+1] = 0;
      }

      // Convert pulsetable
      pulse &= 0xfe;
      if (pulse)
      {
        // Initial pulse setting
        if (fp >= MAX_TABLELEN) goto PULSEDONE;
        pulsestart = fp;
        song.instr[einum].ptr[PTBL] = fp+1;
        song.ltable[PTBL][fp] = 0x80 | (pulse >> 4);
        song.rtable[PTBL][fp] = pulse << 4;
        fp++;

        // Pulse modulation
        if (pulseadd)
        {
          int startpulse = pulse*16;
          int currentpulse = pulse*16;
          // Phase 1: From startpos to high limit
          int pulsedist = pulselimithigh*16 - currentpulse;
          if (pulsedist > 0)
          {
            int pulsetime = pulsedist/pulseadd;
            currentpulse += pulsetime*pulseadd;
            while (pulsetime)
            {
              int acttime = pulsetime;
              if (acttime > 127) acttime = 127;
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              song.ltable[PTBL][fp] = acttime;
              song.rtable[PTBL][fp] = pulseadd / 2;
              fp++;
              pulsetime -= acttime;
            }
          }

          int hlpos = fp;
          // Phase 2: from high limit to low limit
          pulsedist = currentpulse - pulselimitlow*16;
          if (pulsedist > 0)
          {
            int pulsetime = pulsedist/pulseadd;
            currentpulse -= pulsetime*pulseadd;
            while (pulsetime)
            {
              int acttime = pulsetime;
              if (acttime > 127) acttime = 127;
              if (fp >= MAX_TABLELEN) goto PULSEDONE;
              song.ltable[PTBL][fp] = acttime;
              song.rtable[PTBL][fp] = -(pulseadd / 2);
              fp++;
              pulsetime -= acttime;
            }
          }

          // Phase 3: from low limit back to startpos/high limit
          if ((startpulse < pulselimithigh*16) && (startpulse > currentpulse))
          {
            pulsedist = startpulse - currentpulse;
            if (pulsedist > 0)
            {
              int pulsetime = pulsedist/pulseadd;
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                song.ltable[PTBL][fp] = acttime;
                song.rtable[PTBL][fp] = pulseadd / 2;
                fp++;
                pulsetime -= acttime;
              }
            }
            // Pulse jump back to beginning
            if (fp >= MAX_TABLELEN) goto PULSEDONE;
            song.ltable[PTBL][fp] = 0xff;
            song.rtable[PTBL][fp] = song.instr[einum].ptr[PTBL] + 1;
            fp++;
          }
          else
          {
            pulsedist = pulselimithigh*16 - currentpulse;
            if (pulsedist > 0)
            {
              int pulsetime = pulsedist/pulseadd;
              while (pulsetime)
              {
                int acttime = pulsetime;
                if (acttime > 127) acttime = 127;
                if (fp >= MAX_TABLELEN) goto PULSEDONE;
                song.ltable[PTBL][fp] = acttime;
                song.rtable[PTBL][fp] = pulseadd / 2;
                fp++;
                pulsetime -= acttime;
              }
            }
            // Pulse jump back to beginning
            if (fp >= MAX_TABLELEN) goto PULSEDONE;
            song.ltable[PTBL][fp] = 0xff;
            song.rtable[PTBL][fp] = hlpos + 1;
            fp++;
          }
        }
        else
        {
          // Pulse stopped
          if (fp >= MAX_TABLELEN) goto PULSEDONE;
          song.ltable[PTBL][fp] = 0xff;
          song.rtable[PTBL][fp] = 0;
          fp++;
        }
PULSEDONE:
        pulseend = fp;
      }

      // Convert filter (if any)
      if ((song.instr[einum].ptr[FTBL]) && (ff < MAX_TABLELEN-2))
      {
        std::fread(filtertemp, sizeof filtertemp, 1, handle);
        // Filter set
        if (filtertemp[0])
        {
          song.ltable[FTBL][ff] = 0x80 + (filtertemp[1] & 0x70);
          song.rtable[FTBL][ff] = filtertemp[0];
          ff++;
          if (filtertemp[2])
          {
            song.ltable[FTBL][ff] = 0x00;
            song.rtable[FTBL][ff] = filtertemp[2];
            ff++;
          }
        }
        else
        {
          // Filter modulation
          int time = filtertemp[1];

          while (time)
          {
            int acttime = time;
            if (acttime > 127) acttime = 127;
            song.ltable[FTBL][ff] = acttime;
            song.rtable[FTBL][ff] = filtertemp[2];
            ff++;
            time -= acttime;
          }
        }

        // Jump to next step: always end the filter
        song.ltable[FTBL][ff] = 0xff;
        song.rtable[FTBL][ff] = 0;
        ff++;
      }
    }

    std::fclose(handle);

    // Convert pulsemodulation speed of < v2.4 instruments
    if ((ident[3] < '4') && (pulsestart != -1))
    {
      for (int c = pulsestart; (c < pulseend) && (c < MAX_TABLELEN); c++)
      {
        if ((song.ltable[PTBL][c] < 0x80) && (song.rtable[PTBL][c]))
        {
          int speed = ((signed char)song.rtable[PTBL][c]);
          speed <<= 1;
          if (speed > 127) speed = 127;
          if (speed < -128) speed = -128;
          song.rtable[PTBL][c] = speed;
        }
      }
    }
    // Convert old legato/nohr parameters
    if (ident[3] < '5')
    {
      if (song.instr[einum].firstwave >= 0x80)
      {
        song.instr[einum].firstwave &= 0x7f;
        song.instr[einum].gatetimer |= 0x80;
      }
      if (!song.instr[einum].firstwave) song.instr[einum].gatetimer |= 0x40;
    }
  }
}

void clearsong(bool cs, bool cp, bool ci, bool ct, bool cn)
{
  int maxChns = getMaxChannels();

  if (!(cs | cp | ci | ct | cn)) return;

  stopsong();

  masterfader = 0x0f;
  epmarkchn = -1;
  tables.clear();
  esmarkchn = -1;
  followplay = false;

  for (int c = 0; c < maxChns; c++)
  {
    chn[c].mute = 0;
    if (multiplier)
      chn[c].tempo = multiplier*6-1;
    else
      chn[c].tempo = 6-1;
    chn[c].pattptr = 0;
    if (cs)
    {
      std::memset(loadedsongfilename, 0, sizeof loadedsongfilename);
      for (int d = 0; d < MAX_SONGS; d++)
      {
        std::memset(&song.order[d][c][0], 0, MAX_SONGLEN+2);
        if (!d)
        {
          song.order[d][c][0] = c;
          song.order[d][c][1] = LOOPSONG;
        }
        else
        {
          song.order[d][c][0] = LOOPSONG;
        }
      }
      epnum[c] = song.order[0][c][0];
      espos[c] = 0;
      esend[c] = 0;
    }
  }
  if (cs)
  {
    eseditpos = 0;
    escolumn = 0;
    eschn = 0;
    esnum = 0;
    eppos = 0;
    for (int i=0; i<MAX_CHN; i++)
    {
        epview[i] =-VISIBLEPATTROWS/2;
        esview[i] = 0;
    }
    epcolumn = 0;
    epchn = 0;
  }
  if (cn)
  {
    std::memset(song.title, 0, sizeof song.title);
    std::memset(song.author, 0, sizeof song.author);
    std::memset(song.released, 0, sizeof song.released);
    enpos = 0;
  }
  if (cp)
  {
    std::memset(loadedsongfilename, 0, sizeof loadedsongfilename);
    for (int c = 0; c < MAX_PATT; c++)
      clearpattern(c);
  }
  if (ci)
  {
    clearinstr();
  }
  if (ct == 1)
  {
    for (int c = MAX_TABLES-1; c >= 0; c--)
    {
      std::memset(song.ltable[c], 0, MAX_TABLELEN);
      std::memset(song.rtable[c], 0, MAX_TABLELEN);
      tables.settableview(c, 0);
    }
  }
  countpatternlengths();
}

void countpatternlengths()
{
  int maxChns = getMaxChannels();

  highestusedpattern = 0;
  highestusedinstr = 0;
  for (int c = 0; c < MAX_PATT; c++)
  {
    int d;
    for (d = 0; d <= MAX_PATTROWS; d++)
    {
      if (song.pattern[c][d*4] == ENDPATT) break;
      if ((song.pattern[c][d*4] != REST) || (song.pattern[c][d*4+1]) || (song.pattern[c][d*4+2]) || (song.pattern[c][d*4+3]))
        highestusedpattern = c;
      if (song.pattern[c][d*4+1] > highestusedinstr) highestusedinstr = song.pattern[c][d*4+1];
    }
    pattlen[c] = d;
  }

  for (int e = 0; e < MAX_SONGS; e++)
  {
    for (int c = 0; c < maxChns; c++)
    {
      int d;
      for (d = 0; d < MAX_SONGLEN; d++)
      {
        if (song.order[e][c][d] >= LOOPSONG) break;
        if ((song.order[e][c][d] < REPEAT) &&
            (song.order[e][c][d] > highestusedpattern))
        {
          highestusedpattern = song.order[e][c][d];
        }
      }
      song.len[e][c] = d;
    }
  }
}

void countthispattern()
{
  int c = epnum[epchn];
  int d;
  for (d = 0; d <= MAX_PATTROWS; d++)
  {
    if (song.pattern[c][d*4] == ENDPATT) break;
  }
  pattlen[c] = d;

  int e = esnum;
  c = eschn;
  for (d = 0; d < MAX_SONGLEN; d++)
  {
    if (song.order[e][c][d] >= LOOPSONG) break;
    if (song.order[e][c][d] > highestusedpattern)
    {
      highestusedpattern = song.order[e][c][d];
    }
  }
  song.len[e][c] = d;
}

int insertpattern(int p)
{
  int maxChns = getMaxChannels();

  findusedpatterns();
  if (p >= MAX_PATT-2) return 0;
  if (pattused[MAX_PATT-1]) return 0;
  std::memmove(song.pattern[p+2], song.pattern[p+1], (MAX_PATT-p-2)*(MAX_PATTROWS*4+4));  
  countpatternlengths();

  for (int c = 0; c < MAX_SONGS; c++)
  {
    if ((song.len[c][0]) && (song.len[c][1]) && (song.len[c][2]))
    {
      for (int d = 0; d < maxChns; d++)
      {
        int tmp = song.len[c][d];
        for (int e = 0; e < tmp; e++)
        {
          if ((song.order[c][d][e] < REPEAT) &&
              (song.order[c][d][e] > p) &&
              (song.order[c][d][e] != MAX_PATT-1))
          {
            song.order[c][d][e]++;
          }
        }
      }
    }
  }

  for (int c = 0; c < maxChns; c++)
  {
    if ((epnum[c] > p) && (epnum[c] != MAX_PATT-1)) epnum[c]++;
  }

  return 1;
}

void deletepattern(int p)
{
  int maxChns = getMaxChannels();

  if (p == MAX_PATT-1) return;

  std::memmove(song.pattern[p], song.pattern[p+1], (MAX_PATT-p-1)*(MAX_PATTROWS*4+4));
  clearpattern(MAX_PATT-1);
  countpatternlengths();

  for (int c = 0; c < MAX_SONGS; c++)
  {
    if ((song.len[c][0]) && (song.len[c][1]) && (song.len[c][2]))
    {
      for (int d = 0; d < maxChns; d++)
      {
        int tmp = song.len[c][d];
        for (int e = 0; e < tmp; e++)
        {
          if ((song.order[c][d][e] < REPEAT) &&
              (song.order[c][d][e] > p))
          {
            song.order[c][d][e]--;
          }
        }
      }
    }
  }

  for (int c = 0; c < maxChns; c++)
  {
    if (epnum[c] > p) epnum[c]--;
  }
}

void clearpattern(int p)
{
  std::memset(song.pattern[p], 0, MAX_PATTROWS*4);
  for (int c = 0; c < defaultpatternlength; c++) song.pattern[p][c*4] = REST;
  for (int c = defaultpatternlength; c <= MAX_PATTROWS; c++) song.pattern[p][c*4] = ENDPATT;
}

void findusedpatterns()
{
  int maxChns = getMaxChannels();

  countpatternlengths();
  std::memset(pattused, 0, sizeof pattused);
  for (int c = 0; c < MAX_SONGS; c++)
  {
    if ((song.len[c][0]) && (song.len[c][1]) && (song.len[c][2]))
    {
      for (int d = 0; d < maxChns; d++)
      {
        int tmp = song.len[c][d];
        for (int e = 0; e < tmp; e++)
        {
          if (song.order[c][d][e] < REPEAT)
          {
            pattused[song.order[c][d][e]] = 1;
          }
        }
      }
    }
  }
}

void mergesong()
{
  // Determine amount of patterns & instruments
  countpatternlengths();
  highestusedinstr = 0;
  for (int c = 1; c < MAX_INSTR; c++)
  {
    if ((song.instr[c].ad) || (song.instr[c].sr) || (song.instr[c].ptr[0]) || (song.instr[c].ptr[1]) ||
        (song.instr[c].ptr[2]) || (song.instr[c].vibdelay) || (song.instr[c].ptr[3]))
    {
      if (c > highestusedinstr) highestusedinstr = c;
    }
  }

  // Determine amount of songs
  int c = MAX_SONGS - 1;
  for (;;)
  {
    if ((song.len[c][0]) &&
        (song.len[c][1]) &&
        (song.len[c][2])) break;
    if (c == 0) break;
    c--;
  }

  int pattbase = highestusedpattern + 1;
  int instrbase = highestusedinstr;
  int songbase = c + 1;

  int tablebase[MAX_TABLES];

  for (int c = 0; c < MAX_TABLES; c++)
  {
    tablebase[c] = gettablelen(c);
  }

  char ident[4];

  FILE *handle = std::fopen(songfilename, "rb");

  if (handle)
  {
    std::fread(ident, 4, 1, handle);
    if ((!std::memcmp(ident, "GTS3", 4)) || (!std::memcmp(ident, "GTS4", 4)) || (!std::memcmp(ident, "GTS5", 4)))
    {
      int length;
      int loadsize;

      // Skip infotexts
      std::fseek(handle, sizeof song.title + sizeof song.author + sizeof song.released, SEEK_CUR);

      // Read song.orderlists
      int channelstoload = determinechannels(handle);
      int amount = fread8(handle);
      if (amount + songbase > MAX_SONGS)
        goto ABORT;
      for (int d = 0; d < amount; d++)
      {
        for (int c = 0; c < channelstoload; c++)
        {
          length = fread8(handle);
          loadsize = length;
          loadsize++;
          std::fread(song.order[songbase + d][c], loadsize, 1, handle);
          // Remap patterns
          for (int e = 0; e < loadsize - 1; e++)
          {
            if (song.order[songbase + d][c][e] < REPEAT)
                song.order[songbase + d][c][e] += pattbase;
          }
        }
      }
      // Read instruments
      amount = fread8(handle);
      if (amount + instrbase > MAX_INSTR)
        goto ABORT;
      for (int c = 1; c <= amount; c++)
      {
        song.instr[c + instrbase].ad = fread8(handle);
        song.instr[c + instrbase].sr = fread8(handle);
        song.instr[c + instrbase].ptr[WTBL] = fread8(handle);
        song.instr[c + instrbase].ptr[PTBL] = fread8(handle);
        song.instr[c + instrbase].ptr[FTBL] = fread8(handle);
        song.instr[c + instrbase].ptr[STBL] = fread8(handle);
        if (song.instr[c + instrbase].ptr[WTBL] > 0)
          song.instr[c + instrbase].ptr[WTBL] += tablebase[WTBL];
        if (song.instr[c + instrbase].ptr[PTBL] > 0)
          song.instr[c + instrbase].ptr[PTBL] += tablebase[PTBL];
        if (song.instr[c + instrbase].ptr[FTBL] > 0)
          song.instr[c + instrbase].ptr[FTBL] += tablebase[FTBL];
        if (song.instr[c + instrbase].ptr[STBL] > 0)
          song.instr[c + instrbase].ptr[STBL] += tablebase[STBL];
        song.instr[c + instrbase].vibdelay = fread8(handle);
        song.instr[c + instrbase].gatetimer = fread8(handle);
        song.instr[c + instrbase].firstwave = fread8(handle);
        std::fread(&song.instr[c + instrbase].name, MAX_INSTRNAMELEN, 1, handle);
      }
      // Read tables
      for (int c = 0; c < MAX_TABLES; c++)
      {
        loadsize = fread8(handle);
        if (loadsize + tablebase[c] > MAX_TABLELEN)
          goto ABORT;
        std::fread(&song.ltable[c][tablebase[c]], loadsize, 1, handle);
        std::fread(&song.rtable[c][tablebase[c]], loadsize, 1, handle);
        // Remap jumps and tablecommands
        for (int d = tablebase[c]; d < tablebase[c] + loadsize; d++)
        {
          if (song.ltable[c][d] == 0xff && song.rtable[c][d] > 0)
            song.rtable[c][d] += tablebase[c];
          if (c == 0 && (song.ltable[c][d] >= WAVECMD && song.ltable[c][d] <= WAVELASTCMD))
          {
            int cmd = song.ltable[c][d] & 0xf;
            if (cmd == CMD_SETWAVEPTR && song.rtable[c][d] > 0)
              song.rtable[c][d] += tablebase[WTBL];
            if (cmd == CMD_SETPULSEPTR && song.rtable[c][d] > 0)
              song.rtable[c][d] += tablebase[PTBL];
            if (cmd == CMD_SETFILTERPTR && song.rtable[c][d] > 0)
              song.rtable[c][d] += tablebase[FTBL];
            if (((cmd >= CMD_PORTAUP && cmd <= CMD_VIBRATO) || cmd == CMD_FUNKTEMPO) && song.rtable[c][d] > 0)
              song.rtable[c][d] += tablebase[STBL];
          }
        }
      }
      // Read patterns
      amount = fread8(handle);
      if (amount + pattbase > MAX_PATT)
        goto ABORT;

      for (int c = 0; c < amount; c++)
      {
        length = fread8(handle) * 4;
        std::fread(song.pattern[c + pattbase], length, 1, handle);
        // Remap song.pattern instruments and commands
        for (int d = 0; d < length; d += 4)
        {
          if (song.pattern[c + pattbase][d + 1] > 0)
            song.pattern[c + pattbase][d + 1] += instrbase;
          if (song.pattern[c + pattbase][d + 2] == CMD_SETWAVEPTR && song.pattern[c + pattbase][d + 3] > 0)
            song.pattern[c + pattbase][d + 3] += tablebase[WTBL];
          if (song.pattern[c + pattbase][d + 2] == CMD_SETPULSEPTR && song.pattern[c + pattbase][d + 3] > 0)
            song.pattern[c + pattbase][d + 3] += tablebase[PTBL];
          if (song.pattern[c + pattbase][d + 2] == CMD_SETFILTERPTR && song.pattern[c + pattbase][d + 3] > 0)
            song.pattern[c + pattbase][d + 3] += tablebase[FTBL];
          if (((song.pattern[c + pattbase][d + 2] >= CMD_PORTAUP && song.pattern[c + pattbase][d + 2] <= CMD_VIBRATO) ||
            song.pattern[c + pattbase][d + 2] == CMD_FUNKTEMPO) && song.pattern[c + pattbase][d + 3] > 0)
            song.pattern[c + pattbase][d + 3] += tablebase[STBL];
        }
      }
    }
  }

ABORT:
  std::fclose(handle);
  countpatternlengths();
  songchange();
}

int determinechannels(FILE* handle)
{
    int returnpos = std::ftell(handle);
    int songs = fread8(handle);
    unsigned char songbuffer[257];

    for (int d = 0; d < songs; d++)
    {
        for (int c = 0; c < MAX_CHN; c++)
        {
            int loadsize = fread8(handle);
            loadsize++;
            std::memset(songbuffer, 0, 257);
            std::fread(songbuffer, loadsize, 1, handle);

            // Check that each track of each song has a valid endmark.
            // Should fail if it's a mono song (not certain)
            if ((songbuffer[loadsize - 2] != 0xff) || (songbuffer[loadsize - 1] >= loadsize))
            {
                std::fseek(handle, returnpos, SEEK_SET);
                return MAX_CHN_MONO;
            }
        }
    }

    std::fseek(handle, returnpos, SEEK_SET);
    return MAX_CHN;
}

int getPattlen(int patt)
{
    return pattlen[patt];
}
