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

#include "colors.h"

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

int cursorcolortable[4] = { CWHITE, CLGREY, CGREY, CLGREY };

void Colorscheme::init(bool dark)
{
  CBKGND   = dark ? CBLACK : CDBLUE;

  CNORMAL  = (dark ? CGREY : CLBLUE)|(CBKGND<<4);
  CMUTE    = CDGREY |(CBKGND<<4);
  CEDIT    = CLGREEN|(CBKGND<<4);
  CPLAYING = CLRED  |(CBKGND<<4);
  CCOMMAND = CLGREY |(CBKGND<<4);
  CTITLE   = CWHITE |(CBKGND<<4);

  CHDRBG   = dark ? CDBLUE : CLBLUE;
  CHDRFG   = CYELLOW;

  CMESSAGE = CLGREY;
  CTOOLTIP = dark ? CDGREY : CLBLUE;

  CHEADER  = CHDRFG|(CHDRBG<<4);

  rainbowtable[0] = dark ? CLBLUE : CDBLUE;
  rainbowtable[1] = CDGREEN;
  rainbowtable[2] = CYELLOW;
  rainbowtable[3] = CLBROWN;
  rainbowtable[4] = CDRED;
}

int Colorscheme::getAutoadvanceColor(int autoadvance)
{
  switch(autoadvance)
  {
    default: // unreachable
    case 0: return CLRED;
    case 1: return CLBLUE;
    case 2: return CGREY;
  }
}

int Colorscheme::getCursorColor(int i)
{
    return cursorcolortable[i];
}

int Colorscheme::getRainbowColor(int i)
{
    return rainbowtable[i]|(CHDRBG<<4);
}
