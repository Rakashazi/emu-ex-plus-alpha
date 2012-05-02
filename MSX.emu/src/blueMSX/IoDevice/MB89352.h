/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/MB89352.h,v $
**
** $Revision: 1.4 $
**
** $Date: 2007-03-28 17:35:35 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
#ifndef MB89352_H
#define MB89352_H

#include "MsxTypes.h"

typedef struct MB89352 MB89352;

MB89352* mb89352Create(int hdId);
void mb89352Destroy(MB89352* spc);
void mb89352Reset(MB89352* spc, int scsireset);
void mb89352SaveState(MB89352* spc);
void mb89352LoadState(MB89352* spc);
UInt8 mb89352ReadRegister(MB89352* spc, UInt8 reg);
UInt8 mb89352PeekRegister(MB89352* spc, UInt8 reg);
UInt8 mb89352ReadDREG(MB89352* spc);
void mb89352WriteRegister(MB89352* spc, UInt8 reg, UInt8 value);
void mb89352WriteDREG(MB89352* spc, UInt8 value);

#endif
