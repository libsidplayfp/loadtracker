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

#define MAX_OPTIONS 7

enum
{
  FORMAT_SID = 0,
  FORMAT_PRG = 1,
  FORMAT_BIN = 2
};

enum
{
  TYPE_NONE     = 0,
  TYPE_OVERFLOW = 1,
  TYPE_JUMP     = 2
};

#ifndef RELOC_C
extern unsigned char pattused[MAX_PATT];
extern unsigned char instrused[MAX_INSTR];
extern unsigned char tableused[MAX_TABLES][MAX_TABLELEN+1];
extern unsigned char pattmap[MAX_PATT];
extern unsigned char instrmap[MAX_INSTR];
extern unsigned char tablemap[MAX_TABLES][MAX_TABLELEN+1];
extern int tableerror;
#endif

void relocator();
void relocator_stereo();
unsigned char swapnybbles(unsigned char n);

#endif
