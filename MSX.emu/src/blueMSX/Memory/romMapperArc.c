/*****************************************************************************
** $Source:  $
**
** $Revision: 1.12 $
**
** $Date: 2008-03-30 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Ricardo Bittencourt
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
#include "romMapperArc.h"
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

    UInt8 offset;
} Arc;

static void destroy(Arc* rm)
{
    ioPortUnregister(0x7f);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm->romData);
    free(rm);
}

static void saveState(Arc* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperArc");

    saveStateSet(state, "offset", rm->offset);

    saveStateClose(state);
}

static void loadState(Arc* rm)
{
    SaveState* state = saveStateOpenForRead("mapperArc");

    rm->offset = (UInt8)saveStateGet(state, "offset", 0);

    saveStateClose(state);
}

static UInt8 readIo(Arc* rm, UInt16 ioPort)
{	
    printf("R: %.2x\n", ((rm->offset & 0x03) == 0x03) ? 0xda : 0xff);

    return ((rm->offset & 0x03) == 0x03) ? 0xda : 0xff;
}

static void writeIo(Arc* rm, UInt16 ioPort, UInt8 value)
{
    printf("W: %.2x\n", value);
    if (value == 0x35) {
        rm->offset++;
    }
}

static void reset(Arc* rm)
{
    rm->offset = 0;
}

static void getDebugInfo(Arc* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "Parallax ARC", 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x7f, DBG_IO_READWRITE, readIo(rm, 0x7f));
}

int romMapperArcCreate(const char* filename, UInt8* romData,
                       int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Arc* rm;
    int pages = size / 0x2000;
    int i;

    rm = malloc(sizeof(Arc));

    rm->deviceHandle = deviceManagerRegister(ROM_ARC, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_CART, "Parallax ARC", &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);

    size = (size + 0x3fff) & ~0x3fff;

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
    }
    
    ioPortRegister(0x7f, readIo, writeIo, rm);

    reset(rm);

    return 1;
}

