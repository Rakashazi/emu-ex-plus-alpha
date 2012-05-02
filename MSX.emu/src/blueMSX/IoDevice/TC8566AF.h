/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/TC8566AF.h,v $
**
** $Revision: 1.6 $
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
#ifndef TC8566AF_H
#define TC8566AF_H

#include "MsxTypes.h"

typedef struct TC8566AF TC8566AF;

TC8566AF* tc8566afCreate();
void tc8566afDestroy(TC8566AF* tc);
void tc8566afReset(TC8566AF* tc);
UInt8 tc8566afReadRegister(TC8566AF* tc, UInt8 reg);
UInt8 tc8566afPeekRegister(TC8566AF* tc, UInt8 reg);
void tc8566afWriteRegister(TC8566AF* tc, UInt8 reg, UInt8 value);
int tc8566afDiskChanged(TC8566AF* tc, int drive);

void tc8566afSaveState(TC8566AF* tc);
void tc8566afLoadState(TC8566AF* tc);

#endif

