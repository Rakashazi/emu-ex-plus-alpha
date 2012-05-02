/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSf7000Ipl.c,v $
**
** $Revision: 1.4 $
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
#include "romMapperSf7000Ipl.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
} RomMapperSf7000Ipl;

static void destroy(RomMapperSf7000Ipl* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperSf7000Ipl* rm, UInt16 address, UInt8 value) 
{
    UInt8* ramPage = boardGetRamPage(address / 0x2000);

    if (ramPage != NULL) {
        ramPage[address & 0x1fff] = value;
    }
}

int romMapperSf7000IplCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperSf7000Ipl* rm;
    int pages = 2;
    int i;

    rm = malloc(sizeof(RomMapperSf7000Ipl));

    rm->deviceHandle = deviceManagerRegister(ROM_NORMAL, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, NULL, NULL, write, destroy, rm);

    rm->romData = malloc(0x4000);
    if (size > 0x4000) {
        size = 0x4000;
    }
    memcpy(rm->romData, romData, size);
    memset(rm->romData + size, 0xff, 0x4000 - size);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
    }

    return 1;
}

