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

#ifndef PLAY_C
extern unsigned char masterfader;
extern int lastsonginit;
#endif

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
