/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMsxPrn.c,v $
**
** $Revision: 1.10 $
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
#include "romMapperMsxPrn.h"
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
} RomMapperMsxPrn;

static void saveState(RomMapperMsxPrn* prn)
{
    SaveState* state = saveStateOpenForWrite("MsxPrn");

    saveStateSet(state, "prnData",  prn->prnData);
    saveStateSet(state, "prnStrobe",  prn->prnStrobe);
    
    saveStateClose(state);
}

static void loadState(RomMapperMsxPrn* prn)
{
    SaveState* state = saveStateOpenForRead("MsxPrn");

    prn->prnData  = (UInt8)saveStateGet(state, "prnData",  0);
    prn->prnStrobe  = (UInt8)saveStateGet(state, "prnStrobe",  0);

    saveStateClose(state);
}

static void destroy(RomMapperMsxPrn* prn)
{
    deviceManagerUnregister(prn->deviceHandle);
    debugDeviceUnregister(prn->debugHandle);

    ioPortUnregister(0x90);
    ioPortUnregister(0x91);

    printerIODestroy(prn->printerIO);

    free(prn);
}

static UInt8 readIo(RomMapperMsxPrn* prn, UInt16 ioPort) 
{
    if (printerIOGetStatus(prn->printerIO)) 
        return 0xfd;
	    
    return 0xff;
}

static void writeIo(RomMapperMsxPrn* prn, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort) {
        case 0x90:
            if (printerIODoStrobe(prn->printerIO)) {
                if ((prn->prnStrobe & 2) && !(value & 2)) {
                    printerIOWrite(prn->printerIO, prn->prnData);
                }
            }
            prn->prnStrobe = value;
            break;
        case 0x91:
            prn->prnData = value;

            if (!printerIODoStrobe(prn->printerIO)) {
                printerIOWrite(prn->printerIO, prn->prnData);
            }
            break;
    }
}  

static void reset(RomMapperMsxPrn* prn)
{
    prn->prnStrobe = 0;
    prn->prnData = 0;
}

static void getDebugInfo(RomMapperMsxPrn* prn, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevPrinter(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0x90, DBG_IO_READWRITE, readIo(prn, 0x90));
    dbgIoPortsAddPort(ioPorts, 1, 0x91, DBG_IO_READWRITE, readIo(prn, 0x91));
}

int romMapperMsxPrnCreate(void) 
{
    DeviceCallbacks callbacks = {destroy, reset, saveState, loadState};
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperMsxPrn* prn;

    prn = malloc(sizeof(RomMapperMsxPrn));

    prn->printerIO = printerIOCreate();

    prn->deviceHandle = deviceManagerRegister(ROM_MSXPRN, &callbacks, prn);
    prn->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevPrinter(), &dbgCallbacks, prn);

    ioPortRegister(0x90, readIo, writeIo, prn);
    ioPortRegister(0x91, readIo, writeIo, prn);

    reset(prn);

    return 1;
}
