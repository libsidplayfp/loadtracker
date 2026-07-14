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

#ifndef TABLE_H
#define TABLE_H

#include "common.h"

enum
{
  MST_NOFINEVIB     = 0,
  MST_FINEVIB       = 1,
  MST_FUNKTEMPO     = 2,
  MST_PORTAMENTO    = 3,
  MST_RAW           = 4
};

void tablecommands();
int makespeedtable(unsigned data, int mode, bool makenew);
void optimizetable(int num);
void deleteinstrtable(int i);
int gettablelen(int num);
int gettablepartlen(int num, int pos);
void gototable(int num, int pos);
void exectable(int num, int ptr);
int findfreespeedtable();

class Tables
{
private:
    int m_view[MAX_TABLES];
public:
    int m_num;
    int m_pos;
    int m_column;
private:
    int m_marknum = -1;
    int m_markstart;
    int m_markend;
    bool m_lock = true;

public:
    inline int view(int num) const { return m_view[num]; }
    inline int curview() const { return m_view[m_num]; }
    inline int num() const { return m_num; }
    inline int pos() const { return m_pos; }
    inline int column() const { return (m_column & 1)+(m_column/2)*3; }
    inline int marknum() const { return m_marknum; }
    inline int markstart() const { return m_markstart; }
    inline int markend() const { return m_markend; }
    inline bool islocked() const { return m_lock; }
    void setnum(int num);
    void setpos(int pos);
    void setcolumn(int column);
    void resetmarknum();
    void setmarkstart(int num, int markstart);
    void setmarkend(int markend);
    void fliplock();
    void clear();

    void validatetableview();
    void tableup(bool shiftpressed);
    void tabledown(bool shiftpressed);
    void settableview(int num, int pos);
    void settableviewfirst(int num, int pos);
};

extern Tables tables;

#endif
