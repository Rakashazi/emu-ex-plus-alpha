/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8254.h,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-31 19:42:19 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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
#ifndef I8254_H
#define I8254_H

#include "MsxTypes.h"

typedef void (*I8254Out) (void*, int);

typedef enum { I8254_COUNTER_1, I8254_COUNTER_2, I8254_COUNTER_3 } I8254Counter;

typedef struct I8254 I8254;

UInt8 i8254Peek(I8254* i8254, UInt16 port);
UInt8 i8254Read(I8254* i8254, UInt16 port);
void i8254Write(I8254* i8254, UInt16 port, UInt8 value);

void i8254LoadState(I8254* i8254);
void i8254SaveState(I8254* i8254);

void i8254Reset(I8254* i8254);
void i8254Destroy(I8254* i8254); 

void i8254SetGate(I8254* i8254, I8254Counter counter, int state);

UInt32 i8254GetFrequency(I8254* i8254, I8254Counter counter);

I8254* i8254Create(UInt32 frequency, I8254Out out1, I8254Out out2, I8254Out out3, void* ref);

#endif
