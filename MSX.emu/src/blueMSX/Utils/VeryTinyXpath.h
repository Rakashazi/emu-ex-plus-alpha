/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Utils/VeryTinyXpath.h,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:47 $
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
#ifndef VERY_TINY_XPATH_H
#define VERY_TINY_XPATH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct VtXpath VtXpath;

#define VTXPATH_INT_NOT_FOUND 0x789abcde

VtXpath* vtXpathOpenForRead(const char* fileName);
VtXpath* vtXpathOpenForWrite(const char* fileName);
void vtXpathClose(VtXpath* xpath);

int vtXpathGetInt(VtXpath* xpath, int numLevels, const char* first, ...);
const char* vtXpathGetString(VtXpath* xpath, int numLevels, const char* first, ...);

void vtXpathSetInt(VtXpath* xpath, int value, int numLevels, const char* first, ...);
void vtXpathSetString(VtXpath* xpath, const char* value, int numLevels, const char* first, ...);

#ifdef __cplusplus
}
#endif

#endif // VERY_TINY_XPATH_H