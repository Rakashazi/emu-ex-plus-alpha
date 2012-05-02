/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Microwire93Cx6.c,v $
**
** $Revision: 1.4 $
**
** $Date: 2008-03-30 18:38:40 $
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
#include "Microwire93Cx6.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"
#include <stdlib.h>
#include <string.h>

// Emulates the Microchip Technology Inc. 93C76/86 eeprom chips

typedef struct Microwire93Cx6
{
    UInt8* romData;
    int    romMask;
    int    modeX8;

    int    phase;
    UInt32 command;
    UInt32 commandIdx;
    UInt32 value;
    UInt32 valueIdx;
    int    programEnable;
    
    int Di;
    int Do;
    int Cs;
    int Clk;

    BoardTimer* timer;
    
    char sramFilename[512];
};

#define PHASE_IDLE                  0
#define PHASE_COMMAND               1
#define PHASE_DATATRANSFER_WRITE    2
#define PHASE_DATATRANSFER_READ     3
#define PHASE_PROGRAMMING           4
#define PHASE_COMMAND_DONE          5


static void test(Microwire93Cx6* rm);


static void romWrite(Microwire93Cx6* rm, UInt32 address, UInt32 value)
{
    if (rm->modeX8) {
        ((UInt8*)(rm->romData))[address & rm->romMask] = (UInt8)value;
    }
    else {
        ((UInt16*)(rm->romData))[address & (rm->romMask / 2)] = (UInt16)value;
    }
}

static UInt32 romRead(Microwire93Cx6* rm, UInt32 address)
{
    if (rm->modeX8) {
        return ((UInt8*)(rm->romData))[address & rm->romMask];
    }
    else {
        return ((UInt16*)(rm->romData))[address & (rm->romMask / 2)];
    }
}

static void onTimer(Microwire93Cx6* rm, UInt32 time)
{
    if (rm->Do == 1) {
        rm->phase = PHASE_IDLE;
    }
    else {
        rm->phase = PHASE_COMMAND_DONE;
    }
}

Microwire93Cx6* microwire93Cx6Create(int size, int mode, void* imgData, int imgSize, const char* sramFilename)
{
    Microwire93Cx6* rm = calloc(1, sizeof(Microwire93Cx6));

    if (sramFilename != NULL) {
        strcpy(rm->sramFilename, sramFilename);
    }

    // Allocate memory
    rm->romMask = (size - 1) & 0x01ff;
    rm->romData = malloc(size);
    memset(rm->romData, 0xff, size);

    // Load rom data if present
    if (imgData != NULL) {
        if (imgSize > size) {
            imgSize = size;
        }
        memcpy(rm->romData, imgData, imgSize);
    }

    // Set mode (8/16 bit)
    rm->modeX8 = mode == 16 ? 0 : 1;

    rm->timer = boardTimerCreate(onTimer, rm);

    microwire93Cx6Reset(rm);

    return rm;
}

void microwire93Cx6Destroy(Microwire93Cx6* rm)
{
    if (rm->sramFilename[0]) {
        sramSave(rm->sramFilename, rm->romData, rm->romMask + 1, NULL, 0);
    }

    boardTimerDestroy(rm->timer);

    free(rm->romData);
    free(rm);
}

void microwire93Cx6Reset(Microwire93Cx6* rm)
{
    rm->Cs = 0;
    rm->Do = 1;
    rm->Di = 0;

    rm->phase = PHASE_IDLE;
    rm->programEnable = 0;
}

void microwire93Cx6SaveState(Microwire93Cx6* rm)
{
    SaveState* state = saveStateOpenForWrite("Microwire93Cx6");

    saveStateSet(state, "phase",        rm->phase);
    saveStateSet(state, "command",      rm->command);
    saveStateSet(state, "commandIdx",   rm->commandIdx);
    saveStateSet(state, "value",        rm->value);
    saveStateSet(state, "valueIdx",     rm->valueIdx);
    saveStateSet(state, "programEnable",rm->programEnable);
    saveStateSet(state, "Di",           rm->Di);
    saveStateSet(state, "Do",           rm->Do);
    saveStateSet(state, "Cs",           rm->Cs);
    saveStateSet(state, "Clk",          rm->Clk);

    saveStateClose(state);
}

void microwire93Cx6LoadState(Microwire93Cx6* rm)
{
    SaveState* state = saveStateOpenForRead("Microwire93Cx6");

    rm->phase           = saveStateGet(state, "phase",        PHASE_IDLE);
    rm->command         = saveStateGet(state, "command",      0);
    rm->commandIdx      = saveStateGet(state, "commandIdx",   0);
    rm->value           = saveStateGet(state, "value",        0);
    rm->valueIdx        = saveStateGet(state, "valueIdx",     0);
    rm->programEnable   = saveStateGet(state, "programEnable",0);
    rm->Di              = saveStateGet(state, "Di",           0);
    rm->Do              = saveStateGet(state, "Do",           1);
    rm->Cs              = saveStateGet(state, "Cs",           0);
    rm->Clk             = saveStateGet(state, "Clk",          0);

    if (rm->phase == PHASE_PROGRAMMING) {
        onTimer(rm, 0);
    }

    saveStateClose(state);
}

void microwire93Cx6SetCs(Microwire93Cx6* rm, int value)
{
    rm->Cs = value ? 1 : 0;

    if (rm->Cs == 0) {
        rm->Do = 1;
        if (rm->phase == PHASE_COMMAND_DONE || rm->phase == PHASE_DATATRANSFER_READ) {
            rm->phase = PHASE_IDLE;
        }
    }
}

void microwire93Cx6SetDi(Microwire93Cx6* rm, int value)
{
    rm->Di = value ? 1 : 0;
}

int microwire93Cx6GetDo(Microwire93Cx6* rm)
{
    return rm->Do;
}

void microwire93Cx6SetClk(Microwire93Cx6* rm, int value)
{
    value = value ? 1 : 0;

    if (rm->Clk == value) {
         // No edge
        return;
    }
    
    rm->Clk = value;
    
    if (rm->Cs == 0 || rm->Clk == 0) {
         // Falling edge or chip not selected
        return;
    }

    switch (rm->phase) {
    case PHASE_IDLE:
        if (rm->Cs && rm->Di) {
            rm->phase = PHASE_COMMAND;
            rm->command = 0;
            rm->commandIdx = 12 + rm->modeX8;
        }
        break;

    case PHASE_COMMAND:
        rm->command |= rm->Di << --rm->commandIdx;
        if (rm->commandIdx != 0) {
            break;
        }

        switch ((rm->command >> (10 + rm->modeX8)) & 0x03) {
        case 0:
            switch ((rm->command >> (8 + rm->modeX8)) & 0x03) {
            case 0:
                // EWDS command
                rm->programEnable = 0;
                rm->phase = PHASE_COMMAND_DONE;
                break;
            case 1:
                // WRAL command
                rm->value = 0;
                rm->valueIdx = rm->modeX8 ? 8 : 16;
                rm->phase = PHASE_DATATRANSFER_WRITE;
                break;
            case 2:
                // ERAL command
                if (rm->programEnable) {
                    memset(rm->romData, 0xff, rm->romMask + 1);
                    rm->Do = 0;
                    boardTimerAdd(rm->timer, boardSystemTime() + boardFrequency() * 8 / 1000);
                    rm->phase = PHASE_PROGRAMMING;
                }
                else {
                    rm->Do = 1;
                    rm->phase = PHASE_COMMAND_DONE;
                }
                break;
            case 3:
                // EWEN command
                rm->programEnable = 1;
                rm->phase = PHASE_COMMAND_DONE;
                break;
            }
            break;
        case 1:
            // WRITE command
            rm->value = 0;
            rm->valueIdx = rm->modeX8 ? 8 : 16;
            rm->phase = PHASE_DATATRANSFER_WRITE;
            break;
        case 2:
            // READ command
            rm->value = romRead(rm, rm->command);
            rm->valueIdx = rm->modeX8 ? 8 : 16;
            rm->phase = PHASE_DATATRANSFER_READ;
            break;
        case 3:
            // ERASE command
            if (rm->programEnable) {
                romWrite(rm, rm->command, 0xffff);
                rm->Do = 0;
                boardTimerAdd(rm->timer, boardSystemTime() + boardFrequency() * 3 / 1000);
                rm->phase = PHASE_PROGRAMMING;
            }
            else {
                rm->Do = 1;
                rm->phase = PHASE_COMMAND_DONE;
            }
            break;
        }
        break;

    case PHASE_DATATRANSFER_READ:
    case PHASE_DATATRANSFER_WRITE:
        switch ((rm->command >> (10 + rm->modeX8)) & 0x03) {
        case 0:
            if (((rm->command >> (8 + rm->modeX8)) & 0x03) == 1) {
                // WRAL command
                rm->value |= rm->Di << --rm->valueIdx;
                
                if (rm->valueIdx == 0) {
                    if (rm->programEnable) {
                        int i;
                        for (i = 0; i <= rm->romMask; i++) {
                            romWrite(rm, i, rm->value);
                        }
                        rm->Do = 0;
                        boardTimerAdd(rm->timer, boardSystemTime() + boardFrequency() * 16 / 1000);
                        rm->phase = PHASE_PROGRAMMING;
                    }
                    else {
                        rm->Do = 1;
                        rm->phase = PHASE_COMMAND_DONE;
                    }
                }
            }
            break;
        case 1:
            // WRITE command
            rm->value |= rm->Di << --rm->valueIdx;
            
            if (rm->valueIdx == 0) {
                if (rm->programEnable) {
                    romWrite(rm, rm->command, rm->value);
                    rm->Do = 0;
                    boardTimerAdd(rm->timer, boardSystemTime() + boardFrequency() * 3 / 1000);
                    rm->phase = PHASE_PROGRAMMING;
                }
                else {
                    rm->Do = 1;
                    rm->phase = PHASE_COMMAND_DONE;
                }
            }
            break;
        case 2:
            // READ command
            rm->Do = (rm->value >> --rm->valueIdx) & 1;

            if (rm->valueIdx == 0) {
                rm->command = (rm->command & 0xfe00) | ((rm->command + 1) & 0x01ff);

                rm->value    = romRead(rm, rm->command);
                rm->valueIdx = rm->modeX8 ? 8 : 16;
            }
            break;
        }
        break;
    }
}








#if 0

/////////////////////////////////////////////////////////
//
// Test code
//
/////////////////////////////////////////////////////////




static void testEWEN(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x03 << 9);
    UInt16 m;

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    
    microwire93Cx6SetCs(rm, 0); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
}

static void testEWDS(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x00 << 9);
    UInt16 m;

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    
    microwire93Cx6SetCs(rm, 0); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
}

static void testWRITE(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x04 << 9) | 0x0010; // Write at address 10;
    UInt16 val = 0xa5;
    UInt16 m;

    // Set write mode
    testEWEN(rm);

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }

    // Write data
    for (m = 1 << 7; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, val & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
}

static void testWRAL(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x01 << 9);
    UInt16 val = 0x39;
    UInt16 m;

    // Set write mode
    testEWEN(rm);

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }

    // Write data
    for (m = 1 << 7; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, val & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
}

static void testERASE(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x0c << 9) | 0x0010; // Write at address 10;
    UInt16 m;

    // Set write mode
    testEWEN(rm);

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    rm->romData[0x10] = 44;

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    
    printf("Erase %.2x\n", rm->romData[0x10]);
}

static void testERAL(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x02 << 9);
    UInt16 m;

    // Set write mode
    testEWEN(rm);

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    memset(rm->romData, 0, 0x100);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    printf("%.2x %.2x %.2x %.2x\n", rm->romData[0], rm->romData[1], rm->romData[2], rm->romData[3]);
}

static void testREAD(Microwire93Cx6* rm)
{
    UInt16 cmd = (0x08 << 9) | 0x0010; // Write at address 10;
    UInt16 val;
    UInt16 m;

    // Set read mode
    testEWDS(rm);

    // Start Command Engine
    microwire93Cx6SetCs(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    microwire93Cx6SetDi(rm, 1); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);

    // Write command
    for (m = 1 << 12; m > 0; m /= 2) {
        microwire93Cx6SetDi(rm, cmd & m); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }

    rm->romData[0x10] = 0x5a;
    rm->romData[0x11] = 0x4b;
    rm->romData[0x12] = 0x3c;

    // Read data
    val = 0;
    for (m = 1 << 7; m > 0; m /= 2) {
        val |= m * microwire93Cx6GetDo(rm); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    printf("Read 0x%.2x,  Expected 0x%.2x\n", val, rm->romData[0x10]);

    val = 0;
    for (m = 1 << 7; m > 0; m /= 2) {
        val |= m * microwire93Cx6GetDo(rm); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    printf("Read 0x%.2x,  Expected 0x%.2x\n", val, rm->romData[0x11]);

    val = 0;
    for (m = 1 << 7; m > 0; m /= 2) {
        val |= m * microwire93Cx6GetDo(rm); microwire93Cx6SetClk(rm, 1); microwire93Cx6SetClk(rm, 0);
    }
    printf("Read 0x%.2x,  Expected 0x%.2x\n", val, rm->romData[0x12]);
}

static void test(Microwire93Cx6* rm)
{
    testERASE(rm);
}

#endif
