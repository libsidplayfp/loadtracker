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

#ifndef TABLE_H
#define TABLE_H

#define MST_NOFINEVIB 0
#define MST_FINEVIB 1
#define MST_FUNKTEMPO 2
#define MST_PORTAMENTO 3
#define MST_RAW 4

#ifndef TABLE_C
extern int etview[MAX_TABLES];
extern int etnum;
extern int etpos;
extern int etcolumn;
extern int etlock;
extern int etmarknum;
extern int etmarkstart;
extern int etmarkend;
#endif

void tablecommands();
int makespeedtable(unsigned data, int mode, int makenew);
void optimizetable(int num);
void deleteinstrtable(int i);
int gettablelen(int num);
int gettablepartlen(int num, int pos);
void gototable(int num, int pos);
void settableview(int num, int pos);
void settableviewfirst(int num, int pos);
void validatetableview();
void exectable(int num, int ptr);
int findfreespeedtable();

#endif
