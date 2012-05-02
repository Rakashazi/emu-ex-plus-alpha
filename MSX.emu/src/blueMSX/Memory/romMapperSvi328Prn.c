/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSvi328Prn.c,v $
**
** $Revision: 1.8 $
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
#include "romMapperSvi328Prn.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "PrinterIO.h"
#include "Language.h"
#include <stdlib.h>

typedef struct {
    int deviceHandle;
    int debugHandle;
    UInt8 prnData;
    UInt8 prnStrobe;
    PrinterIO* printerIO;
} RomMapperSvi328Prn;

static void saveState(RomMapperSvi328Prn* prn)
{
    SaveState* state = saveStateOpenForWrite("Svi328Prn");

    saveStateSet(state, "prnData", prn->prnData);
    saveStateSet(state, "prnStrobe", prn->prnStrobe);

    saveStateClose(state);
}

static void loadState(RomMapperSvi328Prn* prn)
{
    SaveState* state = saveStateOpenForRead("Svi328Prn");

    prn->prnData  = (UInt8)saveStateGet(state, "prnData",  0);
    prn->prnStrobe  = (UInt8)saveStateGet(state, "prnStrobe",  0);

    saveStateClose(state);
}

static void destroy(RomMapperSvi328Prn* prn)
{
    ioPortUnregister(0x10);
    ioPortUnregister(0x11);
    ioPortUnregister(0x12);

    deviceManagerUnregister(prn->deviceHandle);
    debugDeviceUnregister(prn->debugHandle);

    printerIODestroy(prn->printerIO);

    free(prn);
}

static UInt8 peekIo(RomMapperSvi328Prn* prn, UInt16 ioPort) 
{
    switch (ioPort) {
        case 0x10:
            return prn->prnData;
        case 0x11:
            return prn->prnStrobe;
        case 0x12:
            if (printerIOGetStatus(prn->printerIO)) 
                return 0xfe;
    }
    return 0xff;
}

static UInt8 readIo(RomMapperSvi328Prn* prn, UInt16 ioPort) 
{
    if (printerIOGetStatus(prn->printerIO)) 
        return 0xfe;

    return 0xff;
}

static void writeIo(RomMapperSvi328Prn* prn, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort) {
        case 0x10:
            prn->prnData = value;
            break;
        case 0x11:
            if ((prn->prnStrobe & 1) && !(value & 1))
                printerIOWrite(prn->printerIO, prn->prnData);

            prn->prnStrobe = value;
            break;
    }
}  

static void reset(RomMapperSvi328Prn* prn)
{
    prn->prnStrobe = 0;
    prn->prnData = 0;
}

static void getDebugInfo(RomMapperSvi328Prn* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevSviPrn(), 3);
    dbgIoPortsAddPort(ioPorts, 0, 0x10, DBG_IO_READWRITE, peekIo(rm, 0x10));
    dbgIoPortsAddPort(ioPorts, 1, 0x11, DBG_IO_READWRITE, peekIo(rm, 0x11));
    dbgIoPortsAddPort(ioPorts, 2, 0x12, DBG_IO_READWRITE, peekIo(rm, 0x12));
}

int romMapperSvi328PrnCreate(void) 
{
    DeviceCallbacks callbacks = {destroy, reset, saveState, loadState};
    DebugCallbacks dbgCallbacks = {getDebugInfo, NULL, NULL, NULL};
    RomMapperSvi328Prn* prn;

    prn = malloc(sizeof(RomMapperSvi328Prn));

    prn->printerIO = printerIOCreate();

    prn->deviceHandle = deviceManagerRegister(ROM_SVI328PRN, &callbacks, prn);
    prn->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevSviPrn(), &dbgCallbacks, prn);

    ioPortRegister(0x10, NULL, writeIo, prn);
    ioPortRegister(0x11, NULL, writeIo, prn);
    ioPortRegister(0x12, readIo, NULL, prn);

    reset(prn);

    return 1;
}
