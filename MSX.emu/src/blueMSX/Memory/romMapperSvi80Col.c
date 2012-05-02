/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSvi80Col.c,v $
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
#include "romMapperSvi80Col.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "CRTC6845.h"
#include "Language.h"
#include <stdlib.h>

typedef struct {
    int deviceHandle;
    int debugHandle;
    UInt8 memBankCtrl;
    CRTC6845* crtc6845;
} RomMapperSvi80Col;

static RomMapperSvi80Col* svi80colInstance = NULL;

static void saveState(RomMapperSvi80Col* svi80col)
{
    SaveState* state = saveStateOpenForWrite("Svi80Col");

    saveStateSet(state, "memBankCtrl", svi80col->memBankCtrl);
    
    saveStateClose(state);
}

static void loadState(RomMapperSvi80Col* svi80col)
{
    SaveState* state = saveStateOpenForRead("Svi80Col");

    svi80col->memBankCtrl = (UInt8)saveStateGet(state, "memBankCtrl", 0);

    saveStateClose(state);
}

static void destroy(RomMapperSvi80Col* svi80col)
{

    ioPortUnregister(0x50);
    ioPortUnregister(0x51);
    ioPortUnregister(0x58);

    deviceManagerUnregister(svi80col->deviceHandle);
    debugDeviceUnregister(svi80col->debugHandle);

    free(svi80col);
}

static UInt8 peekIo(RomMapperSvi80Col* svi80col, UInt16 ioPort) 
{
    switch (ioPort) {
        case 0x50:
            return svi80col->crtc6845->registers.address;
        case 0x51:
            return 0xff;
        case 0x58:
            return svi80col->memBankCtrl;
    }
    return 0xff;
}
static UInt8 readIo(RomMapperSvi80Col* svi80col, UInt16 ioPort) 
{
    switch (ioPort) {
        case 0x51:
            return crtcRead(svi80col->crtc6845);
        case 0x58:
//            svi80col->memBankCtrl = 1;
            break;
    }
    return 0xff;
}

static void writeIo(RomMapperSvi80Col* svi80col, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort) {
        case 0x50:
            crtcWriteLatch(svi80col->crtc6845, value);
            break;
        case 0x51:
            crtcWrite(svi80col->crtc6845, value);
            break;
        case 0x58:
            svi80col->memBankCtrl = value & 1;
            break;
    }
}  

int svi80colMemBankCtrlStatus(void)
{
   return svi80colInstance->memBankCtrl;
}

void svi80colMemWrite(UInt16 address, UInt8 value)
{
    crtcMemWrite(svi80colInstance->crtc6845, address, value);
}

UInt8 svi80colMemRead(UInt16 address)
{
    return crtcMemRead(svi80colInstance->crtc6845, address);
}

static void reset(RomMapperSvi80Col* svi80col)
{
    svi80col->memBankCtrl = 0;
}

static void getDebugInfo(RomMapperSvi80Col* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevSvi80Col(), 3);
    dbgIoPortsAddPort(ioPorts, 0, 0x50, DBG_IO_READWRITE, peekIo(rm, 0x50));
    dbgIoPortsAddPort(ioPorts, 1, 0x51, DBG_IO_READWRITE, peekIo(rm, 0x51));
    dbgIoPortsAddPort(ioPorts, 2, 0x58, DBG_IO_READWRITE, peekIo(rm, 0x58));
}

int romMapperSvi80ColCreate(int frameRate, UInt8* romData, int size)
{
    DeviceCallbacks callbacks = {destroy, reset, saveState, loadState};
    DebugCallbacks dbgCallbacks = {getDebugInfo, NULL, NULL, NULL};
    RomMapperSvi80Col* svi80col;

    if (size != 0x1000)
    	return 0;

    svi80col = malloc(sizeof(RomMapperSvi80Col));
    svi80colInstance = svi80col;

    svi80col->deviceHandle = deviceManagerRegister(ROM_SVI80COL, &callbacks, svi80col);

    svi80col->crtc6845 = NULL;
    svi80col->crtc6845 = crtc6845Create(frameRate, romData, size, 0x800, 7, 0, 82, 4);

    svi80col->debugHandle = debugDeviceRegister(DBGTYPE_VIDEO, langDbgDevSvi80Col(), &dbgCallbacks, svi80col);

    ioPortRegister(0x50, NULL,   writeIo, svi80col);
    ioPortRegister(0x51, readIo, writeIo, svi80col);
    ioPortRegister(0x58, readIo, writeIo, svi80col);

    reset(svi80col);

    return 1;
}
