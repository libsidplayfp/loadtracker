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

#ifndef CHANNELS_H
#define CHANNELS_H

#include "common.h"

struct Chn
{
  int pattptr;
  unsigned short freq;
  unsigned short pulse;
  unsigned char trans;
  unsigned char instr;
  unsigned char note;
  unsigned char lastnote;
  unsigned char newnote;
  unsigned char pattnum;
  unsigned char songptr;
  unsigned char repeat;
  unsigned char gate;
  unsigned char wave;
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
  unsigned char gatetimer;
  bool advance;
  bool mute;
};

#ifndef CHANNELS_C
extern Chn chn[MAX_CHN];
extern unsigned char funktable[2];
#endif

void initchannels();

#endif
