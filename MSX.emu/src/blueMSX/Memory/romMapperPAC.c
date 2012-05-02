/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperPAC.c,v $
**
** $Revision: 1.7 $
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
#include "romMapperPAC.h"
#include "MediaDb.h"
#include "IoPort.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "sramLoader.h"
#include <stdlib.h>
#include <string.h>


static char pacHeader[] = "PAC2 BACKUP DATA";

typedef struct {
    int deviceHandle;
    UInt8 reg1ffe;
    UInt8 reg1fff;
    UInt8 sram[0x2000];
    char sramFilename[512];
    int slot;
    int sslot;
    int startPage;
    int sramEnabled;
} RomMapperPAC;

static void saveState(RomMapperPAC* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperPAC");

    saveStateSetBuffer(state, "sram", rm->sram, sizeof(rm->sram));
    
    saveStateClose(state);
}

static void loadState(RomMapperPAC* rm)
{
    SaveState* state = saveStateOpenForRead("mapperPAC");

    saveStateGetBuffer(state, "sram", rm->sram, sizeof(rm->sram));
    
    saveStateClose(state);

    rm->sramEnabled = rm->sram[0x1ffe] == 0x4d && rm->sram[0x1fff] == 0x69;

    if (rm->sramEnabled) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage, rm->sram, 1, 0);
    }
    else {
        slotUnmapPage(rm->slot, rm->sslot, rm->startPage);
    }
}

static void destroy(RomMapperPAC* rm)
{
    sramSave(rm->sramFilename, rm->sram, 0x1ffe, pacHeader, strlen(pacHeader));

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}

static UInt8 read(RomMapperPAC* rm, UInt16 address) 
{
    switch (address & 0x3fff) {
    case 0x1ffe:
        return rm->sram[0x1ffe];
    case 0x1fff:
        return rm->sram[0x1fff];
    }
    return 0xff;
}

static void write(RomMapperPAC* rm, UInt16 address, UInt8 value) 
{
    int update = 0;

    address &= 0x3fff;

    switch (address) {
    case 0x1ffe:
        rm->sram[0x1ffe] = value;
        rm->sramEnabled = rm->sram[0x1ffe] == 0x4d && rm->sram[0x1fff] == 0x69;
        update = 1;
        break;
    case 0x1fff:
        rm->sram[0x1fff] = value;
        rm->sramEnabled = rm->sram[0x1ffe] == 0x4d && rm->sram[0x1fff] == 0x69;
        update = 1;
        break;
	default:
		if (rm->sramEnabled && address < 0x1ffe) {
            rm->sram[address] = value;
		}
        break;
    }

    if (update) {
        if (rm->sramEnabled) {
            slotMapPage(rm->slot, rm->sslot, rm->startPage, rm->sram, 1, 0);
        }
        else {
            slotMapPage(rm->slot, rm->sslot, rm->startPage, NULL, 0, 0);
        }
    }
}

int romMapperPACCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperPAC* rm;

    rm = malloc(sizeof(RomMapperPAC));

    rm->deviceHandle = deviceManagerRegister(ROM_PAC, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 2, read, read, write, destroy, rm);

    memset(rm->sram, 0xff, 0x2000);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->sramEnabled = 0;
    strcpy(rm->sramFilename, sramCreateFilename(filename));

    sramLoad(rm->sramFilename, rm->sram, 0x1ffe, pacHeader, strlen(pacHeader));

    slotMapPage(rm->slot, rm->sslot, rm->startPage,     NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);

    return 1;
}

