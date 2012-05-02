/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperTurboRPcm.c,v $
**
** $Revision: 1.13 $
**
** $Date: 2009-07-03 21:27:14 $
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
#include "romMapperTurboRPcm.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include "DAC.h"
#include "IoPort.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    DAC*   dac;
    int    deviceHandle;
    int    debugHandle;
    UInt8  sample;
    UInt8  status;
    UInt8  time;
    UInt32 refTime;
    UInt32 refFrag;
    Mixer* mixer;
} RomMapperTurboRPcm;

static void saveState(RomMapperTurboRPcm* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperTurboRPcm");

    saveStateSet(state, "sample",  rm->sample);
    saveStateSet(state, "status",  rm->status);
    saveStateSet(state, "time",    rm->time);
    saveStateSet(state, "refTime", rm->refTime);
    saveStateSet(state, "refFrag", rm->refFrag);
    
    saveStateClose(state);
}

static void loadState(RomMapperTurboRPcm* rm)
{
    SaveState* state = saveStateOpenForRead("mapperTurboRPcm");

    rm->sample  = (UInt8)saveStateGet(state, "sample",  0);
    rm->status  = (UInt8)saveStateGet(state, "status",  0);
    rm->time    = (UInt8)saveStateGet(state, "time",    0);
    rm->refTime =        saveStateGet(state, "refTime", 0);
    rm->refFrag =        saveStateGet(state, "refFrag", 0);

    mixerSetEnable(rm->mixer, rm->status & 1);

    saveStateClose(state);
}

static void destroy(RomMapperTurboRPcm* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    debugDeviceUnregister(rm->debugHandle);
    dacDestroy(rm->dac);

    ioPortUnregister(0xa4);
    ioPortUnregister(0xa5);

    free(rm);
}

static UInt8 getTimerCounter(RomMapperTurboRPcm* rm)
{
    UInt64 elapsed;
    UInt32 systemTime = boardSystemTime();

    elapsed      = 15750 * (UInt64)(systemTime - rm->refTime) + rm->refFrag;
    rm->refTime  = systemTime;
    rm->refFrag  = (UInt32)(elapsed % boardFrequency());
    rm->time += (UInt8)(elapsed / boardFrequency());

    return rm->time & 0x03;
}

static UInt8 read(RomMapperTurboRPcm* rm, UInt16 ioPort)
{
    switch (ioPort & 0x01) {
	case 0: 
		return getTimerCounter(rm);
	case 1:
		return (~rm->sample & 0x80) | rm->status;
    }
    return 0xff;
}

static UInt8 peek(RomMapperTurboRPcm* rm, UInt16 ioPort)
{
    switch (ioPort & 0x01) {
	case 0: 
        return rm->time & 0x03;
	case 1:
		return (~rm->sample & 0x80) | rm->status;
    }
    return 0xff;
}

static void write(RomMapperTurboRPcm* rm, UInt16 ioPort, UInt8 value)
{
	switch (ioPort & 0x01) {
	case 0:
        getTimerCounter(rm);
        rm->time = 0;
		rm->sample = value;
		if (rm->status & 0x02) {
            dacWrite(rm->dac, DAC_CH_MONO, rm->sample);
		}
		break;

    case 1:
        if ((value & 0x03) == 0x03 && (~rm->status & 0x01)) {
            dacWrite(rm->dac, DAC_CH_MONO, rm->sample);
		}
		rm->status = value & 0x1f;
        
        mixerSetEnable(rm->mixer, rm->status & 2);
        break;
    }
}

static void reset(RomMapperTurboRPcm* rm)
{
    rm->refTime = boardSystemTime();
    rm->refFrag = 0;
    rm->time    = 0;
}

static void getDebugInfo(RomMapperTurboRPcm* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevPcm(), 2);
    dbgIoPortsAddPort(ioPorts, 0, 0xa4, DBG_IO_READWRITE, peek(rm, 0xa4));
    dbgIoPortsAddPort(ioPorts, 1, 0xa5, DBG_IO_READWRITE, peek(rm, 0xa5));
}

int romMapperTurboRPcmCreate() 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperTurboRPcm* rm = malloc(sizeof(RomMapperTurboRPcm));

    rm->deviceHandle = deviceManagerRegister(ROM_TURBORPCM, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevPcm(), &dbgCallbacks, rm);

    rm->mixer  = boardGetMixer();

    rm->dac    = dacCreate(rm->mixer, DAC_MONO);
	rm->status = 0;
    rm->time   = 0;

    ioPortRegister(0xa4, read, write, rm);
    ioPortRegister(0xa5, read, write, rm);

    return 1;
}

