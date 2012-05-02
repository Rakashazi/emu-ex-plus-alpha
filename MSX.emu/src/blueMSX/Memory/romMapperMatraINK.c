/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMatraINK.c,v $
**
** $Revision: 1.2 $
**
** $Date: 2008-03-22 10:12:57 $
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
#include "romMapperMatraINK.h"
#include "AmdFlash.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int deviceHandle;
    AmdFlash* flash;
    int slot;
    int sslot;
    int startPage;
} RomMapperMatraINK;

static void saveState(RomMapperMatraINK* rm)
{
    amdFlashSaveState(rm->flash);
}

static void loadState(RomMapperMatraINK* rm)
{ 
    amdFlashLoadState(rm->flash);
}

static void destroy(RomMapperMatraINK* rm)
{
    amdFlashDestroy(rm->flash);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}


static UInt8 read(RomMapperMatraINK* rm, UInt16 address) 
{
    return amdFlashRead(rm->flash, address);
}

static UInt8 peek(RomMapperMatraINK* rm, UInt16 address) 
{
    return amdFlashRead(rm->flash, address);
}

static void write(RomMapperMatraINK* rm, UInt16 address, UInt8 value) 
{
    amdFlashWrite(rm->flash, address, value);
}

int romMapperMatraINKCreate(const char* filename, UInt8* romData,
                             int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperMatraINK* rm;
    int i;

    rm = calloc(1, sizeof(RomMapperMatraINK));

    rm->deviceHandle = deviceManagerRegister(ROM_MEGAFLSHSCC, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, read, peek, write, destroy, rm);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->flash = amdFlashCreate(AMD_TYPE_2, 0x10000, 0x10000, 0xff, romData, size, NULL, 0);

    for (i = 0; i < 8; i++) {
        slotMapPage(slot, sslot, startPage + i, NULL, 0, 0);
    }

    return 1;
}
