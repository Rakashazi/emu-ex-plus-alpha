/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8250.h,v $
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
#ifndef I8250_H
#define I8250_H

#include "MsxTypes.h"

typedef int  (*I8250Transmit) (void*, UInt8);
typedef int  (*I8250Signal) (void*);
typedef void (*I8250Set) (void*, int);
typedef int  (*I8250Get) (void*);


typedef struct I8250 I8250;

UInt8 i8250Read(I8250* i8250, UInt16 port);
void i8250Write(I8250* i8250, UInt16 port, UInt8 value);

void i8250Receive(I8250* i8250, UInt8 value);
void i8250RxData(I8250* uart, UInt8 value);

void i8250LoadState(I8250* i8250);
void i8250SaveState(I8250* i8250);

void i8250Reset(I8250* i8250);
void i8250Destroy(I8250* i8250); 

I8250* i8250Create(UInt32 frequency, I8250Transmit transmit,    I8250Signal   signal,
                   I8250Set      setDataBits, I8250Set      setStopBits,
                   I8250Set      setParity,   I8250Set      setRxReady,
                   I8250Set      setDtr,      I8250Set      setRts,
                   I8250Get      getDtr,      I8250Get      getRts,
                   void* ref);

#endif
