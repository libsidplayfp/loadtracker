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

Colors colors;

void initcolorscheme(bool dark)
{
  colors.CBKGND   = dark ? CBLACK : CDBLUE;

  colors.CNORMAL  = (dark ? CGREY : CLBLUE)|(colors.CBKGND<<4);
  colors.CMUTE    = CDGREY |(colors.CBKGND<<4);
  colors.CEDIT    = CLGREEN|(colors.CBKGND<<4);
  colors.CPLAYING = CLRED  |(colors.CBKGND<<4);
  colors.CCOMMAND = CLGREY |(colors.CBKGND<<4);
  colors.CTITLE   = CWHITE |(colors.CBKGND<<4);

  colors.CHDRBG   = dark ? CDBLUE : CLBLUE;
  colors.CHDRFG   = CYELLOW;

  colors.CHEADER  = colors.CHDRFG|(colors.CHDRBG<<4);
}
