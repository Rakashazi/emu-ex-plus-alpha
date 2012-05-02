/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Z80/R800Debug.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-04-18 04:09:54 $
**
** Author: Daniel Vik
**
** Description: Debugger support for an R800 object
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
#include "R800Debug.h"
#include "SlotManager.h"
#include "DebugDeviceManager.h"
#include "Language.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Board.h"


extern void debuggerTrace(const char* str);
extern void archTrap(UInt8 value);

struct R800Debug {
    int debugHandle;
    R800* r800;
};

static R800Debug* dbg;


static void getDebugInfo(R800Debug* dbg, DbgDevice* dbgDevice)
{
    static UInt8 mappedRAM[0x10000];
    DbgRegisterBank* regBank;
    int freqAdjust;
    int i;

    for (i = 0; i < 0x10000; i++) {
        mappedRAM[i] = slotPeek(NULL, i);
    }

    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemVisible(), 0, 0, 0x10000, mappedRAM);

#ifdef ENABLE_CALLSTACK
    if (dbg->r800->callstackSize > 255) {
        static UInt16 callstack[0x100];
        int beginning = dbg->r800->callstackSize & 0xff;
        int reminder = 256 - beginning;
        memcpy(callstack, dbg->r800->callstack + beginning, reminder * sizeof(UInt16));
        memcpy(callstack + reminder, dbg->r800->callstack, beginning * sizeof(UInt16));
        dbgDeviceAddCallstack(dbgDevice, langDbgCallstack(), callstack, 256);
    }
    else {
        dbgDeviceAddCallstack(dbgDevice, langDbgCallstack(), dbg->r800->callstack, dbg->r800->callstackSize);
    }
#endif

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegsCpu(), 20);

    dbgRegisterBankAddRegister(regBank,  0, "AF",  16, dbg->r800->regs.AF.W);
    dbgRegisterBankAddRegister(regBank,  1, "BC",  16, dbg->r800->regs.BC.W);
    dbgRegisterBankAddRegister(regBank,  2, "DE",  16, dbg->r800->regs.DE.W);
    dbgRegisterBankAddRegister(regBank,  3, "HL",  16, dbg->r800->regs.HL.W);
    dbgRegisterBankAddRegister(regBank,  4, "AF1", 16, dbg->r800->regs.AF1.W);
    dbgRegisterBankAddRegister(regBank,  5, "BC1", 16, dbg->r800->regs.BC1.W);
    dbgRegisterBankAddRegister(regBank,  6, "DE1", 16, dbg->r800->regs.DE1.W);
    dbgRegisterBankAddRegister(regBank,  7, "HL1", 16, dbg->r800->regs.HL1.W);
    dbgRegisterBankAddRegister(regBank,  8, "IX",  16, dbg->r800->regs.IX.W);
    dbgRegisterBankAddRegister(regBank,  9, "IY",  16, dbg->r800->regs.IY.W);
    dbgRegisterBankAddRegister(regBank, 10, "SP",  16, dbg->r800->regs.SP.W);
    dbgRegisterBankAddRegister(regBank, 11, "PC",  16, dbg->r800->regs.PC.W);
    dbgRegisterBankAddRegister(regBank, 12, "I",   8,  dbg->r800->regs.I);
    dbgRegisterBankAddRegister(regBank, 13, "R",   8,  dbg->r800->regs.R);
    dbgRegisterBankAddRegister(regBank, 14, "IM",  8,  dbg->r800->regs.im);
    dbgRegisterBankAddRegister(regBank, 15, "IFF1",8,  dbg->r800->regs.iff1);
    dbgRegisterBankAddRegister(regBank, 16, "IFF2",8,  dbg->r800->regs.iff2);
    
    switch (dbg->r800->cpuMode) {
    default:
    case CPU_Z80:
        freqAdjust = R800_MASTER_FREQUENCY / (dbg->r800->frequencyZ80 - 1);
        break;
    case CPU_R800:
        freqAdjust = R800_MASTER_FREQUENCY / (dbg->r800->frequencyR800 - 1);
        break;
    }

    dbgRegisterBankAddRegister(regBank, 17, "CLKH",16,  (UInt16)(dbg->r800->systemTime / freqAdjust / 0x10000));
    dbgRegisterBankAddRegister(regBank, 18, "CLKL",16,  (UInt16)(dbg->r800->systemTime / freqAdjust));
    dbgRegisterBankAddRegister(regBank, 19, "CNT",16,  (UInt16)dbg->r800->instCnt);
}


static int dbgWriteMemory(R800Debug* dbg, char* name, void* data, int start, int size)
{
    UInt8* dataBuffer = data;
    int i;
    int rv = 1;

    if (strcmp(name, langDbgMemVisible()) || start + size > 0x10000) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        slotWrite(NULL, start + i, dataBuffer[i]);
        rv &= dataBuffer[i] == slotPeek(NULL, start + i);
    }

    return rv;
}

static int dbgWriteRegister(R800Debug* dbg, char* name, int regIndex, UInt32 value)
{
    switch (regIndex) {
    case  0: dbg->r800->regs.AF.W = (UInt16)value; break;
    case  1: dbg->r800->regs.BC.W = (UInt16)value; break;
    case  2: dbg->r800->regs.DE.W = (UInt16)value; break;
    case  3: dbg->r800->regs.HL.W = (UInt16)value; break;
    case  4: dbg->r800->regs.AF1.W = (UInt16)value; break;
    case  5: dbg->r800->regs.BC1.W = (UInt16)value; break;
    case  6: dbg->r800->regs.DE1.W = (UInt16)value; break;
    case  7: dbg->r800->regs.HL1.W = (UInt16)value; break;
    case  8: dbg->r800->regs.IX.W = (UInt16)value; break;
    case  9: dbg->r800->regs.IY.W = (UInt16)value; break;
    case 10: dbg->r800->regs.SP.W = (UInt16)value; break;
    case 11: dbg->r800->regs.PC.W = (UInt16)value; break;
    case 12: dbg->r800->regs.I = (UInt8)value; break;
    case 13: dbg->r800->regs.R = (UInt8)value; break;
    case 14: dbg->r800->regs.im = value > 2 ? 2 : (UInt8)value; break;
    case 15: dbg->r800->regs.iff1 = value > 2 ? 2 : (UInt8)value; break; 
    case 16: dbg->r800->regs.iff2 = value > 2 ? 2 : (UInt8)value; break; 
    }

    return 1;
}


static void breakpointCb(R800Debug* dbg, UInt16 pc)
{
    boardOnBreakpoint(pc);
}

static void debugCb(R800Debug* dbg, int command, const char* data) 
{
    int slot, page, addr, rv;
    switch (command) {
    case ASDBG_TRACE:
        debuggerTrace(data);
        break;
    case ASDBG_SETBP:
        rv = sscanf(data, "%x %x %x", &slot, &page, &addr);
        if (rv == 3) {
            debuggerSetBreakpoint((UInt16)slot, (UInt16)page, (UInt16)addr);
        }
        break;
    }
}

void trapCb(R800* r800, UInt8 value)
{
    archTrap(value);
}

void r800DebugCreate(R800* r800)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, dbgWriteRegister, NULL };
    
    dbg = (R800Debug*)malloc(sizeof(R800Debug));
    dbg->r800 = r800;
    dbg->debugHandle = debugDeviceRegister(DBGTYPE_CPU, langDbgDevZ80(), &dbgCallbacks, dbg);

    r800->debugCb      = debugCb;
    r800->breakpointCb = breakpointCb;
    r800->trapCb       = trapCb;
}

void r800DebugDestroy()
{   
    debugDeviceUnregister(dbg->debugHandle);
    free(dbg);
}

