/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/Moonsound.cpp,v $
**
** $Revision: 1.22 $
**
** $Date: 2008-03-30 18:38:45 $
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
#include "Moonsound.h"
#include <string.h>
#include "OpenMsxYMF262.h"
#include "OpenMsxYMF278.h"
extern "C" {
#include "Board.h"
#include "SaveState.h"
#include "Language.h"
}

#define FREQUENCY        3579545
 
struct Moonsound {
    Moonsound() :
        timerValue1(0), timerValue2(0), timerRef1(0xff), timerRef2(0xff),
        opl3latch(0), opl4latch(0) {
        memset(defaultBuffer, 0, sizeof(defaultBuffer));
    }

    Mixer* mixer;
    Int32 handle;

    YMF278* ymf278;
    YMF262* ymf262;
    Int32  buffer[AUDIO_STEREO_BUFFER_SIZE];
    Int32  defaultBuffer[AUDIO_STEREO_BUFFER_SIZE];
    BoardTimer* timer1;
    BoardTimer* timer2;
    UInt32 timeout1;
    UInt32 timeout2;
    UInt32 timerValue1;
    UInt32 timerValue2;
    UInt32 timerStarted1;
    UInt32 timerStarted2;
    UInt8  timerRef1;
    UInt8  timerRef2;
    int opl3latch;
    UInt8 opl4latch;
};


void moonsoundTimerStart(void* ref, int timer, int start, UInt8 timerRef);

static void onTimeout1(void* ref, UInt32 time)
{
    Moonsound* moonsound = (Moonsound*)ref;

    moonsoundTimerStart(moonsound, 1, 1, moonsound->timerRef1);
    moonsound->ymf262->callback(moonsound->timerRef1);
}

static void onTimeout2(void* ref, UInt32 time)
{
    Moonsound* moonsound = (Moonsound*)ref;

    moonsoundTimerStart(moonsound, 4, 1, moonsound->timerRef2);
    moonsound->ymf262->callback(moonsound->timerRef2);
}

void moonsoundTimerSet(void* ref, int timer, int count)
{
    Moonsound* moonsound = (Moonsound*)ref;

    if (timer == 1) {
        moonsound->timerValue1 = count;
        if (moonsound->timerStarted1) {
            moonsoundTimerStart(moonsound, timer, 1, moonsound->timerRef1);
        }
    }
    else {
        moonsound->timerValue2 = count;
        if (moonsound->timerStarted2) {
            moonsoundTimerStart(moonsound, timer, 1, moonsound->timerRef2);
        }
    }
}

void moonsoundTimerStart(void* ref, int timer, int start, UInt8 timerRef)
{
    Moonsound* moonsound = (Moonsound*)ref;

    if (timer == 1) {
        moonsound->timerRef1 = timerRef;
        moonsound->timerStarted1 = start;
        if (start) {
            moonsound->timeout1 = boardCalcRelativeTimeout(12380, moonsound->timerValue1);
            boardTimerAdd(moonsound->timer1, moonsound->timeout1);
        }
        else {
            boardTimerRemove(moonsound->timer1);
        }
    }
    else {
        moonsound->timerRef2 = timerRef;
        moonsound->timerStarted2 = start;
        if (start) {
            moonsound->timeout2 = boardCalcRelativeTimeout(12380, moonsound->timerValue2);
            boardTimerAdd(moonsound->timer2, moonsound->timeout2);
        }
        else {
            boardTimerRemove(moonsound->timer2);
        }
    }
}

extern "C" {

void moonsoundDestroy(Moonsound* moonsound) 
{
    mixerUnregisterChannel(moonsound->mixer, moonsound->handle);

    delete moonsound->ymf262;
    delete moonsound->ymf278;
    
    boardTimerDestroy(moonsound->timer1);
    boardTimerDestroy(moonsound->timer2);

    delete moonsound;
}

void moonsoundSaveState(Moonsound* moonsound)
{
    SaveState* state = saveStateOpenForWrite("moonsound");

    saveStateSet(state, "timerValue1",    moonsound->timerValue1);
    saveStateSet(state, "timeout1",       moonsound->timeout1);
    saveStateSet(state, "timerStarted1",  moonsound->timerStarted1);
    saveStateSet(state, "timerRef1",      moonsound->timerRef1);
    saveStateSet(state, "timerValue2",    moonsound->timerValue2);
    saveStateSet(state, "timeout2",       moonsound->timeout2);
    saveStateSet(state, "timerStarted2",  moonsound->timerStarted2);
    saveStateSet(state, "timerRef2",      moonsound->timerRef2);
    saveStateSet(state, "opl3latch", moonsound->opl3latch);
    saveStateSet(state, "opl4latch", moonsound->opl4latch);

    saveStateClose(state);

    moonsound->ymf262->saveState();
    moonsound->ymf278->saveState();
}

void moonsoundLoadState(Moonsound* moonsound)
{
    SaveState* state = saveStateOpenForRead("moonsound");

    moonsound->timerValue1    =        saveStateGet(state, "timerValue1",    0);
    moonsound->timeout1       =        saveStateGet(state, "timeout1",       0);
    moonsound->timerStarted1  =        saveStateGet(state, "timerStarted1",  0);
    moonsound->timerRef1      = (UInt8)saveStateGet(state, "timerRef1",      0);
    moonsound->timerValue2    =        saveStateGet(state, "timerValue2",    0);
    moonsound->timeout2       =        saveStateGet(state, "timeout2",       0);
    moonsound->timerStarted2  =        saveStateGet(state, "timerStarted2",  0);
    moonsound->timerRef2      = (UInt8)saveStateGet(state, "timerRef2",      0);
    moonsound->opl3latch =        saveStateGet(state, "opl3latch", 0);
    moonsound->opl4latch = (UInt8)saveStateGet(state, "opl4latch", 0);

    saveStateClose(state);

    moonsound->ymf262->loadState();
    moonsound->ymf278->loadState();
    
    if (moonsound->timerStarted1) {
        boardTimerAdd(moonsound->timer1, moonsound->timeout1);
    }

    if (moonsound->timerStarted2) {
        boardTimerAdd(moonsound->timer2, moonsound->timeout2);
    }
}


static char* regText(int d)
{
    static char text[5];
    sprintf(text, "R%.2x", d);
    return text;
}

static char* slotRegText(int s, int r)
{
    static char text[5];
    sprintf(text, "S%d:%d", s, r);
    return text;
}

static char regsAvailYMF262[] = {
    0,1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0x00
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, // 0x20
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, // 0x40
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, // 0x60
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0, // 0x80
    1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,1,0,0, // 0xa0
    1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0xc0
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0  // 0xe0
};

void moonsoundGetDebugInfo(Moonsound* moonsound, DbgDevice* dbgDevice)
{
    UInt32 systemTime = boardSystemTime();
    DbgRegisterBank* regBank;
    int r;

    // Add YMF262 registers
    int c = 1;
    for (r = 0; r < sizeof(regsAvailYMF262); r++) {
        c += regsAvailYMF262[r];
    }

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegsYmf262(), c);

    c = 0;
    dbgRegisterBankAddRegister(regBank, c++, "SR", 8, moonsound->ymf262->peekStatus());

    for (r = 0; r < sizeof(regsAvailYMF262); r++) {
        if (regsAvailYMF262[r]) {
            if (r <= 8) {
                dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf262->peekReg(r|0x100));
            }
            else {
                dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf262->peekReg(r));
            }
        }
    }

    // Add YMF278 registers
    c = 1 + 7 + 2 + 10 * 10;
    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegsYmf278(), c);

    c = 0;
    dbgRegisterBankAddRegister(regBank, c++, "SR", 8, moonsound->ymf278->peekStatus(systemTime));
    
    r=0x00; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x01; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x02; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x03; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x04; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x05; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0x06; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0xf8; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
    r=0xf9; dbgRegisterBankAddRegister(regBank, c++, regText(r), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));

    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            int r = 8 + i * 24 + j;
            dbgRegisterBankAddRegister(regBank, c++, slotRegText(i,j), 8, moonsound->ymf278->peekRegOPL4(r, systemTime));
        }
    }

    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemYmf278(), 0, 0, 
                            moonsound->ymf278->getRamSize(), 
                            (UInt8*)moonsound->ymf278->getRam());
}

void moonsoundReset(Moonsound* moonsound)
{
    UInt32 systemTime = boardSystemTime();

    moonsound->timerStarted1 = (UInt32)-1;
    moonsound->timerStarted2 = (UInt32)-1;
    moonsound->ymf262->reset(systemTime);
    moonsound->ymf278->reset(systemTime);

    moonsoundTimerStart(moonsound, 1, 0, moonsound->timerRef1);
    moonsoundTimerStart(moonsound, 4, 0, moonsound->timerRef2);
}

static Int32* moonsoundSync(void* ref, UInt32 count) 
{
    Moonsound* moonsound = (Moonsound*)ref;
    int* genBuf1 = NULL;
    int* genBuf2 = NULL;
    UInt32 i;

    genBuf1 = moonsound->ymf262->updateBuffer(count);
    if (genBuf1 == NULL) {
        genBuf1 = (int*)moonsound->defaultBuffer;
    }

    genBuf2 = moonsound->ymf278->updateBuffer(count);
    if (genBuf2 == NULL) {
        genBuf2 = (int*)moonsound->defaultBuffer;
    }

    for (i = 0; i < 2 * count; i++) {
        moonsound->buffer[i] = genBuf1[i] + genBuf2[i];
    }

    return moonsound->buffer;
}

UInt8 moonsoundPeek(Moonsound* moonsound, UInt16 ioPort)
{
	UInt8 result = 0xff;
    UInt32 systemTime = boardSystemTime();

    if (moonsound == NULL) {
        return 0xff;
    }

	if (ioPort < 0xC0) {
		switch (ioPort & 0x01) {
		case 1: // read wave register
			result = moonsound->ymf278->peekRegOPL4(moonsound->opl4latch, systemTime);
			break;
		}
	} else {
		switch (ioPort & 0x03) {
		case 0: // read status
		case 2:
			result = moonsound->ymf262->peekStatus() | 
                     moonsound->ymf278->peekStatus(systemTime);
			break;
		case 1:
		case 3: // read fm register
			result = moonsound->ymf262->peekReg(moonsound->opl3latch);
			break;
		}
	}

    return result;
}

UInt8 moonsoundRead(Moonsound* moonsound, UInt16 ioPort)
{
	UInt8 result = 0xff;
    UInt32 systemTime = boardSystemTime();

	if (ioPort < 0xC0) {
		switch (ioPort & 0x01) {
		case 1: // read wave register
            mixerSync(moonsound->mixer);
			result = moonsound->ymf278->readRegOPL4(moonsound->opl4latch, systemTime);
			break;
		}
	} else {
		switch (ioPort & 0x03) {
		case 0: // read status
		case 2:
            mixerSync(moonsound->mixer);
			result = moonsound->ymf262->readStatus() | 
                     moonsound->ymf278->readStatus(systemTime);
			break;
		case 1:
		case 3: // read fm register
            mixerSync(moonsound->mixer);
			result = moonsound->ymf262->readReg(moonsound->opl3latch);
			break;
		}
	}

    return result;
}

void moonsoundWrite(Moonsound* moonsound, UInt16 ioPort, UInt8 value)
{
    UInt32 systemTime = boardSystemTime();
	if (ioPort < 0xC0) {
		switch (ioPort & 0x01) {
		case 0: // select register
			moonsound->opl4latch = value;
			break;
		case 1:
            mixerSync(moonsound->mixer);
  			moonsound->ymf278->writeRegOPL4(moonsound->opl4latch, value, systemTime);
			break;
		}
	} else {
		switch (ioPort & 0x03) {
		case 0:
			moonsound->opl3latch = value;
			break;
		case 2: // select register bank 1
			moonsound->opl3latch = value | 0x100;
			break;
		case 1:
		case 3: // write fm register
            mixerSync(moonsound->mixer);
			moonsound->ymf262->writeReg(moonsound->opl3latch, value, systemTime);
			break;
		}
	}
}

void moonsoundSetSampleRate(void* ref, UInt32 rate)
{
	Moonsound* moonsound = (Moonsound*)ref;
	moonsound->ymf262->setSampleRate(rate, boardGetMoonsoundOversampling());
	moonsound->ymf278->setSampleRate(rate, boardGetMoonsoundOversampling());
}

Moonsound* moonsoundCreate(Mixer* mixer, void* romData, int romSize, int sramSize)
{
    Moonsound* moonsound = new Moonsound;
    UInt32 systemTime = boardSystemTime();

    moonsound->mixer = mixer;
    moonsound->timerStarted1 = 0;
    moonsound->timerStarted2 = 0;

    moonsound->timer1 = boardTimerCreate(onTimeout1, moonsound);
    moonsound->timer2 = boardTimerCreate(onTimeout2, moonsound);

    moonsound->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_MOONSOUND, 1, moonsoundSync, moonsoundSetSampleRate, moonsound);

    moonsound->ymf262 = new YMF262(0, systemTime, moonsound);
    moonsound->ymf262->setSampleRate(mixerGetSampleRate(mixer), boardGetMoonsoundOversampling());
	moonsound->ymf262->setVolume(32767 * 9 / 10);

    moonsound->ymf278 = new YMF278(0, sramSize, romData, romSize, systemTime);
    moonsound->ymf278->setSampleRate(mixerGetSampleRate(mixer), boardGetMoonsoundOversampling());
    moonsound->ymf278->setVolume(32767 * 9 / 10);

    return moonsound;
}

}
