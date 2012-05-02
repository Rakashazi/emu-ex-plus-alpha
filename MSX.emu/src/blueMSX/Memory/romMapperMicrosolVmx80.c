/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMicrosolVmx80.c,v $
**
** $Revision: 1.10 $
**
** $Date: 2008-03-31 19:42:22 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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
#include "romMapperMicrosolVmx80.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "CRTC6845.h"
#include "RomLoader.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int deviceHandle;
    UInt8* romData;
    UInt8* charData;
    int slot;
    int sslot;
    int startPage;
    CRTC6845* crtc6845;
} RomMapperMicrosolVmx80;

static void saveState(RomMapperMicrosolVmx80* rm)
{
    SaveState* state = saveStateOpenForWrite("Vmx80");
    saveStateClose(state);
}

static void loadState(RomMapperMicrosolVmx80* rm)
{
    SaveState* state = saveStateOpenForRead("Vmx80");
    saveStateClose(state);
}

static void destroy(RomMapperMicrosolVmx80* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm->charData);
    free(rm);
}

static UInt8 read(RomMapperMicrosolVmx80* rm, UInt16 address)
{
    if (address == 0x3001) {
        return crtcRead(rm->crtc6845);
    }

    if (address > 0x1fff  && address < 0x2800) {
        return crtcMemRead(rm->crtc6845, address & 0x07ff);
    }

    return address < 0x4000 ? rm->romData[address] : 0xff;
}

static void write(RomMapperMicrosolVmx80* rm, UInt16 address, UInt8 value) 
{
    if (address >= 0x2000 && address < 0x2800) {
        crtcMemWrite(rm->crtc6845, address & 0x07ff, value);
    }
    if (address >= 0x3000 && address < 0x3800) {
        if (address & 1) {
            crtcWrite(rm->crtc6845, value);
        }
        else {
	        crtcWriteLatch(rm->crtc6845, value);
        }
    }
}
	
static void reset(RomMapperMicrosolVmx80* rm)
{
}

int romMapperMicrosolVmx80Create(const char* filename, UInt8* romData, int size,
                                 int slot, int sslot, int startPage,
                                 void* charRom, int charSize) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperMicrosolVmx80* rm;
    int pages = 2;
    int i;

    if ((startPage + pages) > 8) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperMicrosolVmx80));

    rm->deviceHandle = deviceManagerRegister(ROM_MICROSOL80, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 2, read, read, write, destroy, rm);

    rm->charData = calloc(1, 0x2000);
    if (charRom != NULL) {
        if (charSize > 0x2000) {
            charSize = 0x2000;
        }
        memcpy(rm->charData, charRom, charSize);
    }

    rm->crtc6845 = NULL;
    rm->crtc6845 = crtc6845Create(50, rm->charData, charSize, 0x0800, 7, 0, 80, 4);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, size);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

    reset(rm);

    return 1;
}
