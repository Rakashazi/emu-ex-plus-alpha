/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperBasic.c,v $
**
** $Revision: 1.6 $
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
#include "romMapperBasic.h"
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
} RomMapperBasic;

static void destroy(void* arg)
{
    RomMapperBasic* rm = (RomMapperBasic*)arg;

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}
int romMapperBasicCreate(const char* filename, UInt8* romData,
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperBasic* rm;

    if (size > 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperBasic));

    rm->romData = malloc(0x8000);
    memset(rm->romData, 0xff, 0x8000);
    memcpy(rm->romData, romData, size);

    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->deviceHandle = deviceManagerRegister(ROM_BASIC, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);

    slotMapPage(slot, sslot, startPage + 0, rm->romData + 0x0000, 1, 0);
    slotMapPage(slot, sslot, startPage + 1, rm->romData + 0x2000, 1, 0);
    slotMapPage(slot, sslot, startPage + 2, rm->romData + 0x4000, 1, 0);
    slotMapPage(slot, sslot, startPage + 3, rm->romData + 0x6000, 1, 0);

    return 1;
}
