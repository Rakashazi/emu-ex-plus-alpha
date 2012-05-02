/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperGIDE.c,v $
**
** $Revision: 1.11 $
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
#include "romMapperGIDE.h"
#include "HarddiskIDE.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "Board.h"
#include "Disk.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    int deviceHandle;
    int debugHandle;
    HarddiskIde* hdide;
    UInt8 intEnable;
    UInt8 drvSelect;
    UInt8 altStatus;
} RomMapperGIde;

static void saveState(RomMapperGIde* rm)
{
    SaveState* state = saveStateOpenForWrite("RomMapperGIde");

    saveStateSet(state, "intEnable",  rm->intEnable);
    saveStateSet(state, "drvSelect",  rm->drvSelect);
    saveStateSet(state, "altStatus",  rm->altStatus);
    
    saveStateClose(state);

    harddiskIdeSaveState(rm->hdide);
}

static void loadState(RomMapperGIde* rm)
{
    SaveState* state = saveStateOpenForRead("RomMapperGIde");

    rm->intEnable  = (UInt8)saveStateGet(state, "intEnable",  0);
    rm->drvSelect  = (UInt8)saveStateGet(state, "drvSelect",  0);
    rm->altStatus  = (UInt8)saveStateGet(state, "altStatus",  0);

    saveStateClose(state);

    harddiskIdeLoadState(rm->hdide);
}

static void destroy(RomMapperGIde* rm)
{
    int portBase;

    portBase = (boardGetType() == BOARD_SVI) ? 0x40:0x60;

    ioPortUnregister(portBase | 0x04);
    ioPortUnregister(portBase | 0x05);
    ioPortUnregister(portBase | 0x06);
    ioPortUnregister(portBase | 0x07);
    ioPortUnregister(portBase | 0x08);
    ioPortUnregister(portBase | 0x09);
    ioPortUnregister(portBase | 0x0a);
    ioPortUnregister(portBase | 0x0b);
    ioPortUnregister(portBase | 0x0c);
    ioPortUnregister(portBase | 0x0d);
    ioPortUnregister(portBase | 0x0e);
    ioPortUnregister(portBase | 0x0f);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    harddiskIdeDestroy(rm->hdide);

    free(rm);
}

UInt8 rtc72421Read(UInt8 rtcReg)
{
    time_t current_time;
    struct tm* tms;

    time( &current_time );
    tms = localtime( &current_time );

    switch(rtcReg & 0x0f)
    {
    case 0x00:    // 1-second digit
        return (tms->tm_sec % 10) & 0xf;

    case 0x01:    // 10-seconds digit
        return (tms->tm_sec / 10) & 0x7;

    case 0x02:    // 1-minute digit
        return (tms->tm_min % 10) & 0xf;

    case 0x03:    // 10-minute digit
        return (tms->tm_min / 10) & 0x7;

    case 0x04:    // 1-hour digit
        return (tms->tm_hour % 10) & 0xf;

    case 0x05:    // 10-hours digit
        return (tms->tm_hour / 10) & 0x7;

    case 0x06:    // 1-day digit (days in month)
        return (tms->tm_mday % 10) & 0xf;

    case 0x07:    // 10-days digit
        return (tms->tm_mday / 10) & 0x3;

    case 0x08:    // 1-month digit
        return ((tms->tm_mon + 1) % 10) & 0xf;

    case 0x09:    // 10-months digit
        return ((tms->tm_mon + 1) / 10) & 0x1;

    case 0x0a:    // 1-year digit
        return (tms->tm_year % 10) & 0xf;

    case 0x0b:    // 10-years digit
        return ((tms->tm_year % 100) / 10) & 0xf;

    case 0x0c:    // day of the week
        return tms->tm_wday & 0x7;

    case 0x0d:    // control D
        return 0;

    case 0x0e:    // control E
        return 0;

    case 0x0f:    // control F
        return 0;
    }
    return 0xff;
}

static void rtc72421Write(UInt8 rtcReg, UInt8 value)
{
}

static UInt8 peekIo(RomMapperGIde* rm, UInt16 ioPort) 
{
    return 0xff;
}

static UInt8 readIo(RomMapperGIde* rm, UInt16 ioPort) 
{
    switch (ioPort & 0x0f)
    {
    case 0x04:    // Reserved for expansion board
        return 0xff;

    case 0x05:    // RTC 72421
        return rtc72421Read(ioPort >> 8);

    case 0x06:    // GIDE alternate status
        return rm->altStatus;

    case 0x07:    // GIDE drive address register
        return rm->drvSelect;

    case 0x08:    // IDE data register
        return (UInt8)harddiskIdeRead(rm->hdide);

    case 0x09:    // IDE error register
        return harddiskIdeReadRegister(rm->hdide, 1);

    case 0x0a:    // IDE sector count register
        return harddiskIdeReadRegister(rm->hdide, 2);

    case 0x0b:    // IDE sector number register
        return harddiskIdeReadRegister(rm->hdide, 3);

    case 0x0c:    // IDE cylinder low register
        return harddiskIdeReadRegister(rm->hdide, 4);

    case 0x0d:    // IDE cylinder high register
        return harddiskIdeReadRegister(rm->hdide, 5);

    case 0x0e:    // IDE drive/head register
        return harddiskIdeReadRegister(rm->hdide, 6);

    case 0x0f:    // IDE status register
        rm->altStatus = harddiskIdeReadRegister(rm->hdide, 7);
        return rm->altStatus;
    }
    return 0xff;
}

static void writeIo(RomMapperGIde* rm, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort & 0x0f)
    {
    case 0x04:    // Reserved for expansion board
        break; 

    case 0x05:    // RTC 72421
        rtc72421Write(ioPort >> 8, value);
        break; 

    case 0x06:    // GIDE digital output register
        rm->intEnable = value & 0x01?1:0;
        if (value & 0x02)
            harddiskIdeReset(rm->hdide);
        break; 

    case 0x07:    // GIDE drive address register
        break; 

    case 0x08:    // IDE data register
        harddiskIdeWrite(rm->hdide, value);
        break; 

    case 0x09:    // IDE write precomp register
        harddiskIdeWriteRegister(rm->hdide, 1, value);
        break; 

    case 0x0a:    // IDE sector count register
        harddiskIdeWriteRegister(rm->hdide, 2, value);
        break; 

    case 0x0b:    // IDE sector number register
        harddiskIdeWriteRegister(rm->hdide, 3, value);
        break; 

    case 0x0c:    // IDE cylinder low register
        harddiskIdeWriteRegister(rm->hdide, 4, value);
        break; 

    case 0x0d:    // IDE cylinder high register
        harddiskIdeWriteRegister(rm->hdide, 5, value);
        break; 

    case 0x0e:    // IDE drive/head register
        rm->drvSelect = value;
        harddiskIdeWriteRegister(rm->hdide, 6, value);
        break; 

    case 0x0f:    // IDE command register
        harddiskIdeWriteRegister(rm->hdide, 7, value);
        break; 
    }
}

static void reset(RomMapperGIde* rm)
{
    harddiskIdeReset(rm->hdide);
}

static void getDebugInfo(RomMapperGIde* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevIdeGide(), 12);
    for (i = 0; i < 12; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0x44 + i, DBG_IO_READWRITE, peekIo(rm, 0x44 + i));
    }
}

int romMapperGIdeCreate(int hdId) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperGIde* rm;
    int portBase;

    rm = malloc(sizeof(RomMapperGIde));

    rm->deviceHandle = deviceManagerRegister(ROM_GIDE, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_PORT, langDbgDevIdeGide(), &dbgCallbacks, rm);

    portBase = (boardGetType() == BOARD_SVI) ? 0x40:0x60;

    ioPortRegister(portBase | 0x04, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x05, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x06, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x07, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x08, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x09, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0a, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0b, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0c, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0d, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0e, readIo, writeIo, rm);
    ioPortRegister(portBase | 0x0f, readIo, writeIo, rm);

    rm->hdide = harddiskIdeCreate(diskGetHdDriveId(hdId, 0));

    reset(rm);

    return 1;
}
