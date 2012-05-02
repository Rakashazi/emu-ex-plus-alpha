/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/wd33c93.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-30 18:38:41 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, Ricardo Bittencourt, white cat
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
#ifndef WD33C93_H
#define WD33C93_H

#include "MsxTypes.h"

typedef struct WD33C93 WD33C93;

WD33C93* wd33c93Create(int hdId);
void	 wd33c93Reset(WD33C93* wd33c93, int scsireset);
void     wd33c93Destroy(WD33C93* wd33c93);

UInt8    wd33c93ReadAuxStatus(WD33C93* wd33c93, UInt16 port);
UInt8    wd33c93ReadCtrl(WD33C93* wd33c93, UInt16 port);
UInt8    wd33c93Peek(WD33C93* wd33c93, UInt16 port);
void     wd33c93WriteAdr(WD33C93* wd33c93, UInt16 port, UInt8 value);
void     wd33c93WriteCtrl(WD33C93* wd33c93, UInt16 port, UInt8 value);

void     wd33c93LoadState(WD33C93* wd33c93);
void     wd33c93SaveState(WD33C93* wd33c93);

#endif
