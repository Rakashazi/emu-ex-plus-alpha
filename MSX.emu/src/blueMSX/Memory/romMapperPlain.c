/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperPlain.c,v $
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
#include "romMapperPlain.h"
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
} RomMapperPlain;

static void destroy(void* arg)
{
    RomMapperPlain* rm = (RomMapperPlain*)arg;

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

        if (size < 0x4000 * startPage + 0x10) {
            continue;
        }
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

    return 0x0000;
}


int romMapperPlainCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperPlain* rm;
    int romMapper[8];
    int i;

    if (size > 0x10000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperPlain));

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
#if 1
            for (i = 0; i < 8; i++) {
                romMapper[i] = (i + 2) & 3;
            }
#else
            for (i = 0; i < 4; i++) {
                romMapper[i] = i & 1;
                romMapper[i + 4] = 2 + (i & 1);
            }
#endif
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
    	free(rm->romData);
        free(rm);
        return 0;
    }

    rm->deviceHandle = deviceManagerRegister(ROM_PLAIN, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, NULL, NULL, NULL, destroy, rm);

    for (i = 0; i < 8; i++) {
        slotMapPage(slot, sslot, startPage + i, rm->romData + 0x2000 * romMapper[i], 1, 0);
    }

    return 1;
}

