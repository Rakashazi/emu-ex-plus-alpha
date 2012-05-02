/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/GameReader.h,v $
**
** $Revision: 1.4 $
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
#ifndef GAMER_READER_H
#define GAMER_READER_H

#include "MsxTypes.h"

typedef void* GrHandle;

int gameReaderSupported();

GrHandle* gameReaderCreate(int slot);
void gameReaderDestroy(GrHandle* grHandle);

int gameReaderRead(GrHandle* grHandle, UInt16 address, void* buffer, int length);
int gameReaderWrite(GrHandle* grHandle, UInt16 address, void* buffer, int length);

int gameReaderReadIo(GrHandle* grHandle, UInt16 port, UInt8* value);
int gameReaderWriteIo(GrHandle* grHandle, UInt16 port, UInt8 value);

#endif
