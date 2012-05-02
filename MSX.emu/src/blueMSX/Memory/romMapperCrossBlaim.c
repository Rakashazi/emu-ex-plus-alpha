/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperCrossBlaim.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-06-14 12:20:25 $
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
#include "romMapperCrossBlaim.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
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
    int size;
    int romMapper[4]; // no need for 4, but kept in for savestate backward compatibility
} RomMapperCrossBlaim;

static void write(RomMapperCrossBlaim* rm, UInt16 address, UInt8 value);


static void saveState(RomMapperCrossBlaim* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperCrossBlaim");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateClose(state);
}

static void loadState(RomMapperCrossBlaim* rm)
{
    SaveState* state = saveStateOpenForRead("mapperCrossBlaim");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
    
    i=rm->romMapper[2];
    rm->romMapper[2]=-1;
    write(rm,0,i);
}

static void destroy(RomMapperCrossBlaim* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperCrossBlaim* rm, UInt16 address, UInt8 value) 
{
    value&=3;

    if (rm->romMapper[2] != value) {
    	
    	rm->romMapper[2] = value;
    	
    	if (value&2) {
    		// ROM 1
    		// page 2 specified bank
    		UInt8* bankData = rm->romData + ((int)value << 14);
    		
	        slotMapPage(rm->slot, rm->sslot, 4, bankData,          1, 0);
        	slotMapPage(rm->slot, rm->sslot, 5, bankData + 0x2000, 1, 0);
    		
    		// page 0 and 3 unmapped
    		slotMapPage(rm->slot, rm->sslot, 0, NULL, 0, 0);
    		slotMapPage(rm->slot, rm->sslot, 1, NULL, 0, 0);
    		slotMapPage(rm->slot, rm->sslot, 6, NULL, 0, 0);
    		slotMapPage(rm->slot, rm->sslot, 7, NULL, 0, 0);
    	}
    	else {
    		int i;
    		
    		// ROM 0
    		// page 2 bank 1, page 0 and 3 mirrors of page 2
    		for (i=0;i<8;i+=2) {
    			if (i==2) continue;
    			slotMapPage(rm->slot, rm->sslot, i,   rm->romData+0x4000, 1, 0);
    			slotMapPage(rm->slot, rm->sslot, i+1, rm->romData+0x6000, 1, 0);
    		}
    	}
    }
}

int romMapperCrossBlaimCreate(const char* filename, UInt8* romData,
                              int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperCrossBlaim* rm;

    if (size < 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperCrossBlaim));

    rm->deviceHandle = deviceManagerRegister(ROM_CROSSBLAIM, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, NULL, NULL, write, destroy, rm); // $0000-$FFFF

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    // page 1 fixed to ROM 0 bank 0
    slotMapPage(rm->slot, rm->sslot, 2, rm->romData+0x0000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, 3, rm->romData+0x2000, 1, 0);
    
    rm->romMapper[0] = 0;
    rm->romMapper[2] = -1;
    write(rm,0,0);

    return 1;
}

