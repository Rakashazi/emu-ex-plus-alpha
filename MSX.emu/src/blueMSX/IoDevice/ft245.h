/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/ft245.h,v $
**
** $Revision: 1.3 $
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
#ifndef FT245_H
#define FT245_H

#include "MsxTypes.h"

typedef struct FT245 FT245;

FT245* ft245Create(int driveId);
void ft245Destroy(FT245* ft);
void ft245Reset(FT245* ft);

UInt8 ft245Read(FT245* ft);
UInt8 ft245Peek(FT245* ft);
void ft245Write(FT245* ft, UInt8 value);

UInt8 ft245GetTxe(FT245* ft);
UInt8 ft245GetRxf(FT245* ft);

void ft245SaveState(FT245* ft);
void ft245LoadState(FT245* ft);

#endif

