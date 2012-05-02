/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSegaBasic.c,v $
**
** $Revision: 1.3 $
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
#include "romMapperSegaBasic.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "Language.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    int debugHandle;
    UInt8* romData;
    UInt8 ram[0x8000];
    int slot;
    int sslot;
    int startPage;
} RomMapperSegaBasic;

static void saveState(RomMapperSegaBasic* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperSegaBasic");

    saveStateSetBuffer(state, "ram", rm->ram, 0x8000);

    saveStateClose(state);
}

static void loadState(RomMapperSegaBasic* rm)
{
    SaveState* state = saveStateOpenForRead("mapperSegaBasic");

    saveStateGetBuffer(state, "ram", rm->ram, 0x8000);

    saveStateClose(state);
}

static void destroy(RomMapperSegaBasic* rm)
{
    debugDeviceUnregister(rm->debugHandle);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void getDebugInfo(RomMapperSegaBasic* rm, DbgDevice* dbgDevice)
{
    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemRamNormal(), 0, 0, 0x8000, rm->ram);
}

static int dbgWriteMemory(RomMapperSegaBasic* rm, char* name, void* data, int start, int size)
{
    if (strcmp(name, "Normal") || start + size >= 0x8000) {
        return 0;
    }

    memcpy(rm->ram+ start, data, size);

    return 1;
}

int romMapperSegaBasicCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, NULL, NULL };
    RomMapperSegaBasic* rm;
    int pages = size / 0x2000 + ((size & 0x1fff) ? 1 : 0);
    int i;

    if (size != 0x8000 || startPage != 0) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperSegaBasic));

    rm->deviceHandle = deviceManagerRegister(ROM_SEGABASIC, &callbacks, rm);
    
    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, langDbgDevRam(), &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, pages, NULL, NULL, NULL, destroy, rm);

    rm->romData = malloc(pages * 0x2000);
    memcpy(rm->romData, romData, size);
    memset(rm->ram, 0xff, 0x2000);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    
    for (i = 0; i < pages; i++) {
        if (i + startPage >= 2) slot = 0;
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
        
    }
    
    for (; i < 8; i++) {
        slotMapPage(0, 0, i, rm->ram + 0x2000 * (i - 4), 1, 1);
    }

    return 1;
}

