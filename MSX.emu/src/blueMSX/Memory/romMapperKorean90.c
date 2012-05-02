/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperKorean90.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-06-08 13:02:48 $
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
#include "romMapperKorean90.h"
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
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int size;
    int romMapper[4];
} RomMapperKorean90;

static void saveState(RomMapperKorean90* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperKorean90");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateClose(state);
}

static void loadState(RomMapperKorean90* rm)
{
    SaveState* state = saveStateOpenForRead("mapperKorean90");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }
}

static void destroy(RomMapperKorean90* rm)
{
    if (ioPortGetRef(0x77)&&ioPortGetRef(0x77)==rm) ioPortUnregister(0x77);
    
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperKorean90* rm, UInt16 address, UInt8 value) 
{
    int page = ((value & 0x7f) << 1) & (rm->size / 8192 - 1);
    int i;

    if (value&0x80) {
        // 32K mode
        rm->romMapper[0] = (page & 0xfc) + 0;
        rm->romMapper[1] = (page & 0xfc) + 1;
        rm->romMapper[2] = (page & 0xfc) + 2;
        rm->romMapper[3] = (page & 0xfc) + 3;
    }
    else {
        // 16K mode
        rm->romMapper[0] = page + 0;
        rm->romMapper[1] = page + 1;
        rm->romMapper[2] = page + 0;
        rm->romMapper[3] = page + 1;
    }

    for (i = 0; i < 4; i++) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }
}

static void getDebugInfo(RomMapperKorean90* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevKorean90(), 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x77, DBG_IO_WRITE, 0);
}

int romMapperKorean90Create(const char* filename, UInt8* romData,
                            int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperKorean90* rm;
    int i;

    if (size < 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperKorean90));

    rm->deviceHandle = deviceManagerRegister(ROM_KOREAN90, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_CART, langDbgDevKorean90(), &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper[0] = 0;
    rm->romMapper[1] = 1;
    rm->romMapper[2] = 0;
    rm->romMapper[3] = 1;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }
    
    // bankswitch I/O at port $77, 1st insertion only
    // it's very likely though that writes to that port get intercepted by every inserted Korean90 cart, and not just 1
    ioPortRegister(0x77, NULL, write, rm);

    return 1;
}

