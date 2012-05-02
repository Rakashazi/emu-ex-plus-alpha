/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMsxMusic.c,v $
**
** $Revision: 1.11 $
**
** $Date: 2009-07-18 14:35:59 $
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
#include "romMapperMsxMusic.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SlotManager.h"
#include "IoPort.h"
#include "YM2413.h"
#include "Board.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int      deviceHandle;
    int      debugHandle;
    YM_2413* ym2413;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
} MsxMusic;

static void destroy(MsxMusic* rm)
{
    ioPortUnregister(0x7c);
    ioPortUnregister(0x7d);

    if (rm->ym2413 != NULL) {
        ym2413Destroy(rm->ym2413);
    }

    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);

    free(rm->romData);
    free(rm);
}

static void reset(MsxMusic* rm) 
{
    if (rm->ym2413 != NULL) {
        ym2413Reset(rm->ym2413);
    }
}

static void loadState(MsxMusic* rm)
{
    if (rm->ym2413 != NULL) {
        ym2413LoadState(rm->ym2413);
    }
}

static void saveState(MsxMusic* rm)
{
    if (rm->ym2413 != NULL) {
        ym2413SaveState(rm->ym2413);
    }
}

static void write(MsxMusic* rm, UInt16 ioPort, UInt8 data)
{
    switch (ioPort & 1) {
    case 0:
        ym2413WriteAddress(rm->ym2413, data);
        break;
    case 1:
        ym2413WriteData(rm->ym2413, data);
        break;
    }
}

static void getDebugInfo(MsxMusic* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    if (rm->ym2413 == NULL) {
        return;
    }

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMsxMusic(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0x7c, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 1, 0x7d, DBG_IO_WRITE, 0);
    
    ym2413GetDebugInfo(rm->ym2413, dbgDevice);
}

int romMapperMsxMusicCreate(const char* filename, UInt8* romData,
                            int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    MsxMusic* rm = malloc(sizeof(MsxMusic));
    int pages = size / 0x2000 + ((size & 0x1fff) ? 1 : 0);
    int i;

    if (pages == 0 || (startPage + pages) > 8) {
    	free(rm);
        return 0;
    }

    rm->deviceHandle = deviceManagerRegister(ROM_MSXMUSIC, &callbacks, rm);
    
    rm->ym2413 = NULL;
    if (boardGetYm2413Enable()) {
        rm->ym2413 = ym2413Create(boardGetMixer());
        rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevMsxMusic(), &dbgCallbacks, rm);
        ioPortRegister(0x7c, NULL, write, rm);
        ioPortRegister(0x7d, NULL, write, rm);
    }

    rm->romData = malloc(pages * 0x2000);
    memcpy(rm->romData, romData, size);

    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, rm->romData + 0x2000 * i, 1, 0);
    }

    reset(rm);

    return 1;
}
