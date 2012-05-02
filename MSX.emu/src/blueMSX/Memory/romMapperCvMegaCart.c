/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperCvMegaCart.c,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:43 $
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
#include "romMapperCvMegaCart.h"
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
    UInt32 romMask;
    int romMapper;
} RomMapperCvMegaCart;

static void destroy(RomMapperCvMegaCart* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void saveState(RomMapperCvMegaCart* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperCvMegaCart");

    saveStateSet(state, "romMapper", rm->romMapper);

    saveStateClose(state);
}

static void loadState(RomMapperCvMegaCart* rm)
{
    SaveState* state = saveStateOpenForRead("mapperCvMegaCart");
    UInt8* bankData;

    rm->romMapper = saveStateGet(state, "romMapper", 0);

    saveStateClose(state);

    bankData = rm->romData + (rm->romMapper << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, bankData + 0x2000, 0, 0);
}

static UInt8 read(RomMapperCvMegaCart* rm, UInt16 address) 
{
    UInt8* bankData;

    // Only one 16kB page is mapped to this read method, so the mask just removes high bits.
    address &= 0x3fff;

    if (address < 0x3fc0) {
        return rm->romData[(rm->romMapper << 14) + address];
    }

    rm->romMapper = address & rm->romMask;

    bankData = rm->romData + (rm->romMapper << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, bankData + 0x2000, 0, 0);

    return rm->romMapper;
}

static UInt8 peek(RomMapperCvMegaCart* rm, UInt16 address) 
{
    // Only one 16kB page is mapped to this read method, so the mask just removes high bits.
    address &= 0x3fff;

    if (address < 0x3fc0) {
        return rm->romData[(rm->romMapper << 14) + address];
    }

    return rm->romMapper;
}

int romMapperCvMegaCartCreate(const char* filename, UInt8* romData,
                           int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperCvMegaCart* rm;
    UInt8* bankData;

    if (size & 0x3fff) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperCvMegaCart));

    rm->deviceHandle = deviceManagerRegister(ROM_CVMEGACART, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, peek, NULL, destroy, rm);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, size);
    rm->romMask = size / 0x4000 - 1;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->romMapper = 0;

    bankData = rm->romData + (rm->romMask << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, bankData + 0x2000, 1, 0);

    bankData = rm->romData + (rm->romMapper << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, bankData + 0x2000, 0, 0);
 
    return 1;
}

