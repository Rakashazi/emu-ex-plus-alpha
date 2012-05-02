/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8255.h,v $
**
** $Revision: 1.7 $
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
#ifndef I8255_H
#define I8255_H

#include "MsxTypes.h"

typedef struct I8255 I8255;

typedef void  (*I8255Write)(void*, UInt8);
typedef UInt8 (*I8255Read) (void*);

I8255* i8255Create(I8255Read peekA,   I8255Read readA,   I8255Write writeA, 
                   I8255Read peekB,   I8255Read readB,   I8255Write writeB,
                   I8255Read peekCLo, I8255Read readCLo, I8255Write writeCLo,
                   I8255Read peekCHi, I8255Read readCHi, I8255Write writeCHi,
                   void* ref);
void i8255Destroy(I8255* i8255); 
void i8255Reset(I8255* i8255);
UInt8 i8255Peek(I8255* i8255, UInt16 port);
UInt8 i8255Read(I8255* i8255, UInt16 port);
void i8255Write(I8255* i8255, UInt16 port, UInt8 value);
void i8255LoadState(I8255* i8255);
void i8255SaveState(I8255* i8255);

#endif


#if 0

/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8255.h,v $
**
** $Revision: 1.7 $
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
#ifndef I8255_H
#define I8255_H

#include "MsxTypes.h"

typedef struct I8255 I8255;

typedef void  (*I8255Write)(void*, UInt8);
typedef UInt8 (*I8255Read) (void*);

I8255* i8255Create(I8255Read peekA,   I8255Read readA,   I8255Write writeA, 
                   I8255Read peekB,   I8255Read readB,   I8255Write writeB,
                   I8255Read peekCLo, I8255Read readCLo, I8255Write writeCLo,
                   I8255Read peekCHi, I8255Read readCHi, I8255Write writeCHi,
                   void* ref, int useInputLatch);
void i8255Destroy(I8255* i8255); 
void i8255Reset(I8255* i8255);
UInt8 i8255Peek(I8255* i8255, UInt16 port);
UInt8 i8255Read(I8255* i8255, UInt16 port);
void i8255Write(I8255* i8255, UInt16 port, UInt8 value);

void i8255WriteLatchA(I8255* i8255, UInt8 value);
void i8255WriteLatchB(I8255* i8255, UInt8 value);
void i8255WriteLatchC(I8255* i8255, UInt8 value);

void i8255LoadState(I8255* i8255);
void i8255SaveState(I8255* i8255);

#endif
#endif