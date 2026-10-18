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

#ifndef INPUT_H
#define INPUT_H

struct Input
{
    int key             = 0;
    int rawkey          = 0;
    int mouseb          = 0;
    int prevmouseb      = 0;
    int mouseheld       = 0;
    int mousex          = 0;
    int mousey          = 0;
    int wheel           = 0;
    bool shiftpressed   = false;
    bool altpressed     = false;

    bool iskeyyes();
    void clearkeys();
};

extern Input input;

#endif
