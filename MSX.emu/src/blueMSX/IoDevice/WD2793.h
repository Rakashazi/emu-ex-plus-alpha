/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/WD2793.h,v $
**
** $Revision: 1.10 $
**
** $Date: 2008-03-30 18:38:41 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Ricardo Bittencourt
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
#ifndef WD2793_H
#define WD2793_H

#include "MsxTypes.h"

typedef enum
{
        FDC_TYPE_WD1772,
        FDC_TYPE_WD1793,
        FDC_TYPE_WD2793
} Wd2793FdcType;

typedef struct WD2793 WD2793;

WD2793* wd2793Create(Wd2793FdcType type);
void    wd2793Destroy(WD2793* tc);
void    wd2793Reset(WD2793* tc);

void    wd2793LoadState(WD2793* tc);
void    wd2793SaveState(WD2793* tc);

int     wd2793GetSide(WD2793* wd);
void    wd2793SetSide(WD2793* wd, int side);
int     wd2793GetDrive(WD2793* wd);
void    wd2793SetDrive(WD2793* wd, int drive);
void    wd2793SetMotor(WD2793* wd, int drive);
int     wd2793DiskChanged(WD2793* wd, int drive);
void    wd2793SetCommandReg(WD2793* wd, UInt8 value);
UInt8   wd2793PeekStatusReg(WD2793* wd);
UInt8   wd2793GetStatusReg(WD2793* wd);
void    wd2793SetDataReg(WD2793* wd, UInt8 value);
UInt8   wd2793PeekDataReg(WD2793* wd);
UInt8   wd2793GetDataReg(WD2793* wd);
UInt8   wd2793PeekSectorReg(WD2793* wd);
UInt8   wd2793GetSectorReg(WD2793* wd);
void    wd2793SetSectorReg(WD2793* wd, UInt8 value);
UInt8   wd2793PeekTrackReg(WD2793* wd);
UInt8   wd2793GetTrackReg(WD2793* wd);
void    wd2793SetTrackReg(WD2793* wd, UInt8 value);
int     wd2793PeekIrq(WD2793* wd);
int     wd2793GetIrq(WD2793* wd);
int     wd2793PeekDataRequest(WD2793* wd);
int     wd2793GetDataRequest(WD2793* wd);
void    wd2793SetSide(WD2793* wd, int side);
void    wd2793SetDensity(WD2793* wd, int density);

#endif

