/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/Y8950.h,v $
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
#ifndef Y8950_H
#define Y8950_H

#include "MsxTypes.h"
#include "AudioMixer.h"
#include "DebugDeviceManager.h"

/* Type definitions */
typedef struct Y8950 Y8950;

/* Constructor and destructor */
Y8950* y8950Create(Mixer* mixer);
void y8950Destroy(Y8950* y8950);
void y8950Reset(Y8950* y8950);
void y8950LoadState(Y8950* y8950);
void y8950SaveState(Y8950* y8950);
UInt8 y8950Peek(Y8950* y8950, UInt16 ioPort);
UInt8 y8950Read(Y8950* y8950, UInt16 ioPort);
void y8950Write(Y8950* y8950, UInt16 ioPort, UInt8 value);
void y8950GetDebugInfo(Y8950* y8950, DbgDevice* dbgDevice);

#endif
