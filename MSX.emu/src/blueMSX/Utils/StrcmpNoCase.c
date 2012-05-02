/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Utils/StrcmpNoCase.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 21:38:43 $
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
#include "ctype.h"
#include <string.h>
#include <stdlib.h>

int strcmpnocase(const char* str1, const char* str2) {
    char s1[128];
    char s2[128];
    int i;

    memset(s1, 0, 128);
    memset(s2, 0, 128);

    for (i = 0; str1[i]; i++) s1[i] = toupper(str1[i]);
    for (i = 0; str2[i]; i++) s2[i] = toupper(str2[i]);

    return strcmp(s1, s2);
}