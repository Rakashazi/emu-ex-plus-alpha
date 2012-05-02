/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMicrosol.c,v $
**
** $Revision: 1.12 $
**
** $Date: 2008-03-30 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Ricardo Bittencourt
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
#include "romMapperMicrosol.h"
#include "WD2793.h"
#include "Disk.h"
#include "MediaDb.h"
#include "SlotManager.h"
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
    WD2793* fdc;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
} Microsol;

static void destroy(Microsol* rm)
{
    ioPortUnregister(0xd0);
    ioPortUnregister(0xd1);
    ioPortUnregister(0xd2);
    ioPortUnregister(0xd3);
    ioPortUnregister(0xd4);

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    wd2793Destroy(rm->fdc);

    free(rm->romData);
    free(rm);
}

static void saveState(Microsol* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMicrosol");

    saveStateClose(state);

    wd2793SaveState(rm->fdc);
}

static void loadState(Microsol* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMicrosol");

    saveStateClose(state);

    wd2793LoadState(rm->fdc);
}


static UInt8 peekIo(Microsol* rm, UInt16 ioPort)
{	
    switch (ioPort) {
	case 0xd0:
		return wd2793PeekStatusReg(rm->fdc);
	case 0xd1:
		return wd2793PeekTrackReg(rm->fdc);
	case 0xd2:
		return wd2793PeekSectorReg(rm->fdc);
	case 0xd3:
		return wd2793PeekDataReg(rm->fdc);
	case 0xd4:
        return 0x3f | 
               (wd2793PeekIrq(rm->fdc)         ? 0x80 : 0) | 
               (wd2793PeekDataRequest(rm->fdc) ? 0    : 0x40);
    }
    return 0xff;
}

static UInt8 readIo(Microsol* rm, UInt16 ioPort)
{	
    switch (ioPort) {
	case 0xd0:
		return wd2793GetStatusReg(rm->fdc);
	case 0xd1:
		return wd2793GetTrackReg(rm->fdc);
	case 0xd2:
		return wd2793GetSectorReg(rm->fdc);
	case 0xd3:
		return wd2793GetDataReg(rm->fdc);
	case 0xd4:
        return 0x3f | 
               (wd2793GetIrq(rm->fdc)         ? 0x80 : 0) | 
               (wd2793GetDataRequest(rm->fdc) ? 0 : 0x40);
    }
    return 0xff;
}

static void writeIo(Microsol* rm, UInt16 ioPort, UInt8 value)
{
    switch (ioPort) {
	case 0xd0:
		wd2793SetCommandReg(rm->fdc, value);
        break;
	case 0xd1:
	    wd2793SetTrackReg(rm->fdc, value);
        break;
	case 0xd2:
		wd2793SetSectorReg(rm->fdc, value);
        break;
	case 0xd3:
		wd2793SetDataReg(rm->fdc, value);
        break;
	case 0xd4:
		switch (value & 0x0f) {
		case 1:
			wd2793SetDrive(rm->fdc, 0);
            wd2793SetMotor(rm->fdc, value & 0x20);
			break;
		case 2:
			wd2793SetDrive(rm->fdc, 1);
            wd2793SetMotor(rm->fdc, value & 0x20);
			break;
		case 4:
			wd2793SetDrive(rm->fdc, 2);
            wd2793SetMotor(rm->fdc, 0);
			break;
		case 8:
			wd2793SetDrive(rm->fdc, 3);
            wd2793SetMotor(rm->fdc, 0);
			break;
		default:
			wd2793SetDrive(rm->fdc, -1);
            wd2793SetMotor(rm->fdc, 0);
		}

        wd2793SetSide(rm->fdc, value & 0x10 ? 1 : 0);

        break;
    }
}

static void reset(Microsol* rm)
{
    wd2793Reset(rm->fdc);
}

static void getDebugInfo(Microsol* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevFdcMicrosol(), 2);
    for (i = 0; i < 5; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0xd0, DBG_IO_READWRITE, peekIo(rm, 0xd0 + i));
    }
}

int romMapperMicrosolCreate(const char* filename, UInt8* romData,
                            int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Microsol* rm;
    int pages = size / 0x2000;
    int i;

    rm = malloc(sizeof(Microsol));

    rm->deviceHandle = deviceManagerRegister(ROM_MICROSOL, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevFdcMicrosol(), &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);

    size = (size + 0x3fff) & ~0x3fff;

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
    }
    
    ioPortRegister(0xd0, readIo, writeIo, rm);
    ioPortRegister(0xd1, readIo, writeIo, rm);
    ioPortRegister(0xd2, readIo, writeIo, rm);
    ioPortRegister(0xd3, readIo, writeIo, rm);
    ioPortRegister(0xd4, readIo, writeIo, rm);

    rm->fdc = wd2793Create(FDC_TYPE_WD2793);

    reset(rm);

    return 1;
}

