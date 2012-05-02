/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/SunriseIDE.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:40 $
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
#ifndef SUNRISE_IDE_H
#define SUNRISE_IDE_H

#include "MsxTypes.h"

typedef struct SunriseIde SunriseIde;

SunriseIde* sunriseIdeCreate(int hdId);
void sunriseIdeDestroy(SunriseIde* ide);

void sunriseIdeReset(SunriseIde* ide);

UInt16 sunriseIdeRead(SunriseIde* ide);
UInt16 sunriseIdePeek(SunriseIde* ide);
void sunriseIdeWrite(SunriseIde* ide, UInt16 value);

UInt8 sunriseIdeReadRegister(SunriseIde* ide, UInt8 reg);
UInt8 sunriseIdePeekRegister(SunriseIde* ide, UInt8 reg);
void sunriseIdeWriteRegister(SunriseIde* ide, UInt8 reg, UInt8 value);

void sunriseIdeSaveState(SunriseIde* ide);
void sunriseIdeLoadState(SunriseIde* ide);

#endif

