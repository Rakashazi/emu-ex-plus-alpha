/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperObsonet.c,v $
**
** $Revision: 1.14 $
**
** $Date: 2008-09-09 04:33:47 $
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
#include "romMapperObsonet.h"
#include "AmdFlash.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"
#include "rtl8019.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int deviceHandle;
    AmdFlash* amdFlash;
    RTL8019*  rtl8019;
    int slot;
    int sslot;
    int startPage;
    UInt8 romMapper;
    UInt8 regBank;
    UInt8* flashPage;
} RomMapperObsonet;



static void saveState(RomMapperObsonet* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperObsonet");

    saveStateSet(state, "romMapper", rm->romMapper);
    saveStateSet(state, "regBank", rm->regBank);

    saveStateClose(state);

    amdFlashSaveState(rm->amdFlash);
    rtl8019SaveState(rm->rtl8019);
}

static void loadState(RomMapperObsonet* rm)
{
    SaveState* state = saveStateOpenForRead("mapperObsonet");

    rm->romMapper = (UInt8)saveStateGet(state, "romMapper", 0);
    rm->regBank = (UInt8)saveStateGet(state, "regBank", 0);

    saveStateClose(state);

    amdFlashLoadState(rm->amdFlash);
    rtl8019LoadState(rm->rtl8019);

    rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
}

static void destroy(RomMapperObsonet* rm)
{
    amdFlashDestroy(rm->amdFlash);
    rtl8019Destroy(rm->rtl8019);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}

static void reset(RomMapperObsonet* rm)
{
    rm->regBank   = 0;
    rm->romMapper = 0;
    amdFlashReset(rm->amdFlash);
    rtl8019Reset(rm->rtl8019);
    rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
}

static UInt8 read(RomMapperObsonet* rm, UInt16 address) 
{
    if ((address & 0x3fe0) == 0x3fe0) { 
        UInt8 value = rtl8019Read(rm->rtl8019, address & 0x1f);
//        printf("R %d: %.4x  %.2x\n", rm->regBank, address & 0x1f, value);
        return value;
    }

    if (address < 0x4000) {
        // This is reads to 0x6000-0x7FDF, rest are directly mapped
        return rm->flashPage[address];
    }

    return 0xff;
}

static UInt8 peek(RomMapperObsonet* rm, UInt16 address) 
{
    
    if ((address & 0x3fe0) == 0x3fe0) return 0xff;
    return read(rm, address);
}

static void write(RomMapperObsonet* rm, UInt16 address, UInt8 value) 
{
    if ((address & 0x3fe0) == 0x3fe0) {
        if (rm->regBank < 3) {
//        printf("W %d: %.4x  %.2x\n", rm->regBank, address & 0x1f, value);
        }
        switch (address & 0x1f) {
        case 0:
            rm->regBank = value >> 6;
            break;
        case 2:
            if (rm->regBank == 3) {
                rm->romMapper = value & 0x1f;
                rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);
                slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
            }
            break;
        }
        rtl8019Write(rm->rtl8019, address & 0x1f, value);
    }
    else if (address < 0x4000) {
        amdFlashWrite(rm->amdFlash, address + 0x4000 * rm->romMapper, value);
    }
}

int romMapperObsonetCreate(const char* filename, UInt8* romData,
                           int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperObsonet* rm;

    rm = malloc(sizeof(RomMapperObsonet));

    rm->deviceHandle = deviceManagerRegister(ROM_OBSONET, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, peek, write, destroy, rm);

    rm->amdFlash = amdFlashCreate(AMD_TYPE_1, 0x80000, 0x10000, 0, romData, size, sramCreateFilenameWithSuffix("obsonet.rom", "", ".rom"), 0);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->rtl8019 = rtl8019Create();

    rm->flashPage = amdFlashGetPage(rm->amdFlash, 0);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, rm->flashPage, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, NULL, 0, 0);

    reset(rm);

    return 1;
}

