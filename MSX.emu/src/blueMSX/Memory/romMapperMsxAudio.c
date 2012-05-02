/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperMsxAudio.c,v $
**
** $Revision: 1.20 $
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
#include "romMapperMsxAudio.h"
#include "MediaDb.h"
#include "Switches.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "IoPort.h"
#include "Board.h"
#include "Y8950.h"
#include "MSXMidi.h"
#include "MidiIO.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ArchEvent.h"

#define RX_QUEUE_SIZE 256

typedef struct {
    MidiIO* midiIo;
    UInt8   command;
    UInt8   rxData;
    UInt8   status;
    UInt8   txBuffer;
    int     txPending;
    UInt8   rxQueue[RX_QUEUE_SIZE];
    int     rxPending;
    int     rxHead;
    void*   semaphore;
    UInt32      charTime;
    BoardTimer* timerRecv;
    UInt32      timeRecv;
    BoardTimer* timerTrans;
    UInt32      timeTrans;
} PhilipsMidi;

#define STAT_RXRDY      0x01
#define STAT_TXEMPTY    0x02
#define STAT_OE         0x20
#define ST_INT          0x80

#define CMD_WRINT  0x20
#define CMD_RDINT  0x80

static void midiInCallback(PhilipsMidi* midi, UInt8* buffer, UInt32 length)
{
    archSemaphoreWait(midi->semaphore, -1);
    if (midi->rxPending + length < RX_QUEUE_SIZE) {
        while (length--) {
            midi->rxQueue[midi->rxHead & (RX_QUEUE_SIZE - 1)] = *buffer++;
            midi->rxHead++;
            midi->rxPending++;
        }
    }
    archSemaphoreSignal(midi->semaphore);
}

static void onRecv(PhilipsMidi* midi, UInt32 time)
{
    midi->timeRecv = 0;

	if (midi->status & STAT_RXRDY) {
		midi->status |= STAT_OE;
	} 
    else if (midi->rxPending != 0) {
        archSemaphoreWait(midi->semaphore, -1);
        midi->rxData = midi->rxQueue[(midi->rxHead - midi->rxPending) & (RX_QUEUE_SIZE - 1)];
        midi->rxPending--;
        archSemaphoreSignal(midi->semaphore);
        midi->status |= STAT_RXRDY;
        if (midi->command & CMD_RDINT) {
            boardSetInt(0x400);
            midi->status |= ST_INT;
        }
    }
    
    midi->timeRecv = boardSystemTime() + midi->charTime;
    boardTimerAdd(midi->timerRecv, midi->timeRecv);
}

static void onTrans(PhilipsMidi* midi, UInt32 time)
{
    midi->timeTrans = 0;

    if (midi->status & STAT_TXEMPTY) {
        midi->txPending = 0;
    }
    else {
        midiIoTransmit(midi->midiIo, midi->txBuffer);
        midi->timeTrans = boardSystemTime() + midi->charTime;
        boardTimerAdd(midi->timerTrans, midi->timeTrans);

        midi->status |= STAT_TXEMPTY;
        if (midi->command & CMD_WRINT) {
            boardSetInt(0x400);
            midi->status |= ST_INT;
        }
    }
}

void philipsMidiReset(PhilipsMidi* midi)
{
    midi->status = STAT_TXEMPTY;
    midi->txPending = 0;
    midi->rxPending = 0;
    midi->command = 0;
    midi->timeRecv = 0;
    midi->timeTrans = 0;
    midi->charTime = 10 * boardFrequency() / 31250;

    boardTimerRemove(midi->timerRecv);
    boardTimerRemove(midi->timerTrans);
    
    midi->timeRecv = boardSystemTime() + midi->charTime;
    boardTimerAdd(midi->timerRecv, midi->timeRecv);
}

PhilipsMidi* philipsMidiCreate()
{
    PhilipsMidi* midi = (PhilipsMidi*)calloc(1, sizeof(PhilipsMidi));

    midi->midiIo = midiIoCreate(midiInCallback, midi);
    midi->semaphore = archSemaphoreCreate(1);
    midi->timerRecv   = boardTimerCreate(onRecv, midi);
    midi->timerTrans  = boardTimerCreate(onTrans, midi);

    return midi;
}

void philipsMidiDestroy(PhilipsMidi* midi)
{
    boardTimerDestroy(midi->timerTrans);
    boardTimerDestroy(midi->timerRecv);
    midiIoDestroy(midi->midiIo);
    archSemaphoreDestroy(midi->semaphore);
    free(midi);
}

UInt8 philipsMidiReadStatus(PhilipsMidi* midi)
{
    UInt8 val = midi->status;

    boardClearInt(0x400);
    midi->status &= ~ST_INT;

    return val;
}

UInt8 philipsMidiReadData(PhilipsMidi* midi)
{
    midi->status &= ~(STAT_RXRDY | STAT_OE);
    return midi->rxData;
}

void philipsMidiWriteCommand(PhilipsMidi* midi, UInt8 value)
{
    int baudrate      = 1;
    int dataBits      = 8;
    int parityEnable  = 0;
    int stopBits      = 1;
    UInt64 charLength;

    midi->command = value;

    switch (value & 0x03) {
    case 0:
        baudrate = 1;
        break;
    case 1:
        baudrate = 16;
        break;
    case 2:
        baudrate = 64;
        break;
    case 3:
        philipsMidiReset(midi);
        break;
    }
    
    switch (value & 0x1c) {
    case 0:
        dataBits     = 7;
        parityEnable = 1;
        stopBits     = 2;
        break;
    case 1:
        dataBits     = 7;
        parityEnable = 1;
        stopBits     = 2;
        break;
    case 2:
        dataBits     = 7;
        parityEnable = 1;
        stopBits     = 1;
        break;
    case 3:
        dataBits     = 7;
        parityEnable = 1;
        stopBits     = 1;
        break;
    case 4:
        dataBits     = 8;
        parityEnable = 0;
        stopBits     = 2;
        break;
    case 5:
        dataBits     = 8;
        parityEnable = 0;
        stopBits     = 1;
        break;
    case 6:
        dataBits     = 8;
        parityEnable = 0;
        stopBits     = 1;
        break;
    case 7:
        dataBits     = 8;
        parityEnable = 1;
        stopBits     = 1;
        break;
    }

	charLength = (dataBits + parityEnable + stopBits) * baudrate;
    midi->charTime = (UInt32)(charLength * boardFrequency() / 500000);
    
    midi->timeRecv = boardSystemTime() + midi->charTime;
    boardTimerAdd(midi->timerRecv, midi->timeRecv);
}

void philipsMidiWriteData(PhilipsMidi* midi, UInt8 value)
{
    if (!(midi->status & STAT_TXEMPTY)) {
        return;
    }

    if (midi->txPending == 0) {
        midiIoTransmit(midi->midiIo, value);
        midi->timeTrans = boardSystemTime() + midi->charTime;
        boardTimerAdd(midi->timerTrans, midi->timeTrans);
        midi->txPending = 1;
    }
    else {
        midi->status &= ~STAT_TXEMPTY;
        midi->txBuffer = value;
    }
}


typedef struct {
    int deviceHandle;
    int debugHandle;
    Y8950* y8950;
    int ioBase;
    UInt8* romData;
    UInt8 ram[0x1000];
    int bankSelect; 
    int sizeMask;

    PhilipsMidi* midi;

    int slot;
    int sslot;
    int startPage;
    int is_fs_ca1;
} RomMapperMsxAudio;


static int deviceCount = 0;

static void saveState(RomMapperMsxAudio* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperMsxAudio");

    saveStateSet(state, "bankSelect", rm->bankSelect);
    saveStateSetBuffer(state, "ram", rm->ram, sizeof(rm->ram));
    
    saveStateClose(state);

    if (rm->y8950 != NULL) {
        y8950SaveState(rm->y8950);
    }
}

static void loadState(RomMapperMsxAudio* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMsxAudio");

    rm->bankSelect = saveStateGet(state, "bankSelect", 0);
    saveStateGetBuffer(state, "ram", rm->ram, sizeof(rm->ram));

    saveStateClose(state);
    
    if (rm->y8950 != NULL) {
        y8950LoadState(rm->y8950);
    }
}

static void destroy(RomMapperMsxAudio* rm)
{
    if (rm->midi) {
        philipsMidiDestroy(rm->midi);
    }

    ioPortUnregister(0x00);
    ioPortUnregister(0x01);
    ioPortUnregister(0x04);
    ioPortUnregister(0x05);

    ioPortUnregister(rm->ioBase + 0);
    ioPortUnregister(rm->ioBase + 1);
    
    if (rm->y8950) {
    	if (ioPortGetRef(0xc0)==rm->y8950&&ioPortGetRef(0xc1)==rm->y8950) {
    		ioPortUnregister(0xc0); ioPortUnregister(0xc1);
    	}
    	if (ioPortGetRef(0xc2)==rm->y8950&&ioPortGetRef(0xc3)==rm->y8950) {
    		ioPortUnregister(0xc2); ioPortUnregister(0xc3);
    	}
    }

    deviceCount--;

    if (rm->y8950 != NULL) {
        y8950Destroy(rm->y8950);
    }

    if (rm->sizeMask != -1) {
        slotUnregister(rm->slot, rm->sslot, rm->startPage);
    }

    debugDeviceUnregister(rm->debugHandle);
    deviceManagerUnregister(rm->deviceHandle);

    if (rm->romData != NULL) {
        free(rm->romData);
    }
    free(rm);
}

static UInt8 read(RomMapperMsxAudio* rm, UInt16 address) 
{
	if (rm->bankSelect == 0 && (address & 0x3fff) >= 0x3000) {
		return rm->ram[(address & 0x3fff) - 0x3000];
    }

	return rm->romData[(0x8000 * rm->bankSelect + (address & 0x7fff)) & rm->sizeMask];
}

static void write(RomMapperMsxAudio* rm, UInt16 address, UInt8 value) 
{
	address &= 0x7fff;
	
#if 0
	// FS-CA1 port select, bit 0:c0/c1, bit 1:c2/c3
	if (rm->is_fs_ca1&&address==0x7fff&&rm->y8950) {
		if (value&1) {
			ioPortRegister(0xc0, y8950Read, y8950Write, rm->y8950);
			ioPortRegister(0xc1, y8950Read, y8950Write, rm->y8950);
		}
		else if (ioPortGetRef(0xc0)==rm->y8950&&ioPortGetRef(0xc1)==rm->y8950) {
			ioPortUnregister(0xc0); ioPortUnregister(0xc1);
		}
		
		if (value&2) {
			ioPortRegister(0xc2, y8950Read, y8950Write, rm->y8950);
			ioPortRegister(0xc3, y8950Read, y8950Write, rm->y8950);
		}
		else if (ioPortGetRef(0xc2)==rm->y8950&&ioPortGetRef(0xc3)==rm->y8950) {
			ioPortUnregister(0xc2); ioPortUnregister(0xc3);
		}
	}
#endif
	// bankswitch
	if (address==0x7ffe) rm->bankSelect = value & 3;
	
	address &= 0x3fff;
	if (rm->bankSelect == 0 && address >= 0x3000) {
		rm->ram[address - 0x3000] = value;
	}
}


static void reset(RomMapperMsxAudio* rm) 
{
    if (rm->y8950 != NULL) {
        y8950Reset(rm->y8950);
    }

    if (rm->midi) {
        philipsMidiReset(rm->midi);
    }
    
    // FS-CA1
    write (rm,0x7ffe,0);
    write (rm,0x7fff,0);
}


static void midiWrite(RomMapperMsxAudio* rm, UInt16 ioPort, UInt8 value)
{
    if (!rm->midi) {
        return;
    }

    switch (ioPort & 1) {
    case 0x00:
        philipsMidiWriteCommand(rm->midi, value);
        break;
    case 0x01:
        philipsMidiWriteData(rm->midi, value);
        break;
    }
}


static UInt8 midiRead(RomMapperMsxAudio* rm, UInt16 ioPort)
{
    if (!rm->midi) {
        return 0xff;
    }

    switch (ioPort & 1) {
    case 0x00:
        return philipsMidiReadStatus(rm->midi);
    case 0x01:
        return philipsMidiReadData(rm->midi);
    }

    return 0xff;
}

static void getDebugInfo(RomMapperMsxAudio* rm, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    if (rm->y8950 == NULL) {
        return;
    }

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMsxAudio(), 2);
    dbgIoPortsAddPort(ioPorts, 0, rm->ioBase + 0, DBG_IO_READWRITE, y8950Peek(rm->y8950, 0));
    dbgIoPortsAddPort(ioPorts, 1, rm->ioBase + 1, DBG_IO_READWRITE, y8950Peek(rm->y8950, 1));
    
    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMsxAudioMidi(), 4);
    dbgIoPortsAddPort(ioPorts, 0, 0x00, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 1, 0x01, DBG_IO_WRITE, 0);
    dbgIoPortsAddPort(ioPorts, 2, 0x04, DBG_IO_READ, midiRead(rm, 0x04));
    dbgIoPortsAddPort(ioPorts, 3, 0x05, DBG_IO_READ, midiRead(rm, 0x05));

    y8950GetDebugInfo(rm->y8950, dbgDevice);
}

int romMapperMsxAudioCreate(const char* filename, UInt8* romData,
                            int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperMsxAudio* rm;
    int i;

    rm = malloc(sizeof(RomMapperMsxAudio));

    rm->deviceHandle = deviceManagerRegister(ROM_MSXAUDIO, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevMsxAudio(), &dbgCallbacks, rm);

    rm->ioBase = 0xc0 + deviceCount++ * 2;

    rm->romData = NULL;

    if (size > 0) {
        int pages=8;
        rm->is_fs_ca1 = (size == 0x20000); // meh
        // pages=rm->is_fs_ca1?4:8;
        
        // For FS-CA1, $8000-$FFFF is unmapped
        // firmware locks up, needs more testing
        slotRegister(slot, sslot, startPage, pages, read, read, write, destroy, rm);

        rm->romData = malloc(size);
        memcpy(rm->romData, romData, size);
        memset(rm->ram, 0, 0x1000);
        rm->bankSelect = 0;
        rm->sizeMask = size - 1;
        rm->slot  = slot;
        rm->sslot = sslot;
        rm->startPage  = startPage;
        rm->midi = NULL;

        if (!switchGetAudio()) {
            // FS-CA1 BIOS hack, ret z -> nop
            // not needed if port select register is emulated
            rm->romData[0x408e] = 0;
        }
        
        for (i = 0; i < pages; i++) {
            slotMapPage(rm->slot, rm->sslot, rm->startPage + i, NULL, 0, 0);
        }
    }

    rm->y8950 = NULL;

    if (boardGetY8950Enable()) {
        rm->y8950 = y8950Create(boardGetMixer());
       	
       	ioPortRegister(rm->ioBase + 0, y8950Read, y8950Write, rm->y8950);
       	ioPortRegister(rm->ioBase + 1, y8950Read, y8950Write, rm->y8950);
	
        ioPortRegister(0x00, NULL, midiWrite, rm);
        ioPortRegister(0x01, NULL, midiWrite, rm);
        ioPortRegister(0x04, midiRead, NULL, rm);
        ioPortRegister(0x05, midiRead, NULL, rm);
    }
    
    if (deviceCount == 1) {
        rm->midi = philipsMidiCreate();
    }
    
    reset(rm);

    return 1;
}

