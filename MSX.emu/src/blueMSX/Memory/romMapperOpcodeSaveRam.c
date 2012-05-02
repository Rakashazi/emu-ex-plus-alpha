/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperOpcodeSaveRam.c,v $
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
#include "romMapperOpcodeSaveRam.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "SaveState.h"
#include "sramLoader.h"
#include "AY8910.h"
#include "Board.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    int     slot;
    int     sslot;
    int     startPage;

    int     cmdIdx;
    int     protectEnable;

    int    deviceHandle;
    int    debugHandle;

    UInt8   saveRam[0x8000];
    char    saveRamFilename[512];
} RomMapperOpcodeSaveRam;



static void saveState(RomMapperOpcodeSaveRam* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperOpcodeSaveRam");

    
    saveStateSetBuffer(state, "saveRam", rm->saveRam, 0x8000);
    
    saveStateClose(state);
}

static void loadState(RomMapperOpcodeSaveRam* rm)
{
    SaveState* state = saveStateOpenForRead("mapperOpcodeSaveRam");
    
    saveStateGetBuffer(state, "saveRam", rm->saveRam, 0x8000);

    saveStateClose(state);
}

static void destroy(RomMapperOpcodeSaveRam* rm)
{
    sramSave(rm->saveRamFilename, rm->saveRam, sizeof(rm->saveRam), NULL, 0);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm);
}

static void write(RomMapperOpcodeSaveRam* rm, UInt16 address, UInt8 value)
{
    switch (rm->cmdIdx++) {
    case 0: 
        if (address == 0x5555 && value == 0xaa) return;
        break;
    case 1:
        if (address == 0x2aaa && value == 0x55) return;
        break;
    case 2: 
        if (address == 0x5555 && value == 0xa0) {
            rm->protectEnable = 1;
            rm->cmdIdx = 0;
            return;
        }
        if (address == 0x5555 && value == 0x80) return;
        break;
    case 3:
        if (address == 0x5555 && value == 0xaa) return;
        break;
    case 4: 
        if (address == 0x2aaa && value == 0x55) return;
        break;
    case 5:
        if (address == 0x5555 && value == 0x20) {
            rm->protectEnable = 0;
            rm->cmdIdx = 0;
            return;
        }
        break;
    }

    if (!rm->protectEnable) {
        rm->saveRam[address] = value;
    }
    rm->cmdIdx = 0;
}

static void reset(RomMapperOpcodeSaveRam* rm)
{
    rm->protectEnable = 0;
    rm->cmdIdx = 0;
}

static void getDebugInfo(RomMapperOpcodeSaveRam* rm, DbgDevice* dbgDevice)
{
    dbgDeviceAddMemoryBlock(dbgDevice, "Save Ram", 0, 0, sizeof(rm->saveRam), rm->saveRam);
}

int romMapperOpcodeSaveRamCreate(int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    
    RomMapperOpcodeSaveRam* rm = malloc(sizeof(RomMapperOpcodeSaveRam));
    
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    
    memset(rm->saveRam, 0xff, sizeof(rm->saveRam));
    
    slotRegister(rm->slot, rm->sslot, rm->startPage, 4, NULL, NULL, write, destroy, rm);

    rm->deviceHandle = deviceManagerRegister(ROM_OPCODESAVE, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_RAM, "SAVERAM", &dbgCallbacks, rm);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->saveRam + 0x0000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->saveRam + 0x2000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->saveRam + 0x4000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->saveRam + 0x6000, 1, 1);

    strcpy(rm->saveRamFilename, sramCreateFilename("SaveRam"));

    sramLoad(rm->saveRamFilename, rm->saveRam, sizeof(rm->saveRam), NULL, 0);

    reset(rm);

    return 1;
}

