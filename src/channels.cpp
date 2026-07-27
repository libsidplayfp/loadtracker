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

#include "channels.h"

#include "settings.h"

#include <cstring>

Chn chn[MAX_CHN];
unsigned char funktable[2];

void initchannels()
{
  int maxChns = config.getMaxChannels();
  Chn *cptr = &chn[0];

  std::memset(chn, 0, sizeof chn);

  for (int c = 0; c < maxChns; c++)
  {
    chn[c].trans = 0;
    chn[c].instr = 1;
    if (config.multiplier)
      cptr->tempo = 6*config.multiplier-1;
    else
      cptr->tempo = 6-1;
    cptr++;
  }

  if (config.multiplier)
  {
    funktable[0] = 9*config.multiplier-1;
    funktable[1] = 6*config.multiplier-1;
  }
  else
  {
    funktable[0] = 9-1;
    funktable[1] = 6-1;
  }
}
