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
// file selector
// =============================================================================

#define FILE_C

#include "loadtrk.h"
#include "bme_main.h"
#include "bme_win.h"

#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdlib>

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#ifdef __WIN32__
#include <windows.h>
#endif

#define MAX_DIRFILES 16384
#define DOUBLECLICKDELAY 15

typedef struct
{
  char *name;
  int attribute;
} DIRENTRY;

DIRENTRY direntry[MAX_DIRFILES];

int cmpname(char *string1, char *string2);

void initpaths()
{
  for (int c = 0; c < MAX_DIRFILES; c++)
     direntry[c].name = nullptr;

  std::memset(loadedsongfilename, 0, sizeof loadedsongfilename);
  std::memset(songfilename, 0, sizeof songfilename);
  std::memset(instrfilename, 0, sizeof instrfilename);
  std::memset(songpath, 0, sizeof songpath);
  std::memset(instrpath, 0, sizeof instrpath);
  std::memset(packedpath, 0, sizeof packedpath);
  std::strcpy(songfilter, "*.sng");
  std::strcpy(instrfilter, "*.ins");

  getcwd(songpath, MAX_PATHNAME);
  std::strcpy(instrpath, songpath);
  std::strcpy(packedpath, songpath);
}

int fileselector(char *name, char *path, char *filter, const char *title, int filemode)
{
#ifdef __WIN32__
  char drivestr[] = "A:\\";
char driveexists[26];
  #endif
  char cmpbuf[MAX_PATHNAME];
  char tempname[MAX_PATHNAME];

  // Set initial path (if any)
  if (std::strlen(path)) chdir(path);

  // Scan for all existing drives
#ifdef __WIN32__
  for (int c = 0; c < 26; c++)
  {
    drivestr[0] = 'A'+c;
    if (GetDriveType(drivestr) > 1) driveexists[c] = 1;
    else driveexists[c] = 0;
  }
#endif

  // Read new directory
  NEWPATH:
  getcwd(path, MAX_PATHNAME);
  int files = 0;
  // Deallocate old names
  for (int c = 0; c < MAX_DIRFILES; c++)
  {
    if (direntry[c].name)
    {
      std::free(direntry[c].name);
      direntry[c].name = nullptr;
    }
  }
#ifdef __WIN32__
  // Create drive letters
  for (int c = 0; c < 26; c++)
  {
    if (driveexists[c])
    {
      drivestr[0] = 'A'+c;
      direntry[files].name = strdup(drivestr);
      direntry[files].attribute = 2;
      files++;
    }
  }
#endif

  // Process directory
  DIR *dir;
#ifdef __amigaos__
  dir = opendir("");
#else
  dir = opendir(".");
#endif
  if (dir)
  {
    char *filtptr = std::strstr(filter, "*");
    if (!filtptr) filtptr = filter;
    else filtptr++;
    for (size_t i = 0; i < std::strlen(filter); i++)
      filter[i] = std::tolower(filter[i]);

    struct dirent *de;
    while ((de = readdir(dir)))
    {
      if ((files < MAX_DIRFILES) && (std::strlen(de->d_name) < MAX_FILENAME))
      {
        struct stat st;
        direntry[files].name = strdup(de->d_name);
        direntry[files].attribute = 0;
        stat(de->d_name, &st);
        if (S_ISDIR(st.st_mode))
        {
          direntry[files].attribute = 1;
          files++;
        }
        else
        {
          // If a file, must match filter
          strcpy(cmpbuf, de->d_name);
          if ((!std::strcmp(filtptr, "*")) || (!strcmp(filtptr, ".*")))
            files++;
          else
          {
            for (size_t i = 0; i < strlen(cmpbuf); i++)
              cmpbuf[i] = std::tolower(cmpbuf[i]);
            if (std::strstr(cmpbuf, filtptr))
              files++;
            else
            {
              std::free(direntry[files].name);
              direntry[files].name = nullptr;
            }
          }
        }
      }
    }
    closedir(dir);
  }
  // Sort the filelist in a most horrible fashion
  for (int c = 0; c < files; c++)
  {
    int lowest = c;
    for (int d = c+1; d < files; d++)
    {
      if (direntry[d].attribute < direntry[lowest].attribute)
      {
        lowest = d;
      }
      else
      {
        if (direntry[d].attribute == direntry[lowest].attribute)
        {
          if (cmpname(direntry[d].name, direntry[lowest].name) < 0)
          {
            lowest = d;
          }
        }
      }
    }
    if (lowest != c)
    {
      DIRENTRY swaptemp = direntry[c];
      direntry[c] = direntry[lowest];
      direntry[lowest] = swaptemp;
    }
  }

  // Search for the current filename
  int fileview = 0;
  int filepos = 0;
  int lastclick = 0;
  int lastfile = 0;

  for (int c = 0; c < files; c++)
  {
    if ((!direntry[c].attribute) && (!cmpname(name, direntry[c].name)))
    {
      filepos = c;
    }
  }

  int exitfilesel = -1;
  while (exitfilesel < 0)
  {
    int cc = cursorcolortable[cursorflash];
    if (cursorflashdelay >= 6)
    {
      cursorflashdelay %= 6;
      cursorflash++;
      cursorflash &= 3;
    }
    fliptoscreen();
    getkey();
    if (lastclick) lastclick--;

    if (win_quitted)
    {
      exitprogram = 1;
      for (int c = 0; c < MAX_DIRFILES; c++)
      {
        if (direntry[c].name)
        {
          std::free(direntry[c].name);
          direntry[c].name = nullptr;
        }
      }
      return 0;
    }

    if (mouseb)
    {
      // Cancel (click outside)
      if ((mousey < 3) || (mousey > 3+VISIBLEFILES+6) || (mousex <= 4+10) || (mousex >= 75+10))
      {
        if ((!prevmouseb) && (lastclick)) exitfilesel = 0;
      }

      // Select dir,name,filter
      if ((mousey >= 3+VISIBLEFILES+3) && (mousey <= 3+VISIBLEFILES+5) && (mousex >= 14+10) && (mousex <= 73+10))
      {
        filemode = mousey - (3+VISIBLEFILES+3) + 1;
        if ((filemode == 3) && (!prevmouseb) && (lastclick)) goto ENTERFILE;
      }

      // Select file from list
      if ((mousey >= 3) && (mousey <= 3+VISIBLEFILES+2) && (mousex >= 6+10) && (mousex <= 73+10))
      {
        filemode = 0;
        filepos = mousey - 4 - 1 + fileview;
        if (filepos < 0) filepos = 0;
        if (filepos > files-1) filepos = files - 1;

        if (!direntry[filepos].attribute)
          strcpy(name, direntry[filepos].name);

        if ((!prevmouseb) && (lastclick) && (lastfile == filepos)) goto ENTERFILE;
      }
    }

    if (!filemode)
    {
      if (((key >= '0') && (key <= '0')) || ((key >= 'a') && (key <= 'z')) || ((key >= 'A') && (key <= 'Z')))
      {
        char k = std::tolower(key);
        int oldfilepos = filepos;

        for (filepos = oldfilepos + 1; filepos < files; filepos++)
          if (std::tolower(direntry[filepos].name[0]) == k) break;
        if (filepos >= files)
        {
          for (filepos = 0; filepos < oldfilepos; filepos++)
             if (std::tolower(direntry[filepos].name[0]) == k) break;
        }

        if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
      }
    }

    switch(rawkey)
    {
      case KEY_ESC:
      exitfilesel = 0;
      break;

      case KEY_BACKSPACE:
      if (!filemode)
      {
#ifdef __amigaos__
        chdir("/");
#else
        chdir("..");
#endif
        goto NEWPATH;
      }
      break;

      case KEY_HOME:
      if (!filemode)
      {
        filepos = 0;
        if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
      }
      break;

      case KEY_END:
      if (!filemode)
      {
        filepos = files-1;
        if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
      }
      break;

      case KEY_PGUP:
      for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      {
        if ((!filemode) && (filepos > 0))
        {
          filepos--;
          if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
        }
      }
      break;

      case KEY_UP:
      if ((!filemode) && (filepos > 0))
      {
        filepos--;
        if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
      }
      break;

      case KEY_PGDN:
      for (int scrrep = PGUPDNREPEAT; scrrep; scrrep--)
      {
        if ((!filemode) && (filepos < files-1))
        {
          filepos++;
          if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
        }
      }
      break;

      case KEY_DOWN:
      if ((!filemode) && (filepos < files-1))
      {
        filepos++;
        if (!direntry[filepos].attribute) strcpy(name, direntry[filepos].name);
      }
      break;

      case KEY_TAB:
      if (!shiftpressed)
      {
        filemode++;
        if (filemode > 3) filemode = 0;
      }
      else
      {
        filemode--;
        if (filemode < 0) filemode = 3;
      }
      break;

      case KEY_ENTER:
      ENTERFILE:
      switch(filemode)
      {
        case 0:
        switch (direntry[filepos].attribute)
        {
          case 0:
          strcpy(name, direntry[filepos].name);
          exitfilesel = 1;
          break;

          case 1:
          chdir(direntry[filepos].name);
          goto NEWPATH;

          case 2:
          strcpy(tempname, direntry[filepos].name);
          if (strlen(tempname))
          {
            if (tempname[strlen(tempname)-1] != '\\')
              strcat(tempname, "\\");
          }
          chdir(tempname);
          goto NEWPATH;
        }
        break;

        case 1:
        chdir(path);
        /* fall through */
        case 2:
        filemode = 0;
        goto NEWPATH;

        case 3:
        exitfilesel = 1;
        break;
      }
      break;
    }

    switch(filemode)
    {
      case 1:
      editstring(path, MAX_PATHNAME);
      break;

      case 2:
      editstring(filter, MAX_FILENAME);
      break;

      case 3:
      editstring(name, MAX_FILENAME);
      break;
    }

    // Validate filelist view
    if (filepos < fileview) fileview = filepos;
    if (filepos - fileview >= VISIBLEFILES) fileview = filepos - VISIBLEFILES + 1;

    // Refresh fileselector display
    if (isplaying()) printstatus();
    for (int c = 0; c < VISIBLEFILES+7; c++)
    {
      printblank(dpos.loadboxX-(MAX_FILENAME+10)/2, dpos.loadboxY+c, MAX_FILENAME+10);
    }
    drawbox(dpos.loadboxX-(MAX_FILENAME+10)/2, dpos.loadboxY, 15, MAX_FILENAME+10, VISIBLEFILES+7);
    printblankc(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+1, colors.CHEADER, MAX_FILENAME+8);
    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+1, colors.CHEADER, title);

    for (int c = 0; c < VISIBLEFILES; c++)
    {
      if ((fileview+c >= 0) && (fileview+c < files))
      {
        switch (direntry[fileview+c].attribute)
        {
          case 0:
          std::sprintf(textbuffer, "%-60s        ", direntry[fileview+c].name);
          break;

          case 1:
          std::sprintf(textbuffer, "%-60s   <DIR>", direntry[fileview+c].name);
          break;

          case 2:
          std::sprintf(textbuffer, "%-60s   <DRV>", direntry[fileview+c].name);
          break;
        }
      }
      else
      {
        std::sprintf(textbuffer, "                                                                    ");
      }
      int color = colors.CNORMAL;
      if ((fileview+c) == filepos) color = colors.CEDIT;
      textbuffer[68] = 0;
      printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+2+c, color, textbuffer);
      if ((!filemode) && ((fileview+c) == filepos)) printbg(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+2+c, cc, 68);
    }

    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+3+VISIBLEFILES, 15, "PATH:   ");
    std::sprintf(textbuffer, "%-60s", path);
    textbuffer[MAX_FILENAME] = 0;
    int color = (filemode == 1) ? colors.CEDIT : colors.CNORMAL;
    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+9, dpos.loadboxY+3+VISIBLEFILES, color, textbuffer);
    if ((filemode == 1) && (strlen(path) < MAX_FILENAME)) printbg(dpos.loadboxX-(MAX_FILENAME+10)/2+9+strlen(path), dpos.loadboxY+3+VISIBLEFILES, cc, 1);

    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+4+VISIBLEFILES, 15, "FILTER: ");
    std::sprintf(textbuffer, "%-60s", filter);
    textbuffer[MAX_FILENAME] = 0;
    color = (filemode == 2) ? colors.CEDIT : colors.CNORMAL;
    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+9, dpos.loadboxY+4+VISIBLEFILES, color, textbuffer);
    if (filemode == 2) printbg(dpos.loadboxX-(MAX_FILENAME+10)/2+9+strlen(filter), dpos.loadboxY+4+VISIBLEFILES, cc, 1);

    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+1, dpos.loadboxY+5+VISIBLEFILES, 15, "NAME:   ");
    std::sprintf(textbuffer, "%-60s", name);
    textbuffer[MAX_FILENAME] = 0;
    color = (filemode == 3) ? colors.CEDIT : colors.CNORMAL;
    printtext(dpos.loadboxX-(MAX_FILENAME+10)/2+9, dpos.loadboxY+5+VISIBLEFILES, color, textbuffer);
    if (filemode == 3) printbg(dpos.loadboxX-(MAX_FILENAME+10)/2+9+strlen(name), dpos.loadboxY+5+VISIBLEFILES, cc, 1);

    if (win_quitted) exitfilesel = 0;

    if ((mouseb) && (!prevmouseb))
    {
      lastclick = DOUBLECLICKDELAY;
      lastfile = filepos;
    }
  }

  // Deallocate all used names
  for (int c = 0; c < MAX_DIRFILES; c++)
  {
    if (direntry[c].name)
    {
      std::free(direntry[c].name);
      direntry[c].name = nullptr;
    }
  }

  // Restore screen & exit
  printmainscreen();
  return exitfilesel;
}

int cmpname(char *string1, char *string2)
{
  for (;;)
  {
    unsigned char char1 = std::tolower(*string1++);
    unsigned char char2 = std::tolower(*string2++);
    if (char1 < char2) return -1;
    if (char1 > char2) return 1;
    if ((!char1) || (!char2)) return 0;
  }
}

