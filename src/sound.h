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

#ifndef SOUND_H
#define SOUND_H

#define DEFAULTMIXRATE 48000

#define PALFRAMERATE 50
#define PALCLOCKRATE 985248
#define NTSCFRAMERATE 60
#define NTSCCLOCKRATE 1022727

int sound_init(unsigned mr, bool writer, unsigned m, unsigned ntsc,
               unsigned multiplier, unsigned interpolate, unsigned customclockrate,
               unsigned exsid, float filterbias, unsigned combwaves);
void sound_uninit();

#endif
