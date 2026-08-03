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

#ifndef RELOC_H
#define RELOC_H

#include "common.h"

enum
{
  FORMAT_SID = 0,
  FORMAT_PRG = 1,
  FORMAT_BIN = 2
};

enum
{
  PLAYER_BUFFERED       =   8,
  PLAYER_SOUNDEFFECTS   =  16,
  PLAYER_VOLUME         =  32,
  PLAYER_AUTHORINFO     =  64,
  PLAYER_ZPGHOSTREGS    = 128,
  PLAYER_NOOPTIMIZATION = 256,
  PLAYER_FULLBUFFERED   = 512
};

void relocator(const char* filename);
void optimizetable(int num);

#endif
