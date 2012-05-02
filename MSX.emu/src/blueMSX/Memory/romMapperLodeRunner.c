/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperLodeRunner.c,v $
**
** $Revision: 1.6 $
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
#include "romMapperLodeRunner.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int size;
    int romMapper;
} RomMapperLodeRunner;

static void saveState(RomMapperLodeRunner* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperLodeRunner");

    saveStateSet(state, "romMapper", rm->romMapper);
    
    saveStateClose(state);
}

static void loadState(RomMapperLodeRunner* rm)
{
    SaveState* state = saveStateOpenForRead("mapperLodeRunner");
    UInt8* bankData;

    rm->romMapper = saveStateGet(state, "romMapper", 0);

    saveStateClose(state);

    bankData = rm->romData + (rm->romMapper << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage,     bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, bankData + 0x2000, 1, 0);
}

static void destroy(RomMapperLodeRunner* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    free(rm->romData);
    free(rm);
}

static void write(RomMapperLodeRunner* rm, UInt16 address, UInt8 value) 
{
    value &= (rm->size / 0x4000 - 1);

    if (rm->romMapper != value) {
        UInt8* bankData = rm->romData + ((int)value << 14);
        
        rm->romMapper = value;

        slotMapPage(rm->slot, rm->sslot, rm->startPage,     bankData, 1, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, bankData + 0x2000, 1, 0);
    }
}

int romMapperLodeRunnerCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperLodeRunner* rm;

    if (size != 0x20000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperLodeRunner));

    rm->deviceHandle = deviceManagerRegister(ROM_LODERUNNER, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);
    slotRegisterWrite0(write, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper = 0;

    slotMapPage(rm->slot, rm->sslot, rm->startPage,     rm->romData + rm->romMapper * 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->romData + rm->romMapper * 0x2000 + 0x2000, 1, 0);

    return 1;
}

