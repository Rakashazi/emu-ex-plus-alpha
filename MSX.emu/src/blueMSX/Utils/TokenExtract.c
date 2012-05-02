/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Utils/TokenExtract.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008/03/30 18:38:47 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "StrcmpNoCase.h"
#include <string.h>
#include <stdlib.h>

char* extractToken(char* szLine, int argNo) {
    static char argBuf[512];
    int i;

    for (i = 0; i <= argNo; i++) {
        char* arg = argBuf;

        while (*szLine == ' ') szLine++;

        if (*szLine == 0) return NULL;

        if (*szLine == '\"') {
            szLine++;
            while (*szLine != '\"' && *szLine != 0) {
                *arg++ = *szLine++;
            }
            *arg = 0;
            if (*szLine != 0) szLine++;
        }
        else {
            do {
                *arg++ = *szLine++;
            } while (*szLine != ' ' && *szLine != '\t' && *szLine != '\r' && *szLine != '\n' && *szLine != 0);
            *arg = 0;
            if (*szLine != 0) szLine++;
        }
    }
    return argBuf;
}

char* extractTokenEx(char* szLine, int argNo, char *dir) {
    static char argBuf[512];
    char *p;

    p = extractToken(szLine, argNo);
    if (dir == NULL) {
        return p;
    }
    if( p ) {
        strcpy(argBuf, dir);
        strcat(argBuf, "/");
        strcat(argBuf, p);
        return argBuf;
    }else{
        return NULL;
    }
}

char* extractTokens(char* szLine, int argNo) {
    static char argBuf[512];
    char* buf;

    argBuf[0] = 0;

    buf = extractToken(szLine, argNo++);

    while (buf != NULL) {
        strcat(argBuf, buf);
        buf = extractToken(szLine, argNo++);
        strcat(argBuf, buf != NULL ? " " : "");
    }

    return argBuf;
}
