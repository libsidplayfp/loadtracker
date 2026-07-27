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

#include "settings.h"

#include "common.h"
#include "pattern.h"
#include "reloc.h"
#include "sound.h"

Settings config;

// config
float panning = 1.0f;
unsigned residdelay = 0;
unsigned customclockrate = 0;
float basepitch = 0.0f;
float filterbias = 0.5f;
unsigned combwaves = 1;
float equaldivisionsperoctave = 12.0f;
char specialnotenames[186];
char scalatuningfilepath[MAX_PATHNAME];
unsigned exsid = 0;
unsigned darkmode = 0;

bool usefinevib = false;

int Settings::getMaxChannels()
{
    return (numsids == 1) ? MAX_CHN_MONO : MAX_CHN;
}

int Settings::getVisibleOrderlist()
{
    return (numsids == 1) ? 23 : 14;
}

Settings::Settings() :
    mixrate(DEFAULTMIXRATE),
    sidmodel(1),
    numsids(1),
    ntsc(0),
    fileformat(FORMAT_PRG),
    playeradr(0x1000),
    zeropageadr(0xfc),
    playerversion(0),
    keypreset(KEY_TRACKER),
    defaultpatternlength(64),
    stepsize(4),
    multiplier(1),
    adparam(0x0f00),
    interpolate(1),
    patterndispmode(2),
    sidaddress(0xd400),
    sid2address(0xd500),
    finevibrato(1),
    optimizepulse(1),
    optimizerealtime(1)
{}
