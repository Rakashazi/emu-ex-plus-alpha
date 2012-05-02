/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Debugger/Debugger.c,v $
**
** $Revision: 1.19 $
**
** $Date: 2009-07-01 05:00:23 $
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

#include "Debugger.h"
#include "DebugDeviceManager.h"
#include "Emulator.h"
#include "Actions.h"
#include "Board.h"
#include <stdlib.h>

struct BlueDebugger {
    DebuggerEvent onEmulatorStart;
    DebuggerEvent onEmulatorStop;
    DebuggerEvent onEmulatorPause;
    DebuggerEvent onEmulatorResume;
    DebuggerEvent onEmulatorReset;
    DebuggerTrace onDebugTrace;
    DebuggerSetBp onDebugSetBp;
    void* ref;
};

#define MAX_DEVICES 64
#define MAX_DEBUGGERS 8

struct DbgSnapshot {
    int count;
    DbgDevice* dbgDevice[MAX_DEVICES];
};

static BlueDebugger* debuggerList[MAX_DEBUGGERS];
static DbgState  dbgState = DBG_STOPPED;
static int debuggerVramAccessEnable = 0;

static void onDefault(void* ref) {
}

static void onDefTrace(void* ref, const char* dummy) {
}

static void onDefSetBp(void* ref, UInt16 d1, UInt16 d2, UInt16 d3) {
}

BlueDebugger* debuggerCreate(DebuggerEvent onEmulatorStart,
                         DebuggerEvent onEmulatorStop,
                         DebuggerEvent onEmulatorPause,
                         DebuggerEvent onEmulatorResume,
                         DebuggerEvent onEmulatorReset,
                         DebuggerTrace onDebugTrace,
                         DebuggerSetBp onDebugSetBp,
                         void* ref)
{
    BlueDebugger* debugger = malloc(sizeof(BlueDebugger));
    int i;

    debugger->onEmulatorStart  = onEmulatorStart  ? onEmulatorStart  : onDefault;
    debugger->onEmulatorStop   = onEmulatorStop   ? onEmulatorStop   : onDefault;
    debugger->onEmulatorPause  = onEmulatorPause  ? onEmulatorPause  : onDefault;
    debugger->onEmulatorResume = onEmulatorResume ? onEmulatorResume : onDefault;
    debugger->onEmulatorReset  = onEmulatorReset  ? onEmulatorReset  : onDefault;
    debugger->onDebugTrace     = onDebugTrace     ? onDebugTrace     : onDefTrace;
    debugger->onDebugSetBp     = onDebugSetBp     ? onDebugSetBp     : onDefSetBp;
    debugger->ref = ref;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] == NULL) {
            debuggerList[i] = debugger;
            break;
        }
    }

    return debugger;
}


void debuggerDestroy(BlueDebugger* debugger)
{
    int i;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] == debugger) {
            debuggerList[i] = NULL;
            break;
        }
    }

    free(debugger);
}

int debuggerCheckVramAccess(void)
{
    return debuggerVramAccessEnable > 0;
}

void debuggerNotifyEmulatorStart()
{
    int i;
    
    dbgState = DBG_RUNNING;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onEmulatorStart(debuggerList[i]->ref);
        }
    }
}

void debuggerNotifyEmulatorStop()
{
    int i;

    dbgState = DBG_STOPPED;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onEmulatorStop(debuggerList[i]->ref);
        }
    }
}

void debuggerNotifyEmulatorPause()
{
    int i;
    
    dbgState = DBG_PAUSED;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onEmulatorPause(debuggerList[i]->ref);
        }
    }
}

void debuggerNotifyEmulatorResume()
{
    int i;
    
    dbgState = DBG_RUNNING;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onEmulatorResume(debuggerList[i]->ref);
        }
    }
}

void debuggerNotifyEmulatorReset()
{
    int i;
    
    dbgState = DBG_RUNNING;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onEmulatorReset(debuggerList[i]->ref);
        }
    }
}

void debuggerTrace(const char* str)
{
    int i;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onDebugTrace(debuggerList[i]->ref, str);
        }
    }
}

void debuggerSetBreakpoint(UInt16 slot, UInt16 page, UInt16 address)
{
    int i;

    for (i = 0; i < MAX_DEBUGGERS; i++) {
        if (debuggerList[i] != NULL) {
            debuggerList[i]->onDebugSetBp(debuggerList[i]->ref, slot, page, address);
        }
    }
}

DbgSnapshot* dbgSnapshotCreate() 
{
    DbgSnapshot* dbgSnapshot;
    
    if (dbgState != DBG_PAUSED) {
        return NULL;
    }

    dbgSnapshot = malloc(sizeof(DbgSnapshot));

    debugDeviceGetSnapshot(dbgSnapshot->dbgDevice, &dbgSnapshot->count);

    return dbgSnapshot;
}

int dbgDeviceWriteMemory(DbgMemoryBlock* memoryBlock, void* data, int startAddr, int size)
{
    return debugDeviceWriteMemory(memoryBlock, data, startAddr, size);
}

int dbgDeviceWriteRegister(DbgRegisterBank* regBank, int regIndex, UInt32 value)
{
    return debugDeviceWriteRegister(regBank, regIndex, value);
}

int dbgDeviceWriteIoPort(DbgIoPorts* ioPorts, int portIndex, UInt32 value)
{
    return debugDeviceWriteIoPort(ioPorts, portIndex, value);
}

void dbgSnapshotDestroy(DbgSnapshot* dbgSnapshot)
{
    int i;

    for (i = 0; i < dbgSnapshot->count; i++) {
        DbgDevice* dbgDevice = dbgSnapshot->dbgDevice[i];
        int j;
        for (j = 0; j < MAX_DBG_COMPONENTS; j++) {
            if (dbgDevice->memoryBlock[j] != NULL) {
                free(dbgDevice->memoryBlock[j]);
            }
            if (dbgDevice->registerBank[j] != NULL) {
                free(dbgDevice->registerBank[j]);
            }
            if (dbgDevice->ioPorts[j] != NULL) {
                free(dbgDevice->ioPorts[j]);
            }
        }

        free(dbgDevice);
    }
    free(dbgSnapshot);
}

DbgState dbgGetState()
{
    return dbgState;
}

int dbgSnapshotGetDeviceCount(DbgSnapshot* dbgSnapshot)
{
    return dbgSnapshot->count;
}

const DbgDevice* dbgSnapshotGetDevice(DbgSnapshot* dbgSnapshot, int index)
{
    if (index >= dbgSnapshot->count) {
        return NULL;
    }
    return dbgSnapshot->dbgDevice[index];
}

int dbgDeviceGetMemoryBlockCount(DbgDevice* dbgDevice)
{
    return dbgDevice->memoryBlockCount;
}

const DbgMemoryBlock* dbgDeviceGetMemoryBlock(DbgDevice* dbgDevice, int index)
{
    if (index >= dbgDevice->memoryBlockCount) {
        return NULL;
    }
    return dbgDevice->memoryBlock[index];
}

int dbgDeviceGetRegisterBankCount(DbgDevice* dbgDevice)
{
    return dbgDevice->registerBankCount;
}

const DbgRegisterBank* dbgDeviceGetRegisterBank(DbgDevice* dbgDevice, int index)
{
    if (index >= dbgDevice->registerBankCount) {
        return NULL;
    }
    return dbgDevice->registerBank[index];
}

int dbgDeviceGetCallstackCount(DbgDevice* dbgDevice)
{
    return dbgDevice->callstack != NULL ? 1 : 0;
}

const DbgCallstack* dbgDeviceGetCallstack(DbgDevice* dbgDevice, int index)
{
    if (index >= dbgDeviceGetCallstackCount(dbgDevice)) {
        return NULL;
    }
    return dbgDevice->callstack;
}

int dbgDeviceGetIoPortsCount(DbgDevice* dbgDevice)
{
    return dbgDevice->ioPortsCount;
}

const DbgIoPorts* dbgDeviceGetIoPorts(DbgDevice* dbgDevice, int index)
{
    if (index >= dbgDevice->ioPortsCount) {
        return NULL;
    }
    return dbgDevice->ioPorts[index];
}

void dbgRun()
{
    if (emulatorGetState() != EMU_RUNNING) {
        actionEmuTogglePause();
    }
}

void dbgStop()
{
    emulatorStop();
}

void dbgPause()
{
    if (emulatorGetState() == EMU_RUNNING) {
        actionEmuTogglePause();
    }
}

void dbgStep()
{
    if (emulatorGetState() == EMU_PAUSED) {
        actionEmuStep();
    }
}

void dbgSetBreakpoint(UInt16 address)
{
    boardSetBreakpoint(address);
}

void dbgClearBreakpoint(UInt16 address)
{
    boardClearBreakpoint(address);
}

void dbgEnableVramAccessCheck(int enable)
{
    if (enable) {
        debuggerVramAccessEnable++;
    }
    else {
        debuggerVramAccessEnable--;
    }
}    

