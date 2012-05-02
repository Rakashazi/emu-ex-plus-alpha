/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/SG1000.c,v $
**
** $Revision: 1.26 $
**
** $Date: 2008-04-18 04:09:54 $
**
** More info: http://www.bluemsx.com
**
** Author: Ricardo Bittencourt
** Copyright (C) 2003-2006 Daniel Vik, Ricardo Bittencourt
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SG1000.h"

#include "R800.h"
#include "R800Dasm.h"
#include "R800SaveState.h"
#include "R800Debug.h"

#include "SN76489.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "SlotManager.h"
#include "Led.h"
#include "Switches.h"
#include "IoPort.h"
#include "Disk.h"
#include "MegaromCartridge.h"
#include "Sg1000JoyIo.h"
#include "Sc3000PPI.h"
#include "Sf7000PPI.h"


/* Hardware */
static Sg1000JoyIo* joyIo;
static SN76489*     sn76489;
static R800*        r800;
static UInt8*       sfRam;
static UInt32       sfRamSize;
static UInt32       sfRamStart;


/* Although the Sega machines doesn't have the slotted layout
   described below, it makes handling of carts and boot roms
   easy when memory is layed out as below:

       SLOT 0    SLOT 1    SLOT 2    SLOT 3
0000 +---------+---------+---------+---------+
     | RAM*    |   IPL*  |  CART   |         |
4000 +---------+---------+---------+---------+
     | RAM*    |         |  CART   |         |
8000 +---------+---------+---------+---------+
     | RAM*    |         |         |         |
C000 +---------+---------+---------+---------+
     | RAM*    |         |         |         |
     +---------+---------+---------+---------+

     * All RAM is not present in all Sega machines. Some use
       1kB mirrored ram, some use normal ram
     * The IPL boot rom is only present in SF-7000 machines
*/


// ---------------------------------------------
// SG-1000 Joystick and PSG handler


static void sg1000Sn76489Write(void* dummy, UInt16 ioPort, UInt8 value) 
{
    sn76489WriteData(sn76489, ioPort, value);
}

static UInt8 joyIoRead(void* dummy, UInt16 ioPort)
{
    switch (ioPort & 1) {
    case 0:
        return boardCaptureUInt8(0, (UInt8)(sg1000JoyIoRead(joyIo) & 0xff));
    case 1:
        return boardCaptureUInt8(1, (UInt8)(sg1000JoyIoRead(joyIo) >> 8));
    }

    return 0xff;
}

static void sg1000IoPortDestroy(void* dummy)
{
	int i;

	for (i=0x40; i<0x80; i++)
		ioPortUnregister(i);

	for (i=0xC0; i<0x100; i+=2)
		ioPortUnregister(i);

	ioPortUnregister(0xc1);
	ioPortUnregister(0xdd);
}

static void sg1000IoPortCreate()
{
    DeviceCallbacks callbacks = { sg1000IoPortDestroy, NULL, NULL, NULL };
	int i;
   
	for (i=0x40; i<0x80; i++)
		ioPortRegister(i, NULL, sg1000Sn76489Write, NULL);

	for (i=0xC0; i<0x100; i+=2)
		ioPortRegister(i, joyIoRead, NULL, NULL);
    
	ioPortRegister(0xc1, joyIoRead, NULL, NULL);
	ioPortRegister(0xdd, joyIoRead, NULL, NULL);
}

// -----------------------------------------------------

static void reset()
{
    UInt32 systemTime = boardSystemTime();

    slotManagerReset();

    if (r800 != NULL) {
        r800Reset(r800, systemTime);
    }

    if (sn76489 != NULL) {
        sn76489Reset(sn76489);
    }
    
    ledSetCapslock(0);

    deviceManagerReset();
}

static void destroy() 
{    
    boardRemoveExternalDevices();
    sn76489Destroy(sn76489);
    r800DebugDestroy();
    slotManagerDestroy();
    deviceManagerDestroy();
    r800Destroy(r800);
}

static int getRefreshRate()
{
    return vdpGetRefreshRate();
}

static void saveState()
{    
    r800SaveState(r800);
    deviceManagerSaveState();
    slotSaveState();
    sn76489SaveState(sn76489);
}

static void loadState()
{
    r800LoadState(r800);
    boardInit(&r800->systemTime);
    deviceManagerLoadState();
    slotLoadState();
    sn76489LoadState(sn76489);
}

static UInt8* getRamPage(int page) {
    int start = page * 0x2000 - (int)sfRamStart;

    if (sfRam == NULL) {
        return NULL;
    }

    if (start < 0 || start >= (int)sfRamSize) {
        return NULL;
    }

	return sfRam + start;
}

static void changeCartridge(void* ref, int cartNo, int inserted)
{
    if (cartNo == 0) {
        int slot = inserted ? 2 + cartNo : 0;

        slotSetRamSlot(0, slot);
    }
}

int sg1000Create(Machine* machine, 
                 VdpSyncMode vdpSyncMode,
                 BoardInfo* boardInfo)
{
    int success;
    int i;

    sfRam = NULL;

    r800 = r800Create(0, slotRead, slotWrite, ioPortRead, ioPortWrite, NULL, boardTimerCheckTimeout, NULL, NULL, NULL, NULL);

    boardInfo->cartridgeCount   = 1;
    boardInfo->diskdriveCount   = 0;
    boardInfo->casetteCount     = 0;
    boardInfo->cpuRef           = r800;

    boardInfo->destroy          = destroy;
    boardInfo->softReset        = reset;
    boardInfo->loadState        = loadState;
    boardInfo->saveState        = saveState;
    boardInfo->getRefreshRate   = getRefreshRate;
    boardInfo->getRamPage       = getRamPage;

    boardInfo->run              = r800Execute;
    boardInfo->stop             = r800StopExecution;
    boardInfo->setInt           = r800SetInt;
    boardInfo->clearInt         = r800ClearInt;
    boardInfo->setCpuTimeout    = r800SetTimeoutAt;
    boardInfo->setBreakpoint    = r800SetBreakpoint;
    boardInfo->clearBreakpoint  = r800ClearBreakpoint;
    boardInfo->setDataBus       = r800SetDataBus;

    boardInfo->changeCartridge  = changeCartridge;

    deviceManagerCreate();
    
    boardInit(&r800->systemTime);
    ioPortReset();

    r800Reset(r800, 0);
    mixerReset(boardGetMixer());

    r800DebugCreate(r800);

    sn76489 = sn76489Create(boardGetMixer());

    slotManagerCreate();

    if (vdpSyncMode == VDP_SYNC_AUTO) {
        vdpSyncMode = VDP_SYNC_60HZ;
    }
    vdpCreate(VDP_SG1000, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000);

    joyIo = sg1000JoyIoCreate();
    if (machine->board.type == BOARD_SC3000) {
        sc3000PPICreate(joyIo);
    }
    if (machine->board.type == BOARD_SF7000) {
        sc3000PPICreate(joyIo);
        sf7000PPICreate();
            
        diskEnable(0, machine->fdc.count > 0);
        diskEnable(1, machine->fdc.count > 1);
    }
    sg1000IoPortCreate();

	ledSetCapslock(0);

    for (i = 0; i < 4; i++) {
        slotSetSubslotted(i, 0);
    }
    for (i = 0; i < 2; i++) {
        cartridgeSetSlotInfo(i, 2 + i, 0);
    }

    success = machineInitialize(machine, &sfRam, &sfRamSize, &sfRamStart);

    for (i = 0; i < 8; i++) {
        slotMapRamPage(0, 0, i);
    }

    if (machine->board.type == BOARD_SF7000) {
        slotSetRamSlot(0, 1);
    }

    if (success) {
        success = boardInsertExternalDevices();
    }

    r800SetFrequency(r800, CPU_Z80,  machine->cpu.freqZ80);
    r800SetFrequency(r800, CPU_R800, machine->cpu.freqR800);

    if (!success) {
        destroy();
    }

    return success;
}
