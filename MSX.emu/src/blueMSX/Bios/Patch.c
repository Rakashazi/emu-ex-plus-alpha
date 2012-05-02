/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Bios/Patch.c,v $
**
** $Revision: 1.7 $
**
** $Date: 2009-07-18 15:08:04 $
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
#include "R800.h"
#include "Board.h"
#include "Casette.h"
#include "Disk.h"
#include "IoPort.h"
#include "SlotManager.h"
#include "Led.h"
#include <string.h>

 typedef struct {
    int sectors;
    UInt8 heads;
    UInt8 dirEntries;
    UInt8 sectorsPerTrack;
    UInt8 sectorsPerFAT;
    UInt8 sectorsPerCluster;
} FormatInfo;

static const UInt8 bootSector[] = {
    0xEB, 0xFE, 0x90, 0x56, 0x46, 0x42, 0x2D, 0x31,
    0x39, 0x38, 0x39, 0x00, 0x02, 0x02, 0x01, 0x00,
    0x02, 0x70, 0x00, 0xA0, 0x05, 0xF9, 0x03, 0x00,
    0x09, 0x00, 0x02, 0x00, 0x00, 0x00, 0xD0, 0xED,
    0x53, 0x58, 0xC0, 0x32, 0xC2, 0xC0, 0x36, 0x55,
    0x23, 0x36, 0xC0, 0x31, 0x1F, 0xF5, 0x11, 0x9D,
    0xC0, 0x0E, 0x0F, 0xCD, 0x7D, 0xF3, 0x3C, 0x28,
    0x28, 0x11, 0x00, 0x01, 0x0E, 0x1A, 0xCD, 0x7D,
    0xF3, 0x21, 0x01, 0x00, 0x22, 0xAB, 0xC0, 0x21,
    0x00, 0x3F, 0x11, 0x9D, 0xC0, 0x0E, 0x27, 0xCD,
    0x7D, 0xF3, 0xC3, 0x00, 0x01, 0x57, 0xC0, 0xCD,
    0x00, 0x00, 0x79, 0xE6, 0xFE, 0xFE, 0x02, 0x20,
    0x07, 0x3A, 0xC2, 0xC0, 0xA7, 0xCA, 0x22, 0x40,
    0x11, 0x77, 0xC0, 0x0E, 0x09, 0xCD, 0x7D, 0xF3,
    0x0E, 0x07, 0xCD, 0x7D, 0xF3, 0x18, 0xB4, 0x42,
    0x6F, 0x6F, 0x74, 0x20, 0x65, 0x72, 0x72, 0x6F,
    0x72, 0x0D, 0x0A, 0x50, 0x72, 0x65, 0x73, 0x73,
    0x20, 0x61, 0x6E, 0x79, 0x20, 0x6B, 0x65, 0x79,
    0x20, 0x66, 0x6F, 0x72, 0x20, 0x72, 0x65, 0x74,
    0x72, 0x79, 0x0D, 0x0A, 0x24, 0x00, 0x4D, 0x53,
    0x58, 0x44, 0x4F, 0x53, 0x20, 0x20, 0x53, 0x59,
    0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF3, 0x2A,
    0x51, 0xF3, 0x11, 0x00, 0x01, 0x19, 0x01, 0x00,
    0x01, 0x11, 0x00, 0xC1, 0xED, 0xB0, 0x3A, 0xEE,
    0xC0, 0x47, 0x11, 0xEF, 0xC0, 0x21, 0x00, 0x00,
    0xCD, 0x51, 0x52, 0xF3, 0x76, 0xC9, 0x18, 0x64,
    0x3A, 0xAF, 0x80, 0xF9, 0xCA, 0x6D, 0x48, 0xD3,
    0xA5, 0x0C, 0x8C, 0x2F, 0x9C, 0xCB, 0xE9, 0x89,
    0xD2, 0x00, 0x32, 0x26, 0x40, 0x94, 0x61, 0x19,
    0x20, 0xE6, 0x80, 0x6D, 0x8A, 0x00, 0x00, 0x00
};

static int patchEnabled = 0;
static int patchBoardType = 0;

void PatchDiskSetBusy(int driveId, int busy)
{
    if (driveId < MAXDRIVES && patchEnabled) {
        if (driveId == 0) ledSetFdd1(busy);
        if (driveId == 1) ledSetFdd2(busy);
    }
}

static const FormatInfo formatInfo[8] = {
    { 720,  1, 112, 9, 2, 2 },
    { 1440, 2, 112, 9, 3, 2 },
    { 640,  1, 112, 8, 1, 2 },
    { 1280, 2, 112, 8, 2, 2 },
    { 360,  1, 64,  9, 2, 1 },
    { 720,  2, 112, 9, 2, 2 },
    { 320,  1, 64,  8, 1, 1 },
    { 640,  2, 112, 8, 1, 2 }
};

static void phydio(void* ref, CpuRegs* cpu);
static void dskchg(void* ref, CpuRegs* cpu);
static void getdpb(void* ref, CpuRegs* cpu);
static void dskfmt(void* ref, CpuRegs* cpu);
static void drvoff(void* ref, CpuRegs* cpu);
static void tapion(void* ref, CpuRegs* cpu);
static void tapin(void* ref, CpuRegs* cpu);
static void tapiof(void* ref, CpuRegs* cpu);
static void tapoon(void* ref, CpuRegs* cpu);
static void stmotr(void* ref, CpuRegs* cpu);
static void tapoof(void* ref, CpuRegs* cpu);
static void tapout(void* ref, CpuRegs* cpu);
static void casout(void* ref, CpuRegs* cpu);

void vdpCmdFlushAll();

void PatchReset(BoardType boardType) {
    patchEnabled = 0;
    patchBoardType = boardType;
}

void PatchZ80(void* ref, CpuRegs* cpu)
{
    switch (patchBoardType) {
    default:
    case BOARD_MSX:
    case BOARD_MSX_S3527:
    case BOARD_MSX_S1985:
    case BOARD_MSX_T9769B:
    case BOARD_MSX_T9769C:
        switch (cpu->PC.W - 2) {
        case 0x4010: phydio(ref, cpu); break;
        case 0x4013: dskchg(ref, cpu); break;
        case 0x4016: getdpb(ref, cpu); break;
        case 0x401c: dskfmt(ref, cpu); break;
        case 0x401f: drvoff(ref, cpu); break;
        case 0x00e1: tapion(ref, cpu); break;
        case 0x00e4: tapin(ref, cpu);  break;
        case 0x00e7: tapiof(ref, cpu); break;
        case 0x00ea: tapoon(ref, cpu); break;
        case 0x00ed: tapout(ref, cpu); break;
        case 0x00f0: tapoof(ref, cpu); break;
        case 0x00f3: stmotr(ref, cpu); break;
        }
        break;
    case BOARD_SVI:
        switch (cpu->PC.W - 2) {
        /* SVI-328 BIOS */
        case 0x0069: tapion(ref, cpu); break; // CSRDON
        case 0x006C: tapin(ref, cpu);  break; // CASIN
        case 0x006F: tapiof(ref, cpu); break; // CTOFF
        case 0x0072: tapoon(ref, cpu); break; // CWRTON
        case 0x0075: tapout(ref, cpu); break; // CASOUT
        case 0x0078: tapoof(ref, cpu); break; // CTWOFF

        /* SVI-328 BASIC */
        case 0x20E6: casout(ref, cpu); break; // CASOUT
        case 0x210A: tapion(ref, cpu); break; // CSRDON
        case 0x21A9: tapin(ref, cpu); break;  // CASIN
        }
        break;
    }

    vdpCmdFlushAll();
}

static void phydio(void* ref, CpuRegs* cpu) {
    UInt8 buffer[512];
    UInt8 drive;
    UInt16 sector;
    UInt16 address;
    int write;
    UInt8 origSlotPri;
    UInt8 origSlotSec;
    UInt8 slotPri;
    UInt8 slotSec;
    int i;

    patchEnabled = 1;

    cpu->iff1 = 1;

    drive   = cpu->AF.B.h;
    sector  = cpu->DE.W;
    address = cpu->HL.W;
    write   = cpu->AF.B.l & C_FLAG;

    if (!diskPresent(drive)) {
        cpu->AF.W=0x0201;
        return;
    }

    if (address + cpu->BC.B.h * 512 > 0x10000) {
        cpu->BC.B.h = (0x10000 - address) / 512;
    }

    origSlotPri = ioPortRead(ref, 0xa8);
    origSlotSec = slotRead(ref, 0xffff) ^ 0xff;
    slotPri     = 0x55 * ((origSlotPri >> 6) & 0x03);
    slotSec     = 0x55 * ((origSlotSec >> 6) & 0x03);


    ioPortWrite(ref, 0xa8, slotPri);
    slotWrite(ref, 0xffff, slotSec);

    while (cpu->BC.B.h) {
        PatchDiskSetBusy(drive, 1);
        if (write) {
            for (i = 0; i < 512; i++) {
                buffer[i]=slotRead(ref, address++);
            }

            if (!diskWrite(drive, buffer, sector)) {
                cpu->AF.W=0x0a01;
                slotWrite(ref, 0xffff,origSlotSec);
                ioPortWrite(ref, 0xa8,origSlotPri);
                return;
            }
        }
        else {
            if (diskRead(drive, buffer, sector) != DSKE_OK) {
                cpu->AF.W = 0x0401;
                slotWrite(ref, 0xffff, origSlotSec);
                ioPortWrite(ref, 0xa8, origSlotPri);
                return;
            }
            for (i = 0; i < 512; i++) {
                slotWrite(ref, address++, buffer[i]);
            }
        }
        cpu->BC.B.h--;
        sector++;
    }

    // restore memory settings
    slotWrite(ref, 0xffff, origSlotSec);
    ioPortWrite(ref, 0xa8, origSlotPri);

    cpu->AF.B.l&=~C_FLAG;
}

static void dskchg(void* ref, CpuRegs* cpu) {
    UInt8 buffer[512];
    UInt8 drive = cpu->AF.B.h;

    cpu->iff1 = 1;

    if (!diskPresent(cpu->AF.B.h)) {
        cpu->AF.W = 0x0201;
        return;
    }

    PatchDiskSetBusy(drive, 1);
    if (diskRead(drive, buffer, 1) != DSKE_OK) {
        cpu->AF.W = 0x0a01;
        return;
    }

    cpu->BC.B.h = buffer[0];

    getdpb(ref, cpu);

    if (cpu->AF.B.l & C_FLAG) {
        cpu->AF.W = 0x0a01;
    }

    cpu->BC.B.h = 0;
}

static void getdpb(void* ref, CpuRegs* cpu) {
    UInt16 dirSectorNo;
    UInt16 dataSectorNo;
    UInt8  fatSectorNo;
    UInt16 maxClusters;
    UInt16 address;
    UInt8  mediaDescriptor;

    if (!diskPresent(cpu->AF.B.h)) {
        cpu->AF.W = 0x0201;
        return;
    }

    mediaDescriptor = cpu->BC.B.h;
    address = cpu->HL.W;

    switch (mediaDescriptor) {
    case 0xf8:
        fatSectorNo = 2;
        maxClusters = 355;
        break;
    case 0xf9:
        fatSectorNo = 3;
        maxClusters = 714;
        break;
    case 0xfa:
        fatSectorNo = 1;
        maxClusters = 316;
        break;
    case 0xfb:
        fatSectorNo = 2;
        maxClusters = 635;
        break;
    case 0xfc:
        fatSectorNo = 2;
        maxClusters = 316;
        break;
    default:
        cpu->AF.W=0x0C01;
        return;
    }

    dirSectorNo = 1 + fatSectorNo * 2;
    dataSectorNo = dirSectorNo + 7;

    slotWrite(ref, address +  1, mediaDescriptor);
    slotWrite(ref, address +  2, 0);
    slotWrite(ref, address +  3, 2);
    slotWrite(ref, address +  4, 15);
    slotWrite(ref, address +  5, 4);
    slotWrite(ref, address +  6, 1);
    slotWrite(ref, address +  7, 2);
    slotWrite(ref, address +  8, 1);
    slotWrite(ref, address +  9, 0);
    slotWrite(ref, address + 10, 2);
    slotWrite(ref, address + 11, 112);
    slotWrite(ref, address + 12, dataSectorNo & 0xff);
    slotWrite(ref, address + 13, dataSectorNo >> 8);
    slotWrite(ref, address + 14, maxClusters & 0xff);
    slotWrite(ref, address + 15, maxClusters >> 8);
    slotWrite(ref, address + 16, fatSectorNo);
    slotWrite(ref, address + 17, dirSectorNo & 0xff);
    slotWrite(ref, address + 18, dirSectorNo >> 8);

    /* Return success      */
    cpu->AF.B.l&=~C_FLAG;
}

static void dskfmt(void* ref, CpuRegs* cpu) {
    UInt8 buffer[512];
    UInt8 index;
    int j;
    int i;
    int dirSize;
    int dataSize;
    int sector;

    cpu->AF.B.l |= C_FLAG;

    cpu->iff1 = 1;

    /* If invalid choice, return "Bad parameter": */
    if (cpu->AF.B.h == 0x87) {
        cpu->AF.B.h = 2;
    }

    index = cpu->AF.B.h - 1;
    if (index > 1) {
        cpu->AF.W = 0x0c01;
        return;
    }

    /* If no disk, return "Not ready": */
    if(!diskPresent(cpu->DE.B.h)) {
        cpu->AF.W=0x0201;
        return;
    }

    memset(buffer, 0, 512);
    memcpy(buffer, bootSector, sizeof(bootSector));

    memcpy(buffer + 3, "blueMSX", 8);

    /* sectors per cluster */
    buffer[13] = formatInfo[index].sectorsPerCluster;

    /* Number of names     */
    buffer[17] = formatInfo[index].dirEntries;
    buffer[18] = 0;

    /* Number of sectors   */
    buffer[19] = formatInfo[index].sectors & 0xff;
    buffer[20] = (formatInfo[index].sectors >> 8) & 0xff;

    /* Format ID [F8h-FFh] */
    buffer[21] = index + 0xf8;

    /* sectors per FAT     */
    buffer[22] = formatInfo[index].sectorsPerFAT;
    buffer[23] = 0;

    /* sectors per track   */
    buffer[24] = formatInfo[index].sectorsPerTrack;
    buffer[25] = 0;

    /* Number of heads     */
    buffer[26] = formatInfo[index].heads;
    buffer[27] = 0;

    /* If can't write bootblock, return "Write protected": */
    PatchDiskSetBusy(cpu->DE.B.h, 1);
    if (!diskWrite(cpu->DE.B.h, buffer, 0)) {
        cpu->AF.W = 0x0001;
        return;
    }


    /* Writing FATs: */
    sector = 1;
    for (j = 0; j < 2; j++) {
        buffer[0] = index + 0xf8;
        buffer[1] = 0xff;
        buffer[2] = 0xff;
        memset(buffer + 3, 0x00, 509);

        if (!diskWrite(cpu->DE.B.h, buffer, sector++)) {
            cpu->AF.W = 0x0a01;
            return;
        }

        memset(buffer, 0x00, 512);

        for(i = formatInfo[index].sectorsPerFAT; i > 1; i--) {
            if (!diskWrite(cpu->DE.B.h, buffer, sector++)) {
                cpu->AF.W = 0x0A01;
                return;
            }
        }
    }

    dirSize = formatInfo[index].dirEntries / 16;
    dataSize = formatInfo[index].sectors - 2*formatInfo[index].sectorsPerFAT - dirSize - 1;

    memset(buffer, 0x00, 512);
    while (dirSize--) {
        if (!diskWrite(cpu->DE.B.h, buffer, sector++)) {
            cpu->AF.W = 0x0A01;
            return;
        }
    }

    memset(buffer, 0xFF, 512);
    while (dataSize--) {
        if (!diskWrite(cpu->DE.B.h, buffer, sector++)) {
            cpu->AF.W = 0x0a01;
            return;
        }
    }

    /* Return success      */
    cpu->AF.B.l &= ~C_FLAG;
}


static void drvoff(void* ref, CpuRegs* cpu) {
}

static void tapion(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l|=C_FLAG;

    if (tapeReadHeader()) {
        cpu->AF.B.l&=~C_FLAG;
    }
}

static void tapin(void* ref, CpuRegs* cpu) {
    UInt8 value;

    cpu->AF.B.l |= C_FLAG;

    if (tapeRead(&value)) {
        cpu->AF.B.h = value;
        cpu->AF.B.l &= ~C_FLAG;
    }
}

static void tapiof(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l &= ~C_FLAG;
}

static void tapoon(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l |= C_FLAG;

    if (tapeWriteHeader()) {
        cpu->AF.B.l &= ~C_FLAG;
    }
}

static void tapout(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l &= ~C_FLAG;

    if (tapeWrite(cpu->AF.B.h)) {
        cpu->AF.B.l &= ~C_FLAG;
    }
}

static void tapoof(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l |= C_FLAG;
}

static void stmotr(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l &= ~C_FLAG;
}

static void casout(void* ref, CpuRegs* cpu) {
    cpu->AF.B.l &= ~C_FLAG;

    if (tapeWrite(cpu->AF.B.h)) {
        cpu->AF.B.l &= ~C_FLAG;
    }
        cpu->PC.W = 0x20ED;
}
