/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSvi328Rs232.c,v $
**
** $Revision: 1.9 $
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
#include "romMapperSvi328Rs232.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "I8250.h"
#include "ArchUart.h"
#include "Language.h"
#include <stdlib.h>

typedef struct {
    int connector;
    int deviceHandle;
    int debugHandle;
    int serialLink;
    UInt8 baseAddress;
    I8250* i8250;
} RomMapperSvi328Rs232;

static RomMapperSvi328Rs232* rs232;


static void saveState(RomMapperSvi328Rs232* rs232)
{
    SaveState* state = saveStateOpenForWrite("Svi328Rs232");

    saveStateSet(state, "connector", rs232->connector);
    saveStateSet(state, "deviceHandle", rs232->deviceHandle);
    
    saveStateClose(state);
    
    i8250SaveState(rs232->i8250);
}

static void loadState(RomMapperSvi328Rs232* rs232)
{
    SaveState* state = saveStateOpenForRead("Svi328Rs232");

    rs232->connector = saveStateGet(state, "connector",  0);
    rs232->deviceHandle = (UInt8)saveStateGet(state, "deviceHandle",  0);

    saveStateClose(state);

    i8250LoadState(rs232->i8250);
}

static void destroy(RomMapperSvi328Rs232* rs232)
{
    switch (rs232->connector) {
    case SVI328_MODEM:
        ioPortUnregister(0x20);
        ioPortUnregister(0x21);
        ioPortUnregister(0x22);
        ioPortUnregister(0x23);
        ioPortUnregister(0x24);
        ioPortUnregister(0x25);
        ioPortUnregister(0x26);
        ioPortUnregister(0x27);
        break;

    case SVI328_RS232:
        ioPortUnregister(0x28);
        ioPortUnregister(0x29);
        ioPortUnregister(0x2A);
        ioPortUnregister(0x2B);
        ioPortUnregister(0x2C);
        ioPortUnregister(0x2D);
        ioPortUnregister(0x2E);
        ioPortUnregister(0x2F);
        break;
    }
    
    i8250Destroy(rs232->i8250);

    archUartDestroy();

    deviceManagerUnregister(rs232->deviceHandle);
    debugDeviceUnregister(rs232->debugHandle);

    free(rs232);
}

static UInt8 peekIo(RomMapperSvi328Rs232* rs232, UInt16 ioPort) 
{
    return 0xff;
}
	
static UInt8 readIo(RomMapperSvi328Rs232* rs232, UInt16 ioPort) 
{
    return i8250Read(rs232->i8250, ioPort - rs232->baseAddress);
}

static void writeIo(RomMapperSvi328Rs232* rs232, UInt16 ioPort, UInt8 value) 
{
    i8250Write(rs232->i8250, ioPort - rs232->baseAddress, value);
}  

static int romMapperSvi328Rs232TransmitCallback(UInt8 value)
{
    if (rs232->serialLink) {
        archUartTransmit(value);
        return 1;
    }
    return 0;
}

static void romMapperSvi328Rs232ReceiveCallback(UInt8 value)
{
    //i8250Receive(rs232->i8250, value);
    i8250RxData(rs232->i8250, value);
}

static void getDebugInfo(RomMapperSvi328Rs232* rs232, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevRs232(), 8);

    for (i = 0; i < 8; i++) {
        dbgIoPortsAddPort(ioPorts, i, rs232->baseAddress + i, DBG_IO_READWRITE, peekIo(rs232, rs232->baseAddress + i));
    }
}

int romMapperSvi328Rs232Create(Svi328UartConnector connector)
{
    DeviceCallbacks callbacks = {destroy, NULL, saveState, loadState};
    DebugCallbacks dbgCallbacks = {getDebugInfo, NULL, NULL, NULL};

    rs232 = malloc(sizeof(RomMapperSvi328Rs232));

    rs232->connector  = connector;
    rs232->deviceHandle = deviceManagerRegister(ROM_SVI328RS232, &callbacks, rs232);
    rs232->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevRs232(), &dbgCallbacks, rs232);

    rs232->i8250 = NULL;
    rs232->i8250 = i8250Create(3072000, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, rs232);

    rs232->serialLink = archUartCreate(romMapperSvi328Rs232ReceiveCallback);

    switch (rs232->connector) {
    case SVI328_MODEM:
        rs232->baseAddress = 0x20;
        ioPortRegister(0x20, readIo, writeIo, rs232);
        ioPortRegister(0x21, readIo, writeIo, rs232);
        ioPortRegister(0x22, readIo, NULL,    rs232);
        ioPortRegister(0x23, readIo, writeIo, rs232);
        ioPortRegister(0x24, readIo, writeIo, rs232);
        ioPortRegister(0x25, readIo, NULL,    rs232);
        ioPortRegister(0x26, readIo, NULL,    rs232);
        ioPortRegister(0x27, readIo, writeIo, rs232);
        break;

    case SVI328_RS232:
        rs232->baseAddress = 0x28;
        ioPortRegister(0x28, readIo, writeIo, rs232);
        ioPortRegister(0x29, readIo, writeIo, rs232);
        ioPortRegister(0x2A, readIo, NULL,    rs232);
        ioPortRegister(0x2B, readIo, writeIo, rs232);
        ioPortRegister(0x2C, readIo, writeIo, rs232);
        ioPortRegister(0x2D, readIo, NULL,    rs232);
        ioPortRegister(0x2E, readIo, NULL,    rs232);
        ioPortRegister(0x2F, readIo, writeIo, rs232);
        break;

    default:
        return 0;
    }

    return 1;
}
