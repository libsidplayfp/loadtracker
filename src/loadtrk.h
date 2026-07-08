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
#include "console.h"
#include "sound.h"
#include "sid.h"
#include "song.h"
#include "play.h"
#include "display.h"
#include "reloc.h"
#include "file.h"
#include "pattern.h"
#include "order.h"
#include "instr.h"
#include "table.h"

#define EDIT_PATTERN 0
#define EDIT_ORDERLIST 1
#define EDIT_INSTRUMENT 2
#define EDIT_TABLES 3
#define EDIT_NAMES 4

#define KEY_TRACKER 0
#define KEY_DMC 1
#define KEY_JANKO 2

#define VISIBLEPATTROWS 34
#define VISIBLEORDERLIST 23
#define VISIBLETABLEROWS 15
#define VISIBLEFILES 28

#define PGUPDNREPEAT 8

#ifndef LOADTRK_C
extern int menu;
extern int editmode;
extern int recordmode;
extern int followplay;
extern int hexnybble;
extern int stepsize;
extern int autoadvance;
extern int defaultpatternlength;
extern int cursorflash;
extern int cursorcolortable[];
extern int exitprogram;
extern int eacolumn;
extern int eamode;
extern int ebmode;
extern unsigned keypreset;
extern unsigned playerversion;
extern int fileformat;
extern int zeropageadr;
extern int playeradr;
extern unsigned sidmodel;
extern unsigned multiplier;
extern unsigned adparam;
extern unsigned ntsc;
extern unsigned patterndispmode;
extern unsigned sidaddress;
extern unsigned sid2address;
extern float panning;
extern unsigned finevibrato;
extern unsigned optimizepulse;
extern unsigned optimizerealtime;
extern unsigned usefinevib;
extern unsigned b;
extern unsigned mr;
extern unsigned writer;
extern unsigned exsid;
extern unsigned interpolate;
extern unsigned numsids;
extern unsigned monomode;
extern float basepitch;
extern char configbuf[MAX_PATHNAME];
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
extern char textbuffer[MAX_PATHNAME];
extern unsigned char hexkeytbl[16];
extern unsigned char datafile[];
extern COLORS colors;
#endif

void waitkey();
void waitkeymouse();
void waitkeynoupdate();
void waitkeymousenoupdate();
void onlinehelp(int standalone, int context);

#endif
