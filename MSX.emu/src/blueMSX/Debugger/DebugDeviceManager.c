/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Debugger/DebugDeviceManager.c,v $
**
** $Revision: 1.14 $
**
** $Date: 2008-03-30 18:38:39 $
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
#include "DebugDeviceManager.h"
#include <stdlib.h>
#include <string.h>

#define MAX_DEVICES 64

typedef struct {
    int handle;
    DebugCallbacks callbacks;
    void* ref;
    char  name[32];
    DbgDeviceType type;
} DebugDeviceInfo;

typedef struct {
    DebugDeviceInfo di[MAX_DEVICES];
    int count;
    int lastHandle;
} DebugDeviceManager;

static DebugDeviceManager devManager;

void debugDeviceManagerReset() 
{
    devManager.count = 0;
//    devManager.lastHandle = 0;
}

int debugDeviceRegister(DbgDeviceType type, const char* name, DebugCallbacks* callbacks, void* ref)
{
    if (devManager.count >= MAX_DEVICES) {
        return 0;
    }

    devManager.di[devManager.count].handle    = ++devManager.lastHandle;
    devManager.di[devManager.count].callbacks = *callbacks;
    devManager.di[devManager.count].ref       = ref;
    devManager.di[devManager.count].type      = type;

    strcpy(devManager.di[devManager.count].name, name);

    devManager.count++;

    return devManager.lastHandle - 1;
}

void debugDeviceUnregister(int handle)
{
    int i;

    if (devManager.count == 0) {
        return;
    }

    for (i = 0; i < devManager.count; i++) {
        if (devManager.di[i].handle == handle + 1) {
            break;
        }
    }

    if (i == devManager.count) {
        return;
    }

    devManager.count--;
    while (i < devManager.count) {
        devManager.di[i] = devManager.di[i + 1];
        i++;
    }
}

void debugDeviceGetSnapshot(DbgDevice** dbgDeviceList, int* count)
{
    int index = 0;
    int i;

    for (i = 0; i < devManager.count; i++) {
        if (devManager.di[i].handle != 0) {
            dbgDeviceList[index] = calloc(1, sizeof(DbgDevice));
            strcpy(dbgDeviceList[index]->name, devManager.di[i].name);
            dbgDeviceList[index]->type = devManager.di[i].type;
            dbgDeviceList[index]->deviceHandle = devManager.di[i].handle;
            if (devManager.di[i].callbacks.getDebugInfo != NULL) {
                devManager.di[i].callbacks.getDebugInfo(devManager.di[i].ref, dbgDeviceList[index++]);
            }
        }
    }

    *count = index;
}

int debugDeviceWriteMemory(DbgMemoryBlock* memoryBlock, void* data, int startAddr, int size)
{
    int i;

    for (i = 0; i < devManager.count; i++) {
        if (devManager.di[i].handle == memoryBlock->deviceHandle) {
            if (devManager.di[i].callbacks.writeMemory != NULL) {
                return devManager.di[i].callbacks.writeMemory(devManager.di[i].ref, memoryBlock->name, data, startAddr, size);
            }
        }
    }
    return 0;
}

int debugDeviceWriteRegister(DbgRegisterBank* regBank, int regIndex, UInt32 value)
{
    int i;

    for (i = 0; i < devManager.count; i++) {
        if (devManager.di[i].handle == regBank->deviceHandle) {
            if (devManager.di[i].callbacks.writeRegister != NULL) {
                return devManager.di[i].callbacks.writeRegister(devManager.di[i].ref, regBank->name, regIndex, value);
            }
        }
    }
    return 0;
}

int debugDeviceWriteIoPort(DbgIoPorts* ioPorts, int portIndex, UInt32 value)
{
    int i;

    for (i = 0; i < devManager.count; i++) {
        if (devManager.di[i].handle == ioPorts->deviceHandle) {
            if (devManager.di[i].callbacks.writeIoPort != NULL) {
                return devManager.di[i].callbacks.writeIoPort(devManager.di[i].ref, ioPorts->name, portIndex, value);
            }
        }
    }
    return 0;
}

DbgDevice* dbgDeviceCreate(int handle)
{
    DbgDevice* device = calloc(1, sizeof(DbgDevice));

    strcpy(device->name, devManager.di[handle].name);
    device->deviceHandle = devManager.di[handle].handle;

    return device;
}

DbgMemoryBlock* dbgDeviceAddMemoryBlock(DbgDevice* dbgDevice,
                                        const char* name,
                                        int   writeProtected,
                                        UInt32 startAddress,
                                        UInt32 size,
                                        UInt8* memory)
{
    DbgMemoryBlock* mem;
    int i;
    for (i = 0; i < MAX_DBG_COMPONENTS; i++) {
        if (dbgDevice->memoryBlock[i] == NULL) {
            break;
        }
    }

    if (i == MAX_DBG_COMPONENTS) {
        return NULL;
    }

    mem = malloc(sizeof(DbgMemoryBlock) + size);
    strcpy(mem->name, name);
    mem->writeProtected = writeProtected;
    mem->startAddress = startAddress;
    mem->size = size;
    mem->deviceHandle = dbgDevice->deviceHandle;
    memcpy(mem->memory, memory, size);

    dbgDevice->memoryBlock[i] = mem;
    dbgDevice->memoryBlockCount = i + 1;

    return mem;
}

DbgCallstack* dbgDeviceAddCallstack(DbgDevice* dbgDevice,
                                    const char* name,
                                    UInt16* callstack, 
                                    int size)
{
    DbgCallstack* stack;
    int i;

    if (dbgDevice->callstack != NULL) {
        return NULL;
    }
    stack = malloc(sizeof(DbgCallstack) + sizeof(UInt32) * size);
    for (i = 0; i < size; i++) {
        stack->callstack[i] = callstack[i];
    }
    stack->size = size;
    stack->deviceHandle = dbgDevice->deviceHandle;
    strcpy(stack->name, name);

    dbgDevice->callstack = stack;
    return stack;
}


DbgRegisterBank* dbgDeviceAddRegisterBank(DbgDevice* dbgDevice,
                                          const char* name,
                                          UInt32 registerCount)
{
    DbgRegisterBank* regBank;
    int i;
    for (i = 0; i < MAX_DBG_COMPONENTS; i++) {
        if (dbgDevice->registerBank[i] == NULL) {
            break;
        }
    }

    if (i == MAX_DBG_COMPONENTS) {
        return NULL;
    }

    regBank = calloc(1, sizeof(DbgRegisterBank) + registerCount * sizeof(struct DbgRegister));
    strcpy(regBank->name, name);
    regBank->count = registerCount;
    regBank->deviceHandle = dbgDevice->deviceHandle;

    dbgDevice->registerBank[i] = regBank;
    dbgDevice->registerBankCount = i + 1;

    return regBank;
}

void dbgRegisterBankAddRegister(DbgRegisterBank* regBank,
                                int index,
                                const char* name,
                                UInt8 width,
                                UInt32 value)
{
    strcpy(regBank->reg[index].name, name);
    regBank->reg[index].width = width;
    regBank->reg[index].value = value;
}

DbgIoPorts* dbgDeviceAddIoPorts(DbgDevice* dbgDevice,
                                const char* name,
                                UInt32 ioPortsCount)
{
    DbgIoPorts* ioPorts;
    int i;
    for (i = 0; i < MAX_DBG_COMPONENTS; i++) {
        if (dbgDevice->ioPorts[i] == NULL) {
            break;
        }
    }

    if (i == MAX_DBG_COMPONENTS) {
        return NULL;
    }

    ioPorts = calloc(1, sizeof(DbgIoPorts) + ioPortsCount * sizeof(struct DbgIoPort));
    strcpy(ioPorts->name, name);
    ioPorts->count = ioPortsCount;
    ioPorts->deviceHandle = dbgDevice->deviceHandle;

    dbgDevice->ioPorts[i] = ioPorts;
    dbgDevice->ioPortsCount = i + 1;

    return ioPorts;
}

void dbgIoPortsAddPort(DbgIoPorts* ioPorts,
                       int index,
                       UInt16 port,
                       DbgIoPortDirection direction,
                       UInt8 value)
{
    if (index >= 0 && (UInt32)index < ioPorts->count) {
        ioPorts->port[index].port       = port;  
        ioPorts->port[index].direction  = direction;    
        ioPorts->port[index].value      = value;
    }
}

