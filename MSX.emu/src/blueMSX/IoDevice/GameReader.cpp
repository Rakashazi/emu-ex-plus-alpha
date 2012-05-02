/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/GameReader.cpp,v $
**
** $Revision: 1.8 $
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
extern "C" {
#include "GameReader.h"
};

#ifdef _WIN32
#include "msxgr.h"
#else
class CMSXGr
{
	public:
		CMSXGr() {};
		~CMSXGr() {};

        int  Init() { return 0; }

        bool IsSlotEnable(int) { return false; }
		bool IsCartridgeInserted(int) { return false; }

		int  ReadMemory(int,char*,int,int) { return 0; }
		int  WriteMemory(int,char*,int,int) { return 0; }
		int  WriteIO(int,char*,int,int) { return 0; }
		int  ReadIO(int,char*,int,int) { return 0; }
};
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#else
#define GlobalAlloc(xxx, addr) malloc(addr)
#define GlobalFree(addr) free(addr)
#endif


//#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define dprintf printf
#else
#define dprintf if (0) printf
#endif


static CMSXGr* MsxGr;

struct GameReader 
{
public:
    GameReader(int grSlot = -1);
    ~GameReader();

    inline bool readMemory(UInt16 address, void* buffer, int length);
    inline bool writeMemory(UInt16 address, void* buffer, int length);
    
    inline bool readIo(UInt16 port, UInt8* value);
    inline bool writeIo(UInt16 port, UInt8 value);

private:
    bool inserted;
    int  slot;
    char* globalBuffer;
};


GameReader::GameReader(int grSlot) : 
    inserted(false),
    slot(grSlot),
    globalBuffer((char*)GlobalAlloc(GPTR, 0x4000))
{
}

GameReader::~GameReader() {
    GlobalFree(globalBuffer);
}

bool GameReader::readMemory(UInt16 address, void* buffer, int length)
{
    if (slot == -1) {
        return false;
    }
    
    if (!inserted) {
        inserted = MsxGr->IsCartridgeInserted(slot);
    }
    
    if (inserted) {
        //printf("### Reading address %.4x - %.4x\n", address, address + length - 1);
        if (MsxGr->ReadMemory(slot, globalBuffer, address, length) != 0) {
            inserted = MsxGr->IsCartridgeInserted(slot);
            return false;
        }
        memcpy(buffer, globalBuffer, length);
    }
    return true;
}

bool GameReader::writeMemory(UInt16 address, void* buffer, int length)
{
    if (slot == -1) {
        return false;
    }
    
    if (!inserted) {
        inserted = MsxGr->IsCartridgeInserted(slot);
    }
    
    if (inserted) {
        memcpy(globalBuffer, buffer, length);
        //printf("### Writing address %.4x - %.4x\n", address, address + length - 1);
        if (MsxGr->WriteMemory(slot, globalBuffer, address, length) != 0) {
            inserted = MsxGr->IsCartridgeInserted(slot);
            return false;
        }
    }
    return true;
}

bool GameReader::readIo(UInt16 port, UInt8* value)
{
    if (slot == -1) {
        return false;
    }
    
    if (!inserted) {
        inserted = MsxGr->IsCartridgeInserted(slot);
    }

    if (inserted) {
        if (MsxGr->ReadIO(slot, globalBuffer, port, 1) != 0) {
            inserted = MsxGr->IsCartridgeInserted(slot);
            return false;
        }
        *value = *(UInt8*)globalBuffer;
    }
    return true;
}

bool GameReader::writeIo(UInt16 port, UInt8 value)
{
    if (slot == -1) {
        return false;
    }
    
    if (!inserted) {
        inserted = MsxGr->IsCartridgeInserted(slot);
    }

    if (inserted) {
        *(UInt8*)globalBuffer = value;
        if (MsxGr->WriteIO(slot, globalBuffer, port, 1) != 0) {
            inserted = MsxGr->IsCartridgeInserted(slot);
            return false;
        }
    }
    return true;
}

#define MAX_GAMEREADERS 2

static GameReader* GameReaders[MAX_GAMEREADERS] = { NULL, NULL };

static void InitializeGameReaders()
{
    if (MsxGr == NULL) {
        MsxGr = new CMSXGr;

        int gameReaderCount = 0;

        if (MsxGr->Init() == 0) {
            for (int i = 0; i < 16 && gameReaderCount < MAX_GAMEREADERS; i++) {
                if (MsxGr->IsSlotEnable(i)) {
                    GameReaders[gameReaderCount++] = new GameReader(i);
                }
            }
        }

        for (; gameReaderCount < 2; gameReaderCount++) {
            GameReaders[gameReaderCount] = new GameReader;
        }
    }
}

static void DeinitializeGameReaders()
{
    if (MsxGr != NULL) {
        for (int i = 0; i < MAX_GAMEREADERS; i++) {
            if (GameReaders[i] != NULL) {
                delete GameReaders[i];
                GameReaders[i] = NULL;
            }
        }
        delete MsxGr;
        MsxGr = NULL;
    }
}

/////////////////////////////////////////////////////////////
//
// Public C interface

extern "C" GrHandle* gameReaderCreate(int slot)
{
    InitializeGameReaders();

    return (GrHandle*)GameReaders[slot];
}

extern "C" void gameReaderDestroy(GrHandle* grHandle)
{
    DeinitializeGameReaders();
}

extern "C" int gameReaderRead(GrHandle* grHandle, UInt16 address, void* buffer, int length)
{
    return ((GameReader*)grHandle)->readMemory(address, buffer, length) ? 1 : 0;
}

extern "C" int gameReaderWrite(GrHandle* grHandle, UInt16 address, void* buffer, int length)
{
    return ((GameReader*)grHandle)->writeMemory(address, buffer, length) ? 1 : 0;
}

extern "C" int gameReaderReadIo(GrHandle* grHandle, UInt16 port, UInt8* value)
{
    return ((GameReader*)grHandle)->readIo(port, value) ? 1 : 0;
}

extern "C" int gameReaderWriteIo(GrHandle* grHandle, UInt16 port, UInt8 value)
{
    return ((GameReader*)grHandle)->writeIo(port, value) ? 1 : 0;
}

extern "C" int gameReaderSupported()
{
#ifdef WII
    return 0;
#else
    return 1;
#endif
}
