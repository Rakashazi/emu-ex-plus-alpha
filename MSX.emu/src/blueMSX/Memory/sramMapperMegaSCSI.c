/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/sramMapperMegaSCSI.c,v $
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
 * MEGA-SCSI and ESE-RAM cartridge:
 *  The mapping does SRAM and MB89352A(MEGA-SCSI) to ASCII8 or
 *  an interchangeable bank controller.
 *
 * Specification:
 *  SRAM(MegaROM) controller: ASCII8 type
 *  SRAM capacity           : 128, 256, 512 and 1024KB
 *  SCSI Protocol Controller: Fujitsu MB89352A
 *
 * Bank changing address:
 *  bank 4(0x4000-0x5fff): 0x6000 - 0x67FF (0x6000 used)
 *  bank 6(0x6000-0x7fff): 0x6800 - 0x6FFF (0x6800 used)
 *  bank 8(0x8000-0xa000): 0x7000 - 0x77FF (0x7000 used)
 *  bank A(0xa000-0xbfff): 0x7800 - 0x7FFF (0x7800 used)
 *
 * ESE-RAM Bank Map:
 *  BANK 00H-7FH (read only)
 *  BANK 80H-FFH (write and read. mirror of 00H-7FH)
 *
 * MEGA-SCSI Bank Map:
 *  BANK 00H-3FH (sram read only. mirror of 80H-BFH)
 *  BANK 40H-7EH (mirror of 7FH. Use is prohibited)
 *  BANK 7FH     (SPC)
 *  BANK 80H-FFH (sram write and read)
 *
 * SPC Bank:
 *  0x0000 - 0x0FFF :
 *      SPC Data register r/w (mirror of all 0x1FFA)
 *  0x1000 - 0x1FEF :
 *      mirror of 0x1FF0 - 0x1FFF
 *      Use is prohibited about the image
 *  0x1FF0 - 0x1FFE :
 *      SPC register
 *  0x1FFF :
 *      un mapped
 *
 * Note:
 *  It is possible to access it by putting it out to 8000H - BFFFH
 *  though the SPC bank is arranged in chiefly 4000H-5FFF.
 *
 */
#include "sramMapperMegaSCSI.h"
#include "MegaSCSIsub.h"
#include "MB89352.h"
#include "DeviceManager.h"
#include "MediaDb.h"
#include "SaveState.h"
#include "SlotManager.h"
#include "sramLoader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define MEGASCSIDEBUG "megascsilog.txt"

#ifdef MEGASCSIDEBUG
static FILE* logFd = NULL;
static int megascsiTotal = 0;
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

#define SPC_BANK 0x7f

typedef struct {
    int deviceHandle;
    int pSlot;
    int sSlot;
    int startPage;
    int mapper[4];          // use 8bit
    UInt32 mapperMask;
    int type;               // 0 = ESE-RAM, 1 = MEGA-SCSI, other = reserve
    int isZip;
    int isAutoName;
    int element;
    MB89352* spc;
    int sramSize;
    UInt8* sramData;
    char sramFilename[512];
} SramMapperMegaSCSI;

// Counter for two or more MEGA-SCSI
// It uses it to decide the file name of SRAM. 128, 256, 512, 1024kb
static int  autoNameCounter[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
static char megascsiName[2][9] = {"eseram", "megascsi"};

static void setMapper(SramMapperMegaSCSI* rm, int page, UInt8 value)
{
    int     writeFlag;
    int     readFlag;
    UInt8*  adr;

    DBGLOG2("setMapper: %x %x\n", page, value);

    if (rm->type && (value & 0xc0) == 0x40) {
        readFlag  = 0;
        writeFlag = 0;
        value = SPC_BANK;
        adr = NULL;
    } else {
        value &= rm->mapperMask;
        readFlag = 1;
        writeFlag = ((0x80 <= value) && (page != 1)) ? 1 : 0;
        adr = rm->sramData + (value & 0x7f) * 0x2000;
    }

    if (rm->mapper[page] != value) {
        rm->mapper[page] = value;
        DBGLOG4("bank change: p%x v%x r%d w%d\n", page, value, readFlag, writeFlag);
        slotMapPage(rm->pSlot, rm->sSlot, rm->startPage + page,
                    adr, readFlag, writeFlag);
    } else {
        DBGLOG("bank not changed\n");
    }
}

static void reset(SramMapperMegaSCSI* rm)
{
    int i;

    for (i = 0; i < 4; ++i) {
        //setMapper(rm, i, 0);
        rm->mapper[i] = 0;
        slotMapPage(rm->pSlot, rm->sSlot, rm->startPage + i, rm->sramData, 1, 0);
    }

    if (rm->type) mb89352Reset(rm->spc, 1);

#ifdef MEGASCSIDEBUG
    fprintf(logFd, "Reset\n");
    fflush(logFd);
#endif
}

static void saveState(SramMapperMegaSCSI* rm)
{
    char tag[16];
    int i;
    SaveState* state = saveStateOpenForWrite("mapperMegaSCSI");

    saveStateSetBuffer(state, "sramData", rm->sramData, rm->sramSize);

    for (i = 0; i < 4; i++) {
        sprintf(tag, "mapper%d", i);
        saveStateSet(state, tag, rm->mapper[i]);
    }
    saveStateClose(state);

    if (rm->type) {
        mb89352SaveState(rm->spc);
    }
}

static void loadState(SramMapperMegaSCSI* rm)
{
    SaveState* state = saveStateOpenForRead("mapperMegaSCSI");
    char tag[16];
    int i;

    DBGLOG("load State\n");

    saveStateGetBuffer(state, "sramData", rm->sramData, rm->sramSize);

    for (i = 0; i < 4; i++) {
        sprintf(tag, "mapper%d", i);
        setMapper(rm, i, (UInt8)saveStateGet(state, tag, 0));
    }
    saveStateClose(state);

    if (rm->type) {
        mb89352LoadState(rm->spc);
    }
}

static void destroy(SramMapperMegaSCSI* rm)
{
    if (!rm->isZip) {
        sramSave(rm->sramFilename, rm->sramData, rm->sramSize, NULL, 0);
    }

    slotUnregister(rm->pSlot, rm->sSlot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    if (rm->type) {
        mb89352Destroy(rm->spc);
    }

    if (rm->isAutoName) {
        --autoNameCounter[rm->type][rm->element];
    }

    free(rm->sramData);
    free(rm);

#ifdef MEGASCSIDEBUG
    --megascsiTotal;
    fprintf(logFd, "%d close\n", megascsiTotal);
    if (!megascsiTotal) {
        fclose(logFd);
        logFd = NULL;
    }
#endif
}

static UInt8 read(SramMapperMegaSCSI* rm, UInt16 address)
{
    int page = (address >> 13);

    if (rm->mapper[page] == SPC_BANK) {
        address &= 0x1fff;

        // SPC read
        if (address < 0x1000) {
            // Data Register
            return mb89352ReadDREG(rm->spc);
        }
        address &= 0x0f;
        return mb89352ReadRegister(rm->spc, (UInt8)address);
    } else {
        return 0xff;
    }
}

static UInt8 peek(SramMapperMegaSCSI* rm, UInt16 address)
{
    int page = (address >> 13);

    if (rm->mapper[page] == SPC_BANK) {
        address &= 0x1fff;

        // SPC read
        if (address < 0x1000) {
            // Data Register
            address = REG_DREG;
        }
        address &= 0x0f;
        return mb89352PeekRegister(rm->spc, (UInt8)address);
    } else {
        return 0xff;
    }
}
static void write(SramMapperMegaSCSI* rm, UInt16 address, UInt8 value)
{
    int page = (address >> 13);

    if (page == 1) {
        setMapper(rm, (address >> 11) & 3, value);
        return;
    }

    if (rm->type && (rm->mapper[page] == SPC_BANK)){
        address &= 0x1fff;
        if (address < 0x1000) {
            mb89352WriteDREG(rm->spc, value);
        } else {
            mb89352WriteRegister(rm->spc, address & 0x0f, value);
        }
    }
}

int sramMapperMegaSCSICreate(const char* filename, UInt8* buf, int size, int pSlot, int sSlot, int startPage, int hdId, int flag)
{
    DeviceCallbacks callbacks = {
        (void*)destroy, (void*)reset, (void*)saveState, (void*)loadState };

    SramMapperMegaSCSI* rm;
    int i;

    if  (((size != 0x100000) && (size != 0x80000)  &&
          (size !=  0x40000) && (size != 0x20000)) || (flag & ~0x81)) {
        return 0;
    }

    rm = malloc(sizeof(SramMapperMegaSCSI));
    rm->type = flag & 1;
    rm->isZip = flag & 0x80;

#ifdef MEGASCSIDEBUG
    if (!megascsiTotal) {
        logFd = fopen(MEGASCSIDEBUG, "w");
    }
    ++megascsiTotal;
    DBGLOG2("%s %d: create\n", megascsiName[rm->type], megascsiTotal);
    if (strlen(filename)) {
        DBGLOG2("filename: %s size: %d\n", filename, size);
    }
#endif

    rm->deviceHandle = deviceManagerRegister(SRAM_MEGASCSI, &callbacks, rm);

    if (rm->type) {
        slotRegister(pSlot, sSlot, startPage, 4,
                    (SlotRead)read, (SlotRead)peek, (SlotWrite)write,
                    (SlotEject)destroy, rm);
    } else {
        slotRegister(pSlot, sSlot, startPage, 4, NULL, NULL, (SlotWrite)write,
                    (SlotEject)destroy, rm);
    }

    rm->pSlot       = pSlot;
    rm->sSlot       = sSlot;
    rm->startPage   = startPage;
    rm->mapperMask  = ((size >> 13) - 1) | 0x80;
    DBGLOG1("mapper mask: %x\n", (unsigned int)rm->mapperMask);

    if (strlen(filename)) {
        rm->isAutoName = 0;
    } else {
        rm->element    = EseRamSize(size);
        rm->isAutoName = 1;
    }

    // create sram
    rm->sramSize = size;
    rm->sramData = calloc(1, rm->sramSize);

    if (rm->isAutoName) {
        sprintf(rm->sramFilename, "%s%d%c.rom", megascsiName[rm->type], size / 1024,
                                   autoNameCounter[rm->type][rm->element] + (int)'A');
        strcpy(rm->sramFilename, sramCreateFilename(rm->sramFilename));
        sramLoad(rm->sramFilename, rm->sramData, rm->sramSize, NULL, 0);
        ++autoNameCounter[rm->type][rm->element];
    } else {
        memcpy(rm->sramData, buf, rm->sramSize);
        strcpy(rm->sramFilename, filename);
    }

    // initialized mapper
    for (i = 0; i < 4; ++i) {
        rm->mapper[i] = 0;
        slotMapPage(rm->pSlot, rm->sSlot, rm->startPage + i, rm->sramData, 1, 0);
    }

    // initialized spc
    rm->spc = rm->type ? mb89352Create(hdId) : NULL;

    return 1;
}
