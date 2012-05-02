/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperKanji.c,v $
**
** $Revision: 1.7 $
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
#include "romMapperKanji.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    UInt8* romData;
    int deviceHandle;
    int debugHandle;
    int size;
    UInt32 address[2];
} RomMapperKanji;

static void saveState(RomMapperKanji* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperKanji");

    saveStateSet(state, "address0", rm->address[0]);
    saveStateSet(state, "address1", rm->address[1]);
    
    saveStateClose(state);
}

static void loadState(RomMapperKanji* rm)
{
    SaveState* state = saveStateOpenForRead("mapperKanji");

    rm->address[0] = saveStateGet(state, "address0", 0);
    rm->address[1] = saveStateGet(state, "address1", 0);

    saveStateClose(state);
}

static void destroy(RomMapperKanji* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0xd9);
    ioPortUnregister(0xd8);
    ioPortUnregister(0xda);
    ioPortUnregister(0xdb);

    free(rm->romData);
    free(rm);
}

static UInt8 peek(RomMapperKanji* rm, UInt16 ioPort)
{
    UInt32* reg = &rm->address[(ioPort & 2) >> 1];
    UInt8 value;

	if (reg == &rm->address[1] && rm->size != 0x40000) {
        return 0xff;
    }

    value = rm->romData[*reg];

    return value;
}

static UInt8 read(RomMapperKanji* rm, UInt16 ioPort)
{
    UInt32* reg = &rm->address[(ioPort & 2) >> 1];
    UInt8 value;

	if (reg == &rm->address[1] && rm->size != 0x40000) {
        return 0xff;
    }

    value = rm->romData[*reg];

    *reg = (*reg & ~0x1f) | ((*reg + 1) & 0x1f);

    return value;
}

static void write(RomMapperKanji* rm, UInt16 ioPort, UInt8 value)
{	
    switch (ioPort & 0x03) {
	case 0:
		rm->address[0] = (rm->address[0] & 0x1f800) | ((value & 0x3f) << 5);
		break;
	case 1:
		rm->address[0] = (rm->address[0] & 0x007e0) | ((value & 0x3f) << 11);
		break;
	case 2:
		rm->address[1] = (rm->address[1] & 0x3f800) | ((value & 0x3f) << 5);
		break;
	case 3:
		rm->address[1] = (rm->address[1] & 0x207e0) | ((value & 0x3f) << 11);
		break;
	}
}

static void getDebugInfo(RomMapperKanji* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevKanji(), 4);
    dbgIoPortsAddPort(ioPorts, 0, 0xd8, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 1, 0xd9, DBG_IO_READWRITE, peek(rm, 0xd9));
    dbgIoPortsAddPort(ioPorts, 2, 0xda, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 3, 0xdb, DBG_IO_READWRITE, peek(rm, 0xdb));
}

int romMapperKanjiCreate(UInt8* romData, int size) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperKanji* rm;

	if (size != 0x20000 && size != 0x40000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperKanji));

    rm->size = size;
    rm->address[0] = 0;
    rm->address[1] = 0x20000;

    rm->deviceHandle = deviceManagerRegister(ROM_KANJI, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevKanji(), &dbgCallbacks, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    
    ioPortRegister(0xd8, NULL, write, rm);
    ioPortRegister(0xd9, read, write, rm);
    ioPortRegister(0xda, NULL, write, rm);
    ioPortRegister(0xdb, read, write, rm);

    return 1;
}

