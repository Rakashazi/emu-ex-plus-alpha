/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperF4device.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:43 $
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
#include "romMapperF4device.h"
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
    int deviceHandle;
    int debugHandle;
    int inverted;
    int status;
} RomMapperF4device;

static void saveState(RomMapperF4device* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperF4device");

    saveStateSet(state, "status", rm->status);

    saveStateClose(state);
}

static void loadState(RomMapperF4device* rm)
{
    SaveState* state = saveStateOpenForRead("mapperF4device");
    
    rm->status = saveStateGet(state, "status", 0);

    saveStateClose(state);
}

static void destroy(RomMapperF4device* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0xf4);

    free(rm);
}

static UInt8 read(RomMapperF4device* rm, UInt16 ioPort)
{
    return rm->status;
}

static void write(RomMapperF4device* rm, UInt16 ioPort, UInt8 value)
{	
	if (rm->inverted) {
		rm->status = value | 0x7f;
	} 
    else {
		rm->status = (rm->status & 0x20) | (value & 0xa0);
	}
}

static void reset(RomMapperF4device* rm)
{
    rm->status = rm->inverted ? 0xff : 0;
}

static void getDebugInfo(RomMapperF4device* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevF4Device(), 1);
    dbgIoPortsAddPort(ioPorts, 0, 0xf4, DBG_IO_READWRITE, read(rm, 0xe4));
}

int romMapperF4deviceCreate(int inverted) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperF4device* rm = malloc(sizeof(RomMapperF4device));

    rm->inverted   = inverted;
    rm->deviceHandle = deviceManagerRegister(inverted ? ROM_F4INVERTED : ROM_F4DEVICE, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevF4Device(), &dbgCallbacks, rm);

    ioPortRegister(0xf4, read, write, rm);

    reset(rm);

    return 1;
}
