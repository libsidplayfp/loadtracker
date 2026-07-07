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

#ifndef SID_H
#define SID_H

#define NUMSIDREGS 0x19
#define SIDWRITEDELAY 14 // lda $xxxx,x 4 cycles, sta $d400,x 5 cycles, dex 2 cycles, bpl 3 cycles
#define SIDWAVEDELAY 4 // and $xxxx,x 4 cycles extra

#ifdef __cplusplus
extern "C" {
#endif

void sid_init(int speed, unsigned m,
              unsigned ntsc, unsigned interpolate,
              unsigned customclockrate, unsigned numsids,
              float filterbias, unsigned combwaves);
int sid_fillbuffer(short *ptr, int samples);
int sid_fillbuffer_stereo(short *lptr, short *rptr, int samples);
unsigned char sid_getorder(unsigned char index);

#ifndef SID_C
extern unsigned char sidreg[NUMSIDREGS];
extern unsigned char sidreg2[NUMSIDREGS];
#endif

#ifdef __cplusplus
}
#endif

#endif
