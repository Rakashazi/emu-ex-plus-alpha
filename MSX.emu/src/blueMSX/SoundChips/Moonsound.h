/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/Moonsound.h,v $
**
** $Revision: 1.10 $
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
#ifndef MOONSOUND_H
#define MOONSOUND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MsxTypes.h"
#include "AudioMixer.h"
#include "DebugDeviceManager.h"
    
typedef struct Moonsound Moonsound;

/* Constructor and destructor */
Moonsound* moonsoundCreate(Mixer* mixer, void* romData, int romSize, int sramSize);
void moonsoundDestroy(Moonsound* moonsound);
void moonsoundReset(Moonsound* moonsound);
UInt8 moonsoundRead(Moonsound* moonsound, UInt16 ioPort);
UInt8 moonsoundPeek(Moonsound* moonsound, UInt16 ioPort);
void moonsoundWrite(Moonsound* moonsound, UInt16 ioPort, UInt8 value);
void moonsoundLoadState(Moonsound* moonsound);
void moonsoundSaveState(Moonsound* moonsound);
void moonsoundGetDebugInfo(Moonsound* moonsound, DbgDevice* dbgDevice);

#ifdef __cplusplus
}
#endif


#endif

