/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSonyHBI55.c,v $
**
** $Revision: 1.12 $
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
#include "romMapperSonyHBI55.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "sramLoader.h"
#include "IoPort.h"
#include "I8255.h"
#include "Board.h"
#include "SaveState.h"
#include "KeyClick.h"
#include "ArchInput.h"
#include "Switches.h"
#include "Language.h"
#include "Led.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int    deviceHandle;
    int    debugHandle;
    I8255* i8255;

    UInt8  sram[0x1000];
    UInt8  mode;
    UInt8  writeLatch;
    UInt8  addrLatch;
    UInt16 readAddr;
    UInt16 writeAddr;
} SonyHBI55;

static void destroy(SonyHBI55* rm)
{
    ioPortUnregister(0xb0);
    ioPortUnregister(0xb1);
    ioPortUnregister(0xb2);
    ioPortUnregister(0xb3);
    
    sramSave(sramCreateFilename("HBI-55.SRAM"), rm->sram, 0x1000, NULL, 0);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    i8255Destroy(rm->i8255);

    free(rm);
}

static void reset(SonyHBI55* rm) 
{
    rm->mode       = 0;
    rm->addrLatch  = 0;
    rm->writeLatch = 0;
    rm->writeAddr  = 0;
    rm->readAddr   = 0;

    i8255Reset(rm->i8255);
}

static void loadState(SonyHBI55* rm)
{
    SaveState* state = saveStateOpenForRead("SonyHBI55");

    rm->mode       = (UInt8) saveStateGet(state, "mode",       0);
    rm->addrLatch  = (UInt8) saveStateGet(state, "addrLatch",  0);
    rm->writeLatch = (UInt8) saveStateGet(state, "writeLatch", 0);
    rm->writeAddr  = (UInt16)saveStateGet(state, "writeAddr",  0);
    rm->readAddr   = (UInt16)saveStateGet(state, "readAddr",   0);

    saveStateClose(state);
    
    i8255LoadState(rm->i8255);
}

static void saveState(SonyHBI55* rm)
{
    SaveState* state = saveStateOpenForWrite("SonyHBI55");
    
    saveStateSet(state, "mode",       rm->mode);
    saveStateSet(state, "addrLatch",  rm->addrLatch);
    saveStateSet(state, "writeLatch", rm->writeLatch);
    saveStateSet(state, "writeAddr",  rm->writeAddr);
    saveStateSet(state, "readAddr",   rm->readAddr);

    saveStateClose(state);

    i8255SaveState(rm->i8255);
}

static void writeA(SonyHBI55* rm, UInt8 value)
{
	rm->addrLatch = value; 
}

static void writeB(SonyHBI55* rm, UInt8 value)
{
	UInt16 address = (UInt16)rm->addrLatch | ((value & 0x0f) << 8); 

    rm->mode = value >> 6;

    switch (rm->mode) {
    case 0:
        rm->writeAddr = 0;
        rm->readAddr  = 0;
        break;
    case 1:
        rm->writeAddr = address;
        break;
    case 2:
        if (rm->writeAddr != 0) {
            rm->sram[rm->writeAddr] = rm->writeLatch;
        }
        break;
    case 3:
        rm->readAddr = address;
        break;
    }
}

static void writeCLo(SonyHBI55* rm, UInt8 value)
{
    rm->writeLatch = (rm->writeLatch & 0xf0) | (value << 0);
}

static void writeCHi(SonyHBI55* rm, UInt8 value)
{
    rm->writeLatch = (rm->writeLatch & 0x0f) | (value << 4);

    if (rm->mode == 1) {
        if (rm->writeAddr != 0) {
            rm->sram[rm->writeAddr] = rm->writeLatch;
        }
    }
}

static UInt8 readCLo(SonyHBI55* rm)
{
    return rm->sram[rm->readAddr] & 0x0f;
}


static UInt8 readCHi(SonyHBI55* rm)
{
    return rm->sram[rm->readAddr] >> 4;
}

static void getDebugInfo(SonyHBI55* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevHbi55(), 4);
    dbgIoPortsAddPort(ioPorts, 0, 0xb0, DBG_IO_READWRITE, i8255Peek(rm->i8255, 0xb0));
    dbgIoPortsAddPort(ioPorts, 1, 0xb1, DBG_IO_READWRITE, i8255Peek(rm->i8255, 0xb1));
    dbgIoPortsAddPort(ioPorts, 2, 0xb2, DBG_IO_READWRITE, i8255Peek(rm->i8255, 0xb2));
    dbgIoPortsAddPort(ioPorts, 3, 0xb3, DBG_IO_READWRITE, i8255Peek(rm->i8255, 0xb3));
}

int romMapperSonyHBI55Create()
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    SonyHBI55* rm = malloc(sizeof(SonyHBI55));

    rm->deviceHandle = deviceManagerRegister(ROM_SONYHBI55, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_CART, langDbgDevHbi55(), &dbgCallbacks, rm);

    memset(rm->sram, 0xff, sizeof(rm->sram));
    sramLoad(sramCreateFilename("HBI-55.SRAM"), rm->sram, 0x1000, NULL, 0);

    rm->sram[0] = 0x53;

    rm->i8255 = i8255Create(NULL,    NULL,    writeA,
                            NULL,    NULL,    writeB,
                            readCLo, readCLo, writeCLo,
                            readCHi, readCHi, writeCHi,
                            rm);

    ioPortRegister(0xb0, i8255Read, i8255Write, rm->i8255);
    ioPortRegister(0xb1, i8255Read, i8255Write, rm->i8255);
    ioPortRegister(0xb2, i8255Read, i8255Write, rm->i8255);
    ioPortRegister(0xb3, i8255Read, i8255Write, rm->i8255);

    reset(rm);

    return 1;
}
