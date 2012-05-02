/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSg1000Castle.c,v $
**
** $Revision: 1.5 $
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
#include "romMapperSg1000Castle.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "sramLoader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    UInt8 sram[0x2000];
    char sramFilename[512];
    int slot;
    int sslot;
    int startPage;
} RomMapperSg1000Castle;

static void destroy(RomMapperSg1000Castle* rm)
{
    sramSave(rm->sramFilename, rm->sram, 0x2000, NULL, 0);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

int romMapperSg1000CastleCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperSg1000Castle* rm;
    int pages = size / 0x2000 + ((size & 0x1fff) ? 1 : 0);
    int i;

    if (size != 0x8000 || startPage != 0) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperSg1000Castle));

    rm->deviceHandle = deviceManagerRegister(ROM_SG1000CASTLE, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, NULL, NULL, NULL, destroy, rm);

    rm->romData = malloc(pages * 0x2000);
    memcpy(rm->romData, romData, size);
    memset(rm->sram, 0, 0x2000);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    
    strcpy(rm->sramFilename, sramCreateFilename(filename));
    sramLoad(rm->sramFilename, rm->sram, 0x2000, NULL, 0);

    for (i = 0; i < pages; i++) {
        if (i + startPage >= 2) slot = 0;
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
    }
    // Always map SRAM in slot 0. This is an unfortunate workaround because
    // Sega roms are mapped in slot 2, but the page size is 16kB and the SRAM
    // is only 8kB which makes it impossible in current implementation to
    // map it in the same slot as the cart.
    // Note though that mapping carts to slot 2 is also sort of a workaround to
    // allow carts to be inserted/removed more easily in a running system. This
    // patch prevent removing the cart to be handled correctly though
    slotMapPage(0, 0, 4 + rm->startPage, rm->sram, 1, 1);
//    slotMapPage(rm->slot, rm->sslot, 4 + rm->startPage, rm->sram, 1, 1);

    return 1;
}

