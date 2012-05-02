/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMoonsound.c,v $
**
** $Revision: 1.9 $
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
#include "romMapperMoonsound.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "IoPort.h"
#include "Moonsound.h"
#include "Board.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int      deviceHandle;
    int      debugHandle;
    Moonsound* moonsound;
} RomMapperMoonsound;

static void destroy(RomMapperMoonsound* rm)
{
    ioPortUnregister(0x7e);
    ioPortUnregister(0x7f);
    ioPortUnregister(0xc4);
    ioPortUnregister(0xc5);
    ioPortUnregister(0xc6);
    ioPortUnregister(0xc7);

    if (rm->moonsound != NULL) {
        moonsoundDestroy(rm->moonsound);
    }

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm);
}

static void reset(RomMapperMoonsound* rm) 
{
    if (rm->moonsound != NULL) {
        moonsoundReset(rm->moonsound);
    }
}

static void loadState(RomMapperMoonsound* rm)
{
    SaveState* state = saveStateOpenForRead("RomMapperMoonsound");

    saveStateClose(state);
    
    if (rm->moonsound != NULL) {
        moonsoundLoadState(rm->moonsound);
    }
}

static void saveState(RomMapperMoonsound* rm)
{
    SaveState* state = saveStateOpenForWrite("RomMapperMoonsound");

    saveStateClose(state);

    if (rm->moonsound != NULL) {
        moonsoundSaveState(rm->moonsound);
    }
}

static UInt8 read(RomMapperMoonsound* rm, UInt16 ioPort)
{
    return moonsoundRead(rm->moonsound, ioPort);
}

static UInt8 peek(RomMapperMoonsound* rm, UInt16 ioPort)
{
    return moonsoundPeek(rm->moonsound, ioPort);
}

static void write(RomMapperMoonsound* rm, UInt16 ioPort, UInt8 data)
{
    moonsoundWrite(rm->moonsound, ioPort, data);
}

static void getDebugInfo(RomMapperMoonsound* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    if (rm->moonsound == NULL) {
        return;
    }

    moonsoundGetDebugInfo(rm->moonsound, dbgDevice);

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMoonsound(), 6);
    dbgIoPortsAddPort(ioPorts, 0, 0x7e, DBG_IO_READWRITE, peek(rm, 0x7e));
    dbgIoPortsAddPort(ioPorts, 1, 0x7f, DBG_IO_READWRITE, peek(rm, 0x7f));
    dbgIoPortsAddPort(ioPorts, 2, 0xc4, DBG_IO_READWRITE, peek(rm, 0xc4));
    dbgIoPortsAddPort(ioPorts, 3, 0xc5, DBG_IO_READWRITE, peek(rm, 0xc5));
    dbgIoPortsAddPort(ioPorts, 4, 0xc6, DBG_IO_READWRITE, peek(rm, 0xc6));
    dbgIoPortsAddPort(ioPorts, 5, 0xc7, DBG_IO_READWRITE, peek(rm, 0xc7));
}

int romMapperMoonsoundCreate(const char* filename, UInt8* romData, int size, int sramSize)
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperMoonsound* rm = malloc(sizeof(RomMapperMoonsound));

    rm->deviceHandle = deviceManagerRegister(ROM_MOONSOUND, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevMoonsound(), &dbgCallbacks, rm);
    
    rm->moonsound = NULL;

    if (boardGetMoonsoundEnable()) {
        rm->moonsound = moonsoundCreate(boardGetMixer(), romData, size, sramSize);
        ioPortRegister(0x7e, read, write, rm);
        ioPortRegister(0x7f, read, write, rm);
        ioPortRegister(0xc4, read, write, rm);
        ioPortRegister(0xc5, read, write, rm);
        ioPortRegister(0xc6, read, write, rm);
        ioPortRegister(0xc7, read, write, rm);
    }
    else {
        // moonsound emulation has ownership of rom data. Need to
        // free it if its not being used.
        free(romData);
    }

    reset(rm);

    return 1;
}
