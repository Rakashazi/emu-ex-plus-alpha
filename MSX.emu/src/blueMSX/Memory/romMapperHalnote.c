/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperHalnote.c,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-30 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
#include "romMapperHalnote.h"
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
    int slot;
    int sslot;
    int startPage;
    int size;
    int romMapper[6];
    int sramEnabled;
    int subMapperEnabled;
    UInt8* sramData;
    char sramFilename[512];

} RomMapperHalnote;

static void saveState(RomMapperHalnote* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperHalnote");
    char tag[16];
    int i;

    for (i = 0; i < 6; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateSet(state, "sramEnabled", rm->sramEnabled);
    saveStateSet(state, "subMapperEnabled", rm->subMapperEnabled);

    saveStateClose(state);
}

static void loadState(RomMapperHalnote* rm)
{
    SaveState* state = saveStateOpenForRead("mapperHalnote");
    char tag[16];
    int i;
    int readMode;

    for (i = 0; i < 6; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }

    rm->sramEnabled          = saveStateGet(state, "sramEnabled", 0);
    rm->subMapperEnabled     = saveStateGet(state, "subMapperEnabled", 0);

    saveStateClose(state);


    for (i = 0; i < 4; i++) {
        readMode = 1;
        if (i == 1 && rm->subMapperEnabled) {
            readMode = 0;
        }
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 2 + i, rm->romData + rm->romMapper[i] * 0x2000, readMode, 0);
    }

    if (rm->sramEnabled) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->sramData,          1, 1);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->sramData + 0x2000, 1, 1);
    }
    else {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, NULL, 0, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
    }
    
}

static void destroy(RomMapperHalnote* rm)
{
    sramSave(rm->sramFilename, rm->sramData, 0x4000, NULL, 0);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm->sramData);
    free(rm);
}

static UInt8 read(RomMapperHalnote* rm, UInt16 address)
{
    int bank;

    if (address < 0x7000) {
        return rm->romData[rm->romMapper[1] * 0x2000 + (address & 0x1fff)];
    }
    bank = address < 0x7800 ? 4 : 5;
    return  rm->romData[rm->romMapper[bank] * 0x800 + 0x80000 + (address & 0x7ff)];
}

static void write(RomMapperHalnote* rm, UInt16 address, UInt8 value)
{
    int bank;
    int readMode;
    int mapperMode;
    int mapperChanged;

    if (address < 0x4000 || address > 0xbfff) {
        return;
    }

    if (address == 0x77ff) {
        rm->romMapper[4] = value;
        return;
    }

    if (address == 0x7fff) {
        rm->romMapper[5] = value;
        return;
    }

    if  ((address & 0x1fff) != 0x0fff) {
        return;
    }

    readMode = 1;
    mapperChanged = 0;
    mapperMode = value & 0x80;
    bank = (address >> 13) - 2;

    if (bank == 0) {
        if (mapperMode) {
            if (rm->sramEnabled == 0) {
                slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->sramData,          1, 1);
                slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->sramData + 0x2000, 1, 1);
                rm->sramEnabled = 1;
            }
        }
        else {
            if (rm->sramEnabled) {
                slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, NULL, 0, 0);
                slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
                rm->sramEnabled = 0;
            }
        }
    }
    else {
        if (bank == 1) {
            if (mapperMode) {
                rm->subMapperEnabled = 1;
                readMode = 0;
            }
            else {
                rm->subMapperEnabled = 0;
            }
            mapperChanged = 1;
        }
    }

    value &= (rm->size / 8192 - 1);
    if (rm->romMapper[bank] != value || mapperChanged) {
        UInt8* bankData = rm->romData + ((int)value << 13);

        rm->romMapper[bank] = value;
        slotMapPage(rm->slot, rm->sslot, rm->startPage + bank + 2, bankData, readMode, 0);
    }
}

int romMapperHalnoteCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperHalnote* rm;
    int i;

    if (size != 0x100000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperHalnote));

    rm->deviceHandle = deviceManagerRegister(ROM_HALNOTE, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 6, read, read, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->sramEnabled  = 0;
    rm->subMapperEnabled = 0;

    rm->sramData = malloc(0x4000);
    strcpy(rm->sramFilename, sramCreateFilename(filename));
    sramLoad(rm->sramFilename, rm->sramData, 0x4000, NULL, 0);

    for (i = 0; i < 6; i++) {
        rm->romMapper[i] = 0;
    }

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);

    for (i = 0; i < 4; i++) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 2, rm->romData, 1, 0);
    }

    return 1;
}
