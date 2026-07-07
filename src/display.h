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

#ifndef GDISPLAY_H
#define GDISPLAY_H

#define CBLACK  0x0
#define CDBLUE  0x1
#define CDGREEN 0x2
#define CDGREY  0x3
#define CDRED   0x4
#define CDBROWN 0x5
#define CLBROWN 0x6
#define CLGREY  0x7
#define CGREY   0x8
#define CLBLUE  0x9
#define CLGREEN 0xA
#define CCYAN   0xB
#define CLRED   0xC
#define CPURPLE 0xD
#define CYELLOW 0xE
#define CWHITE  0xF

#ifdef __cplusplus
extern "C" {
#endif

void initcolorscheme(int dark);
void printmainscreen(void);
void displayupdate(void);
void printstatus(void);
void resettime(void);
void incrementtime(void);

#ifdef __cplusplus
}
#endif

#endif
