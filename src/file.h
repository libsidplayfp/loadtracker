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

#ifndef FILE_H
#define FILE_H

#define MAX_FILENAME 60
#define MAX_PATHNAME 256

#ifdef __cplusplus
extern "C" {
#endif

void initpaths();
int fileselector(char *name, char *path, char *filter, const char *title, int filemode);
void editstring(char *buffer, int maxlength);

#ifdef __cplusplus
}
#endif

#endif

