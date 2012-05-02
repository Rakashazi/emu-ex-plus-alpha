/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperBunsetu.c,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:42 $
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
#include "romMapperBunsetu.h"
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
    UInt8* jisyoData;
    UInt32 address;
    int slot;
    int sslot;
    int startPage;
} RomMapperBunsetu;

static void saveState(RomMapperBunsetu* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperBunsetu");

    saveStateSet(state, "address", rm->address);
    
    saveStateClose(state);
}

static void loadState(RomMapperBunsetu* rm)
{
    SaveState* state = saveStateOpenForRead("mapperBunsetu");

    rm->address = saveStateGet(state, "address", 0);

    saveStateClose(state);
}

static void destroy(RomMapperBunsetu* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    if (rm->jisyoData) {
        free(rm->jisyoData);
    }
    free(rm);
}

static UInt8 read(RomMapperBunsetu* rm, UInt16 address) 
{
    address += 0x4000;
    
    if (address == 0xbfff) {
        if (rm->jisyoData) {
            UInt8 value = rm->jisyoData[rm->address];
            rm->address = (rm->address + 1) & 0x0ffff;
            return value;
        }
    }
    
    return rm->romData[address - 0x4000];
}

static UInt8 peek(RomMapperBunsetu* rm, UInt16 address) 
{
    address += 0x4000;
    
    if (address == 0xbfff) {
        if (rm->jisyoData) {
            return rm->jisyoData[rm->address];
        }
    }
    
    return rm->romData[address - 0x4000];
}

static void write(RomMapperBunsetu* rm, UInt16 address, UInt8 value) 
{
    address += 0x4000;
    
	switch (address) {
	case 0xbffc:
		rm->address = (rm->address & 0x1ff00) | value;
		break;
	case 0xbffd:
		rm->address = (rm->address & 0x100ff) | (value << 8);
		break;
	case 0xbffe:
		rm->address = (rm->address & 0x0ffff) | ((value & 1) << 16);
		break;
	}
}

int romMapperBunsetuCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage,
                          void* jisyoRom, int jisyoSize) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperBunsetu* rm;

    if (size != 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperBunsetu));

    rm->deviceHandle = deviceManagerRegister(ROM_BUNSETU, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, peek, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    if (jisyoRom) {
        rm->jisyoData = malloc(jisyoSize);
        memcpy(rm->jisyoData, jisyoRom, jisyoSize);
    }
    else {
        rm->jisyoData = NULL;
    }

    rm->address   = 0;
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->romData + 0x0000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->romData + 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + 0x4000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->romData + 0x6000, 0, 0);

    return 1;
}

