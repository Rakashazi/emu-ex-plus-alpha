/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperOpcodeSlotManager.c,v $
**
** $Revision: 1.1 $
**
** $Date: 2008-11-23 20:26:12 $
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
#include "romMapperOpcodeSlotManager.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "SaveState.h"
#include "AY8910.h"
#include "Board.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    UInt8   slotSelect;

    int    deviceHandle;
    int    debugHandle;
} RomMapperOpcodeSlotManager;


static void slotUpdate(RomMapperOpcodeSlotManager* rm)
{
    int i;

    for (i = 0; i < 4; i++) {
        slotMapRamPage((rm->slotSelect >> (2 * i)) & 3, 0, 4 + i);
    }
}

static void saveState(RomMapperOpcodeSlotManager* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperOpcodeSlotManager");
    saveStateSet(state, "slotSelect", rm->slotSelect);
    saveStateClose(state);
}

static void loadState(RomMapperOpcodeSlotManager* rm)
{
    SaveState* state = saveStateOpenForRead("mapperOpcodeSlotManager");
    rm->slotSelect      = (UInt8)saveStateGet(state, "slotSelect",  0);
    saveStateClose(state);
}

static void destroy(RomMapperOpcodeSlotManager* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0x41);

    free(rm);
}

static UInt8 peek(RomMapperOpcodeSlotManager* rm, UInt16 ioPort)
{
    return rm->slotSelect;
}

static UInt8 read(RomMapperOpcodeSlotManager* rm, UInt16 ioPort)
{
    return rm->slotSelect;
}

static void write(RomMapperOpcodeSlotManager* rm, UInt16 ioPort, UInt8 value)
{
    rm->slotSelect = value;
    slotUpdate(rm);
}

static void reset(RomMapperOpcodeSlotManager* rm)
{
    rm->slotSelect = 0;
    slotUpdate(rm);
}

static void getDebugInfo(RomMapperOpcodeSlotManager* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "SLOTSELECT", 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x41, DBG_IO_READWRITE, peek(rm, 0x41));
}

int romMapperOpcodeSlotManagerCreate() 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    
    RomMapperOpcodeSlotManager* rm = malloc(sizeof(RomMapperOpcodeSlotManager));

    rm->deviceHandle = deviceManagerRegister(ROM_OPCODESLOT, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, "SLOTSELECT", &dbgCallbacks, rm);

    ioPortRegister(0x41, read, write, rm);

    reset(rm);

    return 1;
}

