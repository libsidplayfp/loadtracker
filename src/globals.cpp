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

#include "globals.h"

#include "common.h"
#include "song.h"

#include <cstring>

#include <unistd.h>

const char *programname;

char textbuffer[MAX_PATHNAME];

char songpath[MAX_PATHNAME];
char instrpath[MAX_PATHNAME];
char packedpath[MAX_PATHNAME];
char songfilter[MAX_FILENAME];
char instrfilter[MAX_FILENAME];

Tables tables;
Timer timer;

bool menu = false;
int editmode = EDIT_PATTERN;
bool recordmode = true;
bool followplay = false;
bool exitprogram = false;
int eacolumn = 0;
int epcolumn;
int eamode = 0;
int enpos;
int epoctave = 2;
EditHdr ehmode = EditHdr::NONE;

void initpaths()
{
  std::memset(loadedsongfilename, 0, sizeof loadedsongfilename);
  std::memset(songfilename, 0, sizeof songfilename);
  std::memset(instrfilename, 0, sizeof instrfilename);
  std::memset(songpath, 0, sizeof songpath);
  std::memset(instrpath, 0, sizeof instrpath);
  std::memset(packedpath, 0, sizeof packedpath);
  std::strcpy(songfilter, "*.sng");
  std::strcpy(instrfilter, "*.ins");

  getcwd(songpath, MAX_PATHNAME);
  std::strcpy(instrpath, songpath);
  std::strcpy(packedpath, songpath);
}
