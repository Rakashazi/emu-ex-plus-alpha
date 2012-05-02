/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/YM2413.h,v $
**
** $Revision: 1.8 $
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
#ifndef YM2413_H
#define YM2413_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MsxTypes.h"
#include "AudioMixer.h"
#include "DebugDeviceManager.h"

/* Type definitions */
typedef struct YM_2413 YM_2413;

/* Constructor and destructor */
YM_2413* ym2413Create(Mixer* mixer);
void ym2413Destroy(YM_2413* ym2413);
void ym2413WriteAddress(YM_2413* ym2413, UInt8 address);
void ym2413WriteData(YM_2413* ym2413, UInt8 data);
void ym2413Reset(YM_2413* ref);
void ym2413SaveState(YM_2413* ref);
void ym2413LoadState(YM_2413* ref);
void ym2413GetDebugInfo(YM_2413* ym2413, DbgDevice* dbgDevice);

#ifdef __cplusplus
}
#endif

#endif

