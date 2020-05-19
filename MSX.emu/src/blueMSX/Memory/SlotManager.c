/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/SlotManager.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:42 $
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
#include "SlotManager.h"
#include "SaveState.h"
#include "Led.h"
#include "IoPort.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    UInt8* pageData;
    int    readEnable;
    int    writeEnable;
} RamSlotState;

typedef struct {
    int   subslotted;
    UInt8 state;
    UInt8 substate;
    UInt8 sslReg;
} PrimarySlotState;


typedef struct {
    UInt16        startpage;
    UInt16        pages;
    UInt8*        pageData;
    int           writeEnable;
    int           readEnable;
    SlotRead      read;
    SlotRead      peek;
    SlotWrite     write;
    SlotEject     eject;
    void*         ref;
} Slot;

static RamSlotState     ramslot[8];
static PrimarySlotState pslot[4];
static Slot             slotTable[4][4][8];
static Slot             slotAddr0;
static UInt8            emptyRAM[0x2000];
static Int32            initialized;

void slotMapRamPage(int slot, int sslot, int page)
{
    ramslot[page].readEnable  = slotTable[slot][sslot][page].readEnable;
    ramslot[page].writeEnable = slotTable[slot][sslot][page].writeEnable;
    ramslot[page].pageData    = slotTable[slot][sslot][page].pageData;
}

void slotSetRamSlot(int slot, int psl)
{
    int ssl;

    pslot[slot].state    = psl;
    pslot[slot].substate = (pslot[psl].sslReg >> (slot * 2)) & 3;

    ssl = pslot[psl].subslotted ? pslot[slot].substate : 0;
    
    slotMapRamPage(psl, ssl, 2 * slot);
    slotMapRamPage(psl, ssl, 2 * slot + 1);
}

int slotGetRamSlot(int page)
{
    int i;
    for (i = 0; i < 4; i++) {
        if (pslot[i].state == page) {
            return i;
        }
    }
    return 0;
}

void slotMapPage(int slot, int sslot, int page, UInt8* pageData, 
                 int readEnable, int writeEnable) 
{
    if (!initialized) {
        return;
    }

    slotTable[slot][sslot][page].readEnable  = readEnable;
    slotTable[slot][sslot][page].writeEnable = writeEnable;

    if (pageData != NULL) {
        slotTable[slot][sslot][page].pageData = pageData;
    }

    if (pslot[page >> 1].state == slot && 
        (!pslot[slot].subslotted || pslot[page >> 1].substate == sslot)) 
    {
        slotMapRamPage(slot, sslot, page);
    }
}

void slotUpdatePage(int slot, int sslot, int page, UInt8* pageData, 
                    int readEnable, int writeEnable) 
{
    if (!initialized) {
        return;
    }

    slotMapPage(slot, sslot, page, pageData, readEnable, writeEnable);
    return;
    slotTable[slot][sslot][page].readEnable  = readEnable;
    slotTable[slot][sslot][page].writeEnable = writeEnable;

    if (pageData != NULL) {
        slotTable[slot][sslot][page].pageData = pageData;
    }
}

void slotUnmapPage(int slot, int sslot, int page)
{
    if (!initialized) {
        return;
    }

    slotTable[slot][sslot][page].readEnable  = 0;
    slotTable[slot][sslot][page].writeEnable = 1;
    slotTable[slot][sslot][page].pageData = emptyRAM;

    if (pslot[page >> 1].state == slot && 
        (!pslot[slot].subslotted || pslot[page >> 1].substate == sslot))  
    {
        slotMapRamPage(slot, sslot, page);
    }
}

static UInt8 dummySlotRead(void* a, UInt16 b)
{
	return 0xff;
}

void slotRegisterWrite0(SlotWrite writeCb, void* ref) 
{
    if (!initialized) {
        return;
    }

    slotAddr0.read  = dummySlotRead;
    slotAddr0.write = writeCb;
    slotAddr0.eject = NULL;
    slotAddr0.ref   = ref;
}

void slotUnregisterWrite0() {
    if (!initialized) {
        return;
    }

    memset(&slotAddr0, 0, sizeof(Slot));
    slotAddr0.read  = dummySlotRead;
}

void slotRegister(int slot, int sslot, int startpage, int pages,
                  SlotRead readCb, SlotRead peekCb, SlotWrite writeCb, SlotEject ejectCb, void* ref)
{
    Slot* slotInfo;
    
    if (!initialized) {
        return;
    }

    slotInfo = &slotTable[slot][sslot][startpage];

    slotInfo->pages = pages;

    while (pages--) {
        slotInfo->startpage = startpage;
        slotInfo->read  = readCb ? readCb : dummySlotRead;
        slotInfo->peek  = peekCb;
        slotInfo->write = writeCb;
        slotInfo->eject = ejectCb;
        slotInfo->ref   = ref;
        slotInfo++;
    }
}


void slotUnregister(int slot, int sslot, int startpage)
{
    Slot* slotInfo;
    int pages;

    if (!initialized) {
        return;
    }

    slotInfo = &slotTable[slot][sslot][startpage];
    pages = slotInfo->pages;

    while (pages--) {
        memset(slotInfo, 0, sizeof(Slot));
        slotInfo->read = dummySlotRead;
        slotUnmapPage(slot, sslot, startpage + pages);
        slotInfo++;
    }
}

void slotRemove(int slot, int sslot)
{
    int page;

    if (!initialized) {
        return;
    }

    for (page = 0; page < 8; page++) {
        Slot* slotInfo = &slotTable[slot][sslot][page];
        if (slotInfo->eject != NULL) {
            slotInfo->eject(slotInfo->ref);
        }
    }
}

void slotSetSubslotted(int slot, int subslotted)
{
    if (!initialized) {
        return;
    }

    pslot[slot].subslotted = subslotted;
}

void slotManagerReset() 
{
    int page;

    if (!initialized) {
        return;
    }

    for (page = 0; page < 4; page++) {
        pslot[page].state = 0;
        pslot[page].substate = 0;

        slotMapRamPage(0, 0, 2 * page);
        slotMapRamPage(0, 0, 2 * page + 1);
    }
}

void slotManagerCreate()
{
    int slot;
    int sslot;
    int page;

    memset(emptyRAM, 0xff, 0x2000);
    memset(ramslot, 0, sizeof(ramslot));
    memset(pslot, 0, sizeof(pslot));
    memset(slotTable, 0, sizeof(slotTable));
    memset(&slotAddr0, 0, sizeof(slotAddr0));
    slotAddr0.read  = dummySlotRead;

    for (slot = 0; slot < 4; slot++) {
        for (sslot = 0; sslot < 4; sslot++) {
            for (page = 0; page < 8; page++) {
                slotUnmapPage(slot, sslot, page);
                slotTable[slot][sslot][page].read = dummySlotRead;
            }
        }
    }

    initialized = 1;
}

void slotManagerDestroy() 
{
    initialized = 0;
}

UInt8 slotPeek(void* ref, UInt16 address)
{
    Slot* slotInfo;
    int psl;
    int ssl;

    if (!initialized) {
        return 0xff;
    }

    if (address == 0xffff) {
        UInt8 sslReg = pslot[3].state;
        if (pslot[sslReg].subslotted) {
            return ~pslot[sslReg].sslReg;
        }
    }

    if (ramslot[address >> 13].readEnable) {
        return ramslot[address >> 13].pageData[address & 0x1fff];
    }

    psl = pslot[address >> 14].state;
    ssl = pslot[psl].subslotted ? pslot[address >> 14].substate : 0;

    slotInfo = &slotTable[psl][ssl][address >> 13];

    if (slotInfo->peek != NULL) {
        address -= slotInfo->startpage << 13;
        return slotInfo->peek(slotInfo->ref, address);
    }

    return 0xff;
}

UInt8 slotRead(void* ref, UInt16 address)
{
    Slot* slotInfo;
    int psl;
    int ssl;

    if (!initialized) {
        return 0xff;
    }

    if (address == 0xffff) {
        UInt8 sslReg = pslot[3].state;
        if (pslot[sslReg].subslotted) {
            return ~pslot[sslReg].sslReg;
        }
    }

    if (ramslot[address >> 13].readEnable) {
        return ramslot[address >> 13].pageData[address & 0x1fff];
    }

    psl = pslot[address >> 14].state;
    ssl = pslot[psl].subslotted ? pslot[address >> 14].substate : 0;

    slotInfo = &slotTable[psl][ssl][address >> 13];

    assert(slotInfo->read);
    address -= slotInfo->startpage << 13;
    return slotInfo->read(slotInfo->ref, address);
}

void slotWrite(void* ref, UInt16 address, UInt8 value)
{
    Slot* slotInfo;
    int psl;
    int ssl;
    int page;

    if (!initialized) {
        return;
    }

    if (address == 0xffff) {
        UInt8 pslReg = pslot[3].state;

        if (pslot[pslReg].subslotted) {
            pslot[pslReg].sslReg = value;

            for (page = 0; page < 4; page++) {
                if(pslot[page].state == pslReg) {
                    pslot[page].substate = value & 3;
                    slotMapRamPage(pslReg, value & 3, 2 * page);
                    slotMapRamPage(pslReg, value & 3, 2 * page + 1);
                }

                value >>= 2;
            }

            return;
        }
    }

    if (address == 0) {
        if (slotAddr0.write != NULL) {
            slotAddr0.write(slotAddr0.ref, address, value);
            return;
        }
    }

    if (ramslot[address >> 13].writeEnable) {
        ramslot[address >> 13].pageData[address & 0x1FFF] = value;
        return;
    }

    psl = pslot[address >> 14].state;
    ssl = pslot[psl].subslotted ? pslot[address >> 14].substate : 0;

    slotInfo = &slotTable[psl][ssl][address >> 13];

    if (slotInfo->write != NULL) {
        address -= slotInfo->startpage << 13;
        slotInfo->write(slotInfo->ref, address, value);
    }
}

void slotSaveState()
{
    SaveState* state;
    char tag[32];
    int i;

    if (!initialized) {
        return;
    }

    state = saveStateOpenForWrite("slotManager");

    for (i = 0; i < 4; i++) {
        sprintf(tag, "subslotted%d", i);
        saveStateSet(state, tag, pslot[i].subslotted);
        
        sprintf(tag, "state%d", i);
        saveStateSet(state, tag, pslot[i].state);
        
        sprintf(tag, "substate%d", i);
        saveStateSet(state, tag, pslot[i].substate);
        
        sprintf(tag, "sslReg%d", i);
        saveStateSet(state, tag, pslot[i].sslReg);
    }

    saveStateClose(state);
}

void slotLoadState()
{
    SaveState* state;
    char tag[32];
    int page;
    int i;

    if (!initialized) {
        return;
    }

    state = saveStateOpenForRead("slotManager");

    for (i = 0; i < 4; i++) {
        sprintf(tag, "subslotted%d", i);
        pslot[i].subslotted = saveStateGet(state, tag, 0);
        
        sprintf(tag, "state%d", i);
        pslot[i].state = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "substate%d", i);
        pslot[i].substate = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "sslReg%d", i);
        pslot[i].sslReg = (UInt8)saveStateGet(state, tag, 0);
    }

    saveStateClose(state);

    for (page = 0; page < 4; page++) {
        int psl = pslot[page].state;
        int ssl = pslot[psl].subslotted ? pslot[page].substate : 0;

        slotMapRamPage(psl, ssl, 2 * page);
        slotMapRamPage(psl, ssl, 2 * page + 1);
    }
}
