/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperDRAM.c,v $
**
** $Revision: 1.6 $
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
#include "romMapperDRAM.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))


struct {
    PanasonicDramCallback callback;
    void* ref;
} DramCallbacks[8];

int panasonicDramRegister(PanasonicDramCallback callback, void* ref)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (DramCallbacks[i].callback == NULL) {
            DramCallbacks[i].callback = callback;
            DramCallbacks[i].ref = ref;
            return i;
        }
    }
    return -1;
}

void panasonicDramUnregister(int sramHandle)
{
    if (sramHandle >=0 && sramHandle < 8) {
        DramCallbacks[sramHandle].callback = NULL;
    }
}

void panasonicDramUpdate(int mode)
{
    int i;
    for (i = 0; i < 8; i++) {
        if (DramCallbacks[i].callback != NULL) {
            DramCallbacks[i].callback(DramCallbacks[i].ref, mode);
        }
    }
}

typedef struct {
    int deviceHandle;
    int sramHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int pages;
} RomMapperDram;

static void destroy(RomMapperDram* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    panasonicDramUnregister(rm->sramHandle);

    free(rm->romData);
    free(rm);
}

static void setDram(RomMapperDram* rm, int enable)
{
    if (enable) {
        if (rm->slot == 0 && rm->sslot == 0) {
            int endPage = MIN(4, rm->startPage + rm->pages);
            int page;
            for (page = rm->startPage; page < 4; page++) {
                slotMapPage(rm->slot, rm->sslot, page, boardGetRamPage(page - 8), 1, 0);
            }
        }
        else if (rm->slot == 3 && rm->sslot == 1) {
            int endPage = MIN(4, rm->startPage + rm->pages);
            int page;
            for (page = rm->startPage; page < 4; page++) {
                slotMapPage(rm->slot, rm->sslot, page, boardGetRamPage(page - 4), 1, 0);
            }
        }
    }
    else {
        int endPage = MIN(4, rm->startPage + rm->pages);
        int page;
        for (page = rm->startPage; page < 4; page++) {
            slotMapPage(rm->slot, rm->sslot, page, rm->romData + 0x2000 * (page - rm->startPage), 1, 0);
        }
    }
}

int romMapperDramCreate(const char* filename, UInt8* romData,
                        int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperDram* rm;
    int pages = size / 0x2000 + ((size & 0x1fff) ? 1 : 0);

    if (pages == 0 || (startPage + pages) > 8) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperDram));

    rm->deviceHandle = deviceManagerRegister(ROM_DRAM, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, NULL, NULL, NULL, destroy, rm);
    rm->sramHandle = panasonicDramRegister(setDram, rm);

    rm->romData = malloc(pages * 0x2000);
    memcpy(rm->romData, romData, size);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->pages = pages;

    setDram(rm, 0);

    return 1;
}

