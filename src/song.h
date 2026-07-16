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

#ifndef SONG_H
#define SONG_H

#include "common.h"

struct Song
{
    INSTR instr[MAX_INSTR];
    unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
    unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
    unsigned char order[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
    int len[MAX_SONGS][MAX_CHN];
    unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];

    char title[MAX_STR];
    char author[MAX_STR];
    char released[MAX_STR];
};


#ifndef SONG_C
extern Song song;
#endif

void loadsong();
void mergesong();
void loadinstrument();
bool savesong();
bool saveinstrument();
void clearsong(bool cs, bool cp, bool ci, bool cf, bool cn);
void countpatternlengths();
void countthispattern();
void clearpattern(int p);
int insertpattern(int p);
void deletepattern(int p);
void findusedpatterns();
int getPattlen(int patt);

#endif
