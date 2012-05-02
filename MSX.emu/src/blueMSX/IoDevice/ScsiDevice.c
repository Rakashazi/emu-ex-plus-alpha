/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/ScsiDevice.c,v $
**
** $Revision: 1.10 $
**
** $Date: 2007-03-25 17:05:07 $
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
 * Notes:
 *  It follows the SCSI1(CCS) standard or the SCSI2 standard.
 *  Only the direct access device is supported now.
 *  Message system might be imperfect.
 */
#include "ScsiDevice.h"
#include "ArchCdrom.h"
#include "ScsiDefs.h"
#include "Disk.h"
#include "Board.h"
#include "FileHistory.h"
#include "Led.h"
#include "Properties.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>

#define USE_SPECIALCOMMAND

// Medium type (value like LS-120)
#define MT_UNKNOWN      0x00
#define MT_2DD_UN       0x10
#define MT_2DD          0x11
#define MT_2HD_UN       0x20
#define MT_2HD_12_98    0x22
#define MT_2HD_12       0x23
#define MT_2HD_144      0x24
#define MT_LS120        0x31
#define MT_NO_DISK      0x70
#define MT_DOOR_OPEN    0x71
#define MT_FMT_ERROR    0x72

struct SCSIDEVICE {
    int diskId;
    int scsiId;             // SCSI ID 0..7
    int deviceType;
    int mode;
    int enabled;
    int reset;              // Unit Attention
    int motor;              // Reserved
    int keycode;            // Sense key, ASC, ASCQ
    int inserted;
    int changed;            // Enhanced change flag for MEGA-SCSI driver
    int changeCheck2;       // Disk change control flag
    int sector;
    int sectorSize;
    int length;
    int message;
    int lun;
    ArchCdrom* cdrom;
    UInt8 cdb[12];          // Command Descriptor Block
    UInt8* buffer;
    char* productName;
    FileProperties disk;
};

static const UInt8 inqdata[36] = {
       0,   // bit5-0 device type code.
       0,   // bit7 = 1 removable device
       2,   // bit7,6 ISO version. bit5,4,3 ECMA version.
            // bit2,1,0 ANSI Version (001=SCSI1, 010=SCSI2)
       2,   // bit7 AENC. bit6 TrmIOP.
            // bit3-0 Response Data Format. (0000=SCSI1, 0001=CCS, 0010=SCSI2)
      51,   // addtional length
       0, 0,// reserved
       0,   // bit7 RelAdr, bit6 WBus32, bit5 Wbus16, bit4 Sync, bit3 Linked,
            // bit2 reseved bit1 CmdQue, bit0 SftRe
     'b', 'l', 'u', 'e', 'M', 'S', 'X', ' ',    // vendor ID (8bytes)
     'S', 'C', 'S', 'I', '2', ' ', 'H', 'a',    // product ID (16bytes)
     'r', 'd', 'd', 'i', 's', 'k', ' ', ' ',
     '0', '1', '0', 'a' };                  // product version (ASCII 4bytes)

static const char sdt_name[10][10] =
{
    "Tape      ",
    "Printer   ",
    "Processor ",
    "WORM      ",
    "CD-ROM    ",
    "Scanner   ",
    "MO        ",
    "Jukebox   ",
    "COMM      ",
    "???       "
};

// for FDSFORM.COM
static const char fds120[28]  = "IODATA  LS-120 COSM     0001";

/*
    Log output routine for debug
*/
#ifdef SCSIDEBUG

static int logNumber = 0;
static FILE* scsiLog = NULL;

FILE* scsiDeviceLogCreate()
{
    if (!logNumber) {
        scsiLog = fopen(SCSIDEBUG, "w");
    }
    logNumber++;
    return scsiLog;
}

void scsiDeviceLogFlush()
{
    fflush(scsiLog);
}

void scsiDeviceLogClose()
{
    logNumber--;
    if (!logNumber) {
        fclose(scsiLog);
    }
}

#endif

SCSIDEVICE* scsiDeviceCreate(int scsiId, int diskId, UInt8* buf, char* name,
                  int type, int mode, CdromXferCompCb xferCompCb, void* ref)
{
    SCSIDEVICE* scsi = malloc(sizeof(SCSIDEVICE));

    scsi->scsiId        = scsiId;
    scsi->diskId        = diskId;
    scsi->buffer        = buf;
    scsi->productName   = name;
    scsi->deviceType    = type;
    scsi->mode          = mode;
    scsi->enabled       = 1;
    scsi->sectorSize    = 512;
    scsi->cdrom         = NULL;
/*
    scsi->disk.fileName[0] = 0;
    scsi->disk.fileNameInZip[0] = 0;
    scsi->disk.directory[0] = 0;
    scsi->disk.extensionFilter = 0;
    scsi->disk.type = 0;
*/
    if (type == SDT_CDROM) {
        scsi->sectorSize = 2048;
        scsi->cdrom = archCdromCreate(xferCompCb, ref);
        if (scsi->cdrom == NULL) {
            scsi->enabled = 0;
        }
    }
    scsiDeviceReset(scsi);
    return scsi;
}

void scsiDeviceDestroy(SCSIDEVICE* scsi)
{
    SCSILOG1("hdd %d: close\n", scsi->scsiId);
    if (scsi->deviceType == SDT_CDROM) {
        archCdromDestroy(scsi->cdrom);
    }
    free(scsi);
}

void scsiDeviceReset(SCSIDEVICE* scsi)
{
    if (scsi->deviceType == SDT_CDROM) {
        archCdromHwReset(scsi->cdrom);
    }
    scsi->changed       = 0;
    scsi->keycode       = 0;
    scsi->sector        = 0;
    scsi->length        = 0;
    scsi->motor         = 1;
    scsi->changeCheck2  = 1;    // the first use always
    scsi->reset         = (scsi->mode & MODE_UNITATTENTION) ? 1 : 0;

    scsi->disk = propGetGlobalProperties()->media.disks[scsi->diskId];
    scsi->inserted   = (strlen(scsi->disk.fileName) > 0);
    if (scsi->inserted) {
        SCSILOG1("hdd %d: \n", scsi->scsiId);
        SCSILOG1("filename: %s\n", scsi->disk.fileName);
        SCSILOG1("     zip: %s\n", scsi->disk.fileNameInZip);
    }
    else if ((scsi->mode & MODE_NOVAXIS) && scsi->deviceType != SDT_CDROM) {
        scsi->enabled = 0;
    }
}

void scsiDeviceBusReset(SCSIDEVICE* scsi)
{
    SCSILOG1("SCSI %d: bus reset\n", scsi->scsiId);
    scsi->keycode = 0;
    scsi->reset = (scsi->mode & MODE_UNITATTENTION) ? 1 : 0;
    if (scsi->deviceType == SDT_CDROM) {
        archCdromBusReset(scsi->cdrom);
    }
}

void scsiDeviceDisconnect(SCSIDEVICE* scsi)
{
    if (scsi->deviceType != SDT_CDROM) {
        ledSetHd(0);
    } else {
        archCdromDisconnect(scsi->cdrom);
    }
}

void scsiDeviceEnable(SCSIDEVICE* scsi, int enable)
{
    scsi->enabled = enable;
}

// Check the initiator in the call origin.
int scsiDeviceSelection(SCSIDEVICE* scsi)
{
    scsi->lun = 0;
    if (scsi->mode & MODE_REMOVABLE) {
        if (!scsi->enabled &&
           (scsi->mode & MODE_NOVAXIS) && scsi->deviceType != SDT_CDROM) {
            scsi->enabled = diskPresent(scsi->diskId) ? 1 : 0;
        }
        return scsi->enabled;
    }
    return scsi->enabled && diskPresent(scsi->diskId);
}

static int scsiDeviceGetReady(SCSIDEVICE* scsi)
{
    if (diskPresent(scsi->diskId)) {
        return 1;
    }
    scsi->keycode = SENSE_MEDIUM_NOT_PRESENT;
    return 0;
}

static int scsiDeviceDiskChanged(SCSIDEVICE* scsi)
{
    FileProperties* pDisk;
    int changed = diskChanged(scsi->diskId);

    if (changed) {
        scsi->motor = 1;
        pDisk = &propGetGlobalProperties()->media.disks[scsi->diskId];

        if (scsi->changeCheck2) {
            scsi->changeCheck2 = 0;
            if (scsi->inserted &&
               (strcmp(scsi->disk.fileName, pDisk->fileName) == 0) &&
               (strcmp(scsi->disk.fileNameInZip, pDisk->fileNameInZip) == 0)) {
                SCSILOG("Disk change invalidity\n\n");
                return 0;
            }
        }
        scsi->changed  = 1;
        scsi->disk = *pDisk;
        scsi->inserted = 1;

        SCSILOG1("hdd %d: disk change\n", scsi->scsiId);
        SCSILOG1("filename: %s\n", scsi->disk.fileName);
        SCSILOG1("     zip: %s\n", scsi->disk.fileNameInZip);
    } else {
        if (scsi->inserted & !diskPresent(scsi->diskId)) {
            scsi->inserted = 0;
            scsi->motor    = 0;
            scsi->changed  = 1;
            changed        = 1;
        }
    }

    if (changed && (scsi->mode & MODE_UNITATTENTION)) {
        scsi->reset = 1;
    }
    return changed;
}

static void scsiDeviceTestUnitReady(SCSIDEVICE* scsi)
{
    if ((scsi->mode & MODE_NOVAXIS) == 0) {
        if (scsiDeviceGetReady(scsi) && scsi->changed && (scsi->mode & MODE_MEGASCSI)) {
            // Disk change is surely sent for the driver of MEGA-SCSI.
            scsi->keycode = SENSE_POWER_ON;
        }
    }
    scsi->changed = 0;
}

static void scsiDeviceStartStopUnit(SCSIDEVICE* scsi)
{
    FileProperties* disk = &propGetGlobalProperties()->media.disks[scsi->diskId];

    switch (scsi->cdb[4]) {
    case 2:
        // Eject
        if (diskPresent(scsi->diskId)) {
            disk->fileName[0] = 0;
            disk->fileNameInZip[0] = 0;
            updateExtendedDiskName(scsi->diskId, disk->fileName, disk->fileNameInZip);
            boardChangeDiskette(scsi->diskId, NULL, NULL);
            SCSILOG1("hdd %d eject\n", scsi->scsiId);
        }
        break;
    case 3:
        // Insert
        if (!diskPresent(scsi->diskId)) {
            *disk = scsi->disk;
            updateExtendedDiskName(scsi->diskId, disk->fileName, disk->fileNameInZip);
            boardChangeDiskette(scsi->diskId, disk->fileName, disk->fileNameInZip);
            SCSILOG1("hdd %d insert\n", scsi->scsiId);
        }
        break;
    }
    scsi->motor = scsi->cdb[4] & 1;
    SCSILOG2("hdd %d motor: %d\n", scsi->scsiId, scsi->motor);
}

static int scsiDeviceInquiry(SCSIDEVICE* scsi)
{
    int total       = _diskGetTotalSectors(scsi->diskId);
    int length      = scsi->length;
    UInt8* buffer   = scsi->buffer;
    UInt8 type      = (UInt8)(scsi->deviceType & 0xff);
    UInt8 removable;
    const char* fileName;
    int i;
    int fdsmode = (scsi->mode & MODE_FDS120) && (total > 0) && (total <= 2880);

    if (length == 0) return 0;

    if (fdsmode) {
        memcpy(buffer + 2, inqdata + 2, 6);
        memcpy(buffer + 8, fds120, 28);
        removable = 0x80;
        if (type != SDT_DirectAccess) {
            type = SDT_Processor;
        }
    } else {
        memcpy(buffer + 2, inqdata + 2, 34);
        removable = (scsi->mode & MODE_REMOVABLE) ? 0x80 : 0;

        if (scsi->productName == NULL) {
            int dt = scsi->deviceType;
            if (dt != SDT_DirectAccess) {
                if (dt > SDT_Communications) {
                    dt = SDT_Communications + 1;
                }
                --dt;
                memcpy(buffer + 22, sdt_name[dt], 10);
            }
        } else {
            memcpy(buffer + 16, scsi->productName, 16);
        }
    }

    buffer[0] = type;
    buffer[1] = removable;

    if (!(scsi->mode & BIT_SCSI2)) {
        buffer[2]  = 1;
        buffer[3]  = 1;
        if (!fdsmode) buffer[20] = '1';
    } else {
        if (scsi->mode & BIT_SCSI3) {
            buffer[2]  = 5;
            if (!fdsmode) buffer[20] = '3';
        }
    }

    if ((scsi->mode & MODE_CHECK2) && !fdsmode) {
        buffer[35] = 'A';
    }
    if (scsi->mode & BIT_SCSI3) {
        if (length > 96) length = 96;
        buffer[4] = 91;
        if (length > 56) {
            memset(buffer + 56, 0, 40);
            buffer[58] = 0x03;
            buffer[60] = 0x01;
            buffer[61] = 0x80;
        }
    } else {
        if (length > 56) length = 56;
    }

    if (length > 36){
        buffer += 36;
        memset(buffer, ' ', 20);
        fileName = strlen(scsi->disk.fileNameInZip) ? scsi->disk.fileNameInZip : scsi->disk.fileName;
        fileName = stripPath(fileName);
        for (i = 0; i < 20; ++i) {
            if (*fileName == 0) {
                break;
            }
            *buffer = *fileName;
            ++buffer;
            ++fileName;
        }
    }
    return length;
}

static int scsiDeviceModeSense(SCSIDEVICE* scsi)
{
    int length  = scsi->length;

    if ((length > 0) && (scsi->cdb[2] == 3)) {
        UInt8* buffer   = scsi->buffer;
        int total       = _diskGetTotalSectors(scsi->diskId);
        int media       = MT_UNKNOWN;
        int sectors     = 64;
        int blockLength = scsi->sectorSize >> 8;
        int tracks      = 8;
        int size        = 4 + 24;
        int removable   = scsi->mode & MODE_REMOVABLE ? 0xa0 : 0x80;

        memset(buffer + 2, 0, 34);

        if (total == 0) {
            media = MT_NO_DISK;
        } else {
            if (scsi->mode & MODE_FDS120) {
                if (total == 1440) {
                    media       = MT_2DD;
                    sectors     = 9;
                    blockLength = 2048 >> 8;            // FDS-120 value
                    tracks      = 160;
                    removable   = 0xa0;
                } else {
                    if (total == 2880) {
                        media       = MT_2HD_144;
                        sectors     = 18;
                        blockLength = 2048 >> 8;
                        tracks      = 160;
                        removable   = 0xa0;
                    }
                }
            }
        }

        // Mode Parameter Header 4bytes
        buffer[1] = media;              // Medium Type
        buffer[3] = 8;                  // block descripter length
        buffer   += 4;

        // Disable Block Descriptor check
        if (!(scsi->cdb[1] & 0x08)) {
            // Block Descriptor 8bytes
            buffer[1]  = (UInt8)((total >> 16) & 0xff); // 1..3 Number of Blocks
            buffer[2]  = (UInt8)((total >>  8) & 0xff);
            buffer[3]  = (UInt8)(total & 0xff);
            buffer[6]  = (UInt8)(blockLength & 0xff);   // 5..7 Block Length in Bytes
            buffer    += 8;
            size      += 8;
        }

        // Format Device Page 24bytes
        buffer[0]  = 3;                 //     0 Page
        buffer[1]  = 0x16;              //     1 Page Length
        buffer[3]  = (UInt8)tracks;     //  2, 3 Tracks per Zone
        buffer[11] = (UInt8)sectors;    // 10,11 Sectors per Track
        buffer[12] = (UInt8)blockLength;// 12,13 Data Bytes per Physical Sector
        buffer[20] = removable;         // 20    bit7 Soft Sector bit5 Removable

        scsi->buffer[0] = size - 1;     // sense data length

        if (length > size) {
            length = size;
        }
        return length;
    }
    scsi->keycode = SENSE_INVALID_COMMAND_CODE;
    return 0;
}

static int scsiDeviceRequestSense(SCSIDEVICE* scsi)
{
    int keycode;
    UInt8* buffer = scsi->buffer;
    int length = scsi->length;

    if (scsi->reset) {
        scsi->reset = 0;
        keycode = SENSE_POWER_ON;
    } else {
        keycode = scsi->keycode;
    }

    SCSILOG1("Request Sense: keycode = %X\n", keycode);
    scsi->keycode = SENSE_NO_SENSE;

    memset(buffer + 1, 0, 17);
    if (length == 0) {
        if (scsi->mode & BIT_SCSI2) {
            return 0;
        }
        buffer[0]    = (UInt8)((keycode >> 8) & 0xff);  // Sense code
        length = 4;
    } else {
        buffer[0]  = 0x70;
        buffer[2]  = (UInt8)((keycode >> 16) & 0xff);   // Sense key
        buffer[7]  = 10;                                // Additional sense length
        buffer[12] = (UInt8)((keycode >> 8) & 0xff);    // Additional sense code
        buffer[13] = (UInt8)(keycode & 0xff);           // Additional sense code qualifier
        if (length > 18) length = 18;
    }
    return length;
}

static int scsiDeviceCheckReadOnly(SCSIDEVICE* scsi)
{
    int result = diskReadOnly(scsi->diskId);
    if (result) {
        scsi->keycode = SENSE_WRITE_PROTECT;
    }
    return result;
}

static int scsiDeviceReadCapacity(SCSIDEVICE* scsi)
{
    UInt32 block  = _diskGetTotalSectors(scsi->diskId);
    UInt8* buffer = scsi->buffer;

    if (block == 0) {
        scsi->keycode = SENSE_MEDIUM_NOT_PRESENT;
        SCSILOG1("hdd %d: drive not ready\n", scsi->scsiId);
        return 0;
    }

    SCSILOG1("total block: %u\n", (unsigned int)block);

    --block;
    buffer[0] = (UInt8)((block >> 24) & 0xff);
    buffer[1] = (UInt8)((block >> 16) & 0xff);
    buffer[2] = (UInt8)((block >>  8) & 0xff);
    buffer[3] = (UInt8)(block & 0xff);
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = (UInt8)((scsi->sectorSize >> 8) & 0xff);
    buffer[7] = 0;

    return 8;
}

#ifdef USE_SPECIALCOMMAND

// This routine demands 'BUFFER_SIZE' by 1KB or more.
static int scsiDeviceBlueMSX(SCSIDEVICE* scsi)
{
    FileProperties* disk;
    int length;
    const char* fileName;
    const char scsicat[16] = "SCSI-CATblueMSX ";
    UInt8* buffer = scsi->buffer;
    int cmd;

    SCSILOG1("SCSI-CAT %d\n", scsi->scsiId);
    cmd = (scsi->cdb[4] << 8) + scsi->cdb[5];

    if (cmd == 0) {
        // revision
        memcpy(buffer , scsicat, 16);       // SCSI-CAT(8) + emulator name(8)
        buffer[16] = (UInt8)(scsi->deviceType & 0xff);  // device type
        buffer[17] = scsi->mode & MODE_REMOVABLE ? 0x80 : 0;    // removable device
        buffer[18] = 0;                     // revision MSB
        buffer[19] = 1;                     // LSB
        return 20;
    }
    if (cmd == 1) {
        // inserted
        buffer[0] = diskPresent(scsi->diskId) ? 1 : 0;
        return 1;
    }
    if (cmd < 6) {
        // file info
        disk = &scsi->disk;
        fileName = (cmd < 4) ? disk->fileName : disk->fileNameInZip;
        if ((cmd & 1) == 0) {
            fileName = stripPath(fileName);
        }
        length = strlen(fileName);
        buffer[0] = (UInt8)((length >> 8) & 0xff);
        buffer[1] = (UInt8)(length & 0xff);
        strcpy(buffer + 2, fileName);
        SCSILOG1("file info:\n%s\n", buffer + 2);
        return length + 3;  // + \0
    }
    scsi->keycode = SENSE_INVALID_COMMAND_CODE;
    return 0;
}
#endif

static int scsiDeviceCheckAddress(SCSIDEVICE* scsi)
{
    int total = _diskGetTotalSectors(scsi->diskId);
    if (total == 0) {
        scsi->keycode = SENSE_MEDIUM_NOT_PRESENT;
        SCSILOG1("hdd %d: drive not ready\n", scsi->scsiId);
        return 0;
    }

    if ((scsi->sector >= 0) && (scsi->length > 0) &&
            (scsi->sector + scsi->length <= total)) {
        return 1;
    }
    SCSILOG1("hdd %d: IllegalBlockAddress\n", scsi->scsiId);
    scsi->keycode = SENSE_ILLEGAL_BLOCK_ADDRESS;
    return 0;
}

// Execute scsiDeviceCheckAddress previously.
static int scsiDeviceReadSector(SCSIDEVICE* scsi, int* blocks)
{
    int counter;
    int numSectors;

    ledSetHd(1);
    if (scsi->length >= BUFFER_BLOCK_SIZE) {
        numSectors  = BUFFER_BLOCK_SIZE;
        counter     = BUFFER_SIZE;
    } else {
        numSectors  = scsi->length;
        counter     = scsi->length * 512;
    }

    //SCSILOG("hdd#%d read sector: %d %d\n", scsi->scsiId, scsi->sector, numSectors);
    if (_diskRead2(scsi->diskId, scsi->buffer, scsi->sector, numSectors)) {
        scsi->sector += numSectors;
        scsi->length -= numSectors;
        *blocks = scsi->length;
        return counter;
    }
    *blocks = 0;
    scsi->keycode = SENSE_UNRECOVERED_READ_ERROR;
    return 0;
}

int scsiDeviceDataIn(SCSIDEVICE* scsi, int* blocks)
{
    int counter;

    if (scsi->cdb[0] == SCSIOP_READ10) {
        counter = scsiDeviceReadSector(scsi, blocks);
        if (counter) {
            return counter;
        }
    }
    SCSILOG1("datain error %x\n", scsi->cdb[0]);
    *blocks = 0;
    return 0;
}

// Execute scsiDeviceCheckAddress and scsiDeviceCheckReadOnly previously.
static int scsiDeviceWriteSector(SCSIDEVICE* scsi, int* blocks)
{
    int counter;
    int numSectors;

    ledSetHd(1);
    if (scsi->length >= BUFFER_BLOCK_SIZE) {
        numSectors  = BUFFER_BLOCK_SIZE;
    } else {
        numSectors  = scsi->length;
    }

    SCSILOG3("hdd#%d write sector: %d %d\n", scsi->scsiId, scsi->sector, numSectors);
    if (_diskWrite2(scsi->diskId, scsi->buffer, scsi->sector, numSectors)) {
        scsi->sector += numSectors;
        scsi->length -= numSectors;

        if (scsi->length >= BUFFER_BLOCK_SIZE) {
            *blocks = scsi->length - BUFFER_BLOCK_SIZE;
            counter = BUFFER_SIZE;
        } else {
            *blocks = 0;
            counter = scsi->length * 512;
        }
        return counter;
    }
    scsi->keycode = SENSE_WRITE_FAULT;
    *blocks = 0;
    return 0;
}

int scsiDeviceDataOut(SCSIDEVICE* scsi, int* blocks)
{
    if (scsi->cdb[0] == SCSIOP_WRITE10) {
        return scsiDeviceWriteSector(scsi, blocks);
    }
    SCSILOG1("dataout error %x\n", scsi->cdb[0]);
    *blocks = 0;
    return 0;
}

//  MBR erase only
static void scsiDeviceFormatUnit(SCSIDEVICE* scsi)
{
    if (scsiDeviceGetReady(scsi) && !scsiDeviceCheckReadOnly(scsi)) {
        memset(scsi->buffer, 0, 512);
        if (_diskWrite2(scsi->diskId, scsi->buffer, 0, 1)) {
            scsi->reset   = 1;
            scsi->changed = 1;
        } else {
            scsi->keycode = SENSE_WRITE_FAULT;
        }
    }
}

UInt8 scsiDeviceGetStatusCode(SCSIDEVICE* scsi)
{
    UInt8 result;
    if (scsi->deviceType != SDT_CDROM) {
        result = scsi->keycode ? SCSIST_CHECK_CONDITION : SCSIST_GOOD;
    } else {
        result = archCdromGetStatusCode(scsi->cdrom);
    }
    SCSILOG1("SCSI status code: %x\n", result);
    return result;
}

int scsiDeviceExecuteCmd(SCSIDEVICE* scsi, UInt8* cdb, SCSI_PHASE* phase, int* blocks)
{
    int counter;

    SCSILOG1("SCSI Command: %x\n", cdb[0]);
    memcpy(scsi->cdb, cdb, 12);
    scsi->message = 0;
    *phase = Status;
    *blocks = 0;

    if (scsi->deviceType == SDT_CDROM) {
        int retval;
        scsi->keycode = SENSE_NO_SENSE;
        *phase = Execute;
        retval = archCdromExecCmd(scsi->cdrom, cdb, scsi->buffer, BUFFER_SIZE);
        switch (retval) {
        case 0:
            *phase = Status;
            break;
        case -1:
            break;
        default:
            *phase = DataIn;
            return retval;
        }
        return 0;
    }

    scsiDeviceDiskChanged(scsi);

    // check unit attention
    if (scsi->reset && (scsi->mode & MODE_UNITATTENTION) &&
       (cdb[0] != SCSIOP_INQUIRY) && (cdb[0] != SCSIOP_REQUEST_SENSE)) {
        scsi->reset = 0;
        scsi->keycode = SENSE_POWER_ON;
        if (cdb[0] == SCSIOP_TEST_UNIT_READY) {
            scsi->changed = 0;
        }
        SCSILOG("Unit Attention. This command is not executed.\n");
        return 0;
    }

    // check LUN
    if (((cdb[1] & 0xe0) || scsi->lun) && (cdb[0] != SCSIOP_REQUEST_SENSE) &&
        !(cdb[0] == SCSIOP_INQUIRY && !(scsi->mode & MODE_NOVAXIS))) {
        scsi->keycode = SENSE_INVALID_LUN;
        SCSILOG("check LUN error\n");
        return 0;
    }

    if (cdb[0] != SCSIOP_REQUEST_SENSE) {
        scsi->keycode = SENSE_NO_SENSE;
    }

    if (cdb[0] < SCSIOP_GROUP1) {
        scsi->sector = ((cdb[1] & 0x1f) << 16) |
                       (cdb[2] << 8) | cdb[3];
        scsi->length = cdb[4];

        switch (cdb[0]) {
        case SCSIOP_TEST_UNIT_READY:
            SCSILOG("TestUnitReady\n");
            scsiDeviceTestUnitReady(scsi);
            return 0;

        case SCSIOP_INQUIRY:
            SCSILOG1("Inquiry %d\n", scsi->length);
            counter = scsiDeviceInquiry(scsi);
            if (counter) {
                *phase = DataIn;
            }
            return counter;

        case SCSIOP_REQUEST_SENSE:
            SCSILOG("RequestSense\n");
            counter = scsiDeviceRequestSense(scsi);
            if (counter) {
                *phase = DataIn;
            }
            return counter;

        case SCSIOP_READ6:
            SCSILOG2("Read6: %d %d\n", scsi->sector, scsi->length);
            if (scsi->length == 0) {
                //scsi->length = scsi->sectorSize >> 1;
                scsi->length = 256;
            }
            if (scsiDeviceCheckAddress(scsi)) {
                counter = scsiDeviceReadSector(scsi, blocks);
                if(counter) {
                    scsi->cdb[0] = SCSIOP_READ10;
                    *phase = DataIn;
                    return counter;
                }
            }
            return 0;

        case SCSIOP_WRITE6:
            SCSILOG2("Write6: %d %d\n", scsi->sector, scsi->length);
            if (scsi->length == 0) {
                //scsi->length = scsi->sectorSize >> 1;
                scsi->length = 256;
            }
            if (scsiDeviceCheckAddress(scsi) && !scsiDeviceCheckReadOnly(scsi)) {
                ledSetHd(1);
                if (scsi->length >= BUFFER_BLOCK_SIZE) {
                    *blocks = scsi->length - BUFFER_BLOCK_SIZE;
                    counter = BUFFER_SIZE;
                } else {
                    counter = scsi->length * 512;
                }
                scsi->cdb[0] = SCSIOP_WRITE10;
                *phase = DataOut;
                return counter;
            }
            return 0;

        case SCSIOP_SEEK6:
            SCSILOG1("Seek6: %d\n", scsi->sector);
            ledSetHd(1);
            scsi->length = 1;
            scsiDeviceCheckAddress(scsi);
            return 0;

        case SCSIOP_MODE_SENSE:
            SCSILOG1("ModeSense: %d\n", scsi->length);
            counter = scsiDeviceModeSense(scsi);
            if (counter) {
                *phase = DataIn;
            }
            return counter;

        case SCSIOP_FORMAT_UNIT:
            SCSILOG("FormatUnit\n");
            scsiDeviceFormatUnit(scsi);
            return 0;

        case SCSIOP_START_STOP_UNIT:
            SCSILOG("StartStopUnit\n");
            scsiDeviceStartStopUnit(scsi);
            return 0;

        case SCSIOP_REZERO_UNIT:
        case SCSIOP_REASSIGN_BLOCKS:
        case SCSIOP_RESERVE_UNIT:
        case SCSIOP_RELEASE_UNIT:
        case SCSIOP_SEND_DIAGNOSTIC:
            SCSILOG("SCSI_Group0 dummy\n");
            return 0;
        }
    } else {
        scsi->sector = (cdb[2] << 24) | (cdb[3] << 16) |
                       (cdb[4] << 8)  |  cdb[5];
        scsi->length = (cdb[7] << 8) + cdb[8];

        switch (cdb[0]) {
        case SCSIOP_READ10:
            SCSILOG2("Read10: %d %d\n", scsi->sector, scsi->length);

            if (scsiDeviceCheckAddress(scsi)) {
                counter = scsiDeviceReadSector(scsi, blocks);
                if(counter) {
                    *phase = DataIn;
                    return counter;
                }
            }
            return 0;

        case SCSIOP_WRITE10:
            SCSILOG2("Write10: %d %d\n", scsi->sector, scsi->length);

            if (scsiDeviceCheckAddress(scsi) && !scsiDeviceCheckReadOnly(scsi)) {
                if (scsi->length >= BUFFER_BLOCK_SIZE) {
                    *blocks = scsi->length - BUFFER_BLOCK_SIZE;
                    counter = BUFFER_SIZE;
                } else {
                    counter = scsi->length * 512;
                }
                *phase = DataOut;
                return counter;
            }
            return 0;

        case SCSIOP_READ_CAPACITY:
            SCSILOG("ReadCapacity\n");
            counter = scsiDeviceReadCapacity(scsi);
            if (counter) {
                *phase = DataIn;
            }
            return counter;

        case SCSIOP_SEEK10:
            SCSILOG1("Seek10: %d\n", scsi->sector);
            ledSetHd(1);
            scsi->length = 1;
            scsiDeviceCheckAddress(scsi);
            return 0;

#ifdef USE_SPECIALCOMMAND
        case SCSIOP_BLUE_MSX:
            SCSILOG("blueMSX\n");
            counter = scsiDeviceBlueMSX(scsi);
            if (counter) {
                *phase = DataIn;
            }
            return counter;
#endif
        }
    }

    SCSILOG1("unsupport command %x\n", cdb[0]);
    scsi->keycode = SENSE_INVALID_COMMAND_CODE;
    return 0;
}

int scsiDeviceExecutingCmd(SCSIDEVICE* scsi, SCSI_PHASE* phase, int* blocks)
{
    int result = 0;

    if (archCdromIsXferComplete(scsi->cdrom, &result)) {
        *phase = result ? DataIn : Status;
    } else {
        *phase = Execute;
    }
    *blocks = 0;
    return result;
}

UInt8 scsiDeviceMsgIn(SCSIDEVICE* scsi)
{
    UInt8 result = (UInt8)(scsi->message & 0xff);
    scsi->message = 0;
    return result;
}

/*
scsiDeviceMsgOut()
Notes:
    [out]
          -1: Busfree demand. (Please process it in the call origin.)
        bit2: Status phase demand. Error happend.
        bit1: Make it to a busfree if ATN has not been released.
        bit0: There is a message(MsgIn).
*/
int scsiDeviceMsgOut(SCSIDEVICE* scsi, UInt8 value)
{
    SCSILOG2("SCSI #%d message out: %x\n", scsi->scsiId, value);
    if (value & 0x80) {
        scsi->lun = value & 7;
        return 0;
    }

    switch (value) {
    case MSG_INITIATOR_DETECT_ERROR:
        scsi->keycode = SENSE_INITIATOR_DETECTED_ERR;
        return 6;

    case MSG_BUS_DEVICE_RESET:
        scsiDeviceBusReset(scsi);
    case MSG_ABORT:
        return -1;

    case MSG_REJECT:
    case MSG_PARITY_ERROR:
    case MSG_NO_OPERATION:
        return 2;
    }
    scsi->message = MSG_REJECT;
    return ((value >= 0x04) && (value <= 0x11)) ? 3 : 1;
}

void scsiDeviceSaveState(SCSIDEVICE* scsi)
{
    SaveState* state = saveStateOpenForWrite("scsidevice");

    saveStateSet(state, "enabled",    scsi->enabled);
    saveStateSet(state, "deviceType", scsi->deviceType);
    saveStateSet(state, "mode",       scsi->mode);
    saveStateSet(state, "reset",      scsi->reset);
    saveStateSet(state, "motor",      scsi->motor);
    saveStateSet(state, "keycode",    scsi->keycode);
    saveStateSet(state, "inserted",   scsi->inserted);
    saveStateSet(state, "changed",    scsi->changed);
    saveStateSet(state, "sector",     scsi->sector);
    saveStateSet(state, "sectorSize", scsi->sectorSize);
    saveStateSet(state, "length",     scsi->length);
    saveStateSet(state, "lun",        scsi->lun);
    saveStateSet(state, "message",    scsi->message);

    saveStateSetBuffer(state, "cdb", scsi->cdb, 12);
    saveStateSetBuffer(state, "fileName", scsi->disk.fileName, strlen(scsi->disk.fileName) + 1);
    saveStateSetBuffer(state, "fileNameInZip", scsi->disk.fileNameInZip, strlen(scsi->disk.fileNameInZip) + 1);
    saveStateClose(state);

    if (scsi->deviceType == SDT_CDROM) {
        archCdromSaveState(scsi->cdrom);
    }
}

void scsiDeviceLoadState(SCSIDEVICE* scsi)
{
    SaveState* state = saveStateOpenForRead("scsidevice");

    scsi->enabled    = saveStateGet(state, "enabled",    1);
    scsi->deviceType = saveStateGet(state, "deviceType", 0);
    scsi->mode       = saveStateGet(state, "mode",       MODE_UNITATTENTION);
    scsi->reset      = saveStateGet(state, "reset",      0);
    scsi->motor      = saveStateGet(state, "motor",      1);
    scsi->keycode    = saveStateGet(state, "keycode",    0);
    scsi->inserted   = saveStateGet(state, "inserted",   0);
    scsi->changed    = saveStateGet(state, "changed",    1);
    scsi->sector     = saveStateGet(state, "sector",     0);
    scsi->sectorSize = saveStateGet(state, "sectorSize", 512);
    scsi->length     = saveStateGet(state, "length",     0);
    scsi->lun        = saveStateGet(state, "lun",        0);
    scsi->message    = saveStateGet(state, "message",    0);

    saveStateGetBuffer(state, "cdb", scsi->cdb, 12);
    saveStateGetBuffer(state, "fileName", scsi->disk.fileName, sizeof(scsi->disk.fileName));
    saveStateGetBuffer(state, "fileNameInZip", scsi->disk.fileNameInZip, sizeof(scsi->disk.fileNameInZip));
    saveStateClose(state);

    scsi->changeCheck2  = scsi->mode & MODE_CHECK2;

    if (scsi->deviceType == SDT_CDROM) {
        archCdromLoadState(scsi->cdrom);
    }
}

