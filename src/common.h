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

#ifndef COMMON_H
#define COMMON_H

enum
{
  CMD_DONOTHING         = 0,
  CMD_PORTAUP           = 1,
  CMD_PORTADOWN         = 2,
  CMD_TONEPORTA         = 3,
  CMD_VIBRATO           = 4,
  CMD_SETAD             = 5,
  CMD_SETSR             = 6,
  CMD_SETWAVE           = 7,
  CMD_SETWAVEPTR        = 8,
  CMD_SETPULSEPTR       = 9,
  CMD_SETFILTERPTR      = 10,
  CMD_SETFILTERCTRL     = 11,
  CMD_SETFILTERCUTOFF   = 12,
  CMD_SETMASTERVOL      = 13,
  CMD_FUNKTEMPO         = 14,
  CMD_SETTEMPO          = 15
};

enum
{
  WTBL  = 0,
  PTBL  = 1,
  FTBL  = 2,
  STBL  = 3
};

#define MAX_FILT 64
#define MAX_STR 32
#define MAX_INSTR 64
#define MAX_CHN 6
#define MAX_CHN_MONO 3
#define MAX_PATT 208
#define MAX_TABLES 4
#define MAX_TABLELEN 255
#define MAX_INSTRNAMELEN 16
#define MAX_PATTROWS 128
#define MAX_SONGLEN 254
#define MAX_SONGS 32
#define MAX_NOTES 96

#define REPEAT 0xd0
#define TRANSDOWN 0xe0
#define TRANSUP 0xf0
#define LOOPSONG 0xff

#define ENDPATT 0xff
#define INSTRCHG 0x00
#define FX 0x40
#define FXONLY 0x50
#define FIRSTNOTE 0x60
#define LASTNOTE 0xbc
#define REST 0xbd
#define KEYOFF 0xbe
#define KEYON 0xbf
#define OLDKEYOFF 0x5e
#define OLDREST 0x5f

#define WAVEDELAY 0x1
#define WAVELASTDELAY 0xf
#define WAVESILENT 0xe0
#define WAVELASTSILENT 0xef
#define WAVECMD 0xf0
#define WAVELASTCMD 0xfe

#define VISIBLEPATTROWS 34
#define VISIBLETABLEROWS 15
#define VISIBLEFILES 28

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

struct Instr
{
  unsigned char ad;
  unsigned char sr;
  unsigned char ptr[MAX_TABLES];
  unsigned char vibdelay;
  unsigned char gatetimer;
  unsigned char firstwave;
  char name[MAX_INSTRNAMELEN];
};

struct Selection
{
    int chn = -1;
    int start;
    int end;
};

#endif
