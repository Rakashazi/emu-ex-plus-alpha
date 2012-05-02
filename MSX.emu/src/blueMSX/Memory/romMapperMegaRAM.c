/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMegaRAM.c,v $
**
** $Revision: 1.8 $
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
#include "romMapperMegaRAM.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    int debugHandle;
    UInt8* ramData;
    int slot;
    int sslot;
    int startPage;
    int size;
    int writeEnabled;
    int romMapper[4];
} RomMapperMegaRAM;

static void saveState(RomMapperMegaRAM* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMegaRAM");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }
    
    saveStateSet(state, "writeEnabled", rm->writeEnabled);
    saveStateSet(state, "size",         rm->size);
    
    saveStateSetBuffer(state, "ramData", rm->ramData, rm->size);

    saveStateClose(state);
}

static void loadState(RomMapperMegaRAM* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMegaRAM");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }
    
    rm->writeEnabled = saveStateGet(state, "writeEnabled", 0);
    rm->size         = saveStateGet(state, "size",         0);
    
    saveStateGetBuffer(state, "ramData", rm->ramData, rm->size);

    saveStateClose(state);

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->ramData + rm->romMapper[i] * 0x2000, 1, rm->writeEnabled);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 4, rm->ramData + rm->romMapper[i] * 0x2000, 1, rm->writeEnabled);
    }
}

static void destroy(RomMapperMegaRAM* rm)
{
    ioPortUnregister(0x8e);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm->ramData);
    free(rm);
}

static void write(RomMapperMegaRAM* rm, UInt16 address, UInt8 value) 
{
    int bank;

    address &= 0x7fff;

	bank = address >> 13;
    value %= rm->size >> 13;

    if (rm->romMapper[bank] != value) {
        UInt8* bankData = rm->ramData + ((int)value << 13);

        rm->romMapper[bank] = value;
        
        slotMapPage(rm->slot, rm->sslot, rm->startPage + bank, bankData, 1, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + bank  + 4, bankData, 1, 0);
    }
}

static void writeIo(RomMapperMegaRAM* rm, UInt16 ioPort, UInt8 value)
{
    if (rm->writeEnabled) {
        int i;
        for (i = 0; i < 4; i++) {
            slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->ramData + rm->romMapper[i] * 0x2000, 1, 0);
            slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 4, rm->ramData + rm->romMapper[i] * 0x2000, 1, 0);
        }
    }
    rm->writeEnabled = 0;
}

static UInt8 readIo(RomMapperMegaRAM* rm, UInt16 ioPort)
{
    if (!rm->writeEnabled) {
        int i;
        for (i = 0; i < 4; i++) {
            slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->ramData + rm->romMapper[i] * 0x2000, 1, 1);
            slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 4, rm->ramData + rm->romMapper[i] * 0x2000, 1, 1);
        }
    }
    rm->writeEnabled = 1;

    return 0xff;
}

static void getDebugInfo(RomMapperMegaRAM* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMegaRam(), 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x77, DBG_IO_READWRITE, 0xff);
}


int romMapperMegaRAMCreate(int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperMegaRAM* rm;
    int i;

    if (startPage != 0) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperMegaRAM));

    rm->deviceHandle = deviceManagerRegister(ROM_MEGARAM, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_CART, langDbgDevMegaRam(), &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, 8, NULL, NULL, write, destroy, rm);

    rm->ramData = malloc(size);
    memset(rm->ramData, 0xff, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->writeEnabled = 0;
    rm->startPage  = startPage;

    rm->romMapper[0] = 0;
    rm->romMapper[1] = 0;
    rm->romMapper[2] = 0;
    rm->romMapper[3] = 0;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->ramData + rm->romMapper[i] * 0x2000, 1, 0);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i + 4, rm->ramData + rm->romMapper[i] * 0x2000, 1, 0);
    }
    
    ioPortRegister(0x8e, readIo, writeIo, rm);

    return 1;
}

