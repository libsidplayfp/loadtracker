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

class Tables
{
private:
    int m_view[MAX_TABLES];
    int m_num;
    int m_pos;
    int m_column;
    Selection m_mark;
    bool m_lock = false;

public:
    inline int view(int num) const { return m_view[num]; }
    inline int curview() const { return m_view[m_num]; }
    inline int num() const { return m_num; }
    inline int pos() const { return m_pos; }
    inline int column() const { return m_column; }
    inline int columnpos() const { return (m_column & 1)+(m_column/2)*3; }
    inline int marknum() const { return m_mark.chn; }
    inline int markstart() const { return m_mark.start; }
    inline int markend() const { return m_mark.end; }
    inline bool islocked() const { return m_lock; }

    inline void setnum(int num) { m_num = num; }
    inline void incnum() { m_pos -= curview(); m_num++; if (m_num >= MAX_TABLES) m_num = 0; m_pos += curview(); }
    inline void decnum() { m_pos -= curview(); m_num--; if (m_num < 0) m_num = MAX_TABLES - 1; m_pos += curview(); }
    inline void incpos() { m_pos++; if (m_pos >= MAX_TABLELEN) m_pos = MAX_TABLELEN - 1; }
    inline void setcolumn(int col) { m_column = col; }
    inline bool inccolumn() { m_column++; if (m_column > 3) { m_column = 0; return true; } return false; }
    inline bool deccolumn() { m_column--; if (m_column < 0) { m_column = 3; return true; } return false; }

    void setrow(int num, int pos, int column);
    void resetmarknum();
    void setmarkstart(int num, int markstart);
    void setmarkend(int markend);
    void fliplock();
    void clear();

    void validatetableview();
    void tableup(bool shiftpressed, int n=1);
    void tabledown(bool shiftpressed, int n=1);
    void settableview(int num, int pos);
    void settableviewfirst(int num, int pos);
};

#endif
