/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/rtl8019.h,v $
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
#ifndef RTL8019_H
#define RTL8019_H

#include "MsxTypes.h"

typedef struct RTL8019 RTL8019;

RTL8019* rtl8019Create();
void rtl8019Destroy(RTL8019* rtl);
void rtl8019Reset(RTL8019* rtl);

UInt8 rtl8019Read(RTL8019* rtl, UInt8 address);
void rtl8019Write(RTL8019* rtl, UInt8 address, UInt8 value);

void rtl8019SaveState(RTL8019* rtl);
void rtl8019LoadState(RTL8019* rtl);

#endif

