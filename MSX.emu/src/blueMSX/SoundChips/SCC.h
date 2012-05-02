/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SCC.h,v $
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
#ifndef SCC_H
#define SCC_H

#include <stdio.h>

#include "MsxTypes.h"
#include "AudioMixer.h"
#include "DebugDeviceManager.h"

/* Type definitions */
typedef struct SCC SCC;

typedef enum { SCC_NONE = 0, SCC_REAL, SCC_COMPATIBLE, SCC_PLUS } SccMode;

/* Constructor and destructor */
SCC* sccCreate(Mixer* mixer);
void sccDestroy(SCC* scc);
void sccReset(SCC* scc);
void sccSetMode(SCC* scc, SccMode newMode);

/* Register read/write methods */
UInt8 sccRead(SCC* scc, UInt8 address);
UInt8 sccPeek(SCC* scc, UInt8 address);
void sccWrite(SCC* scc, UInt8 address, UInt8 value);

void sccGetDebugInfo(SCC* scc, DbgDevice* dbgDevice);

void sccLoadState(SCC* scc);
void sccSaveState(SCC* scc);

#endif

