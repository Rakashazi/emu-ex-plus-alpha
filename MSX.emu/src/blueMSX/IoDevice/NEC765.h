/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/NEC765.h,v $
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
#ifndef NEC765_H
#define NEC765_H

#include "MsxTypes.h"

typedef struct NEC765 NEC765;

NEC765* nec765Create();
void nec765Destroy(NEC765* fdc);
void nec765Reset(NEC765* fdc);

UInt8 nec765Read(NEC765* fdc);
UInt8 nec765Peek(NEC765* fdc);
UInt8 nec765ReadStatus(NEC765* fdc);
UInt8 nec765PeekStatus(NEC765* fdc);
void nec765Write(NEC765* fdc, UInt8 value);

int nec765DiskChanged(NEC765* fdc, int drive);

int nec765GetInt(NEC765* fdc);
int nec765GetIndex(NEC765* fdc);

void nec765SaveState(NEC765* fdc);
void nec765LoadState(NEC765* fdc);

#endif
