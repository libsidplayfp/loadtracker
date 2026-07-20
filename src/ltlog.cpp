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

// =============================================================================
// logging functions
// =============================================================================

#include <iostream>

namespace ltlog
{

void log(const char *type, const char *msg)
{
    std::cerr << type << ": " << msg << std::endl;
}

void info(const char *msg)
{
    log("INFO", msg);
}

void warning(const char *msg)
{
    log("WARNING", msg);
}

void error(const char *msg)
{
    log("ERROR", msg);
}


}

