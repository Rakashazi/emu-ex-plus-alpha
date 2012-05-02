/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/ram1kBMirrored.c,v $
**
** $Revision: 1.12 $
**
** $Date: 2008-05-19 19:25:59 $
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
#include "ram1kBMirrored.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    int debugHandle;
    int slot;
    int sslot;
    int startPage;
    int pages;
    UInt32 mask;
    UInt8 ramData[0x2000];
} Ram1kBMirrored;

static void saveState(Ram1kBMirrored* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMirroredRam");

    saveStateSet(state, "mask", rm->mask);
    saveStateSetBuffer(state, "ramData", rm->ramData, rm->mask + 1);

    saveStateClose(state);
}

static void loadState(Ram1kBMirrored* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMirroredRam");

    rm->mask = saveStateGet(state, "mask", 0x400);
    saveStateGetBuffer(state, "ramData", rm->ramData, rm->mask + 1);

    saveStateClose(state);
}

static void destroy(Ram1kBMirrored* rm)
{
    debugDeviceUnregister(rm->debugHandle);

    slotUnregister(rm->slot, rm->sslot, 0);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}

static void getDebugInfo(Ram1kBMirrored* rm, DbgDevice* dbgDevice)
{
    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemRamNormal(), 0, 0, rm->mask + 1, rm->ramData);
}

static int dbgWriteMemory(Ram1kBMirrored* rm, char* name, void* data, int start, int size)
{
    if (strcmp(name, "Normal") || start + size >= (int)rm->mask) {
        return 0;
    }

    memcpy(rm->ramData + start, data, size);

    return 1;
}

static UInt8 read(Ram1kBMirrored* rm, UInt16 address) 
{
    return rm->ramData[address & rm->mask];
}

static void write(Ram1kBMirrored* rm, UInt16 address, UInt8 value) 
{
    rm->ramData[address & rm->mask] = value;
}

int ramMirroredCreate(int size, int slot, int sslot, int startPage, 
                      UInt32 ramBlockSize, UInt8** ramPtr, UInt32* ramSize) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, NULL, NULL };
    Ram1kBMirrored* rm;
    int pages = size / 0x2000;
    int i;

    if (size > 0x10000 || (size & 0x1fff)) {
        return 0;
    }

    // Start page must be zero (only full slot allowed)
    if (startPage + pages > 8) {
        return 0;
    }

    rm = malloc(sizeof(Ram1kBMirrored));

    rm->mask      = ramBlockSize - 1;
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    rm->pages     = pages;

    memset(rm->ramData, 0, sizeof(rm->ramData));

    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, langDbgDevRam(), &dbgCallbacks, rm);

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    } 

    rm->deviceHandle = deviceManagerRegister(ramBlockSize == 0x400 ? RAM_1KB_MIRRORED : RAM_2KB_MIRRORED, 
                                             &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, read, read, write, destroy, rm);

    if (ramPtr != NULL) {
        *ramPtr = rm->ramData;
    }

    if (ramSize != NULL) {
        *ramSize = rm->pages * 0x2000;
    }

    return 1;
}

