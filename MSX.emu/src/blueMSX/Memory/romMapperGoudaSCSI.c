/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperGoudaSCSI.c,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:44 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
#include "romMapperGoudaSCSI.h"
#include "wd33c93.h"
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

#define PORT_BASE   0x34

typedef struct {
    int deviceHandle;
    int debugHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    WD33C93* wd33c93;
} RomMapperGoudaSCSI;

static void saveState(RomMapperGoudaSCSI* rm)
{
    wd33c93SaveState(rm->wd33c93);
}

static void loadState(RomMapperGoudaSCSI* rm)
{
    wd33c93LoadState(rm->wd33c93);
}

// experimental for HSH ROM
static UInt8 dummy(RomMapperGoudaSCSI* rm, UInt16 ioPort)
{
    return 0xb0;
    //bit 4: 1 = Halt on SCSI parity error
}

static void sbicReset(RomMapperGoudaSCSI* rm, UInt16 ioPort, UInt8 value)
{
    wd33c93Reset(rm->wd33c93, 1);
}

static void getDebugInfo(RomMapperGoudaSCSI* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevScsiGouda(), 3);
    dbgIoPortsAddPort(ioPorts, 0, PORT_BASE + 0, DBG_IO_READWRITE, wd33c93Peek(rm->wd33c93, 0));
    dbgIoPortsAddPort(ioPorts, 1, PORT_BASE + 1, DBG_IO_READWRITE, wd33c93Peek(rm->wd33c93, 1));
    dbgIoPortsAddPort(ioPorts, 2, PORT_BASE + 2, DBG_IO_READWRITE, 0xb0);
}

static void reset(RomMapperGoudaSCSI* rm)
{
    wd33c93Reset(rm->wd33c93, 1);
}

static void destroy(RomMapperGoudaSCSI* rm)
{
    ioPortUnregister(PORT_BASE + 0);
    ioPortUnregister(PORT_BASE + 1);
    ioPortUnregister(PORT_BASE + 2);

    debugDeviceUnregister(rm->debugHandle);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    wd33c93Destroy(rm->wd33c93);
    free(rm->romData);
    free(rm);
}

int romMapperGoudaSCSICreate(int hdId, const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage)
{
    DeviceCallbacks callbacks = {
        (void*)destroy, (void*)reset, (void*)saveState, (void*)loadState };
    DebugCallbacks dbgCallbacks = { (void*)getDebugInfo, NULL, NULL, NULL };
    RomMapperGoudaSCSI* rm;
    int i;
    UInt8* pBuf;
    UInt8 id15964[16] = {
        0x4b, 0x4d, 0x63, 0x73, 0x02, 0x01, 0x59, 0xb0,
        0x34, 0x64, 0x00, 0x37, 0x00, 0x00, 0x00, 0x00 };
    UInt8 bugcode[5] = { 0xc1, 0x16, 0x02, 0xc1, 0xc9 };

    if (romData == NULL) {
        size = 0x4000;
    } else {
        if (size != 0x4000) {
            return 0;
        }
    }

    rm = malloc(sizeof(RomMapperGoudaSCSI));

    rm->deviceHandle = deviceManagerRegister(ROM_GOUDASCSI, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 2, NULL, NULL, NULL, (SlotEject)destroy, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_PORT, langDbgDevScsiGouda(), &dbgCallbacks, rm);

    rm->romData = malloc(0x4000);
    if (romData != NULL) {
        memcpy(rm->romData, romData, 0x4000);
        pBuf = rm->romData;
        if (memcmp(pBuf + 0x3ff0, id15964, 16) == 0) {
            // Bug patch for NOVAXIS SCSI bios 1.59.64
            // fixed stack pointer
            if (memcmp(pBuf + 0x91c, bugcode, 5) == 0) {
                pBuf[0x091f] = 0;
            }
        }

        // Bug patch for NOVAXIS (a setting menu comes to function)
        i = 0x3ffd;
        do {
            if (*pBuf == 0xcd && *(pBuf+1) == 0x65 && *(pBuf+2) == 0xf3) {
                *pBuf++ = 0xdb; // in a,(0a8h)
                *pBuf++ = 0xa8;
                *pBuf   = 0x00;
                i -= 2;
            }
            i--;
            pBuf++;
        } while (i > 0);
    }
    else {
        memset(rm->romData, 0xff, 0x4000);
    }

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->wd33c93 = wd33c93Create(hdId);

    slotMapPage(slot, sslot, startPage    , rm->romData         , 1, 0);
    slotMapPage(slot, sslot, startPage + 1, rm->romData + 0x2000, 1, 0);

    ioPortRegister(PORT_BASE + 0, (IoPortRead)wd33c93ReadAuxStatus, (IoPortWrite)wd33c93WriteAdr, rm->wd33c93);
    ioPortRegister(PORT_BASE + 1, (IoPortRead)wd33c93ReadCtrl, (IoPortWrite)wd33c93WriteCtrl, rm->wd33c93);
    ioPortRegister(PORT_BASE + 2, (IoPortRead)dummy, (IoPortWrite)sbicReset, rm);

    return 1;
}

