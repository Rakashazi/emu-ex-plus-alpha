/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperFMPAK.c,v $
**
** $Revision: 1.8 $
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
#include "romMapperFMPAK.h"
#include "MediaDb.h"
#include "IoPort.h"
#include "Board.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "YM2413.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    int debugHandle;
    YM_2413* ym2413;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int size;
} RomMapperFMPAK;


static void saveState(RomMapperFMPAK* rm)
{
    if (rm->ym2413 != NULL) {
        ym2413SaveState(rm->ym2413);
    }
}

static void loadState(RomMapperFMPAK* rm)
{
    if (rm->ym2413 != NULL) {
        ym2413LoadState(rm->ym2413);
    }
}

static void reset(RomMapperFMPAK* rm) 
{
    if (rm->ym2413 != NULL) {
        ym2413Reset(rm->ym2413);
    }
}

static void destroy(void* arg)
{
    RomMapperFMPAK* rm = (RomMapperFMPAK*)arg;

    ioPortUnregister(0x7c);
    ioPortUnregister(0x7d);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}



static UInt16 getRomStart(UInt8* romData, int size) 
{
    int pages[3] = { 0, 0, 0 };
    int startPage;
    int i;

    for (startPage = 0; startPage < 2; startPage++) {
        UInt8* romPtr = romData + 0x4000 * startPage;

	    if (romPtr[0] == 'A' && romPtr[1] =='B') {
		    for (i = 0; i < 4; i++) {
                UInt16 address = romPtr[2 * i + 2] + 256 * (UInt16)romPtr[2 * i + 3];

			    if (address > 0) {
				    UInt16 page = address / 0x4000 - startPage;

                    if (page < 3) {
                        pages[page]++;
				    }
			    }
		    }
        }
	}

    if (pages[1] && (pages[1] >= pages[0]) && (pages[1] >= pages[2])) {
		return 0x4000;
	} 
    
    if (pages[0] && pages[0] >= pages[2]) {
		return 0x0000;
	} 
    
    if (pages[2]) {
		return 0x8000;
	}

    return 0x4000;
}

static void writeIo(RomMapperFMPAK* rm, UInt16 port, UInt8 data)
{
    switch (port & 1) {
    case 0:
        ym2413WriteAddress(rm->ym2413, data);
        break;
    case 1:
        ym2413WriteData(rm->ym2413, data);
        break;
    }
}

static void getDebugInfo(RomMapperFMPAK* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    if (rm->ym2413 == NULL) {
        return;
    }

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevFmpak(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0x7c, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 1, 0x7d, DBG_IO_WRITE, 0);
    
    ym2413GetDebugInfo(rm->ym2413, dbgDevice);
}

int romMapperFMPAKCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperFMPAK* rm;
    int romMapper[8];
    int i;

    if (size > 0x10000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperFMPAK));

    rm->romData = malloc(0x10000);
    memset(rm->romData, 0xff, 0x10000);
    memcpy(rm->romData, romData, size);

    // Align ROM size up to next valid rom size
    if      (size <= 0x2000)  size = 0x2000;
    else if (size <= 0x4000)  size = 0x4000;
    else if (size <= 0x8000)  size = 0x8000;
    else if (size <= 0xc000)  size = 0xc000;
    else if (size <= 0x10000) size = 0x10000;

    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    switch (size) {
    case 0x2000:
        for (i = 0; i < 8; i++) {
            romMapper[i] = 0;
        }
        break;

    case 0x4000:
        for (i = 0; i < 8; i++) {
            romMapper[i] = i & 1;
        }
        break;
        
    case 0x8000:
        if (getRomStart(romData, size) == 0x4000) {
            for (i = 0; i < 4; i++) {
                romMapper[i] = i & 1;
                romMapper[i + 4] = 2 + (i & 1);
            }
        }
        else {
            for (i = 0; i < 8; i++) {
                romMapper[i] = i & 3;
            }
        }
        break;
        
    case 0xc000:
        if (getRomStart(romData, size) == 0x4000) {
            romMapper[0] = 0;
            romMapper[1] = 1;

            for (i = 0; i < 6; i++) {
                romMapper[i + 2] = i;
            }
        }
        else {
            for (i = 0; i < 6; i++) {
                romMapper[i] = i;
            }
            
            romMapper[6] = 0;
            romMapper[7] = 1;
        }
        break;

    case 0x10000:
        for (i = 0; i < 8; i++) {
            romMapper[i] = i;
        }
        break;
        
    default:
        free(rm);
        return 0;
    }
    
    rm->ym2413 = NULL;
    if (boardGetYm2413Enable()) {
        rm->ym2413 = ym2413Create(boardGetMixer());
        rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevFmpak(), &dbgCallbacks, rm);
        ioPortRegister(0x7c, NULL, writeIo, rm);
        ioPortRegister(0x7d, NULL, writeIo, rm);
    }

    rm->deviceHandle = deviceManagerRegister(ROM_FMPAK, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, NULL, NULL, NULL, destroy, rm);

    for (i = 0; i < 8; i++) {
        slotMapPage(slot, sslot, startPage + i, rm->romData + 0x2000 * romMapper[i], 1, 0);
    }

    return 1;
}

