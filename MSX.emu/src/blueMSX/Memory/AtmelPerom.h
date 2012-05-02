/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/AtmelPerom.h,v $
**
** $Revision: 1.1 $
**
** $Date: 2009-03-30 14:28:20 $
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
#ifndef ATMEL_PEROM_H
#define ATMEL_PEROM_H
 
#include "MsxTypes.h"

typedef struct AtmelPerom AtmelPerom;

typedef enum { AMD_TYPE_1, AMD_TYPE_2 } AmdType;


AtmelPerom* atmelPeromCreate(AmdType type, int flashSize, int sectorSize, UInt32 writeProtectMask, 
                         void* romData, int size, const char* sramFilename, int loadSram);
void atmelPeromDestroy(AtmelPerom* rm);

int atmelPeromCmdInProgress(AtmelPerom* rm);

UInt8 atmelPeromRead(AtmelPerom* rm, UInt32 address);
void atmelPeromWrite(AtmelPerom* rm, UInt32 address, UInt8 value);
UInt8* atmelPeromGetPage(AtmelPerom* rm, UInt32 address);
void atmelPeromReset(AtmelPerom* rm);
void atmelPeromSaveState(AtmelPerom* rm);
void atmelPeromLoadState(AtmelPerom* rm);

#endif
