/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/UartIO.h,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-31 19:42:20 $
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
#ifndef UART_IO_H
#define UART_IO_H

#include "MsxTypes.h"

typedef struct UartIO UartIO;

typedef enum {UART_NONE, UART_FILE, UART_HOST } UartType;

UartIO* uartIOCreate(void (*recvCallback)(UInt8));
void uartIODestroy(UartIO* uartIO);

void uartIOTransmit(UartIO* uartIO, UInt8 value);
int  uartIOGetStatus(UartIO* uartIO);

void uartIoSetType(UartType type, const char* fileName);

#endif
