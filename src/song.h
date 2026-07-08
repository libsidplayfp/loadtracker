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

#ifndef SONG_C
extern INSTR instr[MAX_INSTR];
extern unsigned char ltable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char rtable[MAX_TABLES][MAX_TABLELEN];
extern unsigned char songorder[MAX_SONGS][MAX_CHN_MONO][MAX_SONGLEN+2];
extern unsigned char songorder_stereo[MAX_SONGS][MAX_CHN][MAX_SONGLEN+2];
extern unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];
extern char songname[MAX_STR];
extern char authorname[MAX_STR];
extern char copyrightname[MAX_STR];
extern int pattlen[MAX_PATT];
extern int songlen[MAX_SONGS][MAX_CHN_MONO];
extern int songlen_stereo[MAX_SONGS][MAX_CHN];
extern int highestusedpattern;
extern int highestusedinstr;
#endif

void loadsong(void);
void mergesong(void);
void loadinstrument(void);
int savesong(void);
int saveinstrument(void);
void clearsong(int cs, int cp, int ci, int cf, int cn);
void countpatternlengths(void);
void countthispattern(void);
void clearpattern(int p);
int insertpattern(int p);
void deletepattern(int p);
void findusedpatterns(void);
void optimizeeverything(int oi, int ot);

#endif
