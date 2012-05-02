/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/ramMapper.c,v $
**
** $Revision: 1.21 $
**
** $Date: 2009-07-03 21:27:14 $
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
#include "ramMapper.h"
#include "romMapperDRAM.h"
#include "ramMapperIo.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* ramData;
    int handle;
    int debugHandle;
    int dramHandle;
    int dramMode;
    int slot;
    int sslot;
    int mask;
    int size;
} RamMapper;

static void writeIo(RamMapper* rm, UInt16 page, UInt8 value);

static void saveState(RamMapper* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperRam");
    
    saveStateSet(state, "mask",     rm->mask);
    saveStateSet(state, "dramMode", rm->dramMode);
    
    saveStateSetBuffer(state, "ramData", rm->ramData, 0x4000 * (rm->mask + 1));

    saveStateClose(state);
}

static void loadState(RamMapper* rm)
{
    SaveState* state = saveStateOpenForRead("mapperRam");
    int i;
    
    rm->mask     = saveStateGet(state, "mask", 0);
    rm->dramMode = saveStateGet(state, "dramMode", 0);
    
    saveStateGetBuffer(state, "ramData", rm->ramData, 0x4000 * (rm->mask + 1));

    saveStateClose(state);
    
    ramMapperIoRemove(rm->handle);
    rm->handle  = ramMapperIoAdd(0x4000 * (rm->mask + 1), writeIo, rm);

    for (i = 0; i < 4; i++) {
        int value = ramMapperIoGetPortValue(i) & rm->mask;
        int mapped = rm->dramMode && (rm->mask - value < 4) ? 0 : 1;
        slotMapPage(rm->slot, rm->sslot, 2 * i,     rm->ramData + 0x4000 * value, 1, mapped);
        slotMapPage(rm->slot, rm->sslot, 2 * i + 1, rm->ramData + 0x4000 * value + 0x2000, 1, mapped);
    }
}

static void writeIo(RamMapper* rm, UInt16 page, UInt8 value)
{
    int baseAddr = 0x4000 * (value & rm->mask);
    if (rm->dramMode && baseAddr >= (rm->size - 0x10000)) {
        slotMapPage(rm->slot, rm->sslot, 2 * page,     NULL, 0, 0);
        slotMapPage(rm->slot, rm->sslot, 2 * page + 1, NULL, 0, 0);
    }
    else {
        slotMapPage(rm->slot, rm->sslot, 2 * page,     rm->ramData + baseAddr, 1, 1);
        slotMapPage(rm->slot, rm->sslot, 2 * page + 1, rm->ramData + baseAddr + 0x2000, 1, 1);
    }
}

static void setDram(RamMapper* rm, int enable)
{
    int i;

    rm->dramMode = enable;

    for (i = 0; i < 4; i++) {
        writeIo(rm, i, ramMapperIoGetPortValue(i));
    }
}

static void reset(RamMapper* rm)
{
    setDram(rm, 0);
}

static void destroy(RamMapper* rm)
{
    debugDeviceUnregister(rm->debugHandle);
    ramMapperIoRemove(rm->handle);
    slotUnregister(rm->slot, rm->sslot, 0);
    deviceManagerUnregister(rm->deviceHandle);
    panasonicDramUnregister(rm->dramHandle);
    free(rm->ramData);

    free(rm);
}

static void getDebugInfo(RamMapper* rm, DbgDevice* dbgDevice)
{
    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemRamMapped(), 0, 0, rm->size, rm->ramData);
}

static int dbgWriteMemory(RamMapper* rm, char* name, void* data, int start, int size)
{
    if (strcmp(name, "Mapped") || start + size > rm->size) {
        return 0;
    }

    memcpy(rm->ramData + start, data, size);

    return 1;
}

int ramMapperCreate(int size, int slot, int sslot, int startPage, UInt8** ramPtr, UInt32* ramSize) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, NULL, NULL };
    RamMapper* rm;
    int pages = size / 0x4000;
    int i;

    // Check that memory is a power of 2 and at least 64kB
    for (i = 4; i < pages; i <<= 1);

    if (i != pages) {
        return 0;
    }

    size = pages * 0x4000;

    // Start page must be zero (only full slot allowed)
    if (startPage != 0) {
        return 0;
    }

    rm = malloc(sizeof(RamMapper));

    rm->ramData  = malloc(size);
    rm->size     = size;
    rm->slot     = slot;
    rm->sslot    = sslot;
    rm->mask     = pages - 1;
    rm->dramMode = 0;

    memset(rm->ramData, 0xff, size);

    rm->handle  = ramMapperIoAdd(pages * 0x4000, writeIo, rm);
    
    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, langDbgDevRam(), &dbgCallbacks, rm);

    rm->deviceHandle = deviceManagerRegister(RAM_MAPPER, &callbacks, rm);
    slotRegister(slot, sslot, 0, 8, NULL, NULL, NULL, destroy, rm);

    reset(rm);

    if (ramPtr != NULL) {
        // Main RAM
        rm->dramHandle = panasonicDramRegister(setDram, rm);
        *ramPtr = rm->ramData;
    }
    if (ramSize != NULL) {
        *ramSize = size;
    }

    return 1;
}

