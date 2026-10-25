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

#include <bitset>

enum
{
  MST_NOFINEVIB     = 0,
  MST_FINEVIB       = 1,
  MST_FUNKTEMPO     = 2,
  MST_PORTAMENTO    = 3,
  MST_RAW           = 4
};

enum class SngType
{
    NONE,
    STANDARD,
    DUAL
};

struct SongOrder
{
    unsigned char data[MAX_SONGLEN+2];
    int len;
};

struct FxTable
{
    unsigned char l;
    unsigned char r;
};

struct Song
{
    Instr instr[MAX_INSTR];
    FxTable fxtable[MAX_TABLES][MAX_TABLELEN];
    SongOrder order[MAX_SONGS][MAX_CHN];
    unsigned char pattern[MAX_PATT][MAX_PATTROWS*4+4];

    char title[MAX_STR];
    char author[MAX_STR];
    char released[MAX_STR];

public:
    int gettablelen(int num);
    int gettablepartlen(int num, int pos);
    int findfreespeedtable();
    void inserttable(int num, int pos, bool mode);
    void deletetable(int num, int pos);
    void deleteinstrtable(int i);
};


#ifndef SONG_C
extern char loadedsongfilename[MAX_FILENAME];
extern char songfilename[MAX_FILENAME];
extern char instrfilename[MAX_FILENAME];

extern Song song;
extern std::bitset<MAX_PATT> pattused;
#endif

SngType loadsong();
bool savesong();
void mergesong();
void clearsong(bool cs, bool cp, bool ci, bool cf, bool cn);
void loadinstrument();
bool saveinstrument();
void countpatternlengths();
void countthispattern();
void clearpattern(int p);
int insertpattern(int p);
void deletepattern(int p);
void findusedpatterns();
int getPattlen(int patt);
int makespeedtable(unsigned data, int mode, bool makenew);

#endif
