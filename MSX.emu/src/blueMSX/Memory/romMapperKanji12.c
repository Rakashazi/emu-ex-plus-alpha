/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperKanji12.c,v $
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
#include "romMapperKanji12.h"
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
    UInt32 size;
    UInt32 address;
} RomMapperKanji12;

static void saveState(RomMapperKanji12* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperKanji12");

    saveStateSet(state, "address", rm->address);
    
    saveStateClose(state);
}

static void loadState(RomMapperKanji12* rm)
{
    SaveState* state = saveStateOpenForRead("mapperKanji12");

    rm->address = saveStateGet(state, "address", 0);

    saveStateClose(state);
}

static void destroy(RomMapperKanji12* rm)
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

static UInt8 peek(RomMapperKanji12* rm, UInt16 ioPort)
{
    UInt8 value = 0xff;

	switch (ioPort & 0x0f) {
	case 0:
		value = ~0xf7;
		break;
	case 1:
		value = 0x08;
		break;
	case 9:
		if (rm->address < rm->size) {
			value = rm->romData[rm->address];
		} else {
			value = 0xff;
		}
		break;
	}

	return value;
}

static UInt8 read(RomMapperKanji12* rm, UInt16 ioPort)
{
    UInt8 value = 0xff;

	switch (ioPort & 0x0f) {
	case 0:
		value = ~0xf7;
		break;
	case 1:
		value = 0x08;
		break;
	case 9:
		if (rm->address < rm->size) {
			value = rm->romData[rm->address];
		} else {
			value = 0xff;
		}
		rm->address++;
		break;
	}

	return value;
}

static void write(RomMapperKanji12* rm, UInt16 ioPort, UInt8 value)
{	
	switch (ioPort & 0x0f) {
	case 7: 
		rm->address = 0x800 + (value * 192 + ((rm->address - 0x800) / 18) % 192) * 18;
		break;
	case 8: 
		rm->address = 0x800 + (((rm->address - 0x800) / (18 * 192)) * 192 + value) * 18;
		break;
	}
}

static void getDebugInfo(RomMapperKanji12* rm, DbgDevice* dbgDevice)
{
    if (ioPortCheckSub(0xf7)) {
        DbgIoPorts* ioPorts;
        int i;

        ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevKanji12(), 2);

        for (i = 0; i < 16; i++) {
            dbgIoPortsAddPort(ioPorts, i, 0x40 + i, DBG_IO_READWRITE, peek(rm, i));
        }
    }
}

int romMapperKanji12Create(UInt8* romData, int size) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperKanji12* rm;

	if (size != 0x20000 && size != 0x40000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperKanji12));

    rm->size = size;
    rm->address = 0;

    rm->deviceHandle = deviceManagerRegister(ROM_KANJI12, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_CART, langDbgDevKanji12(), &dbgCallbacks, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    
    ioPortRegisterSub(0xf7, read, write, rm);

    return 1;
}

