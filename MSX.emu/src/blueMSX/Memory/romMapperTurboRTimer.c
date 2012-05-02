/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperTurboRTimer.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:44 $
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
#include "romMapperTurboRTimer.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt32 counter;
    UInt32 refTime;
    UInt32 refFrag;
} RomMapperTurboRTimer;

static RomMapperTurboRTimer* theTimer = NULL;

static void saveState(RomMapperTurboRTimer* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperTurboRTimer");

    saveStateSet(state, "counter", rm->counter);
    saveStateSet(state, "refTime", rm->refTime);
    saveStateSet(state, "refFrag", rm->refFrag);
    
    saveStateClose(state);
}

static void loadState(RomMapperTurboRTimer* rm)
{
    SaveState* state = saveStateOpenForRead("mapperTurboRTimer");

    rm->counter = saveStateGet(state, "counter", 0);
    rm->refTime = saveStateGet(state, "refTime", boardSystemTime());
    rm->refFrag = saveStateGet(state, "refFrag", 0);

    saveStateClose(state);
}

static void destroy(RomMapperTurboRTimer* rm)
{
    deviceManagerUnregister(rm->deviceHandle);

    ioPortUnregister(0xe6);
    ioPortUnregister(0xe7);

    free(rm);

    theTimer = NULL;
}

void romMapperTurboRTimerSync()
{
    if (theTimer != NULL) {
        UInt32 systemTime = boardSystemTime();
        UInt64 elapsed = 255682 * (UInt64)(systemTime - theTimer->refTime) + theTimer->refFrag;
        theTimer->refTime = systemTime;
        theTimer->refFrag = (UInt32)(elapsed % boardFrequency());
        theTimer->counter += (UInt32)(elapsed / boardFrequency());
    }
}

static UInt8 read(RomMapperTurboRTimer* rm, UInt16 ioPort)
{
    romMapperTurboRTimerSync();
    
    switch (ioPort & 0x01) {
	case 0:
		return (UInt8)(rm->counter & 0xff);
	case 1:
		return (UInt8)((rm->counter >> 8) & 0xff);
    }

    return 0xff;
}

static void write(RomMapperTurboRTimer* rm, UInt16 ioPort, UInt8 value)
{
    theTimer->counter = 0;
    rm->refTime = boardSystemTime();
    rm->refFrag = 0;
}

int romMapperTurboRTimerCreate() 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperTurboRTimer* rm = malloc(sizeof(RomMapperTurboRTimer));

    rm->deviceHandle = deviceManagerRegister(ROM_TURBORTIMER, &callbacks, rm);

    rm->refTime = boardSystemTime();
    rm->refFrag = 0;
    rm->counter = 0;

    ioPortRegister(0xe6, read, write, rm);
    ioPortRegister(0xe7, read, write, rm);

    theTimer = rm;

    return 1;
}

