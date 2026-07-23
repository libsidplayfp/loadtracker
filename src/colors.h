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

#ifndef COLORS_H
#define COLORS_H

enum
{
  CBLACK  = 0x0,
  CWHITE  = 0x1,
  CDRED   = 0x2,
  CCYAN   = 0x3,
  CPURPLE = 0x4,
  CDGREEN = 0x5,
  CDBLUE  = 0x6,
  CYELLOW = 0x7,
  CLBROWN = 0x8,
  CDBROWN = 0x9,
  CLRED   = 0xA,
  CDGREY  = 0xB,
  CGREY   = 0xC,
  CLGREEN = 0xD,
  CLBLUE  = 0xE,
  CLGREY  = 0xF
};

struct Colors
{
  unsigned char CBKGND;
  unsigned char CNORMAL;
  unsigned char CMUTE;
  unsigned char CEDIT;
  unsigned char CPLAYING;
  unsigned char CCOMMAND;
  unsigned char CTITLE;
  unsigned char CHDRBG;
  unsigned char CHDRFG;
  unsigned char CHEADER;
};

extern Colors colors;

void initcolorscheme(bool dark);

#endif
