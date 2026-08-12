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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "table.h"
#include "timer.h"

extern Tables tables;
extern Timer timer;

extern const char *programname;

extern char textbuffer[MAX_PATHNAME];

extern char songpath[MAX_PATHNAME];
extern char instrpath[MAX_PATHNAME];
extern char packedpath[MAX_PATHNAME];
extern char songfilter[MAX_FILENAME];
extern char instrfilter[MAX_FILENAME];

extern bool menu;
extern int editmode;
extern bool recordmode;
extern bool followplay;
extern bool exitprogram;
extern int eacolumn;
extern int epcolumn;
extern int enpos;
extern int epoctave;
extern EditHdr ehmode;

void initpaths();

#endif
