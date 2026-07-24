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

#ifndef PLAY_H
#define PLAY_H

#include "common.h"

enum
{
  PLAY_PLAYING    = 0x00,
  PLAY_BEGINNING  = 0x01,
  PLAY_POS        = 0x02,
  PLAY_PATTERN    = 0x03,
  PLAY_STOP       = 0x04,
  PLAY_STOPPED    = 0x80
};

struct Chn
{
  unsigned char trans;
  unsigned char instr;
  unsigned char note;
  unsigned char lastnote;
  unsigned char newnote;
  int pattptr;
  unsigned char pattnum;
  unsigned char songptr;
  unsigned char repeat;
  unsigned short freq;
  unsigned char gate;
  unsigned char wave;
  unsigned short pulse;
  unsigned char ptr[2];
  unsigned char pulsetime;
  unsigned char wavetime;
  unsigned char vibtime;
  unsigned char vibdelay;
  unsigned char command;
  unsigned char cmddata;
  unsigned char newcommand;
  unsigned char newcmddata;
  unsigned char tick;
  unsigned char tempo;
  unsigned char mute;
  unsigned char advance;
  unsigned char gatetimer;
};

#ifndef PLAY_C
extern Chn chn[MAX_CHN];
extern unsigned char masterfader;
extern unsigned char freqtbllo[];
extern unsigned char freqtblhi[];
extern int lastsonginit;
#endif

void initchannels();
void initsong(int num, int playmode);
void initsongpos(int num, int playmode, int pattpos);
void stopsong();
void rewindsong();
void playtestnote(int note, int ins, int chnnum);
void releasenote(int chnnum);
void mutechannel(int chnnum);
bool isplaying();
void playroutine();
void playroutine_stereo();

void gettime(char *buf);

#endif
