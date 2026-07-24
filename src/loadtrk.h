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

#ifndef LOADTRK_H
#define LOADTRK_H

#include "common.h"
#include "file.h"

enum
{
  EDIT_PATTERN      = 0,
  EDIT_ORDERLIST    = 1,
  EDIT_INSTRUMENT   = 2,
  EDIT_TABLES       = 3,
  EDIT_NAMES        = 4
};

enum class EditHdr
{
  NONE    = 0,
  ADSR    = 1,
  BPM     = 2
};

#define PGUPDNREPEAT 8

#ifndef LOADTRK_C
extern bool menu;
extern int editmode;
extern bool recordmode;
extern bool followplay;
extern int hexnybble;
extern bool exitprogram;
extern int eacolumn;
extern EditHdr ehmode;

extern bool monomode;
extern char loadedsongfilename[MAX_FILENAME];
extern char songfilename[MAX_FILENAME];
extern char songfilter[MAX_FILENAME];
extern char songpath[MAX_PATHNAME];
extern char instrfilename[MAX_FILENAME];
extern char instrfilter[MAX_FILENAME];
extern char instrpath[MAX_PATHNAME];
extern char packedpath[MAX_PATHNAME];
extern const char *programname;
extern const char *notename[];
#endif

void waitkey();
void waitkeymouse();
void waitkeynoupdate();
void waitkeymousenoupdate();
void onlinehelp(int standalone, int context);

int getMaxChannels();
int getVisibleOrderlist();

#endif
