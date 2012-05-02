/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperA1FM.c,v $
**
** $Revision: 1.3 $
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
#include "romMapperA1FM.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum {
    READ_SRAM,
    READ_EMPTY,
    READ_ROM
};

typedef struct {
    int deviceHandle;
    UInt8* romData;
    UInt8* sram;
    int    readSection;
    int    readOffset;
    UInt8* readBlock;
    int    sramSize;
    char   sramFilename[512];
    int    maxSRAMBank;
    int    romSize;
    UInt8  control;
    int    romMapper[8];
    int    slot;
    int    sslot;
    int    startPage;
} RomMapperA1FM;

static UInt8 emptyRam[0x2000];

static int SRAM_BASE = 0x80;
static int RAM_BASE  = 0x180;

static UInt8* panasonicSramData;
static UInt32 panasonicSramMask = 0;

UInt8 panasonicSramGet(UInt32 address)
{
    if (panasonicSramMask == 0) {
        return 0xff;
    }
    return panasonicSramData[address & panasonicSramMask];
}

void  panasonicSramSet(UInt32 address, UInt8 value)
{
    if (panasonicSramMask > 0) {
        panasonicSramData[address & panasonicSramMask] = value;
    }
}

void panasonicSramCreate(UInt8* sram, UInt32 size)
{
    panasonicSramData = sram;
    panasonicSramMask = size - 1;
}

void panasonicSramDestroy()
{
    panasonicSramMask = 0;
}


static void changeBank(RomMapperA1FM* rm, int region, int bank);

static void saveState(RomMapperA1FM* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperPanasonic");
    char tag[16];
    int i;

    for (i = 0; i < 8; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }
    
    saveStateSet(state, "readSection", rm->readSection);
    saveStateSet(state, "readOffset", rm->readOffset);
    saveStateSet(state, "control", rm->control);

    saveStateClose(state);
}

static void loadState(RomMapperA1FM* rm)
{
    SaveState* state = saveStateOpenForRead("mapperPanasonic");
    int romMapper[8];
    char tag[16];
    int i;

    for (i = 0; i < 8; i++) {
        sprintf(tag, "romMapper%d", i);
        romMapper[i] = (UInt8)saveStateGet(state, tag, 0);
    }
    
    rm->readSection = saveStateGet(state, "readSection", 0);
    rm->readOffset  = saveStateGet(state, "readOffset", 0);
    rm->control     = (UInt8)saveStateGet(state, "control", 0);

    saveStateClose(state);

    for (i = 0; i < 8; i++) {
        changeBank(rm, i, romMapper[i]);
    }

    switch (rm->readSection) {
    case READ_SRAM:
        rm->readBlock = rm->sram + rm->readOffset;
        break;
    case READ_EMPTY:
        rm->readBlock = emptyRam;
        break;
    case READ_ROM:
        rm->readBlock = rm->romData + rm->readOffset;
        break;
    }
}

static void destroy(RomMapperA1FM* rm)
{
    sramSave(rm->sramFilename, rm->sram, rm->sramSize, NULL, 0);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    panasonicSramDestroy();

    free(rm->sram);
    free(rm->romData);
    free(rm);
}

static void changeBank(RomMapperA1FM* rm, int region, int bank)
{
    UInt8* readBlock;

	rm->romMapper[region] = bank;

	if (bank >= 0x80 && bank < 0x90) {
		if (bank & 0x04) {
            if (region == 3) {
                rm->readSection = READ_SRAM;
                rm->readOffset = 0;
                rm->readBlock = rm->sram;
            }
            readBlock = rm->sram;
		} 
        else {
            if (region == 3) {
                rm->readSection = READ_EMPTY;
                rm->readOffset = 0;
                rm->readBlock = emptyRam;
            }
            readBlock = emptyRam;
		}
    } 
    else {
        if (region == 3) {
            rm->readSection = READ_ROM;
            rm->readOffset = 0x2000 * (bank & 0x7f);
            rm->readBlock = rm->romData + rm->readOffset;
        }
        readBlock = rm->romData + 0x2000 * (bank & 0x7f);
	}

    if (region >=6) {
        readBlock = emptyRam;
    }

    slotMapPage(rm->slot, rm->sslot, region, readBlock, region != 3, 0);
}

static UInt8 read(RomMapperA1FM* rm, UInt16 address) 
{
	if ((rm->control & 0x04) && address >= 0x7ff0 && address < 0x7ff8) {
		return rm->romMapper[address & 7];
	} 
    
    return rm->readBlock[address & 0x1fff];
}

static void write(RomMapperA1FM* rm, UInt16 address, UInt8 value) 
{
    int bank;

    if (address >= 0x6000 && address < 0x7ff0) {
        static const int Regions[8] = { 2, 0, 3, 1, 4, -1, 5, -1 };

		int region = Regions[(address >> 10) & 7];
        if (region != -1) {
    		changeBank(rm, region, value);
        }
        return;
	} 
    
    if (address == 0x7ff9) {
		rm->control = value;
        return;
	}

    bank = rm->romMapper[address >> 13];
	if (bank >= 0x80 && bank < 0x90) {
		if (bank & 0x04) {
            rm->sram[address & 0x1fff] = value;
        }
    }
}

static void reset(RomMapperA1FM* rm)
{
    int i;

    rm->control = 0;
    
    for (i = 0; i < 6; i++) {
        changeBank(rm, i, 0xa8);
    }
    changeBank(rm, 6, 0);
    changeBank(rm, 7, 0);
}

int romMapperA1FMCreate(const char* filename, UInt8* romData,
                             int size, int slot, int sslot, int startPage,
                             int sramSize) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperA1FM* rm;
    char suffix[16];

    if (size < 0x8000 || startPage != 0) {
        return 0;
    }

    memset(emptyRam, 0xff, sizeof(emptyRam));

    rm = malloc(sizeof(RomMapperA1FM));

    rm->deviceHandle = deviceManagerRegister(ROM_PANASONIC8, &callbacks, rm);
    slotRegister(slot, sslot, 0, 8, read, read, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->romSize = size;
    rm->sramSize = sramSize;
    rm->sram = malloc(sramSize);
    memset(rm->sram, 0xff, sramSize);
    rm->maxSRAMBank = SRAM_BASE + sramSize / 0x2000;
    memset(rm->romMapper, 0, sizeof(rm->romMapper));
    rm->readBlock = rm->romData;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    sprintf(suffix, "_%d", sramSize / 1024);
    strcpy(rm->sramFilename, sramCreateFilenameWithSuffix(filename, suffix, NULL));

    sramLoad(rm->sramFilename, rm->sram, rm->sramSize, NULL, 0);

    panasonicSramCreate(rm->sram, rm->sramSize);

    reset(rm);

    return 1;
}
