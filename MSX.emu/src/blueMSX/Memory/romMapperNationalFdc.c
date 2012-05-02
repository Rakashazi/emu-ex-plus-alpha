/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNationalFdc.c,v $
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
#include "romMapperNationalFdc.h"
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
    UInt8 reg;
} RomMapperNationalFdc;

static void saveState(RomMapperNationalFdc* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperNationalFdc");

    saveStateSet(state, "reg", rm->reg);
    
    saveStateClose(state);
    
    wd2793SaveState(rm->fdc);
}

static void loadState(RomMapperNationalFdc* rm)
{
    SaveState* state = saveStateOpenForRead("mapperNationalFdc");

    rm->reg = (UInt8)saveStateGet(state, "reg", 0);

    saveStateClose(state);
    
    wd2793LoadState(rm->fdc);
}

static void destroy(RomMapperNationalFdc* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static UInt8 read(RomMapperNationalFdc* rm, UInt16 address) 
{
	switch (address & 0x3fc7) {
	case 0x3f80:
		return wd2793GetStatusReg(rm->fdc);
	case 0x3f81:
		return wd2793GetTrackReg(rm->fdc);
	case 0x3f82:
		return wd2793GetSectorReg(rm->fdc);
	case 0x3f83:
		return wd2793GetDataReg(rm->fdc);
	case 0x3f84:
	case 0x3f85:
	case 0x3f86:
	case 0x3f87:
        return (wd2793GetIrq(rm->fdc) ? 0x80 : 0) | (wd2793GetDataRequest(rm->fdc) ? 0 : 0x40) | 0x3f;
    }

    return address < 0x4000 ? rm->romData[address] : 0xff;
}

static UInt8 peek(RomMapperNationalFdc* rm, UInt16 address) 
{
	switch (address & 0x3fc7) {
	case 0x3f80:
		return 0xff; // TODO Get from fdc
	case 0x3f81:
		return 0xff; // TODO Get from fdc
	case 0x3f82:
		return 0xff; // TODO Get from fdc
	case 0x3f83:
		return 0xff; // TODO Get from fdc
	case 0x3f84:
	case 0x3f85:
	case 0x3f86:
	case 0x3f87:
		return 0xff; // TODO Get from fdc
    }

    return address < 0x4000 ? rm->romData[address] : 0xff;
}

static void write(RomMapperNationalFdc* rm, UInt16 address, UInt8 value) 
{
	switch (address & 0x3fc7) {
	case 0x3f80:
		wd2793SetCommandReg(rm->fdc, value);
		break;
	case 0x3f81:
	    wd2793SetTrackReg(rm->fdc, value);
		break;
	case 0x3f82:
		wd2793SetSectorReg(rm->fdc, value);
		break;
	case 0x3f83:
		wd2793SetDataReg(rm->fdc, value);
		break;
	case 0x3f84:
	case 0x3f85:
	case 0x3f86:
	case 0x3f87:
        switch (value & 3) {
		case 1:
			wd2793SetDrive(rm->fdc, 0);
            wd2793SetMotor(rm->fdc, value & 0x08);
			break;
		case 2:
			wd2793SetDrive(rm->fdc, 1);
            wd2793SetMotor(rm->fdc, value & 0x08);
			break;
		default:
			wd2793SetDrive(rm->fdc, -1);
            wd2793SetMotor(rm->fdc, 0);
		}
        wd2793SetSide(rm->fdc, value & 0x04 ? 1 : 0);
		break;
	}
}	

static void reset(RomMapperNationalFdc* rm)
{
    wd2793Reset(rm->fdc);
}

int romMapperNationalFdcCreate(const char* filename, UInt8* romData,
                               int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperNationalFdc* rm;
    int pages = 4;
    int i;

    if ((startPage + pages) > 8) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperNationalFdc));

    rm->deviceHandle = deviceManagerRegister(ROM_NATIONALFDC, &callbacks, rm);
    slotRegister(slot, sslot, startPage, pages, read, peek, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

    rm->fdc = wd2793Create(FDC_TYPE_WD2793);

    reset(rm);

    return 1;
}

