#if 1
/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperGameReader.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:44 $
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
#include "romMapperGameReader.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "GameReader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static FILE* f = NULL;

#define HISTORY_LENGTH 0x40

#define CACHE_LINE_BITS 8
#define CACHE_LINES     (0x10000 >> CACHE_LINE_BITS)
#define CACHE_LINE_SIZE (1 << CACHE_LINE_BITS)

typedef struct {
    int deviceHandle;
    GrHandle* gameReader;
    int slot;
    int sslot;
    int cartSlot;
    int cacheLineEnabled[CACHE_LINES];
    UInt8 cacheLineData[CACHE_LINES][1 << CACHE_LINE_BITS];
} RomMapperGameReader;

static void saveState(RomMapperGameReader* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperGameReader");
    saveStateClose(state);
}

static void loadState(RomMapperGameReader* rm)
{
    SaveState* state = saveStateOpenForRead("mapperGameReader");
    saveStateClose(state);
}


static void destroy(RomMapperGameReader* rm)
{
    if (rm->gameReader != NULL) {
        gameReaderDestroy(rm->gameReader);
        ioPortUnregisterUnused(rm->cartSlot);
        slotUnregister(rm->slot, rm->sslot, 0);
    }
    deviceManagerUnregister(rm->deviceHandle);

//    fclose(f);
    f = NULL;

    free(rm);
}

static UInt8 readIo(RomMapperGameReader* rm, UInt16 port)
{
    UInt8 value = 0xff;

    if ((port & 0xf8) == 0xb8 ||
        (port & 0xf8) == 0xd8 ||
        (port & 0xfc) == 0x80 ||
        (port & 0xf0) == 0xf0)
    {
        return 0xff;
    }

    if (!gameReaderReadIo(rm->gameReader, port, &value)) {
        return 0xff;
    }

    return value;
}

static void writeIo(RomMapperGameReader* rm, UInt16 port, UInt8 value)
{
    if ((port & 0xf8) == 0xb8 ||
        (port & 0xf8) == 0xd8 ||
        (port & 0xfc) == 0x80 ||
        (port & 0xf0) == 0xf0)
    {
        return;
    }
    gameReaderWriteIo(rm->gameReader, port, value);
}

static UInt8 read(RomMapperGameReader* rm, UInt16 address) 
{
    int bank = address >> CACHE_LINE_BITS;
    if (!rm->cacheLineEnabled[bank]) {
        if (!gameReaderRead(rm->gameReader, bank << CACHE_LINE_BITS, rm->cacheLineData[bank], CACHE_LINE_SIZE)) {
            memset(rm->cacheLineData[bank], 0xff, CACHE_LINE_SIZE);
        }
        rm->cacheLineEnabled[bank] = 1;
    }

//    fprintf(f, "R %.4x : 0x%.2x\n", address, rm->cacheLineData[bank][address & (CACHE_LINE_SIZE - 1)]);
    return rm->cacheLineData[bank][address & (CACHE_LINE_SIZE - 1)];
}

//#define WRITE_CHECK
static void write(RomMapperGameReader* rm, UInt16 address, UInt8 value) 
{
#ifdef WRITE_CHECK
    static UInt8 buf1[0x10000];
    static UInt8 buf2[0x10000];
#endif
    int bank = address >> 13;
    int i;

//    fprintf(f, "W %.4x : 0x%.2x\n", address, value);

    for (i = 0; i < CACHE_LINES; i++) {
        rm->cacheLineEnabled[i] = 0;
    }
    
#ifdef WRITE_CHECK
    gameReaderRead(rm->gameReader, 0, buf1, 0x10000);
#endif

    gameReaderWrite(rm->gameReader, address, &value, 1);

#ifdef WRITE_CHECK
    gameReaderRead(rm->gameReader, 0, buf2, 0x10000);
    printf("WRITE TO GR 0x%.4x : 0x%.2x\n", address, value);
    for (i = 0; i < 0x10000; i++) {
        if (buf1[i] != buf2[i]) {
            printf("Diff from 0x%.4x ", i);
            while (i < 0x10000 && buf1[i] != buf2[i]) {
                i++;
            }
            printf("to 0x%.4x\n", i - 1);
        }
    }
    printf("\n");
#endif
}

int romMapperGameReaderCreate(int cartSlot, int slot, int sslot) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperGameReader* rm;
    int i;

    rm = malloc(sizeof(RomMapperGameReader));

    rm->deviceHandle = deviceManagerRegister(ROM_GAMEREADER, &callbacks, rm);

    rm->slot     = slot;
    rm->sslot    = sslot;
    rm->cartSlot = cartSlot;

//    f = fopen("c:\\grlog.txt", "w+");
    rm->gameReader = gameReaderCreate(cartSlot);

    for (i = 0; i < CACHE_LINES; i++) {
        rm->cacheLineEnabled[i] = 0;
    }

    if (rm->gameReader != NULL) {
        ioPortRegisterUnused(cartSlot, readIo, writeIo, rm);
        slotRegister(slot, sslot, 0, 8, read, read, write, destroy, rm);
        for (i = 0; i < 8; i++) {   
            slotMapPage(rm->slot, rm->sslot, i, NULL, 0, 0);
        }
    }

    return 1;
}

#else

/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperGameReader.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:44 $
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
#include "romMapperGameReader.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "IoPort.h"
#include "GameReader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAPPER_ADDRESS_UNKNOWN 0x0100
#define MAPPER_ADDRESS_MAPPER  0x0101
#define MAPPER_ADDRESS_PORT    0x0102

typedef struct {
	UInt16 address;
	UInt8  value;
	UInt16 start;
	UInt16 length;
	UInt8* data;
	void*  next;
} MapperList;

typedef struct {
    int deviceHandle;
    GrHandle* gameReader;
    int slot;
    int sslot;
    int cartSlot;
	MapperList* mapperList;
	UInt16 mapperStatus[0x10000];
	UInt8 romData[0x10000];
} RomMapperGameReader;

static void saveState(RomMapperGameReader* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperGameReader");
    saveStateClose(state);
}

static void loadState(RomMapperGameReader* rm)
{
    SaveState* state = saveStateOpenForRead("mapperGameReader");
    saveStateClose(state);
}


static void destroy(RomMapperGameReader* rm)
{
    if (rm->gameReader != NULL) {
        gameReaderDestroy(rm->gameReader);
        ioPortUnregisterUnused(rm->cartSlot);
        slotUnregister(rm->slot, rm->sslot, 0);
    }
    deviceManagerUnregister(rm->deviceHandle);

	// TODO: clear mapperList!

    free(rm);
}

static UInt8 readIo(RomMapperGameReader* rm, UInt16 port)
{
    UInt8 value = 0xff;

    if ((port & 0xf8) == 0xb8 ||
        (port & 0xf8) == 0xd8 ||
        (port & 0xfc) == 0x80 ||
        (port & 0xf0) == 0xf0)
    {
        return 0xff;
    }

    if (!gameReaderReadIo(rm->gameReader, port, &value)) {
        return 0xff;
    }

    return value;
}

static void writeIo(RomMapperGameReader* rm, UInt16 port, UInt8 value)
{
    if ((port & 0xf8) == 0xb8 ||
        (port & 0xf8) == 0xd8 ||
        (port & 0xfc) == 0x80 ||
        (port & 0xf0) == 0xf0)
    {
        return;
    }
    gameReaderWriteIo(rm->gameReader, port, value);
}

static UInt8 read(RomMapperGameReader* rm, UInt16 address) 
{
	if (address <  0x4000)
		if (!gameReaderRead(rm->gameReader, 0x0000, rm->romData + 0x0000, 0x4000))
			memset(rm->romData + 0x0000, 0xff, 0x4000);
	if (address >= 0xc000)
		if (!gameReaderRead(rm->gameReader, 0xc000, rm->romData + 0xc000, 0x4000))
			memset(rm->romData + 0xc000, 0xff, 0x4000);
	return rm->romData[address];
}


int isWellKnownPort(int address)
{
	if (
//		 (address==0x9000)                    || // scc
		 (address>=0x9800 && address<=0x988f) || // scc
		 (address>=0xb800 && address<=0xb88f) || // scc
		 (address==0xbffe)                    || // scc
		 (address==0xffff)                       // secondary slot mapper
	   )
	 {
		 return 1;
	 }
	 return 0;
}

static void write(RomMapperGameReader* rm, UInt16 address, UInt8 value) 
{
    int i;
	UInt8 newBuffer[0x10000];
	UInt16 start = 0xffff;
	UInt16 length = 0;
	MapperList* mapperList;

	// address has been classified as a port, do nothing
	if (rm->mapperStatus[address] == MAPPER_ADDRESS_PORT) {
		return;
	}

	// this value was already written to address and nothing happend, do nothing again.
	if (rm->mapperStatus[address] == value) {
		return;
	}

	// address has been classified as a mapper, restore cache
	if (rm->mapperStatus[address] == MAPPER_ADDRESS_MAPPER) {
		mapperList = rm->mapperList;
		while (1) {
			if (mapperList->address == address) {
				length = mapperList->length;
				start = mapperList->start;
				if (mapperList->value == value ) {
					memcpy(rm->romData+start, mapperList->data, length);
					return;
				}
			}
			if (mapperList->next == NULL) {
				break;
			} else {
				mapperList = mapperList->next;
			}
		}
		// new block
		mapperList->next = malloc(sizeof(mapperList));
		mapperList = mapperList->next;
		// init entry
		mapperList->address = address;
		mapperList->start = start;
		mapperList->value = value;
		mapperList->length = length;
		mapperList->data = malloc(length);
		mapperList->next = NULL;
		// read data
		gameReaderWrite(rm->gameReader, address, &value, 1);
		gameReaderRead(rm->gameReader, start, mapperList->data, length);
		// copy to romdata
		memcpy(rm->romData+start, mapperList->data, length);
		return;
	}

	// address has an unknown or read-once status, examine.
	gameReaderWrite(rm->gameReader, address, &value, 1);
	gameReaderRead(rm->gameReader, 0x4000, newBuffer + 0x4000, 0x4000);
	gameReaderRead(rm->gameReader, 0x8000, newBuffer + 0x8000, 0x4000);

	// search for changed area
	for(i=0x4000; i<0xc000; i++) {
		if (i == address || isWellKnownPort(i)) {
			continue; // skip write address!
		}
		if (rm->romData[i] != newBuffer[i]) {
			if (start == 0xffff) {
				start = i;
			}
			length = i-start+1;
		}
	}

	// check for port (second write with a different value, but still no change)
	if (!length) {
		if (rm->mapperStatus[address] != value && rm->mapperStatus[address] <= 0xff ) {
			rm->mapperStatus[address] = MAPPER_ADDRESS_PORT;
		} else {
			rm->mapperStatus[address] = value;
		}
		return;
	}
	
	// round length
	for(i=0;i<0x8000;i+=0x1000) {
		if (length<i) {
			length = i;
			break;
		}
	}

	// we found a mapper!
	rm->mapperStatus[address] = MAPPER_ADDRESS_MAPPER;
	mapperList = rm->mapperList;
	if (mapperList == NULL) {
		mapperList = malloc(sizeof(mapperList));
		rm->mapperList = mapperList;
	} else {
		while (mapperList->next != NULL) {
			mapperList = mapperList->next;
		}
		mapperList->next = malloc(sizeof(mapperList));
		mapperList = mapperList->next;
	}
	mapperList->next = NULL;
	mapperList->start = start;
	mapperList->length = length;
	mapperList->address = address;
	mapperList->value = value;
	mapperList->data = (UInt8*)malloc(length);
	memcpy(mapperList->data,newBuffer+start,length);
	memcpy(rm->romData+start,newBuffer+start,length);
	
	return;
}

int romMapperGameReaderCreate(int cartSlot, int slot, int sslot) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperGameReader* rm;
    int i;

    rm = malloc(sizeof(RomMapperGameReader));

    rm->deviceHandle = deviceManagerRegister(ROM_GAMEREADER, &callbacks, rm);

    rm->slot     = slot;
    rm->sslot    = sslot;
    rm->cartSlot = cartSlot;

    rm->gameReader = gameReaderCreate(cartSlot);

	// read initial buffer
	if (!gameReaderRead(rm->gameReader, 0x4000, rm->romData + 0x4000, 0x4000)) {
        memset(rm->romData, 0xff, 0x10000);
    }
	if (!gameReaderRead(rm->gameReader, 0x8000, rm->romData + 0x8000, 0x4000)) {
        memset(rm->romData, 0xff, 0x10000);
    }

    if (rm->gameReader != NULL) {
        ioPortRegisterUnused(cartSlot, readIo, writeIo, rm);
        slotRegister(slot, sslot, 0, 8, read, read, write, destroy, rm);
        for (i = 0; i < 8; i++) {   
            slotMapPage(rm->slot, rm->sslot, i, NULL, 0, 0);
        }
    }

	for(i = 0; i <= 0xffff; i++) {
		// mark scc as known port
		if (isWellKnownPort(i))	{
			rm->mapperStatus[i]= MAPPER_ADDRESS_PORT;
		} else {
			rm->mapperStatus[i]= MAPPER_ADDRESS_UNKNOWN;
		}
	}

	rm->mapperList = NULL;

    return 1;
}
#endif