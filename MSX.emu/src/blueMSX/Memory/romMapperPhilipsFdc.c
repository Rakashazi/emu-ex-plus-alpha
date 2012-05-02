/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperPhilipsFdc.c,v $
**
** $Revision: 1.10 $
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
#include "romMapperPhilipsFdc.h"
#include "WD2793.h"
#include "MediaDb.h"
#include "Disk.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    WD2793* fdc;
    int slot;
    int sslot;
    int startPage;
    UInt8 sideReg;
    UInt8 driveReg;
} RomMapperPhilipsFdc;

static void saveState(RomMapperPhilipsFdc* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperPhilipsFdc");

    saveStateSet(state, "sideReg",  rm->sideReg);
    saveStateSet(state, "driveReg", rm->driveReg);
    
    saveStateClose(state);

    wd2793SaveState(rm->fdc);
}

static void loadState(RomMapperPhilipsFdc* rm)
{
    SaveState* state = saveStateOpenForRead("mapperPhilipsFdc");

    rm->sideReg  = (UInt8)saveStateGet(state, "sideReg",  0);
    rm->driveReg = (UInt8)saveStateGet(state, "driveReg", 0);

    saveStateClose(state);

    wd2793LoadState(rm->fdc);
}

static void destroy(RomMapperPhilipsFdc* rm)
{
    wd2793Destroy(rm->fdc);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static UInt8 read(RomMapperPhilipsFdc* rm, UInt16 address) 
{
	switch (address & 0x3fff) {
	case 0x3ff8:
		return wd2793GetStatusReg(rm->fdc);
	case 0x3ff9:
		return wd2793GetTrackReg(rm->fdc);
	case 0x3ffa:
		return wd2793GetSectorReg(rm->fdc);
	case 0x3ffb:
		return wd2793GetDataReg(rm->fdc);
	case 0x3ffc:
        return rm->sideReg;
	case 0x3ffd:
        return rm->driveReg;
	case 0x3ffe:
        return 0xff;
	case 0x3fff:
        return (wd2793GetIrq(rm->fdc) ? 0 : 0x40) | (wd2793GetDataRequest(rm->fdc) ? 0 : 0x80);
    }

    return address < 0x4000 ? rm->romData[address] : 0xff;
}

static UInt8 peek(RomMapperPhilipsFdc* rm, UInt16 address) 
{
	switch (address & 0x3fff) {
	case 0x3ff8:
		return 0xff; // Get from fdc
	case 0x3ff9:
		return 0xff; // Get from fdc
	case 0x3ffa:
		return 0xff; // Get from fdc
	case 0x3ffb:
		return 0xff; // Get from fdc
	case 0x3ffc:
        return rm->sideReg;
	case 0x3ffd:
        return rm->driveReg;
	case 0x3ffe:
        return 0xff;
	case 0x3fff:
		return 0xff; // Get from fdc
    }

    return address < 0x4000 ? rm->romData[address] : 0xff;
}

static void write(RomMapperPhilipsFdc* rm, UInt16 address, UInt8 value) 
{
	switch (address & 0x3fff) {
	case 0x3ff8:
		wd2793SetCommandReg(rm->fdc, value);
		break;
	case 0x3ff9:
	    wd2793SetTrackReg(rm->fdc, value);
		break;
	case 0x3ffa:
		wd2793SetSectorReg(rm->fdc, value);
		break;
	case 0x3ffb:
		wd2793SetDataReg(rm->fdc, value);
		break;
	case 0x3ffc:
        rm->sideReg = value;
        wd2793SetSide(rm->fdc, value & 1);
        break;
	case 0x3ffd:
        switch (value & 3) {
        case 0:
		case 2:
			wd2793SetDrive(rm->fdc, 0);
            wd2793SetMotor(rm->fdc, value & 0x80);
			break;
		case 1:
			wd2793SetDrive(rm->fdc, 1);
            wd2793SetMotor(rm->fdc, value & 0x80);
			break;
		default:
			wd2793SetDrive(rm->fdc, -1);
            wd2793SetMotor(rm->fdc, 0);
		}
		break;
	}
}	

static void reset(RomMapperPhilipsFdc* rm)
{
    wd2793Reset(rm->fdc);
    write(rm, 0xffc, 0);
    write(rm, 0xffd, 0);
}

int romMapperPhilipsFdcCreate(const char* filename, UInt8* romData,
                              int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperPhilipsFdc* rm;
    int pages = 4;
    int i;

    if ((startPage + pages) > 8) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperPhilipsFdc));

    rm->deviceHandle = deviceManagerRegister(ROM_PHILIPSFDC, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, read, peek, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->sideReg = 0;
    rm->driveReg = 0;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

    rm->fdc = wd2793Create(FDC_TYPE_WD2793);

    reset(rm);

    return 1;
}
