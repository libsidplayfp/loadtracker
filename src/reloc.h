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

#define FORMAT_SID 0
#define FORMAT_PRG 1
#define FORMAT_BIN 2

#define PLAYER_BUFFERED 8
#define PLAYER_SOUNDEFFECTS 16
#define PLAYER_VOLUME 32
#define PLAYER_AUTHORINFO 64
#define PLAYER_ZPGHOSTREGS 128
#define PLAYER_NOOPTIMIZATION 256
#define PLAYER_FULLBUFFERED 512

#define MAX_OPTIONS 7

#define TYPE_NONE 0
#define TYPE_OVERFLOW 1
#define TYPE_JUMP 2

#define CAUSE_NONE 0
#define CAUSE_PATTERN 1
#define CAUSE_INSTRUMENT 2
#define CAUSE_WAVECMD 3

#define MAX_BYTES_PER_ROW 16

#ifdef __cplusplus
extern "C" {
#endif

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
int testoverlap(int area1start, int area1size, int area2start, int area2size);
int packpattern(unsigned char *dest, unsigned char *src, int rows);
unsigned char swapnybbles(unsigned char n);
void findtableduplicates(int num);
int isusedandselfcontained(int num, int start);
void calcspeedtest(unsigned char pos);

int insertfile(const char *name);
void inserttext(const char *text);
void insertdefine(const char *name, int value);
void insertlabel(const char *name);
void insertbyte(unsigned char byte);
void insertbytes(const unsigned char *bytes, int size);
void insertaddrlo(const char *name);
void insertaddrhi(const char *name);

#ifdef __cplusplus
}
#endif

#endif
