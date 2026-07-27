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

#ifndef SETTINGS_H
#define SETTINGS_H

// Increase if configuration has incompatible changes
constexpr int CFG_VERSION = 2;

#ifndef SETTINGS_C
// config FIXME
extern char specialnotenames[];
extern char scalatuningfilepath[];

#endif

struct Settings
{
    unsigned mixrate;
    unsigned sidmodel;
    unsigned numsids;
    unsigned ntsc;
    int fileformat;
    int playeradr;
    int zeropageadr;
    unsigned playerversion;
    unsigned keypreset;
    int defaultpatternlength;
    int stepsize;
    unsigned multiplier;
    unsigned adparam;
    unsigned interpolate;
    unsigned patterndispmode;
    unsigned sidaddress;
    unsigned sid2address;
    unsigned finevibrato;
    unsigned optimizepulse;
    unsigned optimizerealtime;
    unsigned residdelay;
    unsigned customclockrate;
    unsigned combwaves;
    unsigned exsid;
    unsigned darkmode;
    float panning;
    float basepitch;
    float filterbias;
    float equaldivisionsperoctave;

    bool usefinevib;

    Settings();
    void validate();
    int getMaxChannels();
    int getVisibleOrderlist();
};

extern Settings config;

#endif
