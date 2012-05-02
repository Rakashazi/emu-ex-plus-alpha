/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/IoPort.h,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-05-25 14:22:39 $
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
#ifndef IO_PORT_H
#define IO_PORT_H

#include "MsxTypes.h"

typedef UInt8 (*IoPortRead)(void*, UInt16);
typedef void  (*IoPortWrite)(void*, UInt16, UInt8);

void* ioPortGetRef(int port);
void ioPortRegister(int port, IoPortRead read, IoPortWrite write, void* ref);
void ioPortUnregister(int port);

void ioPortRegisterUnused(int idx, IoPortRead read, IoPortWrite write, void* ref);
void ioPortUnregisterUnused(int idx);

void ioPortRegisterSub(int subport, IoPortRead read, IoPortWrite write, void* ref);
void ioPortUnregisterSub(int subport);
int ioPortCheckSub(int subport);

void  ioPortReset();
UInt8 ioPortRead(void* ref, UInt16 port);
void  ioPortWrite(void* ref, UInt16 port, UInt8 value);

#endif
