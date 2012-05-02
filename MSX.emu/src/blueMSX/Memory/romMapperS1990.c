/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperS1990.c,v $
**
** $Revision: 1.8 $
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
#include "romMapperDRAM.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Switches.h"
#include "IoPort.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern void msxSetCpu(int mode);

typedef struct {
    int   deviceHandle;
    int   debugHandle;
	UInt8 registerSelect;
	UInt8 cpuStatus;
} RomMapperS1990;

static void saveState(RomMapperS1990* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperS1990");

    saveStateSet(state, "registerSelect",  rm->registerSelect);
    saveStateSet(state, "cpuStatus",       rm->cpuStatus);
    
    saveStateClose(state);
}

static void loadState(RomMapperS1990* rm)
{
    SaveState* state = saveStateOpenForRead("mapperS1990");

    rm->registerSelect = (UInt8)saveStateGet(state, "registerSelect",  0);
    rm->cpuStatus      = (UInt8)saveStateGet(state, "cpuStatus",       0);

    saveStateClose(state);
}

static void destroy(RomMapperS1990* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    ioPortUnregister(0xe4);
    ioPortUnregister(0xe5);

    free(rm);
}

static void updateStatus(RomMapperS1990* rm, UInt8 value)
{
	rm->cpuStatus = value & 0x60;
    msxSetCpu((rm->cpuStatus & 0x20) ? 0 : 1);
    panasonicDramUpdate((rm->cpuStatus & 0x40) ? 0 : 1);
}

static UInt8 read(RomMapperS1990* rm, UInt16 ioPort)
{
	switch (ioPort & 0x01) {
	case 0:
		return rm->registerSelect;
	case 1:
		switch (rm->registerSelect) {
		case 5:
			return switchGetFront() ? 0x40 : 0x00;
		case 6:
			return rm->cpuStatus;
		case 13:
			return 0x03;	//TODO
		case 14:
			return 0x2F;	//TODO
		case 15:
			return 0x8B;	//TODO
		default:
			return 0xFF;
		}
	}

    return 0;
}

static void write(RomMapperS1990* rm, UInt16 ioPort, UInt8 value)
{
	switch (ioPort & 0x01) {
	case 0:
		rm->registerSelect = value;
		break;
	case 1:
		switch (rm->registerSelect) {
		case 6:
			updateStatus(rm, value);
			break;
		}
		break;
    }
}

static void reset(RomMapperS1990* rm)
{
    rm->registerSelect = 0;
    updateStatus(rm, 96);
}

static void getDebugInfo(RomMapperS1990* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevS1990(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0xe4, DBG_IO_READWRITE, read(rm, 0xe4));
    dbgIoPortsAddPort(ioPorts, 1, 0xe5, DBG_IO_READWRITE, read(rm, 0xe5));
}

int romMapperS1990Create() 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperS1990* rm;

    rm = malloc(sizeof(RomMapperS1990));

    rm->deviceHandle = deviceManagerRegister(ROM_S1990, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevS1990(), &dbgCallbacks, rm);

    ioPortRegister(0xe4, read, write, rm);
    ioPortRegister(0xe5, read, write, rm);

    reset(rm);

    return 1;
}

