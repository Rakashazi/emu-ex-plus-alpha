/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/MSXMidi.c,v $
**
** $Revision: 1.11 $
**
** $Date: 2007-03-21 22:27:42 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson, Johan van Leur
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
#include "MSXMidi.h"
#include "MidiIO.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include "IoPort.h"
#include "ArchMidi.h"
#include "Language.h"
#include "I8251.h"
#include "I8254.h"
#include <stdlib.h>



typedef struct {
    int deviceHandle;
    int debugHandle;
    MidiIO* midiIo;
    I8251* i8251;
    I8254* i8254;
    
	int timerIRQlatch;
	int timerIRQenabled;
	int rxrdyIRQlatch;
	int rxrdyIRQenabled;

} MSXMidi;

#define INT_TMR   0x100
#define INT_RXRDY 0x200

/*****************************************
** Device Manager callbacks
******************************************
*/
static void saveState(MSXMidi* msxMidi)
{
    SaveState* state = saveStateOpenForWrite("MSXMidi");
    
    saveStateSet(state, "timerIRQlatch",    msxMidi->timerIRQlatch);
    saveStateSet(state, "timerIRQenabled",  msxMidi->timerIRQenabled);
    saveStateSet(state, "rxrdyIRQlatch",    msxMidi->rxrdyIRQlatch);
    saveStateSet(state, "rxrdyIRQenabled",  msxMidi->rxrdyIRQenabled);

    saveStateClose(state);

    i8251SaveState(msxMidi->i8251);
    i8254SaveState(msxMidi->i8254);

    archMidiSaveState();
}

static void loadState(MSXMidi* msxMidi)
{
    SaveState* state = saveStateOpenForRead("MSXMidi");

    msxMidi->timerIRQlatch   = saveStateGet(state, "timerIRQlatch",    0);
    msxMidi->timerIRQenabled = saveStateGet(state, "timerIRQenabled",  0);
    msxMidi->rxrdyIRQlatch   = saveStateGet(state, "rxrdyIRQlatch",    0);
    msxMidi->rxrdyIRQenabled = saveStateGet(state, "rxrdyIRQenabled",  0);

    saveStateClose(state);
    
    i8251LoadState(msxMidi->i8251);
    i8254LoadState(msxMidi->i8254);
    
    archMidiLoadState();
}

static void destroy(MSXMidi* msxMidi)
{
    ioPortUnregister(0xe8);
    ioPortUnregister(0xe9);
    ioPortUnregister(0xea);
    ioPortUnregister(0xeb);
    ioPortUnregister(0xec);
    ioPortUnregister(0xed);
    ioPortUnregister(0xee);
    ioPortUnregister(0xef);

    midiIoDestroy(msxMidi->midiIo);
    
    i8251Destroy(msxMidi->i8251);
    i8254Destroy(msxMidi->i8254);

    deviceManagerUnregister(msxMidi->deviceHandle);
    debugDeviceUnregister(msxMidi->debugHandle);

    free(msxMidi);
}

static void reset(MSXMidi* msxMidi) 
{
    boardClearInt(INT_TMR);
    boardClearInt(INT_RXRDY);

	msxMidi->timerIRQlatch   = 0;
	msxMidi->timerIRQenabled = 0;
	msxMidi->rxrdyIRQlatch   = 0;
	msxMidi->rxrdyIRQenabled = 0;

    i8251Reset(msxMidi->i8251);
    i8254Reset(msxMidi->i8254);
}


static void setTimerIRQ(MSXMidi* msxMidi, int status)
{
	if (msxMidi->timerIRQlatch != status) {
		msxMidi->timerIRQlatch = status;
		if (msxMidi->timerIRQenabled) {
			if (msxMidi->timerIRQlatch) {
                boardSetInt(INT_TMR);
			} 
            else {
                boardClearInt(INT_TMR);
			}
		}

        i8254SetGate(msxMidi->i8254, 2, msxMidi->timerIRQenabled && !msxMidi->timerIRQlatch);
	}
}

static void enableTimerIRQ(MSXMidi* msxMidi, int enabled)
{
	if (msxMidi->timerIRQenabled != enabled) {
		msxMidi->timerIRQenabled = enabled;
		if (msxMidi->timerIRQlatch) {
			if (msxMidi->timerIRQenabled) {
                boardSetInt(INT_TMR);
			} 
            else {
                boardClearInt(INT_TMR);
			}
		}
        i8254SetGate(msxMidi->i8254, 2, msxMidi->timerIRQenabled && !msxMidi->timerIRQlatch);
	}
}

static void setRxRDYIRQ(MSXMidi* msxMidi, int status)
{
	if (msxMidi->rxrdyIRQlatch != status) {
		msxMidi->rxrdyIRQlatch = status;
		if (msxMidi->rxrdyIRQenabled) {
			if (msxMidi->rxrdyIRQlatch) {
                boardSetInt(INT_RXRDY);
			}
            else {
                boardClearInt(INT_RXRDY);
			}
		}
	}
}

static void enableRxRDYIRQ(MSXMidi* msxMidi, int enabled)
{
	if (msxMidi->rxrdyIRQenabled != enabled) {
		msxMidi->rxrdyIRQenabled = enabled;
		if (!msxMidi->rxrdyIRQenabled && msxMidi->rxrdyIRQlatch) {
            boardClearInt(INT_RXRDY);
		}
	}
}

/*****************************************
** IO Port callbacks
******************************************
*/
static UInt8 peekIo(MSXMidi* msxMidi, UInt16 ioPort) 
{
	switch (ioPort & 7) {
		case 0: // UART data register
		case 1: // UART status register
			return i8251Peek(msxMidi->i8251, ioPort & 3);
			break;
		case 2: // timer interrupt flag off
		case 3: // no function
			return 0xff;
		case 4: // counter 0 data port
		case 5: // counter 1 data port
		case 6: // counter 2 data port
		case 7: // timer command register
			return i8254Peek(msxMidi->i8254, ioPort & 3);
	}
	return 0xff;
}

static UInt8 readIo(MSXMidi* msxMidi, UInt16 ioPort) 
{
	switch (ioPort & 7) {
		case 0: // UART data register
		case 1: // UART status register
			return i8251Read(msxMidi->i8251, ioPort & 3);
			break;
		case 2: // timer interrupt flag off
		case 3: // no function
			return 0xff;
		case 4: // counter 0 data port
		case 5: // counter 1 data port
		case 6: // counter 2 data port
		case 7: // timer command register
			return i8254Read(msxMidi->i8254, ioPort & 3);
	}
	return 0xff;
}

static void writeIo(MSXMidi* msxMidi, UInt16 ioPort, UInt8 value) 
{
	switch (ioPort & 7) {
		case 0: // UART data register
		case 1: // UART command register
            i8251Write(msxMidi->i8251, ioPort & 3, value);
			break;
		case 2: // timer interrupt flag off
			setTimerIRQ(msxMidi, 0);
			break;
		case 3: // no function
			break;
		case 4: // counter 0 data port
		case 5: // counter 1 data port
		case 6: // counter 2 data port
		case 7: // timer command register
            i8254Write(msxMidi->i8254, ioPort & 3, value);
			break;
	}
}


/*****************************************
** I8251 callbacks
******************************************
*/
static int transmit(MSXMidi* msxMidi, UInt8 value) 
{
    midiIoTransmit(msxMidi->midiIo, value);
    return 1;
}

static int signal8251(MSXMidi* msxMidi)
{
    return 0;
}

static void setDataBits(MSXMidi* msxMidi, int value) 
{
}

static void setStopBits(MSXMidi* msxMidi, int value) 
{
}

static void setParity(MSXMidi* msxMidi, int value) 
{
}

static void setRxReady(MSXMidi* msxMidi, int status)
{
    setRxRDYIRQ(msxMidi, status);
}

static void setDtr(MSXMidi* msxMidi, int status) 
{
    enableTimerIRQ(msxMidi, status);
}

static void setRts(MSXMidi* msxMidi, int status)
{
    enableRxRDYIRQ(msxMidi, status);
}

static int getDtr(MSXMidi* msxMidi)
{
    return boardGetInt(INT_TMR) ? 1 : 0;
}

static int getRts(MSXMidi* msxMidi)
{
    return 1;
}




/*****************************************
** I8254 callbacks
******************************************
*/
static void pitOut0(MSXMidi* msxMidi, int state) 
{
}

static void pitOut1(MSXMidi* msxMidi, int state) 
{
}

static void pitOut2(MSXMidi* msxMidi, int state) 
{
    setTimerIRQ(msxMidi, 1);
}

/*****************************************
** MSX MIDI Debug callbacks
******************************************
*/

static void getDebugInfo(MSXMidi* msxMidi, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevMsxMidi(), 8);
    for (i = 0; i < 8; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0xe8 + i, DBG_IO_READWRITE, peekIo(msxMidi, 0xe8 + i));
    }
}

static void midiInCallback(MSXMidi* msxMidi, UInt8* buffer, UInt32 length)
{
    while (length--) {
        i8251RxData(msxMidi->i8251, *buffer++);
    }
}


/*****************************************
** MSX MIDI Create Method
******************************************
*/
int MSXMidiCreate()
{
    DeviceCallbacks callbacks = {destroy, reset, saveState, loadState};
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    MSXMidi* msxMidi;

    msxMidi = malloc(sizeof(MSXMidi));
    
    msxMidi->deviceHandle = deviceManagerRegister(ROM_MSXMIDI, &callbacks, msxMidi);
    msxMidi->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, langDbgDevMsxMidi(), &dbgCallbacks, msxMidi);

    msxMidi->i8254 = i8254Create(4000000, pitOut0, pitOut1, pitOut2, msxMidi);
    msxMidi->i8251 = i8251Create(transmit, signal8251, setDataBits, setStopBits, setParity, 
                                 setRxReady, setDtr, setRts, getDtr, getRts, msxMidi);

    ioPortRegister(0xe8, readIo, writeIo, msxMidi);
    ioPortRegister(0xe9, readIo, writeIo, msxMidi);
    ioPortRegister(0xea, readIo, writeIo, msxMidi);
    ioPortRegister(0xeb, readIo, writeIo, msxMidi);
    ioPortRegister(0xec, readIo, writeIo, msxMidi);
    ioPortRegister(0xed, readIo, writeIo, msxMidi);
    ioPortRegister(0xee, readIo, writeIo, msxMidi);
    ioPortRegister(0xef, readIo, writeIo, msxMidi);

    msxMidi->midiIo = midiIoCreate(midiInCallback, msxMidi);

    reset(msxMidi);

    return 1;
}
