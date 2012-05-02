/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperOpcodeModule.c,v $
**
** $Revision: 1.1 $
**
** $Date: 2008-11-23 20:26:12 $
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
#include "romMapperOpcodeModule.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "SaveState.h"
#include "AY8910.h"
#include "Board.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    UInt8   slotSelect;
    UInt8   biosLatch;
    UInt8   megaRamLatch[4];

    AY8910* ay8910;

    int     slot;
    int     sslot;
    int     startPage;

    int    deviceHandle;
    int    debugHandle;

    UInt8   biosRom[0x20000];
    UInt8   ram[0x6000];
    UInt8   rom[0x20000];
    UInt8   megaRam[0x20000];
    UInt8   saveRam[0x8000];
} RomMapperOpcodeModule;


#define SLOT_CARTRIDGE  0
#define SLOT_MEGARAM    1
#define SLOT_SAVERAM    2
#define SLOT_EMPTY      3


static void slotUpdate(RomMapperOpcodeModule* rm)
{
    UInt8 slotSelect = rm->slotSelect;
    int i;

    slotMapPage(rm->slot, rm->sslot, 0, rm->biosRom + rm->biosLatch * 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, 1, rm->ram + 0x0000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, 2, rm->ram + 0x2000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, 3, rm->ram + 0x4000, 1, 1);

    for (i = 0; i < 4; i++, slotSelect >>= 2) {
        switch (slotSelect & 3) {
        case SLOT_CARTRIDGE:
            slotMapPage(rm->slot, rm->sslot, 4 + i, rm->rom + 0x2000 * i, 1, 0);
            break;
        case SLOT_MEGARAM:
            slotMapPage(rm->slot, rm->sslot, 4 + i, rm->megaRam + 0x2000 * rm->megaRamLatch[i], 1, 1);
            break;
        case SLOT_SAVERAM:
            slotMapPage(rm->slot, rm->sslot, 4 + i, rm->saveRam + 0x2000 * i, 1, 0);
            break;
        case SLOT_EMPTY:
            slotMapPage(rm->slot, rm->sslot, 4 + i, NULL, 0, 0);
            break;
        }
    }
}

static void saveState(RomMapperOpcodeModule* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperOpcodeModule");

    saveStateSet(state, "biosLatch",  rm->biosLatch);
    saveStateSet(state, "slotSelect", rm->slotSelect);
    saveStateSet(state, "megaRamLatch0", rm->megaRamLatch[0]);
    saveStateSet(state, "megaRamLatch1", rm->megaRamLatch[1]);
    saveStateSet(state, "megaRamLatch2", rm->megaRamLatch[2]);
    saveStateSet(state, "megaRamLatch3", rm->megaRamLatch[3]);
    
    
    saveStateSetBuffer(state, "ram", rm->ram, 0x6000);
    
    saveStateClose(state);

    ay8910SaveState(rm->ay8910);
}

static void loadState(RomMapperOpcodeModule* rm)
{
    SaveState* state = saveStateOpenForRead("mapperOpcodeModule");

    rm->biosLatch       = (UInt8)saveStateGet(state, "biosLatch",  0);
    rm->slotSelect      = (UInt8)saveStateGet(state, "slotSelect",  0);
    rm->megaRamLatch[0] = (UInt8)saveStateGet(state, "megaRamLatch0",  0);
    rm->megaRamLatch[1] = (UInt8)saveStateGet(state, "megaRamLatch1",  0);
    rm->megaRamLatch[2] = (UInt8)saveStateGet(state, "megaRamLatch2",  0);
    rm->megaRamLatch[3] = (UInt8)saveStateGet(state, "megaRamLatch3",  0);
    
    saveStateGetBuffer(state, "ram", rm->ram, 0x6000);

    saveStateClose(state);
    
    ay8910LoadState(rm->ay8910);

    slotUpdate(rm);
}

static void destroy(RomMapperOpcodeModule* rm)
{
    int i;
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);
    ay8910Destroy(rm->ay8910);

    for (i = 0; i < 16; i++) {
        ioPortUnregister(0x60 + i);
    }

    free(rm);
}

static UInt8 peek(RomMapperOpcodeModule* rm, UInt16 ioPort)
{
    switch (ioPort) {
    case 0x40:
        return rm->biosLatch;
    case 0x52:
        return ay8910PeekData(rm->ay8910, ioPort);
    }
    return 0xff;
}

static UInt8 read(RomMapperOpcodeModule* rm, UInt16 ioPort)
{
    switch (ioPort) {
    case 0x40:
        return rm->biosLatch;
    case 0x52:
        return ay8910ReadData(rm->ay8910, ioPort);
    }

    return 0xff;
}

static void write(RomMapperOpcodeModule* rm, UInt16 ioPort, UInt8 value)
{
    switch (ioPort) {
    case 0x40:
        rm->biosLatch = value & 0x03;
        slotMapPage(rm->slot, rm->sslot, 0, rm->biosRom + rm->biosLatch * 0x2000, 1, 0);
        break;
    case 0x48:
    case 0x49:
    case 0x4a:
    case 0x4b:
        rm->megaRamLatch[ioPort & 3] = value & 0x0f;
        slotUpdate(rm);
        break;
    case 0x50:
        ay8910WriteAddress(rm->ay8910, ioPort, value);
        break;
    case 0x51:
        ay8910WriteData(rm->ay8910, ioPort, value);
        break;
    }
}

static void reset(RomMapperOpcodeModule* rm)
{
    rm->slotSelect      = 0;
    rm->megaRamLatch[0] = 0;
    rm->megaRamLatch[1] = 0;
    rm->megaRamLatch[2] = 0;
    rm->megaRamLatch[3] = 0;
    rm->biosLatch       = 0;

    slotUpdate(0);

    ay8910Reset(rm->ay8910);
}

static void getDebugInfo(RomMapperOpcodeModule* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "AY8910", 3);

    dbgIoPortsAddPort(ioPorts, 0, 0x50, DBG_IO_WRITE, 0xff);
    dbgIoPortsAddPort(ioPorts, 1, 0x51, DBG_IO_WRITE, 0xff);
    dbgIoPortsAddPort(ioPorts, 2, 0x52, DBG_IO_READ,  peek(rm, 0x52));

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, "BIOS", 1);
    dbgIoPortsAddPort(ioPorts, 0, 0x40, DBG_IO_READWRITE, peek(rm, 0x40));
}

int romMapperOpcodeModuleCreate(char* filename, UInt8* romData, 
                                int size, int slot, int sslot, 
                                int startPage,
                                void* biosRom, int biosSize) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    
    RomMapperOpcodeModule* rm = malloc(sizeof(RomMapperOpcodeModule));
    
    rm->slot      = slot;
    rm->sslot     = sslot;
    rm->startPage = startPage;
    
    memset(rm->ram, 0xff, sizeof(rm->ram));
    memset(rm->biosRom, 0xff, sizeof(rm->biosRom));
    memset(rm->rom, 0xff, sizeof(rm->rom));
    memset(rm->megaRam, 0xff, sizeof(rm->megaRam));
    memset(rm->saveRam, 0xff, sizeof(rm->saveRam));

    if (biosRom != NULL) {
        if (biosSize > sizeof(rm->biosRom)) {
            biosSize = sizeof(rm->biosRom);
        }
        memcpy(rm->biosRom, biosRom, biosSize);
    }

    if (romData != NULL) {
        if (size > sizeof(rm->rom)) {
            size = sizeof(rm->rom);
        }
        memcpy(rm->rom, romData, size);
    }

    rm->deviceHandle = deviceManagerRegister(ROM_OPCODEPSG, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, "AY8910", &dbgCallbacks, rm);

    rm->ay8910 = ay8910Create(boardGetMixer(), AY8910_MSX, PSGTYPE_AY8910, 0, NULL);

    ioPortRegister(0x40, read, write, rm);
    ioPortRegister(0x50, NULL, write, rm);
    ioPortRegister(0x51, NULL, write, rm);
    ioPortRegister(0x52, read, NULL, rm);

    reset(rm);

    return 1;
}

