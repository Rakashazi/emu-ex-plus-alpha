/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperForteII.c,v $
**
** $Revision: 1.2 $
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
#include "romMapperForteII.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    int slot;
    int sslot;
    int startPage;
    UInt8 romData[0x10000];
} RomMapperForteII;

static void destroy(RomMapperForteII* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    free(rm);
}

static UInt16 eeprom2cpu16(UInt16 v)
{
    return (((v >> 10) & 1) <<  0) |
           (((v >>  0) & 1) <<  1) |
           (((v >>  1) & 1) <<  2) |
           (((v >>  2) & 1) <<  3) |
           (((v >>  3) & 1) <<  4) |
           (((v >>  4) & 1) <<  5) |
           (((v >>  5) & 1) <<  6) |
           (((v >>  6) & 1) <<  7) |
           (((v >>  7) & 1) <<  8) |
           (((v >> 12) & 1) <<  9) |
           (((v >> 15) & 1) << 10) |
           (((v >> 14) & 1) << 11) |
           (((v >> 13) & 1) << 12) |
           (((v >>  8) & 1) << 13) |
           (((v >>  9) & 1) << 14) |
           (((v >> 11) & 1) << 15);
}

static UInt8 eeprom2cpu8(UInt8 v)
{
    return (((v >>  1) & 1) <<  0) |
           (((v >>  2) & 1) <<  1) |
           (((v >>  4) & 1) <<  2) |
           (((v >>  0) & 1) <<  3) |
           (((v >>  7) & 1) <<  4) |
           (((v >>  6) & 1) <<  5) |
           (((v >>  5) & 1) <<  6) |
           (((v >>  3) & 1) <<  7);
}

int romMapperForteIICreate(const char* filename, UInt8* romData,
                           int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperForteII* rm;
    int i;

    if (size > 0x10000) {
        return size = 0x10000;
    }

    rm = malloc(sizeof(RomMapperForteII));
    memset(rm->romData, 0xff, 0x10000);
    
    for (i = 0; i < size; i++) {
        rm->romData[eeprom2cpu16(i)] = eeprom2cpu8(romData[i]);
    }

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->deviceHandle = deviceManagerRegister(ROM_FORTEII, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 8, NULL, NULL, NULL, destroy, rm);

    for (i = 0; i < 8; i++) {
        slotMapPage(slot, sslot, startPage + i, rm->romData + 0x2000 * i, 1, 0);
    }

    return 1;
}

