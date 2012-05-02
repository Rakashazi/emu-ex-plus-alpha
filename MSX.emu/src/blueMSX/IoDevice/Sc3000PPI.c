/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Sc3000PPI.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-06-04 03:48:41 $
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
#include "Sc3000PPI.h"
#include "Sg1000JoyIo.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "IoPort.h"
#include "I8255.h"
#include "Board.h"
#include "SaveState.h"
#include "Language.h"
#include "InputEvent.h"
#include <stdlib.h>


typedef struct {
    int    deviceHandle;
    int    debugHandle;
    I8255* i8255;

    Sg1000JoyIo* joyIo;

    UInt8 row;
} Sc3000PPI;


static UInt16 getKeyState(Sc3000PPI* ppi)
{
    UInt16 keyState = 0;
    switch (ppi->row) {
    case 0:
        keyState = (inputEventGetState(EC_1      ) <<  0) |
                   (inputEventGetState(EC_Q      ) <<  1) |
                   (inputEventGetState(EC_A      ) <<  2) |
                   (inputEventGetState(EC_Z      ) <<  3) |
                   (inputEventGetState(EC_TORIKE ) <<  4) |
                   (inputEventGetState(EC_COMMA  ) <<  5) |
                   (inputEventGetState(EC_K      ) <<  6) |
                   (inputEventGetState(EC_I      ) <<  7) |
                   (inputEventGetState(EC_8      ) <<  8);
        break;

    case 1:
        keyState = (inputEventGetState(EC_2      ) <<  0) |
                   (inputEventGetState(EC_W      ) <<  1) |
                   (inputEventGetState(EC_S      ) <<  2) |
                   (inputEventGetState(EC_X      ) <<  3) |
                   (inputEventGetState(EC_SPACE  ) <<  4) |
                   (inputEventGetState(EC_PERIOD ) <<  5) |
                   (inputEventGetState(EC_L      ) <<  6) |
                   (inputEventGetState(EC_O      ) <<  7) |
                   (inputEventGetState(EC_9      ) <<  8);
        break;

    case 2:
        keyState = (inputEventGetState(EC_3      ) <<  0) |
                   (inputEventGetState(EC_E      ) <<  1) |
                   (inputEventGetState(EC_D      ) <<  2) |
                   (inputEventGetState(EC_C      ) <<  3) |
                   (inputEventGetState(EC_JIKKOU ) <<  4) |
                   (inputEventGetState(EC_DIV    ) <<  5) |
                   (inputEventGetState(EC_SEMICOL) <<  6) |
                   (inputEventGetState(EC_P      ) <<  7) |
                   (inputEventGetState(EC_0      ) <<  8);
        break;

    case 3:
        keyState = (inputEventGetState(EC_4      ) <<  0) |
                   (inputEventGetState(EC_R      ) <<  1) |
                   (inputEventGetState(EC_F      ) <<  2) |
                   (inputEventGetState(EC_V      ) <<  3) |
                   (inputEventGetState(EC_BKSPACE) <<  4) |
                   (inputEventGetState(EC_UNDSCRE) <<  5) |
                   (inputEventGetState(EC_COLON  ) <<  6) |
                   (inputEventGetState(EC_AT     ) <<  7) |
                   (inputEventGetState(EC_NEG    ) <<  8);
        break;

    case 4:
        keyState = (inputEventGetState(EC_5      ) <<  0) |
                   (inputEventGetState(EC_T      ) <<  1) |
                   (inputEventGetState(EC_G      ) <<  2) |
                   (inputEventGetState(EC_B      ) <<  3) |
                   (inputEventGetState(EC_DOWN   ) <<  5) |
                   (inputEventGetState(EC_RBRACK ) <<  6) |
                   (inputEventGetState(EC_LBRACK ) <<  7) |
                   (inputEventGetState(EC_CIRCFLX) <<  8);
        break;

    case 5:
        keyState = (inputEventGetState(EC_6      ) <<  0) |
                   (inputEventGetState(EC_Y      ) <<  1) |
                   (inputEventGetState(EC_H      ) <<  2) |
                   (inputEventGetState(EC_N      ) <<  3) |
                   (inputEventGetState(EC_LEFT   ) <<  5) |
                   (inputEventGetState(EC_RETURN ) <<  6) |
                   (inputEventGetState(EC_BKSLASH) <<  8) |
                   (inputEventGetState(EC_TAB    ) << 11);
        break;

    case 6:
        keyState = (inputEventGetState(EC_7      ) <<  0) |
                   (inputEventGetState(EC_U      ) <<  1) |
                   (inputEventGetState(EC_J      ) <<  2) |
                   (inputEventGetState(EC_M      ) <<  3) |
                   (inputEventGetState(EC_RIGHT  ) <<  5) |
                   (inputEventGetState(EC_UP     ) <<  6) |
                   (inputEventGetState(EC_STOP   ) <<  8) |
                   (inputEventGetState(EC_GRAPH  ) <<  9) |
                   (inputEventGetState(EC_LSHIFT ) << 11) |
                   (inputEventGetState(EC_RSHIFT ) << 11) |
                   (inputEventGetState(EC_CTRL   ) << 10);
        break;
        
    case 7:
        keyState = ~sg1000JoyIoRead(ppi->joyIo);
        break;
    }

    return ~keyState;
}

static void destroy(Sc3000PPI* ppi)
{
    ioPortUnregister(0xdc);
    ioPortUnregister(0xdd);
    ioPortUnregister(0xde);
    ioPortUnregister(0xdf);

    deviceManagerUnregister(ppi->deviceHandle);
    debugDeviceUnregister(ppi->debugHandle);

    i8255Destroy(ppi->i8255);

    free(ppi);
}

static void reset(Sc3000PPI* ppi) 
{
    ppi->row = 0x07;

    i8255Reset(ppi->i8255);
}

static void loadState(Sc3000PPI* ppi)
{
    SaveState* state = saveStateOpenForRead("Sc3000PPI");

    ppi->row = (UInt8)saveStateGet(state, "row", 0);

    saveStateClose(state);
    
    i8255LoadState(ppi->i8255);
}

static void saveState(Sc3000PPI* ppi)
{
    SaveState* state = saveStateOpenForWrite("Sc3000PPI");
    
    saveStateSet(state, "row", ppi->row);

    saveStateClose(state);

    i8255SaveState(ppi->i8255);
}

static void writeCLo(Sc3000PPI* ppi, UInt8 value)
{
    ppi->row = value & 0x07;
}

static UInt8 readA(Sc3000PPI* ppi)
{
    return boardCaptureUInt8(16 + ppi->row * 2, getKeyState(ppi) & 0xff);
}

static UInt8 readB(Sc3000PPI* ppi)
{
    return boardCaptureUInt8(16 + ppi->row * 2 + 1, getKeyState(ppi) >> 8);
}

static UInt8 peekA(Sc3000PPI* ppi)
{
    return getKeyState(ppi) & 0xff;
}

static UInt8 peekB(Sc3000PPI* ppi)
{
    return getKeyState(ppi) >> 8;
}

static void getDebugInfo(Sc3000PPI* ppi, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevPpi(), 5);
    dbgIoPortsAddPort(ioPorts, 0, 0xdc, DBG_IO_READWRITE, i8255Read(ppi->i8255, 0xdc));
    dbgIoPortsAddPort(ioPorts, 1, 0xdd, DBG_IO_READWRITE, i8255Read(ppi->i8255, 0xdd));
    dbgIoPortsAddPort(ioPorts, 2, 0xde, DBG_IO_READWRITE, i8255Read(ppi->i8255, 0xde));
    dbgIoPortsAddPort(ioPorts, 3, 0xdf, DBG_IO_READWRITE, i8255Read(ppi->i8255, 0xdf));
}

void sc3000PPICreate(Sg1000JoyIo* joyIo)
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    Sc3000PPI* ppi = malloc(sizeof(Sc3000PPI));

    ppi->deviceHandle = deviceManagerRegister(RAM_MAPPER, &callbacks, ppi);
    ppi->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevPpi(), &dbgCallbacks, ppi);

    ppi->joyIo = joyIo;

    ppi->i8255 = i8255Create(peekA, readA, NULL,
                             peekB, readB, NULL,
                             NULL,  NULL,  writeCLo,
                             NULL,  NULL,  NULL,
                             ppi);

    ioPortRegister(0xdc, i8255Read, i8255Write, ppi->i8255); // PPI Port A
    ioPortRegister(0xdd, i8255Read, i8255Write, ppi->i8255); // PPI Port B
    ioPortRegister(0xde, i8255Read, i8255Write, ppi->i8255); // PPI Port C
    ioPortRegister(0xdf, i8255Read, i8255Write, ppi->i8255); // PPI Mode

    reset(ppi);
}


