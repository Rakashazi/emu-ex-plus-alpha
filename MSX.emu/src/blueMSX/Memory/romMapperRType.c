/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperRType.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2009-04-02 22:32:06 $
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
#include "romMapperRType.h"
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
    int romMapper[4];
} RomMapperRType;

static void saveState(RomMapperRType* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperRType");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateClose(state);
}

static void loadState(RomMapperRType* rm)
{
    SaveState* state = saveStateOpenForRead("mapperRType");
    char tag[16];
    int bank = 2;
    UInt8* bankData;

    for (bank = 0; bank < 4; bank++) {
        sprintf(tag, "romMapper%d", bank);
        rm->romMapper[bank] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);

    bankData = rm->romData + (rm->romMapper[bank] << 14);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + bank,     bankData, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + bank + 1, bankData + 0x2000, 1, 0);
}

static void destroy(RomMapperRType* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperRType* rm, UInt16 address, UInt8 value) 
{
    int bank;

    address += 0x4000;

    if (address < 0x4000 || address >= 0xc000) {
        return;
    }
    
    bank = 2;
	value &= (value & 0x10) ? 0x17 : 0x0f;

    if (rm->romMapper[bank] != value) {
        UInt8* bankData = rm->romData + ((int)value << 14);
        
        rm->romMapper[bank] = value;

        slotMapPage(rm->slot, rm->sslot, rm->startPage + bank,     bankData, 1, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + bank + 1, bankData + 0x2000, 1, 0);
    }
}

int romMapperRTypeCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperRType* rm;
    int i;

    if (size < 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperRType));

    rm->deviceHandle = deviceManagerRegister(ROM_RTYPE, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper[0] = 0x2e;
    rm->romMapper[1] = 0x2f;
    rm->romMapper[2] = 0;
    rm->romMapper[3] = 1;

    for (i = 0; i < 4; i += 2) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i,     rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 1, rm->romData + rm->romMapper[i] * 0x2000 + 0x2000, 1, 0);
    }

    return 1;
}

