/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNet.c,v $
**
** $Revision: 1.1 $
**
** $Date: 2008-09-09 04:32:19 $
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
#include "romMapperNet.h"
#include "MediaDb.h"
#include "MidiIO.h"
#include "Switches.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "IoPort.h"
#include "Board.h"
#include "ym2151.h"
#include "SaveState.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// SFG05 Midi: The MIDI Out is probably buffered. If UART is unbuffered, all
//             data will not be transmitted correctly. Question is how big
//             the buffer is.
//             The command bits are not clear at all. Only known bit is the
//             reset bit.

// NOTES: Cmd bit 3: seems to be enable/disable something (checked before RX
//        Cmd bit 4: is set when IM2 is used, cleared when IM1 is used

#include "ArchEvent.h"

#define RX_QUEUE_SIZE 256

typedef struct {
    MidiIO*     midiIo;
    UInt8       command;
    UInt8       rxData;
    UInt8       status;
    UInt8       txBuffer;
    int         txPending;
    UInt8       rxQueue[RX_QUEUE_SIZE];
    int         rxPending;
    int         rxHead;
    void*       semaphore;
    UInt32      charTime;
    UInt8       vector;
    BoardTimer* timerRecv;
    UInt32      timeRecv;
    BoardTimer* timerTrans;
    UInt32      timeTrans;
} YM2148;

#define STAT_RXRDY      0x02
#define STAT_TXEMPTY    0x01
#define STAT_PE         0x10
#define STAT_OE         0x20        //???  MR checks 0x30
#define ST_INT          0x800

#define CMD_RDINT  0x08
#define CMD_RSTER  0x10
#define CMD_WRINT  0x100
#define CMD_RST    0x80

static void ym2148Reset(YM2148* midi);

static void midiInCallback(YM2148* midi, UInt8* buffer, UInt32 length)
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

static void onRecv(YM2148* midi, UInt32 time)
{
    midi->timeRecv = 0;

	if (midi->status & STAT_RXRDY) {
        midi->status |= STAT_OE;
        if (midi->command & CMD_RSTER) {
            ym2148Reset(midi);
            return;
        }
	} 
    
    if (midi->rxPending != 0) {
        archSemaphoreWait(midi->semaphore, -1);
        midi->rxData = midi->rxQueue[(midi->rxHead - midi->rxPending) & (RX_QUEUE_SIZE - 1)];
        midi->rxPending--;
        archSemaphoreSignal(midi->semaphore);
        midi->status |= STAT_RXRDY;
        if (midi->command & CMD_RDINT) {
            boardSetDataBus(midi->vector, 0, 0);
            boardSetInt(0x800);
            midi->status |= ST_INT;
        }
    }
    
    midi->timeRecv = boardSystemTime() + midi->charTime;
    boardTimerAdd(midi->timerRecv, midi->timeRecv);
}

static void onTrans(YM2148* midi, UInt32 time)
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
            boardSetDataBus(midi->vector, 0, 0);
            boardSetInt(0x800);
            midi->status |= ST_INT;
        }
    }
}

static void ym2148Reset(YM2148* midi)
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

static YM2148* ym2148Create()
{
    YM2148* midi = (YM2148*)calloc(1, sizeof(YM2148));

    midi->midiIo = midiIoCreate(midiInCallback, midi);
    midi->semaphore = archSemaphoreCreate(1);
    midi->timerRecv   = boardTimerCreate(onRecv, midi);
    midi->timerTrans  = boardTimerCreate(onTrans, midi);

    midi->timeRecv = boardSystemTime() + midi->charTime;
    boardTimerAdd(midi->timerRecv, midi->timeRecv);

    return midi;
}

static void ym2148Destroy(YM2148* midi)
{
    midiIoDestroy(midi->midiIo);
    archSemaphoreDestroy(midi->semaphore);
}

static UInt8 ym2148ReadStatus(YM2148* midi)
{
    UInt8 val = midi->status;

    boardClearInt(0x800);
    midi->status &= ~ST_INT;

    return val;
}

static UInt8 ym2148ReadData(YM2148* midi)
{
    midi->status &= ~(STAT_RXRDY | STAT_OE);
    return midi->rxData;
}

static void ym2148SetVector(YM2148* midi, UInt8 value)
{
    midi->vector = value;
    boardSetDataBus(midi->vector, 0, 0);
}

static void ym2148WriteCommand(YM2148* midi, UInt8 value)
{
    midi->command = value;

    if (value & 0x02) {
    }

    if (value & CMD_RST) {
        ym2148Reset(midi);
    }

    midi->charTime = (UInt32)((UInt64)144 * boardFrequency() / 500000);
}

static void ym2148WriteData(YM2148* midi, UInt8 value)
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


static void ym2148SaveState(YM2148* midi)
{
    SaveState* state = saveStateOpenForWrite("ym2148");
    
    saveStateSet(state, "command", midi->command);
    saveStateSet(state, "vector", midi->vector);
    
    saveStateClose(state);
}

static void ym2148LoadState(YM2148* midi)
{
    SaveState* state = saveStateOpenForRead("ym2148");
    
    midi->command = (UInt8)saveStateGet(state, "command", 0);
    midi->vector  = (UInt8)saveStateGet(state, "vector", 0);

    saveStateClose(state);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#if 0
IO Write #09
         ym2148WriteCommand(...)

IO Write #0E
         ym2148WriteData(...)

IO Read #0C
        return ym2148ReadStatus(rm->ym2148);
#endif


typedef struct {
    int deviceHandle;
    int debugHandle;
    YM2151* ym2151;
    YM2148* ym2148;
    UInt8* romData; 
    int slot;
    int sslot;
    int startPage;
    int sizeMask;
    MidiIO* ykIo; 
    UInt8 kbdLatch;
} RomMapperNet;

static int deviceCount = 0;

static void saveState(RomMapperNet* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperNet");
    
    saveStateSet(state, "kbdLatch", rm->kbdLatch);
    
    saveStateClose(state);

    ym2151SaveState(rm->ym2151);
    ym2148SaveState(rm->ym2148);
}

static void loadState(RomMapperNet* rm)
{
    SaveState* state = saveStateOpenForRead("mapperNet");

    rm->kbdLatch = (UInt8)saveStateGet(state, "kbdLatch", 0);

    saveStateClose(state);
    
    ym2151LoadState(rm->ym2151);
    ym2148LoadState(rm->ym2148);
}

static void destroy(RomMapperNet* rm)
{
    deviceCount--;

    if (rm->ym2151 != NULL) {
        ym2151Destroy(rm->ym2151);
    }
    if (rm->ym2148 != NULL) {
        ym2148Destroy(rm->ym2148);
    }

    if (rm->ykIo != NULL) {
        ykIoDestroy(rm->ykIo);
    }

    slotUnregister(rm->slot, rm->sslot, rm->startPage);

    debugDeviceUnregister(rm->debugHandle);
    deviceManagerUnregister(rm->deviceHandle);

    if (rm->romData != NULL) {
        free(rm->romData);
    }
    free(rm);
}

#define YK01_KEY_START 37

static UInt8 getKbdStatus(RomMapperNet* rm)
{
    UInt8 val = 0xff;
    int row;

    for (row = 0; row < 8; row++) {
        if ((1 << row) & rm->kbdLatch) {
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 0) ? ~0x01 : 0xff;
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 1) ? ~0x02 : 0xff;
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 2) ? ~0x04 : 0xff;
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 3) ? ~0x10 : 0xff;
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 4) ? ~0x20 : 0xff;
            val &= ykIoGetKeyState(rm->ykIo, YK01_KEY_START + row * 6 + 5) ? ~0x40 : 0xff;
        }
    }

    return val;
}

static UInt8 read(RomMapperNet* rm, UInt16 address) 
{
    if (address < 0x3ff0 || address >= 0x3ff8) {
    	return rm->romData[address & rm->sizeMask];
    }

    switch (address & 0x3fff) {
    case 0x3ff0:
        return ym2151Read(rm->ym2151, 0);
    case 0x3ff1:
        return ym2151Read(rm->ym2151, 1);
    case 0x3ff2:
        return getKbdStatus(rm);
    case 0x3ff5:
        return ym2148ReadData(rm->ym2148);
    case 0x3ff6:
        return ym2148ReadStatus(rm->ym2148);
    }

    return 0xff;
}

static void reset(RomMapperNet* rm) 
{
    ym2151Reset(rm->ym2151);
    ym2148Reset(rm->ym2148);
    rm->kbdLatch = 0;
}

static void write(RomMapperNet* rm, UInt16 address, UInt8 value) 
{
    if (address < 0x3ff0 || address >= 0x3ff8) {
    	return;
    }

    switch (address & 0x3fff) {
    case 0x3ff0:
        ym2151Write(rm->ym2151, 0, value);
        break;
    case 0x3ff1:
        ym2151Write(rm->ym2151, 1, value);
        break;
    case 0x3ff2:
        rm->kbdLatch = value;
        break;
    case 0x3ff3:
        ym2148SetVector(rm->ym2148, value);
        break;
    case 0x3ff4:
        boardSetDataBus(value, value, 1);
        ym2151SetIrqVector(rm->ym2151, value);
        break;
    case 0x3ff5:
        ym2148WriteData(rm->ym2148, value);
        break;
    case 0x3ff6:
        ym2148WriteCommand(rm->ym2148, value);
        break;
    }
}


static void getDebugInfo(RomMapperNet* rm, DbgDevice* dbgDevice)
{
    ym2151GetDebugInfo(rm->ym2151, dbgDevice);
}

int romMapperNetCreate(const char* filename, UInt8* romData,
                       int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    RomMapperNet* rm;
    int i;
    int pages = size / 0x2000;

    if (size != 0x4000 && size != 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperNet));

    rm->deviceHandle = deviceManagerRegister(ROM_YAMAHANET, &callbacks, rm);
    rm->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, "Yamaha Net", &dbgCallbacks, rm);

    slotRegister(slot, sslot, startPage, pages, read, read, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    rm->sizeMask = size - 1;

    for (i = 0; i < pages; i++) {
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, NULL, 0, 0);
    }

    rm->ym2151 = ym2151Create(boardGetMixer());
    rm->ym2148 = ym2148Create();
    rm->ykIo = ykIoCreate();

    reset(rm);

    return 1;
}

