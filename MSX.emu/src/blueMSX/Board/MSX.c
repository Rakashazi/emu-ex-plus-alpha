/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/MSX.c,v $
**
** $Revision: 1.71 $
**
** $Date: 2008-04-18 04:09:54 $
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MSX.h"

#include "R800.h"
#include "R800Dasm.h"
#include "R800SaveState.h"
#include "R800Debug.h"

#include "SaveState.h"
#include "MsxPPI.h"
#include "Board.h"
#include "RTC.h"
#include "MsxPsg.h"
#include "VDP.h"
#include "Casette.h"
#include "Disk.h"
#include "MegaromCartridge.h"
#include "IoPort.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "ramMapperIo.h"
#include "CoinDevice.h"

void PatchZ80(void* ref, CpuRegs* cpuRegs);

// Hardware
static MsxPsg*         msxPsg;
static R800*           r800;
static RTC*            rtc;
static UInt8*          msxRam;
static UInt32          msxRamSize;
static UInt32          msxRamStart;
static UInt32          z80Frequency;

void msxSetCpu(int mode)
{
    switch (mode) {
    default:
    case 0:
        r800SetMode(r800, CPU_Z80);
        break;
    case 1:
        r800SetMode(r800, CPU_R800);
        break;
    }
}

void msxEnableCpuFreq_1_5(int enable) {
    if (enable) {
        r800SetFrequency(r800, CPU_Z80, 3 * z80Frequency / 2);
    }
    else {
        r800SetFrequency(r800, CPU_Z80, z80Frequency);
    }
}

static void reset()
{
    UInt32 systemTime = boardSystemTime();

    slotManagerReset();

    if (r800 != NULL) {
        r800Reset(r800, systemTime);
    }
    
    deviceManagerReset();
}

static void destroy() {
    rtcDestroy(rtc);

    boardRemoveExternalDevices();

    slotManagerDestroy();

    r800DebugDestroy();
    
	ioPortUnregister(0x2e);

    deviceManagerDestroy();

    r800Destroy(r800);
}

int getPC(){return r800->regs.PC.W;}

static int getRefreshRate()
{
    return vdpGetRefreshRate();
}

static UInt8* getRamPage(int page) {
    int start;

    if (msxRam == NULL) {
        return NULL;
    }

    start = page * 0x2000 - (int)msxRamStart;
    if (page < 0) {
        start += msxRamSize;
    }

    if (start < 0 || start >= (int)msxRamSize) {
        return NULL;
    }

	return msxRam + start;
}
    
static void saveState()
{   
    SaveState* state = saveStateOpenForWrite("msx");

    saveStateSet(state, "z80Frequency",    z80Frequency);
    
    saveStateClose(state);

    r800SaveState(r800);
    deviceManagerSaveState();
    slotSaveState();
    rtcSaveState(rtc);
}

static void loadState()
{
    SaveState* state = saveStateOpenForRead("msx");

    z80Frequency = saveStateGet(state, "z80Frequency", 0);

    saveStateClose(state);

    r800LoadState(r800);
    boardInit(&r800->systemTime);

    deviceManagerLoadState();
    slotLoadState();
    rtcLoadState(rtc);
}

static UInt8 testPort(void* dummy, UInt16 ioPort)
{
    return 0x27;
}

int msxCreate(Machine* machine, 
              VdpSyncMode vdpSyncMode,
              BoardInfo* boardInfo)
{
    char cmosName[512];
    int success;
    int i;

    UInt32 cpuFlags = CPU_ENABLE_M1;

    if (machine->board.type == BOARD_MSX_T9769B ||
        machine->board.type == BOARD_MSX_T9769C)
    {
        cpuFlags |= CPU_VDP_IO_DELAY;
    }

    r800 = r800Create(cpuFlags, slotRead, slotWrite, ioPortRead, ioPortWrite, PatchZ80, boardTimerCheckTimeout, NULL, NULL, NULL, NULL);

    boardInfo->cartridgeCount   = machine->board.type == BOARD_MSX_FORTE_II ? 0 : 2;
    boardInfo->diskdriveCount   = machine->board.type == BOARD_MSX_FORTE_II ? 0 : 2;
    boardInfo->casetteCount     = machine->board.type == BOARD_MSX_FORTE_II ? 0 : 1;
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

    deviceManagerCreate();
    boardInit(&r800->systemTime);

    ioPortReset();
    ramMapperIoCreate();

    r800Reset(r800, 0);
    mixerReset(boardGetMixer());

    msxPPICreate(machine->board.type == BOARD_MSX_FORTE_II);
    slotManagerCreate();

    r800DebugCreate(r800);
    
	ioPortRegister(0x2e, testPort, NULL, NULL);

    sprintf(cmosName, "%s" DIR_SEPARATOR "%s.cmos", boardGetBaseDirectory(), machine->name);
    rtc = rtcCreate(machine->cmos.enable, machine->cmos.batteryBacked ? cmosName : 0);

    msxRam = NULL;

    vdpCreate(VDP_MSX, machine->video.vdpVersion, vdpSyncMode, machine->video.vramSize / 0x4000);

    for (i = 0; i < 4; i++) {
        slotSetSubslotted(i, machine->slot[i].subslotted);
    }

    for (i = 0; i < 2; i++) {
        cartridgeSetSlotInfo(i, machine->cart[i].slot, machine->cart[i].subslot);
    }

    success = machineInitialize(machine, &msxRam, &msxRamSize, &msxRamStart);

    msxPsg = msxPsgCreate(machine->board.type == BOARD_MSX || 
                          machine->board.type == BOARD_MSX_FORTE_II 
                          ? PSGTYPE_AY8910 : PSGTYPE_YM2149,
                          machine->audio.psgstereo,
                          machine->audio.psgpan,
                          machine->board.type == BOARD_MSX_FORTE_II ? 1 : 2);

    if (machine->board.type == BOARD_MSX_FORTE_II) {
        CoinDevice* coinDevice = coinDeviceCreate(msxPsg);
        msxPsgRegisterCassetteRead(msxPsg, coinDeviceRead, coinDevice);
    }

    for (i = 0; i < 8; i++) {
        slotMapRamPage(0, 0, i);
    }

    if (success) {
        success = boardInsertExternalDevices();
    }

    z80Frequency = machine->cpu.freqZ80;

    diskEnable(0, machine->fdc.count > 0);
    diskEnable(1, machine->fdc.count > 1);

    r800SetFrequency(r800, CPU_Z80,  machine->cpu.freqZ80);
    r800SetFrequency(r800, CPU_R800, machine->cpu.freqR800);

    if (!success) {
        destroy();
    }

    return success;
}
