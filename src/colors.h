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

#ifndef COLORS_H
#define COLORS_H

class Colorscheme
{
private:
    int rainbowtable[5];

    unsigned char CBKGND;
    unsigned char CNORMAL;
    unsigned char CMUTE;
    unsigned char CEDIT;
    unsigned char CPLAYING;
    unsigned char CCOMMAND;
    unsigned char CTITLE;
    unsigned char CHDRBG;
    unsigned char CHDRFG;
    unsigned char CHEADER;
    unsigned char CMESSAGE;
    unsigned char CTOOLTIP;

public:
    void init(bool dark);

    int getAutoadvanceColor(int autoadvance);

    int getCursorColor(int i);

    int getRainbowColor(int i);

    unsigned char cbkgnd() const { return CBKGND; };
    unsigned char cnormal() const { return CNORMAL; };
    unsigned char cmute() const { return CMUTE; };
    unsigned char cedit() const { return CEDIT; };
    unsigned char cplaying() const { return CPLAYING; };
    unsigned char ccommand() const { return CCOMMAND; };
    unsigned char ctitle() const { return CTITLE; };
    unsigned char chdrbg() const { return CHDRBG; };
    unsigned char chdrfg() const { return CHDRFG; };
    unsigned char cheader() const { return CHEADER; };
    unsigned char cmessage() const { return CMESSAGE; };
    unsigned char ctooltip() const { return CTOOLTIP; };
};

#endif
