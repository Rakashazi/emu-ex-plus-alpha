/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperRsIDE.c,v $
**
** $Revision: 1.0 $
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
#include "romMapperSvi328RsIDE.h"
#include "HarddiskIDE.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "I8255.h"
#include "Disk.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
PPI    NAME   IDE PIN     PPI    NAME   IDE PIN
---    ----   -------     ---    ----   -------
PA0    HD0    17 D0       PB0    HD0    17 D0  
PA1    HD1    15 D1       PB1    HD1    15 D1  
PA2    HD2    13 D2       PB2    HD2    13 D2  
PA3    HD3    11 D3       PB3    HD3    11 D3  
PA4    HD4     9 D4       PB4    HD4     9 D4  
PA5    HD5     7 D5       PB5    HD5     7 D5  
PA6    HD6     5 D6       PB6    HD6     5 D6  
PA7    HD7     3 D7       PB7    HD7     3 D7
                                               
PB0    HD8     4 D8       PC0    HD8     4 D8  
PB1    HD9     6 D9       PC1    HD9     6 D9  
PB2    HD10    8 D10      PC2    HD10    8 D10 
PB3    HD11   10 D11      PC3    HD11   10 D11 
PB4    HD12   12 D12      PC4    HD12   12 D12 
PB5    HD13   14 D13      PC5    HD13   14 D13 
PB6    HD14   16 D14      PC6    HD14   16 D14 
PB7    HD15   18 D15      PC7    HD15   18 D15 
                                               
PC0    HA0    35 A0       PA0    HA0    35 A0  
PC1    HA1    33 A1       PA1    HA1    33 A1  
PC2    HA2    36 A2       PA2    HA2    36 A2  
PC3    N/A                PA3    HCS0   37 CS0 
PC4    N/A                PA4    HCS1   38 CS1 
PC5    HCS    37 /CS0     PA5    HWR    23 IOWR
PC6    HWR    23 /IOWR    PA6    HRD    25 IORD
PC7    HRD    25 /IORD    PA7    HRST   1   RST
*/

typedef struct {
    int deviceHandle;
    int debugHandle;
    HarddiskIde* hdide;
    I8255* i8255;
    UInt8 ideAddress;
    UInt8 ideIoRead;
    UInt8 ideIoWrite;
    UInt16 ideData;
} RomMapperRsIde;

static void saveState(RomMapperRsIde* rm)
{
    SaveState* state = saveStateOpenForWrite("RomMapperRsIde");

    saveStateSet(state, "ideAddress", rm->ideAddress);
    saveStateSet(state, "ideIoRead", rm->ideIoRead);
    saveStateSet(state, "ideIoWrite", rm->ideIoWrite);
    saveStateSet(state, "ideData", rm->ideData);

    saveStateClose(state);

    harddiskIdeSaveState(rm->hdide);
    i8255SaveState(rm->i8255);
}

static void loadState(RomMapperRsIde* rm)
{
    SaveState* state = saveStateOpenForRead("RomMapperRsIde");

    rm->ideAddress = (UInt8)saveStateGet(state, "ideAddress", 0);
    rm->ideIoRead = (UInt8)saveStateGet(state, "ideIoRead", 0);
    rm->ideIoWrite = (UInt8)saveStateGet(state, "ideIoWrite", 0);
    rm->ideData = (UInt8)saveStateGet(state, "ideData", 0);

    saveStateClose(state);

    harddiskIdeLoadState(rm->hdide);
    i8255LoadState(rm->i8255);
}

static void destroy(RomMapperRsIde* rm)
{
    ioPortUnregister(0x14);
    ioPortUnregister(0x15);
    ioPortUnregister(0x16);
    ioPortUnregister(0x17);

    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    harddiskIdeDestroy(rm->hdide);
    i8255Destroy(rm->i8255);

    free(rm);
}

static UInt8 peekIo(RomMapperRsIde* rm, UInt16 ioPort) 
{
    return 0xff;
}

static UInt8 readB(RomMapperRsIde* rm)
{
    return (UInt8)rm->ideData;
}

static UInt8 readCLo(RomMapperRsIde* rm)
{
    return (UInt8)(rm->ideData >> 8);
}

static UInt8 readCHi(RomMapperRsIde* rm)
{
    return (UInt8)(rm->ideData >> 8);
}

static void writeA(RomMapperRsIde* rm, UInt8 value)
{
    rm->ideAddress = value & 0x07;
    rm->ideIoRead = value & 0x40 ? 0:1;
    rm->ideIoWrite = value & 0x20 ? 0:1;

    if (rm->ideIoRead)
    {
        switch (rm->ideAddress)
        {
        case 0:
            rm->ideData = harddiskIdeRead(rm->hdide);
            break;
        default:
            rm->ideData = harddiskIdeReadRegister(rm->hdide, rm->ideAddress);
            break;
        }
    }

    if (rm->ideIoWrite)
    {
        switch (rm->ideAddress)
        {
        case 0:
            harddiskIdeWrite(rm->hdide, rm->ideData);
            break;
        default:
            harddiskIdeWriteRegister(rm->hdide, rm->ideAddress, (UInt8)rm->ideData);
            break;
        }
    }
}

static void writeB(RomMapperRsIde* rm, UInt8 value)
{
    rm->ideData &= 0xff00;
    rm->ideData |= value;
}

static void writeCLo(RomMapperRsIde* rm, UInt8 value)
{
    rm->ideData &= 0x00ff;
    rm->ideData |= value<<8;
}

static void writeCHi(RomMapperRsIde* rm, UInt8 value)
{
    rm->ideData &= 0x00ff;
    rm->ideData |= value<<8;
}

static void reset(RomMapperRsIde* rm)
{
    harddiskIdeReset(rm->hdide);

    i8255Reset(rm->i8255);
}

static void getDebugInfo(RomMapperRsIde* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevIdeSviRs(), 4);
    for (i = 0; i < 4; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0x14 + i, DBG_IO_READWRITE, i8255Peek(rm->i8255, 0x14 + i));
    }
}

int romMapperSvi328RsIdeCreate(int hdId)
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperRsIde* rm;

    rm = malloc(sizeof(RomMapperRsIde));
    
    rm->deviceHandle = deviceManagerRegister(ROM_SVI328RSIDE, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_PORT, langDbgDevIdeSviRs(), &dbgCallbacks, rm);

    rm->i8255 = i8255Create( NULL, NULL,    writeA,
                             NULL, readB,   writeB,
                             NULL, readCLo, writeCLo,
                             NULL, readCHi, writeCHi,
                             rm);

    ioPortRegister(0x14, i8255Read, i8255Write, rm->i8255); // PPI Port A
    ioPortRegister(0x15, i8255Read, i8255Write, rm->i8255); // PPI Port B
    ioPortRegister(0x16, i8255Read, i8255Write, rm->i8255); // PPI Port C
    ioPortRegister(0x17, i8255Read, i8255Write, rm->i8255); // PPI Mode

    rm->hdide = harddiskIdeCreate(diskGetHdDriveId(hdId, 0));

    reset(rm);

    return 1;
}
