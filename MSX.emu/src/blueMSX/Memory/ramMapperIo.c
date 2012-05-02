/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/ramMapperIo.c,v $
**
** $Revision: 1.9 $
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
#include "ramMapperIo.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Language.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int        handle;
    MemIoWrite write;
    void*      ref;
    int        size;
} RamMapperCb;

typedef struct {
    int deviceHandle;
    int debugHandle;
    int handleCount;
    RamMapperCb mapperCb[32];
    int count;
    int mask;
    int port[4];
} RamMapperIo;

RamMapperIo* mapperIo = NULL;


static int ramMapperIoGetMask(RamMapperIo* rm)
{
    int size = 1;
    int i;

    for (i = 0; i < rm->count; i++) {
        while (size < rm->mapperCb[i].size) {
            size <<= 1;
        }
    }

    return (size / 0x4000) - 1;
}

static void saveState(RamMapperIo* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperRamIo");
    saveStateSet(state, "port0", rm->port[0]);
    saveStateSet(state, "port1", rm->port[1]);
    saveStateSet(state, "port2", rm->port[2]);
    saveStateSet(state, "port3", rm->port[3]);

    saveStateClose(state);
}

static void loadState(RamMapperIo* rm)
{
    SaveState* state = saveStateOpenForRead("mapperRamIo");
    rm->port[0] = saveStateGet(state, "port0", 3);
    rm->port[1] = saveStateGet(state, "port1", 2);
    rm->port[2] = saveStateGet(state, "port2", 1);
    rm->port[3] = saveStateGet(state, "port3", 0);
    
    rm->mask = ramMapperIoGetMask(rm);

    saveStateClose(state);
}

static void destroy(RamMapperIo* rm) 
{
    ioPortUnregister(0xfc);
    ioPortUnregister(0xfd);
    ioPortUnregister(0xfe);
    ioPortUnregister(0xff);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(mapperIo);
    mapperIo = NULL;
}

static UInt8 read(RamMapperIo* rm, UInt16 ioPort)
{
    return rm->port[ioPort & 3] | ~rm->mask;
}

static void write(RamMapperIo* rm, UInt16 ioPort, UInt8 value)
{
    ioPort &= 3;

    if (rm->port[ioPort] != value) {
        int i;

        rm->port[ioPort] = value;

        for (i = 0; i < rm->count; i++) {
            if (rm->mapperCb[i].write != NULL) {
                rm->mapperCb[i].write(rm->mapperCb[i].ref, ioPort, value);
            }
        }
    }
}

static void getDebugInfo(RamMapperIo* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevRamMapper(), 4);
    for (i = 0; i < 4; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0xfc + i, DBG_IO_READWRITE, read(rm, 0xfc + i));
    }
}

int ramMapperIoCreate() 
{
    RamMapperIo* rm;
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };

    rm = malloc(sizeof(RamMapperIo));
    rm->count = 0;
    rm->mask  = 0;
    rm->handleCount = 0;
    
    rm->port[0] = 3;
    rm->port[1] = 2;
    rm->port[2] = 1;
    rm->port[3] = 0;

    rm->deviceHandle = deviceManagerRegister(RAM_MAPPER, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevRamMapper(), &dbgCallbacks, rm);

    ioPortRegister(0xfc, read, write, rm);
    ioPortRegister(0xfd, read, write, rm);
    ioPortRegister(0xfe, read, write, rm);
    ioPortRegister(0xff, read, write, rm);

    mapperIo = rm;

    return 1;
}

int ramMapperIoGetPortValue(int ioPort)
{
    RamMapperIo* rm = mapperIo;
    if (rm == NULL) {
        return 0xff;
    }

    return rm->port[ioPort & 3];
}

int ramMapperIoAdd(int size, MemIoWrite write, void* ref)
{
    RamMapperIo* rm = mapperIo;
    RamMapperCb mapperCb = { ++rm->handleCount, write, ref, size };

    if (rm == NULL || rm->count == 32) {
        return 0;
    }

    rm->mapperCb[rm->count++] = mapperCb;

    rm->mask = ramMapperIoGetMask(rm);

    return rm->handleCount;
}

void ramMapperIoRemove(int handle)
{
    RamMapperIo* rm = mapperIo;
    int i;

    if (rm == NULL || rm->count == 0) {
        return;
    }

    for (i = 0; i < rm->count; i++) {
        if (rm->mapperCb[i].handle == handle) {
            break;
        }
    }

    if (i == rm->count) {
        return;
    }

    rm->count--;
    while (i < rm->count) {
        rm->mapperCb[i] = rm->mapperCb[i + 1];
        i++;
    }

    rm->mask = ramMapperIoGetMask(rm);
}
