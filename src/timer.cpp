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

#include "timer.h"

#include "sound.h"

#include <cstdio>

Timer timer;

char timechar[2] = {':', ' '};

void Timer::reset()
{
    timemin = 0;
    timesec = 0;
    timeframe = 0;
}

void Timer::increment()
{
    timeframe++;
    unsigned framerate = m_ntsc ? NTSCFRAMERATE : PALFRAMERATE;
    if (((m_multiplier) && (timeframe >= framerate*m_multiplier))
        || ((!m_multiplier) && (timeframe >= framerate/2)))
    {
      timeframe = 0;
      timesec++;
    }
    if (timesec == 60)
    {
      timesec = 0;
      timemin++;
      timemin %= 60;
    }
}

void Timer::get(char *buf)
{
  int idx;
  if (m_multiplier)
  {
    idx = (m_ntsc ? 30 : 25) * m_multiplier;
  }
  else
  {
    idx = m_ntsc ? 15 : 13;
  }

  std::sprintf(buf, " %02d%c%02d ", timemin, timechar[(timeframe/idx) & 1], timesec);
}
