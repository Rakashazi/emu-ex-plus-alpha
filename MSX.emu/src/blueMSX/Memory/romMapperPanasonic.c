/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperPanasonic.c,v $
**
** $Revision: 1.15 $
**
** $Date: 2009-07-03 21:27:14 $
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
#include "romMapperPanasonic.h"
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
    READ_RAM,
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
    int    mappedSlots;
    UInt8  control;
    int    romMapper[8];
    int    slot;
    int    sslot;
    int    startPage;
} RomMapperPanasonic;

extern void panasonicSramDestroy();
extern void panasonicSramCreate(UInt8* sram, UInt32 size);

static UInt8 emptyRam[0x2000];

static int SRAM_BASE = 0x80;
static int RAM_BASE  = 0x180;

static void changeBank(RomMapperPanasonic* rm, int region, int bank);

static void saveState(RomMapperPanasonic* rm)
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

static void loadState(RomMapperPanasonic* rm)
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
    case READ_RAM:
        rm->readBlock = boardGetRamPage(rm->readOffset);
        if (rm->readBlock == NULL) {
            rm->readBlock = emptyRam;
        }
        break;
    case READ_ROM:
        rm->readBlock = rm->romData + rm->readOffset;
        break;
    }
}

static void destroy(RomMapperPanasonic* rm)
{
    sramSave(rm->sramFilename, rm->sram, rm->sramSize, NULL, 0);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    panasonicSramDestroy();

    free(rm->sram);
    free(rm->romData);
    free(rm);
}

static void changeBank(RomMapperPanasonic* rm, int region, int bank)
{
	if (bank == rm->romMapper[region]) {
		return;
	}
	rm->romMapper[region] = bank;
	if (rm->sramSize > 0 && bank >= SRAM_BASE && bank < rm->maxSRAMBank) {
		int offset = (bank - SRAM_BASE) * 0x2000 & (rm->sramSize - 1);
        if (region == 3) {
            rm->readSection = READ_SRAM;
            rm->readOffset = offset;
            rm->readBlock = rm->sram + offset;
        }
        slotMapPage(rm->slot, rm->sslot, region, rm->sram + offset, region != 3, 0);
	} 
    else if (bank >= RAM_BASE) {
        UInt8* ram;

        ram = boardGetRamPage(bank - RAM_BASE);
        if (ram == NULL) {
            ram = emptyRam;
        }

        if (region == 3) {
            rm->readSection = READ_RAM;
            rm->readOffset = bank - RAM_BASE;
            rm->readBlock = ram;
        }

        slotMapPage(rm->slot, rm->sslot, region, ram, region != 3, 0);
	} 
    else {
		int offset = bank * 0x2000 & (rm->romSize - 1);
        if (region == 3) {
            rm->readSection = READ_ROM;
            rm->readOffset = offset;
            rm->readBlock = rm->romData + offset;
        }
        slotMapPage(rm->slot, rm->sslot, region, rm->romData + offset, region != 3, 0);
	}
}

static UInt8 read(RomMapperPanasonic* rm, UInt16 address) 
{
    int bank;

	if ((rm->control & 0x04) && address >= 0x7ff0 && address < 0x7ff8) {
		return rm->romMapper[address & 7] & 0xff;
	} 
    
    if ((rm->control & 0x10) && address == 0x7ff8) {
        int i;
		UInt8 result = 0;
		for (i = 7; i >= 0; i--) {
			result <<= 1;
			if (rm->romMapper[i] & 0x100) {
				result++;
			}
		}
        return result;
	} 
    
    if ((rm->control & 0x08) && address == 0x7ff9) {
		return rm->control;
	} 

    return rm->readBlock[address & 0x1fff];

	bank = rm->romMapper[address >> 13];
    if (bank < SRAM_BASE) {
        return rm->romData[(bank * 0x2000 & (rm->romSize - 1))  + (address & 0x1fff)];
    }
    if (rm->sramSize > 0 && bank >= SRAM_BASE && bank < rm->maxSRAMBank) {
		int offset = (bank - SRAM_BASE) * 0x2000 & (rm->sramSize - 1);
        return rm->sram[offset + (address & 0x1fff)];
	} 
    if (bank >= RAM_BASE) {
        UInt8* ram = boardGetRamPage(bank - RAM_BASE);
        if (ram == NULL) {
            ram = emptyRam;
        }
        return ram[address & 0x1fff];
	}

    return 0xff;
}

static void write(RomMapperPanasonic* rm, UInt16 address, UInt8 value) 
{
    int region;
    int bank;
    int newBank;

    if (address >= 0x6000 && address < 0x7ff0) {
		region = (address & 0x1c00) >> 10;
        if (region == 5 || region == 6) {
            region ^= 3;
        }

		bank = rm->romMapper[region];
		newBank = (bank & ~0xff) | value;
		changeBank(rm, region, newBank);
        return;
	} 
    
    if (address == 0x7ff8) {
		for (region = 0; region < 8; region++) {
			if (value & 1) {
				changeBank(rm, region, rm->romMapper[region] |  0x100);
			} else {
				changeBank(rm, region, rm->romMapper[region] & ~0x100);
			}
			value >>= 1;
		}
        return;
	} 
    
    if (address == 0x7ff9) {
		rm->control = value;
        return;
	} 

    if (address >= 0x8000 && address < 0xC000) {
		region = address >> 13;
		bank = rm->romMapper[region];
		
        if (rm->sramSize > 0 && bank >= SRAM_BASE && bank < rm->maxSRAMBank) {
		    int offset = (bank - SRAM_BASE) * 0x2000 & (rm->sramSize - 1);
            rm->sram[offset + (address & 0x1fff)] = value;
		} 
        else if (bank >= RAM_BASE) {
            UInt8* ram = boardGetRamPage(bank - RAM_BASE);
            if (ram != NULL) {
                ram[address & 0x1fff] = value;
            }
		}
	} 
}

static void reset(RomMapperPanasonic* rm)
{
    int i;

    rm->control = 0;
    
    for (i = 0; i < rm->mappedSlots; i++) {
        rm->romMapper[i] = 0;
        slotMapPage(rm->slot, rm->sslot, i, rm->romData, i != 3, 0);
    }
}

int romMapperPanasonicCreate(const char* filename, UInt8* romData,
                             int size, int slot, int sslot, int startPage,
                             int sramSize, int mappedSlots) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperPanasonic* rm;
    RomType romType;
    char suffix[16];

    if (size < 0x8000 || startPage != 0) {
        return 0;
    }

    memset(emptyRam, 0xff, sizeof(emptyRam));

    rm = malloc(sizeof(RomMapperPanasonic));

    rm->mappedSlots = mappedSlots;

    if (mappedSlots == 6) {
        rm->maxSRAMBank = SRAM_BASE + 8;
        romType = ROM_PANASONICWX16;
    }
    else {
        rm->maxSRAMBank = SRAM_BASE + sramSize / 0x2000;
        switch (sramSize) {
        default:
        case 0x4000:
            romType = ROM_PANASONIC16;
            break;
        case 0x8000:
            romType = ROM_PANASONIC32;
            break;
        }
    }

    rm->deviceHandle = deviceManagerRegister(romType, &callbacks, rm);
    slotRegister(slot, sslot, 0, rm->mappedSlots, read, read, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->romSize = size;
    rm->sramSize = sramSize;
    rm->sram = malloc(sramSize);
    memset(rm->sram, 0xff, sramSize);
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
