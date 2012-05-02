/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Debugger/DebugDeviceManager.h,v $
**
** $Revision: 1.11 $
**
** $Date: 2008-03-31 19:42:19 $
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
#ifndef DEBUG_DEVICE_MANAGER_H
#define DEBUG_DEVICE_MANAGER_H

#include "MsxTypes.h"
#include "Debugger.h"

typedef struct {
    void (*getDebugInfo)(void* ref, DbgDevice* dbgDevice);
    int (*writeMemory)(void* ref, char* name, void* data, int start, int size);
    int (*writeRegister)(void* ref, char* name, int reg, UInt32 value);
    int (*writeIoPort)(void* ref, char* name, UInt16 port, UInt32 value);
} DebugCallbacks;

void debugDeviceManagerReset();

int debugDeviceRegister(DbgDeviceType type, const char* name, DebugCallbacks* callbacks, void* ref);
void debugDeviceUnregister(int handle);

DbgMemoryBlock* dbgDeviceAddMemoryBlock(DbgDevice* dbgDevice,
                                        const char* name,
                                        int   writeProtected,
                                        UInt32 startAddress,
                                        UInt32 size,
                                        UInt8* memory);

DbgCallstack* dbgDeviceAddCallstack(DbgDevice* dbgDevice,
                                    const char* name,
                                    UInt16* callstack, 
                                    int size);

DbgRegisterBank* dbgDeviceAddRegisterBank(DbgDevice* dbgDevice,
                                          const char* name,
                                          UInt32 registerCount);
void dbgRegisterBankAddRegister(DbgRegisterBank* regBank,
                                int index,
                                const char* name,
                                UInt8 width,
                                UInt32 value);

typedef enum {
    DBG_IO_NONE      = 0,
    DBG_IO_READ      = 1,
    DBG_IO_WRITE     = 2,
    DBG_IO_READWRITE = 3
} DbgIoPortDirection;

DbgIoPorts* dbgDeviceAddIoPorts(DbgDevice* dbgDevice,
                                const char* name,
                                UInt32 ioPortsCount);
void dbgIoPortsAddPort(DbgIoPorts* ioPorts,
                       int index,
                       UInt16 port,
                       DbgIoPortDirection direction,
                       UInt8 value);

void debugDeviceGetSnapshot(DbgDevice** dbgDeviceList, int* count);

int debugDeviceWriteMemory(DbgMemoryBlock* memoryBlock, void* data, int startAddr, int size);
int debugDeviceWriteRegister(DbgRegisterBank* regBank, int regIndex, UInt32 value);
int debugDeviceWriteIoPort(DbgIoPorts* ioPorts, int portIndex, UInt32 value);

#endif /*DEBUG_DEVICE_MANAGER_H*/
