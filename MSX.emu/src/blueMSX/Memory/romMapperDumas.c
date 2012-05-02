/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperDumas.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:43 $
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
#include "romMapperDumas.h"
#include "AmdFlash.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"
#include "sl811hs.h"
#include "Microwire93Cx6.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int deviceHandle;
    AmdFlash* amdFlash;
    SL811HS*  sl811hs;
    Microwire93Cx6* eeprom;
    int slot;
    int sslot;
    int startPage;
    UInt8 romMapper;
    UInt8 regBank;
    UInt8* flashPage;
    UInt8  reg3ffd;
    UInt8 ram[0x4000];
} RomMapperDumas;



static void saveState(RomMapperDumas* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperDumas");

    saveStateSet(state, "romMapper", rm->romMapper);
    saveStateSet(state, "regBank", rm->regBank);

    saveStateClose(state);

    amdFlashSaveState(rm->amdFlash);
    sl811hsSaveState(rm->sl811hs);
    microwire93Cx6SaveState(rm->eeprom);
}

static void loadState(RomMapperDumas* rm)
{
    SaveState* state = saveStateOpenForRead("mapperDumas");

    rm->romMapper = (UInt8)saveStateGet(state, "romMapper", 0);
    rm->regBank = (UInt8)saveStateGet(state, "regBank", 0);

    saveStateClose(state);

    amdFlashLoadState(rm->amdFlash);
    sl811hsLoadState(rm->sl811hs);
    microwire93Cx6LoadState(rm->eeprom);

    rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
}

static void destroy(RomMapperDumas* rm)
{
    amdFlashDestroy(rm->amdFlash);
    sl811hsDestroy(rm->sl811hs);
    microwire93Cx6Destroy(rm->eeprom);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}

static void reset(RomMapperDumas* rm)
{
    memset(rm->ram, 0xff, sizeof(rm->ram));
    rm->regBank   = 0;
    rm->romMapper = 0;
    rm->reg3ffd   = 0;
    amdFlashReset(rm->amdFlash);
    sl811hsReset(rm->sl811hs);
    microwire93Cx6Reset(rm->eeprom);
    rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
}

static UInt8 read(RomMapperDumas* rm, UInt16 address) 
{
    if (address < 0x3ffc) {
        return rm->flashPage[address];
    }

    switch (address)
    {
    case 0x3ffc:
        return rm->romMapper;
        break;
    case 0x3ffd:
        return (rm->reg3ffd & ~0x02) | (microwire93Cx6GetDo(rm->eeprom) ? 0x02 : 0x00);
    case 0x3ffe:
    case 0x3fff:
        return sl811hsRead(rm->sl811hs, address & 1);
    }

    return 0xff;
}

static UInt8 peek(RomMapperDumas* rm, UInt16 address) 
{
    if (address < 0x3ffc) {
        return rm->flashPage[address];
    }
    return 0xff;
}

static void write(RomMapperDumas* rm, UInt16 address, UInt8 value) 
{
    if (address < 0x3ffc) {
        amdFlashWrite(rm->amdFlash, address + 0x4000 * rm->romMapper, value);
        return;
    }

    switch (address)
    {
    case 0x3ffc:
        rm->romMapper = value & 0x1f;
        rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);
        slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
        break;
    case 0x3ffd:
        rm->reg3ffd = value;
        microwire93Cx6SetCs(rm->eeprom,  value & 0x08);
        microwire93Cx6SetDi(rm->eeprom,  value & 0x01);
        microwire93Cx6SetClk(rm->eeprom, value & 0x04);
        break;
    case 0x3ffe:
    case 0x3fff:
        sl811hsWrite(rm->sl811hs, address & 1, value);
        break;
    }
}

int romMapperDumasCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage,
                         UInt8* eepromData, int eepromSize) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperDumas* rm;

    rm = malloc(sizeof(RomMapperDumas));

    rm->deviceHandle = deviceManagerRegister(ROM_DUMAS, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, peek, write, destroy, rm);

    rm->amdFlash = amdFlashCreate(AMD_TYPE_1, 0x80000, 0x10000, 0, romData, size, sramCreateFilenameWithSuffix("dumas.rom", "", ".rom"), 0);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->sl811hs = sl811hsCreate();
    rm->eeprom  = microwire93Cx6Create(0x400, 8, eepromData, eepromSize, sramCreateFilenameWithSuffix("dumas_eeprom.rom", "", ".rom"));

    rm->flashPage = amdFlashGetPage(rm->amdFlash, 0);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->ram + 0x0000, 1, 1);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->ram + 0x2000, 1, 1);

    reset(rm);

    return 1;
}

