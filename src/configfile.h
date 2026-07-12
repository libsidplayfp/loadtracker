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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#ifndef CONFIGFILE_C
// config
extern unsigned mr;
extern unsigned sidmodel;
extern unsigned numsids;
extern unsigned ntsc;
extern int fileformat;
extern int playeradr;
extern int zeropageadr;
extern unsigned playerversion;
extern unsigned keypreset;
extern int defaultpatternlength;
extern int stepsize;
extern unsigned multiplier;
extern unsigned adparam;
extern unsigned interpolate;
extern unsigned patterndispmode;
extern unsigned sidaddress;
extern unsigned sid2address;
extern float panning;
extern unsigned finevibrato;
extern unsigned optimizepulse;
extern unsigned optimizerealtime;
extern unsigned residdelay;
extern unsigned customclockrate;
extern float basepitch;
extern float filterbias;
extern unsigned combwaves;
extern float equaldivisionsperoctave;
extern char specialnotenames[];
extern char scalatuningfilepath[];
extern unsigned exsid;
extern unsigned darkmode;
#endif

void loadconfig();
void saveconfig();

#endif
