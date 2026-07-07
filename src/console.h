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

#ifndef CONSOLE_H
#define CONSOLE_H

#define MAX_COLUMNS 128
#define MAX_ROWS 40

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int channelsX;
    int channelsY;
    int instrumentsX;
    int instrumentsY;
    int loadboxX;
    int loadboxY;
    int octaveX;
    int octaveY;
    int orderlistX;
    int orderlistY;
    int patternsX;
    int patternsY;
    int statusBottomX;
    int statusBottomY;
    int statusTopEndX;
    int statusTopFvX;
    int statusTopX;
    int statusTopY;
} POSITIONS;


int initscreen();
void closescreen();
void clearscreen();
void fliptoscreen();
void printtext(int x, int y, int color, const char *text);
void printtextc(int y, int color, const char *text);
void printtextcp(int cp, int y, int color, const char *text);
void printblank(int x, int y, int length);
void printblankc(int x, int y, int color, int length);
void drawbox(int x, int y, int color, int sx, int sy);
void printbg(int x, int y, int color, int length);
void getkey();
void initDisplayPositions();

#ifndef CONSOLE_C
extern int key, rawkey, shiftpressed, altpressed, cursorflashdelay;
extern int mouseb, prevmouseb;
extern int mouseheld;
extern int mousex, mousey;
extern POSITIONS dpos;
#endif

#ifdef __cplusplus
}
#endif

#endif
