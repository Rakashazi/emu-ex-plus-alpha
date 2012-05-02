/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/IoPort.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-05-25 14:22:39 $
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
#include "IoPort.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct IoPortInfo {
    IoPortRead  read;
    IoPortWrite write;
    void*       ref;
} IoPortInfo;

static IoPortInfo ioTable[256];
static IoPortInfo ioSubTable[256];
static IoPortInfo ioUnused[2];
static int currentSubport;

void ioPortReset()
{
    memset(ioTable, 0, sizeof(ioTable));
    memset(ioSubTable, 0, sizeof(ioSubTable));

    currentSubport = 0;
}

void* ioPortGetRef(int port)
{
	return ioTable[port].ref;
}

void ioPortRegister(int port, IoPortRead read, IoPortWrite write, void* ref)
{
    if (ioTable[port].read  == NULL && 
        ioTable[port].write == NULL && 
        ioTable[port].ref   == NULL)
    {
        ioTable[port].read  = read;
        ioTable[port].write = write;
        ioTable[port].ref   = ref;
    }
}


void ioPortUnregister(int port)
{
    ioTable[port].read  = NULL;
    ioTable[port].write = NULL;
    ioTable[port].ref   = NULL;
}

void ioPortRegisterUnused(int idx, IoPortRead read, IoPortWrite write, void* ref)
{
    ioUnused[idx].read  = read;
    ioUnused[idx].write = write;
    ioUnused[idx].ref   = ref;
}

void ioPortUnregisterUnused(int idx)
{
    ioUnused[idx].read  = NULL;
    ioUnused[idx].write = NULL;
    ioUnused[idx].ref   = NULL;
}

void ioPortRegisterSub(int subport, IoPortRead read, IoPortWrite write, void* ref)
{
    ioSubTable[subport].read  = read;
    ioSubTable[subport].write = write;
    ioSubTable[subport].ref   = ref;
}


void ioPortUnregisterSub(int subport)
{
    ioSubTable[subport].read  = NULL;
    ioSubTable[subport].write = NULL;
    ioSubTable[subport].ref   = NULL;
}

int ioPortCheckSub(int subport)
{
    return currentSubport = subport;
}

UInt8 ioPortRead(void* ref, UInt16 port)
{
    port &= 0xff;

    if (boardGetType() == BOARD_MSX && port >= 0x40 && port < 0x50) {
        if (ioSubTable[currentSubport].read == NULL) {
            return 0xff;
        }

        return ioSubTable[currentSubport].read(ioSubTable[currentSubport].ref, port);
    }

    if (ioTable[port].read == NULL) {
        if (ioUnused[0].read != NULL) {
            return ioUnused[0].read(ioUnused[0].ref, port);
        }
        if (ioUnused[1].read != NULL) {
            return ioUnused[1].read(ioUnused[1].ref, port);
        }
        return 0xff;
    }

    return ioTable[port].read(ioTable[port].ref, port);
}

void  ioPortWrite(void* ref, UInt16 port, UInt8 value)
{
    port &= 0xff;

    if (boardGetType() == BOARD_MSX && port >= 0x40 && port < 0x50) {
        if (port == 0x40) {
            currentSubport = value;
            return;
        }
        
        if (ioSubTable[currentSubport].write != NULL) {
            ioSubTable[currentSubport].write(ioSubTable[currentSubport].ref, port, value);
        }
        return;
    }

    if (ioTable[port].write != NULL) {
        ioTable[port].write(ioTable[port].ref, port, value);
    }
    else if (ioUnused[0].write != NULL) {
        ioUnused[0].write(ioUnused[0].ref, port, value);
    }
    else if (ioUnused[1].write != NULL) {
        ioUnused[1].write(ioUnused[1].ref, port, value);
    }
}


