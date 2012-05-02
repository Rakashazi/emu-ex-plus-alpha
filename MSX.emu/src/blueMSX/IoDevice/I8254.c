/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/I8254.c,v $
**
** $Revision: 1.13 $
**
** $Date: 2008-05-19 19:56:58 $
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

#include "I8254.h"
#include "SaveState.h"
#include "Board.h"
#include <stdlib.h>


typedef struct Counter
{
    I8254Out out;
    void* ref;

    BoardTimer* timer; 
    UInt32      time;

    UInt16 countingElement;
    UInt16 outputLatch;
    UInt16 countRegister;
    UInt8  controlWord;
    UInt8  statusLatch;

    int outputLatched;
    int statusLatched;
    int readPhase;
    int writePhase;
    int mode;
    int gate;

    int counterLatched;

    int outputState;

    int outPhase;
    int endOutPhase1;
    int endOutPhase2;

    volatile int insideTimerLoop;

    UInt32 frequency;
    
    UInt32 refTime;
    UInt32 refFrag;
} Counter;


#define PHASE_NONE 0
#define PHASE_LOW  1
#define PHASE_HI   2

static void counterSetOutput(Counter* counter, int state)
{
    if (state != counter->outputState) {
        counter->out(counter->ref, state);
    }
    counter->outputState = state;
}

static UInt16 counterGetElapsedTime(Counter* counter)
{
    UInt64 elapsed;
    UInt32 elapsedTime;
    UInt32 systemTime = boardSystemTime();

    elapsed          = counter->frequency * (UInt64)(systemTime - counter->refTime) + counter->refFrag;
    counter->refTime = systemTime;
    counter->refFrag = (UInt32)(elapsed % boardFrequency());
    elapsedTime      = (UInt32)(elapsed / boardFrequency());

    return (UInt16)elapsedTime;
}

static void counterSetTimeout(Counter* counter)
{
    int nextTimeout = 0;
    int mode = counter->mode;

    // If counter is disabled, just return
    if (mode != 1 && mode != 5 && counter->gate == 0) {
        return;
    }

    if (counter->outPhase == 1) {
        nextTimeout = counter->countingElement - counter->endOutPhase1;
    }
    else if (counter->outPhase == 2) {
        nextTimeout = counter->countingElement - counter->endOutPhase2;
    }
    
    if (nextTimeout != 0) {
        counter->time = boardSystemTime() + 
                      (UInt64)boardFrequency() * nextTimeout / counter->frequency;
        boardTimerAdd(counter->timer, counter->time);
    }
}

static void counterSync(Counter* counter)
{
    UInt16 elapsedTime;
    int mode;

    // If sync is called recursively, return
    if (counter->insideTimerLoop) {
        return;
    }

    elapsedTime = counterGetElapsedTime(counter);
    mode = counter->mode;

    // If timer is disabled, return 
    if (mode != 1 && mode != 5 && counter->gate == 0) {
        return;
    }

    counter->insideTimerLoop = 1;

    while (counter->insideTimerLoop) {
        if (counter->outPhase == 0) {
            counter->countingElement -= elapsedTime;
            break;
        }

        if (counter->outPhase == 1) {
            if (elapsedTime < counter->countingElement - counter->endOutPhase1) {
                counter->countingElement -= elapsedTime;
                counterSetTimeout(counter);
                break;
            }

            if (mode == 0 || mode == 1) {
                counter->outPhase = 0;
                counter->countingElement -= elapsedTime;
                counterSetOutput(counter, 1);
                break;
            }

            elapsedTime -= counter->countingElement - counter->endOutPhase1;
            counter->countingElement = counter->endOutPhase1;
            counter->outPhase = 2;
            counterSetOutput(counter, 0);
            continue;
        }

        if (counter->outPhase == 2) {
            if (elapsedTime < counter->countingElement - counter->endOutPhase2) {
                counter->countingElement -= elapsedTime;
                counterSetTimeout(counter);
                break;
            }
            
            if (mode == 4 || mode == 5) {
                counter->outPhase = 0;
                counter->countingElement -= elapsedTime;
                counterSetOutput(counter, 1);
                break;
            }
            
            elapsedTime -= counter->countingElement - counter->endOutPhase2;
            counter->countingElement = counter->endOutPhase2;
            counter->outPhase = 1;
            counterSetOutput(counter, 1);
            counter->countingElement = counter->countRegister;
            if (mode == 3) {
                counter->endOutPhase1 = (counter->countRegister + 1) / 2;
            }
            continue;
        }
    }

    counter->insideTimerLoop = 0;
}

static void counterOnTimer(Counter* counter, UInt32 time)
{
    counter->time = 0;
    counterSync(counter);
}

static void counterLoad(Counter* counter)
{
    counter->countingElement = counter->countRegister;

    counter->outPhase = 1;

    switch (counter->mode) {
    case 0:
    case 1:
        counter->endOutPhase1 = 0;
        break;
    case 2:
        counter->endOutPhase1 = 2;
        counter->endOutPhase2 = 1;
        break;
    case 3:
        counter->endOutPhase1 = 1 + (counter->countRegister + 1) / 2;
        counter->endOutPhase2 = 1;
        break;
    case 4:
    case 5:
        counter->endOutPhase1 = 1;
        counter->endOutPhase2 = 0;
        break;
    }

    // Force exit from timer loop
    counter->insideTimerLoop = 0;

    counterSetTimeout(counter);
}

static UInt8 counterPeek(Counter* counter)
{
    UInt16 outputLatch;

    if (counter->statusLatched) {
        return counter->statusLatch;
    }

    // Modify output latch if mode = 3.
    outputLatch = counter->outputLatch;
    if (counter->mode == 3) {
        if (outputLatch > counter->countRegister / 2) {
            outputLatch = outputLatch - counter->countRegister / 2;
        }
        outputLatch *= 2;
    }

    switch ((counter->controlWord & 0x30) >> 4) {
    case 0:
        return 0xff;

    case 1:
        return outputLatch & 0xff;
    case 2:
        return outputLatch >> 8;
    case 3:
        if (counter->readPhase == PHASE_LOW) {
            return outputLatch & 0xff;
        }
        return outputLatch >> 8;
    }

    return 0xff;
}

static UInt8 counterRead(Counter* counter)
{
    UInt16 outputLatch;

    counterSync(counter);
    
    if (!counter->outputLatched) {
        counter->outputLatch = counter->countingElement;
    }

    if (counter->statusLatched) {
        counter->statusLatched = 0;
        return counter->statusLatch;
    }

    // Modify output latch if mode = 3.
    outputLatch = counter->outputLatch;
    if (counter->mode == 3) {
        if (outputLatch > counter->countRegister / 2) {
            outputLatch = outputLatch - counter->countRegister / 2;
        }
        outputLatch *= 2;
    }

    switch ((counter->controlWord & 0x30) >> 4) {
    case 0:
        return 0xff;

    case 1:
        counter->outputLatched = 0;
        return outputLatch & 0xff;
    case 2:
        counter->outputLatched = 0;
        return outputLatch >> 8;
    case 3:
        if (counter->readPhase == PHASE_LOW) {
            counter->readPhase = PHASE_HI;
            return outputLatch & 0xff;
        }
        counter->outputLatched = 0;
        counter->readPhase = PHASE_LOW;
        return outputLatch >> 8;
    }

    return 0xff;
}

static void counterWrite(Counter* counter, UInt8 value)
{
    counterSync(counter);

    switch ((counter->controlWord & 0x30) >> 4) {
    case 0:
        return;
    case 1:
        counter->countRegister = (counter->countRegister & 0xff00) | value;
        break;
    case 2:
        counter->countRegister = (counter->countRegister & 0x00ff) | (value << 8);
        break;
    case 3:
        if (counter->writePhase == PHASE_LOW) {
            counter->countRegister = (counter->countRegister & 0xff00) | value;
            counter->writePhase = PHASE_HI;
            if (counter->mode == 0) {
                counter->outPhase = 0;
            }
            return;
        }
        else {
            counter->countRegister = (counter->countRegister & 0x00ff) | (value << 8);
            counter->writePhase = PHASE_LOW;
        }
        break;
    }

    if (counter->mode != 1 && counter->mode != 5) {
        counterLoad(counter);
    }
}

static void counterSetGate(Counter* counter, int state)
{
    counterSync(counter);

    if (counter->gate == state) {
        return;
    }

    counter->gate = state;

    if (counter->mode & 0x02) {
        if (state) {
            counterLoad(counter);
        }
        else {
            counterSetOutput(counter, 1);
        }
    }
    else if (counter->mode & 0x01) {
        if (state) {
            counterLoad(counter);
        }
        if (counter->mode == 1) {
            counterSetOutput(counter, 0);
        }
    }

    if ((counter->mode & 1) == 0 && counter->gate == 1) {
        counter->insideTimerLoop = 0;
        counterSetTimeout(counter);
    }
}

static void counterLatchOutput(Counter* counter)
{
    counterSync(counter);

    counter->readPhase = PHASE_LOW;
    counter->outputLatch = counter->countingElement;
    counter->outputLatched = 1;
}

static void counterLatchStatus(Counter* counter)
{
    counterSync(counter);
    counter->statusLatch = counter->controlWord | (counter->outputState ? 0x80 : 0);
    counter->statusLatched = 1;

}

static void counterSetControl(Counter* counter, UInt8 value)
{
    counterSync(counter);

    counter->controlWord = value;

    if ((value & 0x30) == 0x00) {
        counterLatchOutput(counter);
    }
    else {
        counter->writePhase = PHASE_LOW;
        counter->mode = (value & (value & 0x04 ? 0x06 : 0x0e)) >> 1;
        counterSetOutput(counter, counter->mode == 0 ? 0 : 1);
    }
}

static void counterReset(Counter* counter)
{
    counter->readPhase     = PHASE_LOW;
    counter->writePhase    = PHASE_LOW;
    counter->outputLatched = 0;
    counter->statusLatched = 0;
    counter->controlWord   = 0x30;
}

static Counter* counterCreate(I8254Out out, void* ref, UInt32 frequency) 
{
    Counter* counter = calloc(1, sizeof(Counter));

    counter->frequency = frequency;
    counter->out = out;
    counter->ref = ref;

    counter->timer = boardTimerCreate(counterOnTimer, counter);

    counterReset(counter);

    return counter;
}

static void counterDestroy(Counter* counter)
{
    boardTimerDestroy(counter->timer);
    free(counter);
}

struct I8254
{
    Counter* counter1;
    Counter* counter2;
    Counter* counter3;
};

UInt8 i8254Peek(I8254* i8254, UInt16 port)
{
	switch (port & 3) {
	case 0:
		return counterPeek(i8254->counter1);
	case 1:
		return counterPeek(i8254->counter2);
	case 2:
		return counterPeek(i8254->counter3);
	}
    return 0xff;
}

UInt8 i8254Read(I8254* i8254, UInt16 port)
{
	switch (port & 3) {
	case 0:
		return counterRead(i8254->counter1);
	case 1:
		return counterRead(i8254->counter2);
	case 2:
		return counterRead(i8254->counter3);
	}
    return 0xff;
}

void i8254Write(I8254* i8254, UInt16 port, UInt8 value)
{
	switch (port & 3) {
	case 0:
		counterWrite(i8254->counter1, value);
        break;
	case 1:
		counterWrite(i8254->counter2, value);
        break;
	case 2:
		counterWrite(i8254->counter3, value);
        break;
    case 3:
        if ((value & 0xc0) == 0xc0) {
            if (value & 0x02) {
                if (~value & 0x10) counterLatchOutput(i8254->counter1);
                if (~value & 0x20) counterLatchStatus(i8254->counter1);
            }
            if (value & 0x04) {
                if (~value & 0x10) counterLatchOutput(i8254->counter2);
                if (~value & 0x20) counterLatchStatus(i8254->counter2);
            }
            if (value & 0x08) {
                if (~value & 0x10) counterLatchOutput(i8254->counter3);
                if (~value & 0x20) counterLatchStatus(i8254->counter3);
            }
        }
        else {
            switch (value >> 6) {
            case 0:
    		    counterSetControl(i8254->counter1, value & 0x3f);
                break;
            case 1:
    		    counterSetControl(i8254->counter2, value & 0x3f);
                break;
            case 2:
    		    counterSetControl(i8254->counter3, value & 0x3f);
                break;
            }

        }
        break;
	}
}

void i8254LoadState(I8254* i8254)
{
    SaveState* state = saveStateOpenForRead("i8254");

    i8254->counter1->time               = saveStateGet(state, "c1_time",              0);
    i8254->counter1->countingElement    = (UInt16)saveStateGet(state, "c1_countingElement",   0);
    i8254->counter1->outputLatch        = (UInt16)saveStateGet(state, "c1_outputLatch",       0);
    i8254->counter1->countRegister      = (UInt16)saveStateGet(state, "c1_countRegister",     0);
    i8254->counter1->controlWord        = (UInt8)saveStateGet(state, "c1_controlWord",       0);
    i8254->counter1->statusLatch        = (UInt8)saveStateGet(state, "c1_statusLatch",       0);
    i8254->counter1->outputLatched      = saveStateGet(state, "c1_outputLatched",     0);
    i8254->counter1->statusLatched      = saveStateGet(state, "c1_statusLatched",     0);
    i8254->counter1->readPhase          = saveStateGet(state, "c1_readPhase",         0);
    i8254->counter1->writePhase         = saveStateGet(state, "c1_writePhase",        0);
    i8254->counter1->mode               = saveStateGet(state, "c1_mode",              0);
    i8254->counter1->gate               = saveStateGet(state, "c1_gate",              0);
    i8254->counter1->counterLatched     = saveStateGet(state, "c1_counterLatched",    0);
    i8254->counter1->outputState        = saveStateGet(state, "c1_outputState",       0);
    i8254->counter1->outPhase           = saveStateGet(state, "c1_outPhase",          0);
    i8254->counter1->endOutPhase1       = saveStateGet(state, "c1_endOutPhase1",      0);
    i8254->counter1->endOutPhase2       = saveStateGet(state, "c1_endOutPhase2",      0);
    i8254->counter1->insideTimerLoop    = saveStateGet(state, "c1_insideTimerLoop",   0);
    i8254->counter1->frequency          = saveStateGet(state, "c1_frequency",         0);
    i8254->counter1->refTime            = saveStateGet(state, "c1_refTime",           0);
    i8254->counter1->refFrag            = saveStateGet(state, "c1_refFrag",           0);

    i8254->counter2->time               = saveStateGet(state, "c2_time",              0);
    i8254->counter2->countingElement    = (UInt16)saveStateGet(state, "c2_countingElement",   0);
    i8254->counter2->outputLatch        = (UInt16)saveStateGet(state, "c2_outputLatch",       0);
    i8254->counter2->countRegister      = (UInt16)saveStateGet(state, "c2_countRegister",     0);
    i8254->counter2->controlWord        = (UInt8)saveStateGet(state, "c2_controlWord",       0);
    i8254->counter2->statusLatch        = (UInt8)saveStateGet(state, "c2_statusLatch",       0);
    i8254->counter2->outputLatched      = saveStateGet(state, "c2_outputLatched",     0);
    i8254->counter2->statusLatched      = saveStateGet(state, "c2_statusLatched",     0);
    i8254->counter2->readPhase          = saveStateGet(state, "c2_readPhase",         0);
    i8254->counter2->writePhase         = saveStateGet(state, "c2_writePhase",        0);
    i8254->counter2->mode               = saveStateGet(state, "c2_mode",              0);
    i8254->counter2->gate               = saveStateGet(state, "c2_gate",              0);
    i8254->counter2->counterLatched     = saveStateGet(state, "c2_counterLatched",    0);
    i8254->counter2->outputState        = saveStateGet(state, "c2_outputState",       0);
    i8254->counter2->outPhase           = saveStateGet(state, "c2_outPhase",          0);
    i8254->counter2->endOutPhase1       = saveStateGet(state, "c2_endOutPhase1",      0);
    i8254->counter2->endOutPhase2       = saveStateGet(state, "c2_endOutPhase2",      0);
    i8254->counter2->insideTimerLoop    = saveStateGet(state, "c2_insideTimerLoop",   0);
    i8254->counter2->frequency          = saveStateGet(state, "c2_frequency",         0);
    i8254->counter2->refTime            = saveStateGet(state, "c2_refTime",           0);
    i8254->counter2->refFrag            = saveStateGet(state, "c2_refFrag",           0);

    i8254->counter3->time               = saveStateGet(state, "c3_time",              0);
    i8254->counter3->countingElement    = (UInt16)saveStateGet(state, "c3_countingElement",   0);
    i8254->counter3->outputLatch        = (UInt16)saveStateGet(state, "c3_outputLatch",       0);
    i8254->counter3->countRegister      = (UInt16)saveStateGet(state, "c3_countRegister",     0);
    i8254->counter3->controlWord        = (UInt8)saveStateGet(state, "c3_controlWord",       0);
    i8254->counter3->statusLatch        = (UInt8)saveStateGet(state, "c3_statusLatch",       0);
    i8254->counter3->outputLatched      = saveStateGet(state, "c3_outputLatched",     0);
    i8254->counter3->statusLatched      = saveStateGet(state, "c3_statusLatched",     0);
    i8254->counter3->readPhase          = saveStateGet(state, "c3_readPhase",         0);
    i8254->counter3->writePhase         = saveStateGet(state, "c3_writePhase",        0);
    i8254->counter3->mode               = saveStateGet(state, "c3_mode",              0);
    i8254->counter3->gate               = saveStateGet(state, "c3_gate",              0);
    i8254->counter3->counterLatched     = saveStateGet(state, "c3_counterLatched",    0);
    i8254->counter3->outputState        = saveStateGet(state, "c3_outputState",       0);
    i8254->counter3->outPhase           = saveStateGet(state, "c3_outPhase",          0);
    i8254->counter3->endOutPhase1       = saveStateGet(state, "c3_endOutPhase1",      0);
    i8254->counter3->endOutPhase2       = saveStateGet(state, "c3_endOutPhase2",      0);
    i8254->counter3->insideTimerLoop    = saveStateGet(state, "c3_insideTimerLoop",   0);
    i8254->counter3->frequency          = saveStateGet(state, "c3_frequency",         0);
    i8254->counter3->refTime            = saveStateGet(state, "c3_refTime",           0);
    i8254->counter3->refFrag            = saveStateGet(state, "c3_refFrag",           0);

    if (i8254->counter1->time != 0) {
        boardTimerAdd(i8254->counter1->timer, i8254->counter1->time);
    }
    if (i8254->counter2->time != 0) {
        boardTimerAdd(i8254->counter2->timer, i8254->counter2->time);
    }
    if (i8254->counter3->time != 0) {
        boardTimerAdd(i8254->counter3->timer, i8254->counter3->time);
    }

    saveStateClose(state);
}

void i8254SaveState(I8254* i8254)
{
    SaveState* state = saveStateOpenForWrite("i8254");

    saveStateSet(state, "c1_time",              i8254->counter1->time);
    saveStateSet(state, "c1_countingElement",   i8254->counter1->countingElement);
    saveStateSet(state, "c1_outputLatch",       i8254->counter1->outputLatch);
    saveStateSet(state, "c1_countRegister",     i8254->counter1->countRegister);
    saveStateSet(state, "c1_controlWord",       i8254->counter1->controlWord);
    saveStateSet(state, "c1_statusLatch",       i8254->counter1->statusLatch);
    saveStateSet(state, "c1_outputLatched",     i8254->counter1->outputLatched);
    saveStateSet(state, "c1_statusLatched",     i8254->counter1->statusLatched);
    saveStateSet(state, "c1_readPhase",         i8254->counter1->readPhase);
    saveStateSet(state, "c1_writePhase",        i8254->counter1->writePhase);
    saveStateSet(state, "c1_mode",              i8254->counter1->mode);
    saveStateSet(state, "c1_gate",              i8254->counter1->gate);
    saveStateSet(state, "c1_counterLatched",    i8254->counter1->counterLatched);
    saveStateSet(state, "c1_outputState",       i8254->counter1->outputState);
    saveStateSet(state, "c1_outPhase",          i8254->counter1->outPhase);
    saveStateSet(state, "c1_endOutPhase1",      i8254->counter1->endOutPhase1);
    saveStateSet(state, "c1_endOutPhase2",      i8254->counter1->endOutPhase2);
    saveStateSet(state, "c1_insideTimerLoop",   i8254->counter1->insideTimerLoop);
    saveStateSet(state, "c1_frequency",         i8254->counter1->frequency);
    saveStateSet(state, "c1_refTime",           i8254->counter1->refTime);
    saveStateSet(state, "c1_refFrag",           i8254->counter1->refFrag);

    saveStateSet(state, "c2_time",              i8254->counter2->time);
    saveStateSet(state, "c2_countingElement",   i8254->counter2->countingElement);
    saveStateSet(state, "c2_outputLatch",       i8254->counter2->outputLatch);
    saveStateSet(state, "c2_countRegister",     i8254->counter2->countRegister);
    saveStateSet(state, "c2_controlWord",       i8254->counter2->controlWord);
    saveStateSet(state, "c2_statusLatch",       i8254->counter2->statusLatch);
    saveStateSet(state, "c2_outputLatched",     i8254->counter2->outputLatched);
    saveStateSet(state, "c2_statusLatched",     i8254->counter2->statusLatched);
    saveStateSet(state, "c2_readPhase",         i8254->counter2->readPhase);
    saveStateSet(state, "c2_writePhase",        i8254->counter2->writePhase);
    saveStateSet(state, "c2_mode",              i8254->counter2->mode);
    saveStateSet(state, "c2_gate",              i8254->counter2->gate);
    saveStateSet(state, "c2_counterLatched",    i8254->counter2->counterLatched);
    saveStateSet(state, "c2_outputState",       i8254->counter2->outputState);
    saveStateSet(state, "c2_outPhase",          i8254->counter2->outPhase);
    saveStateSet(state, "c2_endOutPhase1",      i8254->counter2->endOutPhase1);
    saveStateSet(state, "c2_endOutPhase2",      i8254->counter2->endOutPhase2);
    saveStateSet(state, "c2_insideTimerLoop",   i8254->counter2->insideTimerLoop);
    saveStateSet(state, "c2_frequency",         i8254->counter2->frequency);
    saveStateSet(state, "c2_refTime",           i8254->counter2->refTime);
    saveStateSet(state, "c2_refFrag",           i8254->counter2->refFrag);

    saveStateSet(state, "c3_time",              i8254->counter3->time);
    saveStateSet(state, "c3_countingElement",   i8254->counter3->countingElement);
    saveStateSet(state, "c3_outputLatch",       i8254->counter3->outputLatch);
    saveStateSet(state, "c3_countRegister",     i8254->counter3->countRegister);
    saveStateSet(state, "c3_controlWord",       i8254->counter3->controlWord);
    saveStateSet(state, "c3_statusLatch",       i8254->counter3->statusLatch);
    saveStateSet(state, "c3_outputLatched",     i8254->counter3->outputLatched);
    saveStateSet(state, "c3_statusLatched",     i8254->counter3->statusLatched);
    saveStateSet(state, "c3_readPhase",         i8254->counter3->readPhase);
    saveStateSet(state, "c3_writePhase",        i8254->counter3->writePhase);
    saveStateSet(state, "c3_mode",              i8254->counter3->mode);
    saveStateSet(state, "c3_gate",              i8254->counter3->gate);
    saveStateSet(state, "c3_counterLatched",    i8254->counter3->counterLatched);
    saveStateSet(state, "c3_outputState",       i8254->counter3->outputState);
    saveStateSet(state, "c3_outPhase",          i8254->counter3->outPhase);
    saveStateSet(state, "c3_endOutPhase1",      i8254->counter3->endOutPhase1);
    saveStateSet(state, "c3_endOutPhase2",      i8254->counter3->endOutPhase2);
    saveStateSet(state, "c3_insideTimerLoop",   i8254->counter3->insideTimerLoop);
    saveStateSet(state, "c3_frequency",         i8254->counter3->frequency);
    saveStateSet(state, "c3_refTime",           i8254->counter3->refTime);
    saveStateSet(state, "c3_refFrag",           i8254->counter3->refFrag);

    saveStateClose(state);
}

void i8254Reset(I8254* i8254)
{
    counterReset(i8254->counter1);
    counterReset(i8254->counter2);
    counterReset(i8254->counter3);
}

void i8254Destroy(I8254* i8254) 
{
    free(i8254);
}

void i8254SetGate(I8254* i8254, I8254Counter counter, int state)
{
	switch (counter) {
	case I8254_COUNTER_1:
		counterSetGate(i8254->counter1, state);
        break;
	case I8254_COUNTER_2:
		counterSetGate(i8254->counter2, state);
        break;
	case I8254_COUNTER_3:
		counterSetGate(i8254->counter3, state);
        break;
	}
}

UInt32 i8254GetFrequency(I8254* i8254, I8254Counter counter)
{
	switch (counter) {
	case I8254_COUNTER_1:
		return i8254->counter1->frequency;
        break;
	case I8254_COUNTER_2:
		return i8254->counter2->frequency;
        break;
	case I8254_COUNTER_3:
		return i8254->counter3->frequency;
        break;
	}
    return 0;
}

static void outDummy(void* ref, int state) 
{
}

I8254* i8254Create(UInt32 frequency, I8254Out out1, I8254Out out2, I8254Out out3, void* ref)
{
    I8254* i8254 = calloc(1, sizeof(I8254));
    
    i8254->counter1 = counterCreate(out1 ? out1 : outDummy, ref, frequency);
    i8254->counter2 = counterCreate(out2 ? out2 : outDummy, ref, frequency);
    i8254->counter3 = counterCreate(out3 ? out3 : outDummy, ref, frequency);

    return i8254;
}

///////////////////////

I8254* i8254;


static void i8254out1(void* ref, int state) {
    UInt16 cnt;

    i8254Write(i8254, 3, 0xe2);

    cnt = i8254Read(i8254, 0) | (i8254Read(i8254, 0) << 8);

//    printf("Counter 1 = %d  %.4x\n", state, cnt);

    if (state == 0) {
        i8254SetGate(i8254, I8254_COUNTER_3, 1);
        i8254Write(i8254, 3, 0xB0);
        i8254Write(i8254, 2, 0x20);
        i8254Write(i8254, 2, 0x00);
    }
}

static void i8254out2(void* ref, int state) 
{
    UInt16 cnt;

    i8254Write(i8254, 3, 0xe4);

    cnt = i8254Read(i8254, 1) | (i8254Read(i8254, 1) << 8);

//    printf("Counter 2 = %d  %.4x\n", state, cnt);
}

static void i8254out3(void* ref, int state) 
{
    UInt16 cnt1, cnt2, cnt3;

    i8254Write(i8254, 3, 0xee);

    cnt1 = i8254Read(i8254, 0) | (i8254Read(i8254, 0) << 8);
    cnt2 = i8254Read(i8254, 1) | (i8254Read(i8254, 1) << 8);
    cnt3 = i8254Read(i8254, 2) | (i8254Read(i8254, 2) << 8);

//    printf("Counter 3: %d  %.4x  %.4x  %.4x\n", state, cnt1, cnt2, cnt3);
}


void testI8254() {
    i8254 = i8254Create(4000000, i8254out1, i8254out2, i8254out3, 0);

    i8254SetGate(i8254, I8254_COUNTER_1, 1);
    i8254Write(i8254, 3, 0x34);
    i8254Write(i8254, 0, 0x80);
    i8254Write(i8254, 0, 0x02);

    i8254SetGate(i8254, I8254_COUNTER_2, 1);
    i8254Write(i8254, 3, 0x76);
    i8254Write(i8254, 1, 0x80);
    i8254Write(i8254, 1, 0x01);
}

