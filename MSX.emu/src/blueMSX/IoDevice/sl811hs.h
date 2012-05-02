/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/sl811hs.h,v $
**
** $Revision: 1.2 $
**
** $Date: 2008-03-30 18:38:41 $
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
#ifndef SL811HS_H
#define SL811HS_H

#include "MsxTypes.h"

typedef struct SL811HS SL811HS;

SL811HS* sl811hsCreate();
void sl811hsDestroy(SL811HS* rtl);
void sl811hsReset(SL811HS* rtl);

UInt8 sl811hsRead(SL811HS* rtl, UInt8 address);
void sl811hsWrite(SL811HS* rtl, UInt8 address, UInt8 value);

void sl811hsSaveState(SL811HS* rtl);
void sl811hsLoadState(SL811HS* rtl);

#endif

