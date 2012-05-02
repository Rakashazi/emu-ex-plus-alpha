/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Sf7000PPI.c,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-31 19:42:20 $
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
#include "Sf7000PPI.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "IoPort.h"
#include "I8255.h"
#include "NEC765.h"
#include "Board.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>


typedef struct {
    int    deviceHandle;
    int    debugHandle;
    UInt8  ramSlot;
    I8255* i8255;
    NEC765* nec765;
} Sf7000PPI;


static void destroy(Sf7000PPI* ppi)
{
    ioPortUnregister(0xe4);
    ioPortUnregister(0xe5);
    ioPortUnregister(0xe6);
    ioPortUnregister(0xe7);

    deviceManagerUnregister(ppi->deviceHandle);
    debugDeviceUnregister(ppi->debugHandle);

    nec765Destroy(ppi->nec765);
    i8255Destroy(ppi->i8255);

    free(ppi);
}

static void loadState(Sf7000PPI* ppi)
{
    SaveState* state = saveStateOpenForRead("Sf7000PPI");

    ppi->ramSlot = (UInt8)saveStateGet(state, "ramSlot", 0);
    saveStateClose(state);
    
    i8255LoadState(ppi->i8255);
    nec765LoadState(ppi->nec765);
}

static void saveState(Sf7000PPI* ppi)
{
    SaveState* state = saveStateOpenForWrite("Sf7000PPI");
    
    saveStateSet(state, "ramSlot", ppi->ramSlot);

    saveStateClose(state);

    i8255SaveState(ppi->i8255);
    nec765SaveState(ppi->nec765);
}

static void writeCLo(Sf7000PPI* ppi, UInt8 value)
{
    if (value & 0x08) {
        nec765Reset(ppi->nec765);
        // set int ????
    }
}

static UInt8 readA(Sf7000PPI* ppi)
{
    UInt8 value = 0;

    if (nec765GetInt(ppi->nec765)) {
        value |= 0x01;
    }

    if (!nec765GetIndex(ppi->nec765)) {
        value |= 0x04;
    }

    return value;
}

static void writeCHi(Sf7000PPI* ppi, UInt8 value)
{
    int slot = slotGetRamSlot(0);

    ppi->ramSlot = (~value >> 2) & 0x01;

    if (slot == 0 || slot == 1) {
        slotSetRamSlot(0, ppi->ramSlot);
    }
}

static void reset(Sf7000PPI* ppi) 
{
    i8255Reset(ppi->i8255);
    
    ppi->ramSlot = 1;
    writeCHi(ppi, ppi->ramSlot);
}

static UInt8 read(Sf7000PPI* ppi, UInt16 port)
{
    return i8255Read(ppi->i8255, port);
}

static void write(Sf7000PPI* ppi, UInt16 port, UInt8 value)
{
    i8255Write(ppi->i8255, port, value);
}

static void getDebugInfo(Sf7000PPI* ppi, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevPpi(), 5);
    dbgIoPortsAddPort(ioPorts, 0, 0xdc, DBG_IO_WRITE, i8255Read(ppi->i8255, 0xe4));
    dbgIoPortsAddPort(ioPorts, 1, 0xdd, DBG_IO_WRITE, i8255Read(ppi->i8255, 0xe5));
    dbgIoPortsAddPort(ioPorts, 2, 0xde, DBG_IO_WRITE, i8255Read(ppi->i8255, 0xe6));
    dbgIoPortsAddPort(ioPorts, 3, 0xdf, DBG_IO_WRITE, i8255Read(ppi->i8255, 0xe7));
}

static UInt8 fdcRead(Sf7000PPI* ppi, UInt16 port)
{
    UInt8 value = 0xff;

    switch (port & 1) {
    case 0: value = nec765ReadStatus(ppi->nec765); break;
    case 1: value = nec765Read(ppi->nec765); break;
    }

    return value;
}

static void fdcWrite(Sf7000PPI* ppi, UInt16 port, UInt8 value)
{
    switch (port & 1) {
    case 1: 
        nec765Write(ppi->nec765, value); break;
    }
}

void sf7000PPICreate()
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Sf7000PPI* ppi = malloc(sizeof(Sf7000PPI));

    ppi->deviceHandle = deviceManagerRegister(RAM_MAPPER, &callbacks, ppi);
    ppi->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevPpi(), &dbgCallbacks, ppi);
    
    ppi->nec765 = nec765Create();

    ppi->i8255 = i8255Create(NULL,  readA,  NULL,
                             NULL,  NULL,  NULL,
                             NULL,  NULL,  writeCLo,
                             NULL,  NULL,  writeCHi,
                             ppi);

	ioPortRegister(0xe0, fdcRead, NULL, ppi);
	ioPortRegister(0xe1, fdcRead, fdcWrite, ppi);

    ioPortRegister(0xe4, read, write, ppi); // PPI Port A
    ioPortRegister(0xe5, read, write, ppi); // PPI Port B
    ioPortRegister(0xe6, read, write, ppi); // PPI Port C
    ioPortRegister(0xe7, read, write, ppi); // PPI Mode

    reset(ppi);
}


