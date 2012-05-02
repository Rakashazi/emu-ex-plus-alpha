/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/sramMapperMegaSCSI.h,v $
**
** $Revision: 1.2 $
**
** $Date: 2007-02-21 16:25:24 $
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
#ifndef SRAMMAPPER_MEGASCSI_H
#define SRAMMAPPER_MEGASCSI_H

#include "MsxTypes.h"

int sramMapperMegaSCSICreate(const char* filename, UInt8* buf, int size,
                int pSlot, int sSlot, int startPage, int hdId, int flag);

#endif
