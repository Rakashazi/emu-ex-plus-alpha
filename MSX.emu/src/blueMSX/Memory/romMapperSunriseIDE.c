/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSunriseIDE.c,v $
**
** $Revision: 1.9 $
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
#include "romMapperSunriseIDE.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SunriseIDE.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    UInt32 romMask;

    SunriseIde* ide;
    int    ideEnabled;
    UInt8  readLatch;
    UInt8  writeLatch;
    int    romMapper;
} RomMapperSunriseIde;


static void saveState(RomMapperSunriseIde* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperSunriseIde");

    saveStateSet(state, "ideEnabled", rm->ideEnabled);
    saveStateSet(state, "readLatch", rm->readLatch);
    saveStateSet(state, "writeLatch", rm->writeLatch);
    saveStateSet(state, "romMapper", rm->romMapper);

    saveStateClose(state);

    sunriseIdeSaveState(rm->ide);
}

static void loadState(RomMapperSunriseIde* rm)
{
    SaveState* state = saveStateOpenForRead("mapperSunriseIde");

    rm->ideEnabled = saveStateGet(state, "ideEnabled", 0);
    rm->readLatch = (UInt8)saveStateGet(state, "readLatch", 0);
    rm->writeLatch = (UInt8)saveStateGet(state, "writeLatch", 0);
    rm->romMapper = saveStateGet(state, "romMapper", 0);

    saveStateClose(state);
    
    sunriseIdeLoadState(rm->ide);
}

static void destroy(RomMapperSunriseIde* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    sunriseIdeDestroy(rm->ide);

    free(rm->romData);
    free(rm);
}

static UInt8 read(RomMapperSunriseIde* rm, UInt16 address) 
{
	if (rm->ideEnabled && (address & 0x3e00) == 0x3c00) {
		if ((address & 1) == 0) {
	        UInt16 value = sunriseIdeRead(rm->ide);
	        rm->readLatch = value >> 8;
	        return value & 0xff;
        }
		return rm->readLatch;
	}

	if (rm->ideEnabled && (address & 0x3f00) == 0x3e00) {
        return sunriseIdeReadRegister(rm->ide, address & 0x0f);
	}
    address -= 0x4000;
	if (address < 0x4000) {
		return rm->romData[address + rm->romMapper];
	}

    return 0xff;
}

static UInt8 peek(RomMapperSunriseIde* rm, UInt16 address) 
{
	if (rm->ideEnabled && (address & 0x3e00) == 0x3c00) {
		if ((address & 1) == 0) {
	        UInt16 value = sunriseIdePeek(rm->ide);
	        return value & 0xff;
        }
		return rm->readLatch;
	}

	if (rm->ideEnabled && (address & 0x3f00) == 0x3e00) {
        return sunriseIdePeekRegister(rm->ide, address & 0x0f);
	}
    address -= 0x4000;
	if (address < 0x4000) {
		return rm->romData[address + rm->romMapper];
	}

    return 0xff;
}

static void write(RomMapperSunriseIde* rm, UInt16 address, UInt8 value) 
{
	if ((address & 0xbf04) == 0x0104) {
        rm->ideEnabled = value & 1;
            
	    value = ((value & 0xf0) >> 4) | ((value & 0x0f) << 4);
	    value = ((value & 0xcc) >> 2) | ((value & 0x33) << 2);
	    value = ((value & 0xaa) >> 1) | ((value & 0x55) << 1);
        rm->romMapper = 0x4000 * (value & rm->romMask);
		return;
	}

	if (rm->ideEnabled && (address & 0x3e00) == 0x3c00) {
		if ((address & 1) == 0) {
            rm->writeLatch = value;
		} 
        else {
            sunriseIdeWrite(rm->ide, (value << 8) | rm->writeLatch);
		}
		return;
	}
	if (rm->ideEnabled && ((address & 0x3f00) == 0x3e00)) {
		sunriseIdeWriteRegister(rm->ide, address & 0x0f, value);
		return;
	}
}

static void reset(RomMapperSunriseIde* rm) 
{
#if 0
    rm->ideEnabled = 1;
    rm->romMapper = rm->romMask * 0x4000;
    rm->readLatch = 0;
    rm->writeLatch = 0;
#endif
    sunriseIdeReset(rm->ide);
}


int romMapperSunriseIdeCreate(int hdId, const char* filename, UInt8* romData,
                              int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperSunriseIde* rm;
    int i;
    int origSize = size;

    if (startPage != 0) {
        return 0;
    }

    
    size = 0x4000;
    while (size < origSize) {
        size *= 2;
    }

    if (romData == NULL) {
        size = 0x80000;
    }

    rm = malloc(sizeof(RomMapperSunriseIde));

    rm->deviceHandle = deviceManagerRegister(ROM_SUNRISEIDE, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, read, peek, write, destroy, rm);

    rm->ide = sunriseIdeCreate(hdId);

    rm->romData = calloc(1, size);
    if (romData != NULL) {
        memcpy(rm->romData, romData, origSize);
    }
    else {
        memset(rm->romData, 0xff, size);
    }
    rm->romMask = size / 0x4000 - 1;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper = 0;

    for (i = 0; i < 8; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, NULL, 0, 0);
    }

    rm->ideEnabled = 1;
    rm->romMapper = 0;

    reset(rm);

    return 1;
}

