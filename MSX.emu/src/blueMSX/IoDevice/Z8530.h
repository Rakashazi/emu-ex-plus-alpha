/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Z8530.h,v $
**
** $Revision: 1.1 $
**
** $Date: 2009-04-29 00:05:05 $
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
#ifndef Z8530_H
#define Z8530_H

#include "MsxTypes.h"

typedef struct Z8530 Z8530;

UInt8 z8530Read(Z8530* z8530, UInt16 port);
void z8530Write(Z8530* z8530, UInt16 port, UInt8 value);

void z8530LoadState(Z8530* z8530);
void z8530SaveState(Z8530* z8530);

void z8530Reset(Z8530* z8530);
void z8530Destroy(Z8530* z8530); 

Z8530* z8530Create(void* ref);

#endif
