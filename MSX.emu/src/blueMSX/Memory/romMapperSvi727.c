/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSvi727.c,v $
**
** $Revision: 1.8 $
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
#include "romMapperSvi727.h"
#include "CRTC6845.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "RomLoader.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int deviceHandle;
    UInt8* charData;
    int slot;
    int sslot;
    int startPage;
    CRTC6845* crtc6845;
} RomMapperSvi727;

static void saveState(RomMapperSvi727* rm)
{
    SaveState* state = saveStateOpenForWrite("Svi727");
    saveStateClose(state);
}

static void loadState(RomMapperSvi727* rm)
{
    SaveState* state = saveStateOpenForRead("Svi727");
    saveStateClose(state);
}

static void destroy(RomMapperSvi727* rm)
{
    ioPortUnregister(0x78);
    ioPortUnregister(0x79);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->charData);
    free(rm);
}

static UInt8 read(RomMapperSvi727* rm, UInt16 address)
{
    UInt8 value = 0xff;
    if (address >= 0xb800 && address < 0xc000) {
        value = crtcMemRead(rm->crtc6845, address & 0x07ff);
    }

    return value;
}

static void write(RomMapperSvi727* rm, UInt16 address, UInt8 value) 
{
    if (address >= 0xb800 && address < 0xc000) {
        crtcMemWrite(rm->crtc6845, address & 0x07ff, value);
    }
}

static UInt8 readIo(RomMapperSvi727* rm, UInt16 ioPort) 
{
    UInt8 value = crtcRead(rm->crtc6845);
    return value;
}

static void writeIo(RomMapperSvi727* rm, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort) {
    case 0x78:
        crtcWriteLatch(rm->crtc6845, value);
        break;
    case 0x79:
        crtcWrite(rm->crtc6845, value);
        break;
    }
}

static void reset(RomMapperSvi727* rm)
{
}

int romMapperSvi727Create(const char* filename, UInt8* charRom, int charSize,
                                 int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperSvi727* rm;
    int pages = 8;
    int i;

    startPage = 0;

    rm = malloc(sizeof(RomMapperSvi727));

    rm->deviceHandle = deviceManagerRegister(ROM_SVI727, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, read, read, write, destroy, rm);

    rm->charData = calloc(1, 0x2000);
    if (charRom != NULL) {
        charSize += 0x200;
        if (charSize > 0x2000) {
            charSize = 0x2000;
        }
        memcpy(rm->charData + 0x200, charRom, charSize - 0x200);
    }

    rm->crtc6845 = NULL;
    rm->crtc6845 = crtc6845Create(50, rm->charData, charSize, 0x0800, 7, 0, 80, 4);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

    ioPortRegister(0x78, NULL,   writeIo, rm);
    ioPortRegister(0x79, readIo, writeIo, rm);

    reset(rm);

    return 1;
}
