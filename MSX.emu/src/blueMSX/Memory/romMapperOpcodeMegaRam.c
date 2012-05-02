/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperOpcodeMegaRam.c,v $
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
#include "romMapperOpcodeMegaRam.h"
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
    UInt8   megaRamLatch[4];

    int     slot;
    int     sslot;
    int     startPage;

    int    deviceHandle;
    int    debugHandle;

    UInt8   megaRam[0x20000];
} RomMapperOpcodeMegaRam;


static void slotUpdate(RomMapperOpcodeMegaRam* rm)
{
    int i;

    for (i = 0; i < 4; i++) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->megaRam + 0x2000 * rm->megaRamLatch[i], 1, 1);
    }
}

static void saveState(RomMapperOpcodeMegaRam* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperOpcodeMegaRam");

    saveStateSet(state, "megaRamLatch0", rm->megaRamLatch[0]);
    saveStateSet(state, "megaRamLatch1", rm->megaRamLatch[1]);
    saveStateSet(state, "megaRamLatch2", rm->megaRamLatch[2]);
    saveStateSet(state, "megaRamLatch3", rm->megaRamLatch[3]);
        
    saveStateSetBuffer(state, "megaRam", rm->megaRam, 0x20000);
    
    saveStateClose(state);
}

static void loadState(RomMapperOpcodeMegaRam* rm)
{
    SaveState* state = saveStateOpenForRead("mapperOpcodeMegaRam");

    rm->megaRamLatch[0] = (UInt8)saveStateGet(state, "megaRamLatch0",  0);
    rm->megaRamLatch[1] = (UInt8)saveStateGet(state, "megaRamLatch1",  0);
    rm->megaRamLatch[2] = (UInt8)saveStateGet(state, "megaRamLatch2",  0);
    rm->megaRamLatch[3] = (UInt8)saveStateGet(state, "megaRamLatch3",  0);
    
    saveStateGetBuffer(state, "megaRam", rm->megaRam, 0x20000);

    saveStateClose(state);

    slotUpdate(rm);
}

static void destroy(RomMapperOpcodeMegaRam* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0x48);
    ioPortUnregister(0x49);
    ioPortUnregister(0x4a);
    ioPortUnregister(0x4b);

    free(rm);
}

static UInt8 peek(RomMapperOpcodeMegaRam* rm, UInt16 ioPort)
{
    return rm->megaRamLatch[ioPort & 3];
}

static UInt8 read(RomMapperOpcodeMegaRam* rm, UInt16 ioPort)
{
    return rm->megaRamLatch[ioPort & 3];
}

static void write(RomMapperOpcodeMegaRam* rm, UInt16 ioPort, UInt8 value)
{
    rm->megaRamLatch[ioPort & 3] = value & 0x0f;
    slotUpdate(rm);
}

static void reset(RomMapperOpcodeMegaRam* rm)
{
    rm->megaRamLatch[0] = 0;
    rm->megaRamLatch[1] = 0;
    rm->megaRamLatch[2] = 0;
    rm->megaRamLatch[3] = 0;

    slotUpdate(rm);
}

static void getDebugInfo(RomMapperOpcodeMegaRam* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "MEGARAM", 4);

    dbgIoPortsAddPort(ioPorts, 0, 0x48, DBG_IO_READWRITE, peek(rm, 0x48));
    dbgIoPortsAddPort(ioPorts, 1, 0x49, DBG_IO_READWRITE, peek(rm, 0x49));
    dbgIoPortsAddPort(ioPorts, 2, 0x4a, DBG_IO_READWRITE, peek(rm, 0x4a));
    dbgIoPortsAddPort(ioPorts, 3, 0x4b, DBG_IO_READWRITE, peek(rm, 0x4b));
    
    dbgDeviceAddMemoryBlock(dbgDevice, "Mega Ram", 0, 0, sizeof(rm->megaRam), rm->megaRam);
}

int romMapperOpcodeMegaRamCreate(int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    
    RomMapperOpcodeMegaRam* rm = malloc(sizeof(RomMapperOpcodeMegaRam));
    
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    
    memset(rm->megaRam, 0xff, sizeof(rm->megaRam));

    rm->deviceHandle = deviceManagerRegister(ROM_OPCODEMEGA, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, "MEGARAM", &dbgCallbacks, rm);

    ioPortRegister(0x48, read, write, rm);
    ioPortRegister(0x49, read, write, rm);
    ioPortRegister(0x4a, read, write, rm);
    ioPortRegister(0x4b, read, write, rm);

    reset(rm);

    return 1;
}

