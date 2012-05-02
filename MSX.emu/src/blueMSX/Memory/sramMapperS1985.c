/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/sramMapperS1985.c,v $
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
#include "sramMapperMatsuchita.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "sramLoader.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern int frontSwitchEnabled();

typedef struct {
    int    deviceHandle;
    int    debugHandle;
    UInt8  sram[0x10];
    UInt32 address;
	UInt8  color1;
    UInt8  color2;
	UInt8  pattern;
} SramMapperS1985;

static void saveState(SramMapperS1985* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperS1985");

    saveStateSet(state, "address", rm->address);
    saveStateSet(state, "color1",  rm->color1);
    saveStateSet(state, "color2",  rm->color2);
    saveStateSet(state, "pattern", rm->pattern);
    
    saveStateClose(state);
}

static void loadState(SramMapperS1985* rm)
{
    SaveState* state = saveStateOpenForRead("mapperS1985");

    rm->address =        saveStateGet(state, "address", 0);
    rm->color1  = (UInt8)saveStateGet(state, "color1",  0);
    rm->color2  = (UInt8)saveStateGet(state, "color2",  0);
    rm->pattern = (UInt8)saveStateGet(state, "pattern", 0);

    saveStateClose(state);
}

static void destroy(SramMapperS1985* rm)
{
    sramSave(sramCreateFilename("S1985.SRAM"), rm->sram, 0x10, NULL, 0);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregisterSub(0x08);

    free(rm);
}

static UInt8 peek(SramMapperS1985* rm, UInt16 ioPort)
{
	UInt8 result;
	switch (ioPort & 0x0f) {
	case 0:
		result = ~0xfe;
		break;
	case 2:
		result = rm->sram[rm->address];
		break;
	case 7:
		result = (rm->pattern & 0x80) ? rm->color2 : rm->color1;
		break;
	default:
		result = 0xff;
	}

	return result;
}

static UInt8 read(SramMapperS1985* rm, UInt16 ioPort)
{
	UInt8 result;
	switch (ioPort & 0x0f) {
	case 0:
		result = ~0xfe;
		break;
	case 2:
		result = rm->sram[rm->address];
		break;
	case 7:
		result = (rm->pattern & 0x80) ? rm->color2 : rm->color1;
		rm->pattern = (rm->pattern << 1) | (rm->pattern >> 7);
		break;
	default:
		result = 0xff;
	}

	return result;
}

static void write(SramMapperS1985* rm, UInt16 ioPort, UInt8 value)
{
	switch (ioPort & 0x0f) {
	case 1:
		rm->address = value & 0x0f;
		break;
	case 2:
		rm->sram[rm->address] = value;
		break;
	case 6:
		rm->color2 = rm->color1;
		rm->color1 = value;
		break;
	case 7:
		rm->pattern = value;
		break;
	}
}

static void getDebugInfo(SramMapperS1985* rm, DbgDevice* dbgDevice)
{
    if (ioPortCheckSub(0xfe)) {
        DbgIoPorts* ioPorts;
        int i;

        ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevKanji12(), 16);

        for (i = 0; i < 16; i++) {
            dbgIoPortsAddPort(ioPorts, i, 0x40 + i, DBG_IO_READWRITE, peek(rm, i));
        }
    }
}

int sramMapperS1985Create() 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    SramMapperS1985* rm;

    rm = malloc(sizeof(SramMapperS1985));

    rm->deviceHandle = deviceManagerRegister(SRAM_S1985, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevS1985(), &dbgCallbacks, rm);

    memset(rm->sram, 0xff, 0x10);
    rm->address = 0;

    sramLoad(sramCreateFilename("S1985.SRAM"), rm->sram, 0x10, NULL, 0);

    ioPortRegisterSub(0xfe, read, write, rm);

    return 1;
}

