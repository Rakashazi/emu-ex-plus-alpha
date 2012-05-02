/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/ym2151.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-30 18:38:45 $
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
#ifndef YM2151_H
#define YM2151_H

#include "MsxTypes.h"
#include "AudioMixer.h"
#include "DebugDeviceManager.h"

/* Type definitions */
typedef struct YM2151 YM2151;

/* Constructor and destructor */
YM2151* ym2151Create(Mixer* mixer);
void ym2151Destroy(YM2151* ym2151);
void ym2151Reset(YM2151* ym2151);
void ym2151LoadState(YM2151* ym2151);
void ym2151SaveState(YM2151* ym2151);
UInt8 ym2151Peek(YM2151* ym2151, UInt16 ioPort);
UInt8 ym2151Read(YM2151* ym2151, UInt16 ioPort);
void ym2151Write(YM2151* ym2151, UInt16 ioPort, UInt8 value);
void ym2151SetIrqVector(YM2151* ym2151, UInt8 irqVector);
void ym2151GetDebugInfo(YM2151* ym2151, DbgDevice* dbgDevice);

#endif
