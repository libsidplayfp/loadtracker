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

#include <SDL3/SDL.h>

#include <iostream>
#include <string>

enum class MsgType
{
    INFO,
    WARNING,
    ERROR
};

namespace ltlog
{

void log(MsgType type, const char *msg, const char *detail)
{
    SDL_MessageBoxFlags flags;
    const char *pfx;
    switch (type)
    {
        case MsgType::INFO:
            flags = SDL_MESSAGEBOX_INFORMATION;
            pfx = "INFO";
            break;
        case MsgType::WARNING:
            flags = SDL_MESSAGEBOX_WARNING;
            pfx = "WARNING";
            break;
        case MsgType::ERROR:
            flags = SDL_MESSAGEBOX_ERROR;
            pfx = "ERROR";
            break;
    }

    std::string m(msg);
    if (detail)
    {
        m.append(": ").append(detail);
    }

    if (type == MsgType::ERROR)
    {
        if (SDL_ShowSimpleMessageBox(flags, "Load Tracker", m.c_str(), NULL))
            return;
    }

    std::cerr << pfx << "| " << m.c_str() << std::endl;
}

void info(const char *msg, const char *detail)
{
    log(MsgType::INFO, msg, detail);
}

void warning(const char *msg, const char *detail)
{
    log(MsgType::WARNING, msg, detail);
}

void error(const char *msg, const char *detail)
{
    log(MsgType::ERROR, msg, detail);
}

}
