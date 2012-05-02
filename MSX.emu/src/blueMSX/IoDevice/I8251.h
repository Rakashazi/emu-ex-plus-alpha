/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8251.h,v $
**
** $Revision: 1.7 $
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
#ifndef I8251_H
#define I8251_H

#include "MsxTypes.h"

typedef int  (*I8251Transmit) (void*, UInt8);
typedef int  (*I8251Signal) (void*);
typedef void (*I8251Set) (void*, int);
typedef int  (*I8251Get) (void*);

typedef enum {
    I8251_STOP_INV = 0,
    I8251_STOP_1 = 2,
    I8251_STOP_15 = 3,
    I8251_STOP_2 = 4
} I8251StopBits;

typedef enum {
    I8251_PARITY_NONE,
    I8251_PARITY_EVEN,
    I8251_PARITY_ODD
} I8251Parity;

typedef struct I8251 I8251;

UInt8 i8251Peek(I8251* usart, UInt16 port);
UInt8 i8251Read(I8251* usart, UInt16 port);
void i8251Write(I8251* usart, UInt16 port, UInt8 value);

void i8251RxData(I8251* usart, UInt8 value);

void i8251LoadState(I8251* usart);
void i8251SaveState(I8251* usart);

void i8251Reset(I8251* usart);
void i8251Destroy(I8251* usart); 

I8251* i8251Create(I8251Transmit transmit,    I8251Signal   signal,
                   I8251Set      setDataBits, I8251Set      setStopBits,
                   I8251Set      setParity,   I8251Set      setRxReady,
                   I8251Set      setDtr,      I8251Set      setRts,
                   I8251Get      getDtr,      I8251Get      getRts,
                   void* ref);

#endif
