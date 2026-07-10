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

#ifndef ORDER_H
#define ORDER_H

#ifndef ORDER_C
extern int espos[MAX_CHN];
extern int esend[MAX_CHN];
extern int eseditpos;
extern int esview;
extern int escolumn;
extern int eschn;
extern int esnum;
extern int esmarkchn;
extern int esmarkstart;
extern int esmarkend;
extern int enpos;
#endif

void updateviewtopos();
void orderlistcommands();
void namecommands();
void nextsong();
void prevsong();
void songchange();
void deleteorder();
void insertorder(unsigned char byte);

#endif
