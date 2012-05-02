/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/sramMapperEseSCC.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2007-03-28 17:35:35 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
/*
 * 'Ese-SCC' cartride and 'MEGA-SCSI with SCC'(alias WAVE-SCSI) cartrige.
 *
 * Specification:
 *  SRAM(MegaROM) controller: KONAMI_SCC type (KONAMI5)
 *  SRAM capacity           : 128, 256, 512kB and 1024kb(WAVE-SCSI only)
 *  SCSI Protocol controller: Fujitsu MB89352A
 *
 * ESE-SCC sram write control register:
 *  7FFEH, 7FFFH SRAM write control.
 *   bit4       = 1..SRAM writing in 4000H-7FFDH can be done.
 *                   It is given priority more than bank changing.
 *              = 0..SRAM read only
 *   othet bit  = not used
 *
 * WAVE-SCSI bank control register
 *             6bit register (MA13-MA18, B0-B5)
 *  5000-57FF: 4000-5FFF change
 *  7000-77FF: 6000-7FFF change
 *  9000-97FF: 8000-AFFF change
 *  B000-B7FF: B000-CFFF change
 *
 *  7FFE,7FFF: 2bit register
 *       bit4: bank control MA20 (B7)
 *       bit6: bank control MA19 (B6)
 *  other bit: not used
 *
 * WAVE-SCSI bank map:
 *  00-3F: SRAM read only. mirror of 80-BF.
 *     3F: SCC
 *  40-7F: SPC (MB89352A)
 *  80-FF: SRAM read and write
 *
 * SPC BANK address map:
 * 4000-4FFF spc data register (mirror of 5FFA)
 * 5000-5FEF undefined specification
 * 5FF0-5FFE spc register
 *      5FFF un mapped
 *
 * WAVE-SCSI bank access map:
 *           00-3F 80-BF C0-FF SPC  SCC
 * 4000-5FFF   o     o     o    o    x
 * 6000-7FFF   o     o     x    x    x
 * 8000-9FFF   o     x     x    x    o
 * A000-BFFF   o     x     x    x    x
 *
 */
#include "sramMapperEseSCC.h"
#include "MegaSCSIsub.h"
#include "MB89352.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SCC.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"
#include <stdlib.h>
#include <string.h>

//#define ESESCCDEBUG "esescclog.txt"

#define Offset      0x4000

#ifdef ESESCCDEBUG
#include <stdio.h>
static FILE* logFd = NULL;
static int logNo = 0;
#define DBGLOG(fmt) fprintf(logFd, fmt)
#define DBGLOG1(fmt, arg1) fprintf(logFd, fmt, arg1)
#define DBGLOG2(fmt, arg1, arg2) fprintf(logFd, fmt, arg1, arg2)
#define DBGLOG3(fmt, arg1, arg2, arg3) fprintf(logFd, fmt, arg1, arg2, arg3)
#define DBGLOG4(fmt, arg1, arg2, arg3, arg4) fprintf(logFd, fmt, arg1, arg2, arg3, arg4)
#else
#define DBGLOG(fmt)
#define DBGLOG1(fmt, arg1)
#define DBGLOG2(fmt, arg1, arg2)
#define DBGLOG3(fmt, arg1, arg2, arg3)
#define DBGLOG4(fmt, arg1, arg2, arg3, arg4)
#endif

typedef struct {
    int deviceHandle;
    int pSlot;
    int sSlot;
    int startPage;
    MB89352* spc;
    int mapper[4];
    int mapperHigh;         // MA19, MA20 save value
    int spcEnable;
    int sccEnable;
    int preChange;
    int writeEnable;
    UInt32 mapperMask;
    int type;
    int isZip;
    int isAutoName;
    int element;
    SCC* scc;
    int sramSize;
    UInt8* sramData;
    char sramFilename[512];
} SramMapperEseSCC;

// Counter for two or more Ese-SCC, WAVE-SCSI
// It uses it to decide the file name of SRAM. 128, 256, 512, 1024kb
static int autoNameCounter[2][4] = { { 0, 0, 0, 0}, { 0, 0, 0, 0} };
static char wavescsiName[2][9] = {"esescc", "wavescsi"};

static void setMapperLow(SramMapperEseSCC* rm, int page, UInt8 value)
{
    int readMode  = 1;
    int writeMode = 0;
    int change    = 0;

    DBGLOG2("setMapperLow: p%x v%x\n", page, value);

    value &= 0x3f;                  // use 6bit

    if (page == 0) {
        if (rm->spcEnable) {
            rm->mapper[0] = value & rm->mapperMask;
            rm->preChange = 1;
            DBGLOG1("mapper low change (pre): p0 v%x\n", rm->mapper[0]);
            return;
        } else {
            value |= rm->mapperHigh & 0x40;
            if (rm->writeEnable) {
                writeMode = 1;
            }
            if (rm->preChange) {
                rm->preChange = 0;
                change = 1;
            }
        }
    } else {
        if (page == 2) {
            int newEnable = (value == 0x3f);
            if (rm->sccEnable != newEnable) {
                rm->sccEnable = newEnable;
                change = 1;
            }
            if (newEnable) {
                readMode = 0;
            }
        }
    }

    value &= rm->mapperMask;

    if ((value != rm->mapper[page]) || change)  {
        rm->mapper[page] = value;
        slotMapPage(rm->pSlot, rm->sSlot, rm->startPage + page,
                    rm->sramData + value * 0x2000, readMode, writeMode);
        DBGLOG4("mapper low change: p%x v%x r%x w%x\n", page, value, readMode, writeMode);
    } else {
        DBGLOG2("mapper low not change: p%x v%x\n", page, value);
    }
}

static void setMapperHigh(SramMapperEseSCC* rm, UInt8 value)
{
    int newValue;

    DBGLOG1("setMapperHigh: v%x\n", value);

    if (rm->type) {
        newValue = ((value & 0x10) << 3) | (value & 0x40);
    } else {
        newValue =  (value & 0x10) << 3;
    }

    if (newValue != rm->mapperHigh) {
        int mapperLow0;

        rm->writeEnable = (newValue & 0x80) ? 1 : 0;
        rm->spcEnable   = (newValue == 0x40);
        rm->mapperHigh  = newValue;
        mapperLow0  = rm->mapper[0] & 0x3f;

        if (rm->spcEnable) {
            rm->mapper[0] = mapperLow0;
            rm->preChange = 1;
            slotMapPage(rm->pSlot, rm->sSlot, rm->startPage, NULL, 0, 0);
            DBGLOG1("mapper high change: p0 SPC (hidden %x) r0 w0\n", rm->mapper[0]);
        } else {
            int B6 = newValue & 0x40;
            rm->preChange = 0;
            rm->mapper[0] = (mapperLow0 | B6) & rm->mapperMask;
            slotMapPage(rm->pSlot, rm->sSlot, rm->startPage,
                        rm->sramData + rm->mapper[0] * 0x2000,
                        1, rm->writeEnable);
            DBGLOG2("mapper high change: p0 v%x r1 w%d\n", rm->mapper[0], rm->writeEnable);
        }
    } else {
        DBGLOG("mapper high not change\n");
    }
}

static void reset(SramMapperEseSCC* rm)
{
    int i;

    DBGLOG("Reset\n");

    setMapperHigh(rm, 0);

    for (i = 0; i < 4; ++i) {
        setMapperLow(rm, i, i);
    }

    sccReset(rm->scc);
    if (rm->type) mb89352Reset(rm->spc, 1);

#ifdef ESESCCDEBUG
    fflush(logFd);
#endif
}

static void saveState(SramMapperEseSCC* rm)
{
    char tag[16];
    int i;
    SaveState* state = saveStateOpenForWrite("mapperEseSCC");

    saveStateSetBuffer(state, "sramData", rm->sramData, rm->sramSize);
    saveStateSet(state, "sccEnable"  , rm->sccEnable);
    saveStateSet(state, "spcEnable"  , rm->spcEnable);
    saveStateSet(state, "writeEnable", rm->writeEnable);
    saveStateSet(state, "preChange"  , rm->preChange);
    saveStateSet(state, "mapperHigh" , rm->mapperHigh);

    for (i = 0; i < 4; i++) {
        sprintf(tag, "mapper%d", i);
        saveStateSet(state, tag, rm->mapper[i]);
    }
    saveStateClose(state);

    sccSaveState(rm->scc);
    if (rm->type) mb89352SaveState(rm->spc);
}

static void loadState(SramMapperEseSCC* rm)
{
    SaveState* state = saveStateOpenForRead("mapperEseSCC");
    char tag[16];
    int i;

    DBGLOG("load State\n");

    saveStateGetBuffer(state, "sramData", rm->sramData, rm->sramSize);
    rm->sccEnable   = saveStateGet(state, "sccEnable"  , 0);
    rm->spcEnable   = saveStateGet(state, "spcEnable"  , 0);
    rm->writeEnable = saveStateGet(state, "writeEnable", 0);
    rm->preChange   = saveStateGet(state, "preChange"  , 0);

    setMapperHigh(rm, (UInt8)saveStateGet(state, "mapperHigh", 0));

    for (i = 0; i < 4; i++) {
        sprintf(tag, "mapper%d", i);
        setMapperLow(rm, i, (UInt8)saveStateGet(state, tag, 0));
    }
    saveStateClose(state);

    sccLoadState(rm->scc);
    if (rm->type) mb89352LoadState(rm->spc);
}

static void destroy(SramMapperEseSCC* rm)
{
    if (!rm->isZip) sramSave(rm->sramFilename, rm->sramData, rm->sramSize, NULL, 0);
    if (rm->isAutoName) --autoNameCounter[rm->type][rm->element];

    slotUnregister(rm->pSlot, rm->sSlot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

#ifdef ESESCCDEBUG
    --logNo;
    DBGLOG1("%d close\n", logNo);
    if (!logNo) {
        fclose(logFd);
        logFd = NULL;
    }
#endif

    // device release
    sccDestroy(rm->scc);
    if (rm->type) mb89352Destroy(rm->spc);

    free(rm->sramData);
    free(rm);
}

static UInt8 read(SramMapperEseSCC* rm, UInt16 address)
{
    int page = (address >> 13);

    // SPC
#if 0
    if (rm->spcEnable && (page == 0)) {
        UInt16 fixadr = address & 0x1fff;
        if (fixadr < 0x1000) {
            return mb89352ReadDREG(rm->spc);
        } else {
            if (fixadr >= 0x1ff0) {
                return mb89352ReadRegister(rm->spc, fixadr & 0x0f);
            }
        }
        DBGLOG("Reading of undefined area: %x", address);
        return 0xff;
    }
#else
    if (rm->spcEnable && (page == 0)) {
        address &= 0x1fff;
        if (address < 0x1000) {
            return mb89352ReadDREG(rm->spc);
        }
        return mb89352ReadRegister(rm->spc, address & 0x0f);
    }
#endif

    // SCC bank
    if (rm->sccEnable) {
        if((address >= (0x9800 - Offset)) && (address < (0xa000 - Offset))) {
            return sccRead(rm->scc, (UInt8)(address & 0xff));
        } else {
            return rm->sramData[rm->mapper[2] * 0x2000 + (address & 0x1fff)];
        }
    }

    DBGLOG1("read error??? %x\n", address + Offset);
    return 0xff;
}

static UInt8 peek(SramMapperEseSCC* rm, UInt16 address)
{
    int page = (address >> 13);

    // SPC peek
    if (rm->spcEnable && (page == 0)) {
        address &= 0x1fff;
        if (address < 0x1000) {
            address = REG_DREG;
        }
        return mb89352PeekRegister(rm->spc, address & 0x0f);
    }

    // SCC peek
    address += 0x4000;
    if ((address >= 0x9800) && (address < 0xa000) && rm->sccEnable) {
        return sccPeek(rm->scc, (UInt8)(address & 0xff));
    }

    // SCC bank. sram peek.
    if ((page == 2) && rm->sccEnable) {
        return rm->sramData[rm->mapper[2] * 0x2000 + (address & 0x1fff)];
    }

    DBGLOG1("peek error???: %x", address);
    return 0xff;
}

static void write(SramMapperEseSCC* rm, UInt16 address, UInt8 value)
{
    int page = (address >> 13);

    // SPC Write
#if 0
    if (rm->spcEnable && (page == 0)) {
        UInt16 fixadr = address & 0x1fff;
        if (fixadr < 0x1000) {
            mb89352WriteDREG(rm->spc, value);
            return;
        } else {
            if (fixadr >= 0x1ff0) {
                mb89352WriteRegister(rm->spc, fixadr & 0x0f, value);
                return;
            }
        }
        DBGLOG("bank change? %x\n", address);
    }
#else
    if (rm->spcEnable && (page == 0)) {
        address &= 0x1fff;
        if (address < 0x1000) {
            mb89352WriteDREG(rm->spc, value);
        } else {
            mb89352WriteRegister(rm->spc, address & 0x0f, value);
        }
        return;
    }
#endif

    address += 0x4000;

    // SCC write
    if (rm->sccEnable && (0x9800 <= address) && (address < 0xa000)) {
        sccWrite(rm->scc, address & 0xff, value);
        return;
    }

    // set mapper high control register
    if ((address | 0x0001) == 0x7FFF) {
        setMapperHigh(rm, value);
        return;
    }

    // SRAM write. processing of 6000-7FFDH
    if (rm->writeEnable && (page < 2)) {
        rm->sramData[rm->mapper[page] * 0x2000 + (address & 0x1FFF)] = value;
        DBGLOG2("p1 write: %x %x\n", address, value);
        return;
    }

    // Bank change
    if (((address & 0x1800) == 0x1000) && ((page >= 2) || (rm->writeEnable == 0))) {
        setMapperLow(rm, page, value);
        return;
    }

    DBGLOG1("can't write: %x\n", address);
}

int sramMapperEseSCCCreate(const char* filename, UInt8* buf, int size, int pSlot, int sSlot, int startPage, int hdId, int mode)
{
    DeviceCallbacks callbacks = { (void*)destroy, (void*)reset, (void*)saveState, (void*)loadState };

    SramMapperEseSCC* rm;
    int i;

    if  (((!(size == 0x100000 && (mode & 1))) && size != 0x80000 &&
             size != 0x40000 && size != 0x20000) || (mode & ~0x81)) {
        return 0;
    }

    rm = malloc(sizeof(SramMapperEseSCC));

    rm->type  = mode & 1;
    rm->isZip = mode & 0x80;

    rm->deviceHandle = deviceManagerRegister(SRAM_ESESCC, &callbacks, rm);
    slotRegister(pSlot, sSlot, startPage, 4, (SlotRead)read, (SlotRead)peek,
                (SlotWrite)write, (SlotEject)destroy, rm);

    rm->pSlot          = pSlot;
    rm->sSlot          = sSlot;
    rm->startPage      = startPage;
    rm->mapperMask     = ((size >> 13) - 1);

    if (strlen(filename)) {
        rm->isAutoName = 0;
    } else {
        rm->element    = EseRamSize(size);
        rm->isAutoName = 1;
    }

#ifdef ESESCCDEBUG
    if (!logNo) {
        logFd = fopen(ESESCCDEBUG, "w");
    }
    DBGLOG2("%s %d: create\n", wavescsiName[rm->type], logNo);
    if (strlen(filename)) {
        DBGLOG2("filename: %s\nsize: %d\n", filename, size);
    }
    DBGLOG1("mapper mask: %x\n", (unsigned int)rm->mapperMask);
    ++logNo;
#endif

    // create sram
    rm->sramSize = size;
    rm->sramData = calloc(1, rm->sramSize);

    if (rm->isAutoName) {
        sprintf(rm->sramFilename, "%s%d%c.rom", wavescsiName[rm->type], size / 1024,
                                   autoNameCounter[rm->type][rm->element] + (int)'A');
        strcpy(rm->sramFilename, sramCreateFilename(rm->sramFilename));
        sramLoad(rm->sramFilename, rm->sramData, rm->sramSize, NULL, 0);
        ++autoNameCounter[rm->type][rm->element];
    } else {
        memcpy(rm->sramData, buf, rm->sramSize);
        strcpy(rm->sramFilename, filename);
    }

    // initialized mapper
    rm->mapperHigh  = 0;
    rm->sccEnable   = 0;
    rm->spcEnable   = 0;
    rm->writeEnable = 0;
    rm->preChange   = 0;

    for (i = 0; i < 4; ++i) {
        rm->mapper[i] = i;
        slotMapPage(rm->pSlot, rm->sSlot, rm->startPage + i,
                    rm->sramData + i * 0x2000, 1, 0);
    }

    // initialize SCC
    rm->scc = sccCreate(boardGetMixer());
    sccSetMode(rm->scc, SCC_REAL);

    // initialize SPC
    rm->spc = rm->type ? mb89352Create(hdId) : NULL;

    return 1;
}
