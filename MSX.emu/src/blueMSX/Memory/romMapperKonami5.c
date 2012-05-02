/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperKonami5.c,v $
**
** $Revision: 1.10 $
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
#include "romMapperKonami5.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SCC.h"
#include "Board.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int romMask;
    int romMapper[4];
    int sccEnable;
    SCC* scc;
} RomMapperKonami5;

static void saveState(RomMapperKonami5* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperKonami5");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateSet(state, "sccEnable", rm->sccEnable);

    saveStateClose(state);

    sccSaveState(rm->scc);
}

static void loadState(RomMapperKonami5* rm)
{
    SaveState* state = saveStateOpenForRead("mapperKonami5");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }
    
    rm->sccEnable = saveStateGet(state, "sccEnable", 0);

    saveStateClose(state);

    sccLoadState(rm->scc);

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }
    
    if (rm->sccEnable) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + rm->romMapper[2] * 0x2000, 0, 0);
    }
    else {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + rm->romMapper[2] * 0x2000, 1, 0);
    }
}

static void destroy(RomMapperKonami5* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    sccDestroy(rm->scc);

    free(rm->romData);
    free(rm);
}

static void reset(RomMapperKonami5* rm)
{
    sccReset(rm->scc);
}

static UInt8 read(RomMapperKonami5* rm, UInt16 address) 
{
    address += 0x4000;

    if (address >= 0x9800 && address < 0xa000 && rm->sccEnable) {
        return sccRead(rm->scc, (UInt8)(address & 0xff));
    }

    return rm->romData[rm->romMapper[2] * 0x2000 + (address & 0x1fff)];
}

static UInt8 peek(RomMapperKonami5* rm, UInt16 address) 
{
    address += 0x4000;

    if (address >= 0x9800 && address < 0xa000 && rm->sccEnable) {
        return sccPeek(rm->scc, (UInt8)(address & 0xff));
    }

    return rm->romData[rm->romMapper[2] * 0x2000 + (address & 0x1fff)];
}

static void write(RomMapperKonami5* rm, UInt16 address, UInt8 value) 
{
    int change = 0;
    int bank;

    address += 0x4000;

    if (address >= 0x9800 && address < 0xa000 && rm->sccEnable) {
        sccWrite(rm->scc, address & 0xff, value);
        return;
    }

    address -= 0x5000;
    
    if (address & 0x1800) {
        return;
    }

    bank = address >> 13;

    if (bank == 2) {
        int newEnable = (value & 0x3F) == 0x3F;
        change = rm->sccEnable != newEnable;
        rm->sccEnable = newEnable;
    }

    value &= rm->romMask;
    if (rm->romMapper[bank] != value || change) {
        UInt8* bankData = rm->romData + ((int)value << 13);

        rm->romMapper[bank] = value;
        
        if (bank == 2 && rm->sccEnable) {
            slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + rm->romMapper[2] * 0x2000, 0, 0);
        }
        else {
            slotMapPage(rm->slot, rm->sslot, rm->startPage + bank, bankData, 1, 0);
        }
    }
}

int romMapperKonami5Create(const char* filename, UInt8* romData,
                           int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperKonami5* rm;
    int i;

    int origSize = size;
    
    size = 0x8000;
    while (size < origSize) {
        size *= 2;
    }


    rm = malloc(sizeof(RomMapperKonami5));

    rm->deviceHandle = deviceManagerRegister(ROM_KONAMI5, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, peek, write, destroy, rm);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, origSize);
    rm->romMask = size / 0x2000 - 1;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->scc = sccCreate(boardGetMixer());
    sccSetMode(rm->scc, SCC_REAL);
    rm->sccEnable = 0;

    rm->romMapper[0] = 0;
    rm->romMapper[1] = 1;
    rm->romMapper[2] = 2;
    rm->romMapper[3] = 3;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }

    return 1;
}

