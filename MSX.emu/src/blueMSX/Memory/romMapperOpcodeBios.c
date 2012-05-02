/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperOpcodeBios.c,v $
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
#include "romMapperOpcodeBios.h"
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
    UInt8   biosLatch;

    int     slot;
    int     sslot;
    int     startPage;

    int    deviceHandle;
    int    debugHandle;

    UInt8   biosRom[0x20000];
} RomMapperOpcodeBios;


static void saveState(RomMapperOpcodeBios* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperOpcodeBios");
    saveStateSet(state, "biosLatch",  rm->biosLatch);
    saveStateClose(state);
}

static void loadState(RomMapperOpcodeBios* rm)
{
    SaveState* state = saveStateOpenForRead("mapperOpcodeBios");
    rm->biosLatch       = (UInt8)saveStateGet(state, "biosLatch",  0);
    saveStateClose(state);

    slotMapPage(rm->slot, rm->sslot, 0, rm->biosRom + rm->biosLatch * 0x2000, 1, 0);
}

static void destroy(RomMapperOpcodeBios* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0x40);

    free(rm);
}

static UInt8 peek(RomMapperOpcodeBios* rm, UInt16 ioPort)
{
    return rm->biosLatch;
}

static UInt8 read(RomMapperOpcodeBios* rm, UInt16 ioPort)
{
    return rm->biosLatch;
}

static void write(RomMapperOpcodeBios* rm, UInt16 ioPort, UInt8 value)
{
    rm->biosLatch = value & 0x03;
    slotMapPage(rm->slot, rm->sslot, 0, rm->biosRom + rm->biosLatch * 0x2000, 1, 0);
}

static void reset(RomMapperOpcodeBios* rm)
{
    rm->biosLatch       = 0;
    slotMapPage(rm->slot, rm->sslot, 0, rm->biosRom + rm->biosLatch * 0x2000, 1, 0);
}

static void getDebugInfo(RomMapperOpcodeBios* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "BIOS", 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x40, DBG_IO_READWRITE, peek(rm, 0x40));

    dbgDeviceAddMemoryBlock(dbgDevice, "BIOS", 0, 0, sizeof(rm->biosRom), rm->biosRom);
}

int romMapperOpcodeBiosCreate(const char* filename, UInt8* romData,
                              int size, int slot, int sslot, 
                              int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    
    RomMapperOpcodeBios* rm = malloc(sizeof(RomMapperOpcodeBios));
    
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    
    memset(rm->biosRom, 0xff, sizeof(rm->biosRom));

    if (romData != NULL) {
        if (size > sizeof(rm->biosRom)) {
            size = sizeof(rm->biosRom);
        }
        memcpy(rm->biosRom, romData, size);
    }

    rm->deviceHandle = deviceManagerRegister(ROM_OPCODEBIOS, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, "BIOS", &dbgCallbacks, rm);

    ioPortRegister(0x40, read, write, rm);

    reset(rm);

    return 1;
}

