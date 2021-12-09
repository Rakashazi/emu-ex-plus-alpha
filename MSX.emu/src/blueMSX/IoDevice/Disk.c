/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Disk.c,v $
**
** $Revision: 1.25 $
**
** $Date: 2009-07-18 15:08:04 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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
#include "Disk.h"
#include "DirAsDisk.h"
#include "ziphelper.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>


// PacketFileSystem.h Need to be included after all other includes
#include "PacketFileSystem.h"

#define MAXSECTOR (2 * 9 * 81) 

#define DISK_ERRORS_HEADER      "DiskImage errors\r\n\032"
#define DISK_ERRORS_HEADER_SIZE 0x14
#define DISK_ERRORS_SIZE        ((MAXSECTOR+7)/8)

static int   drivesEnabled[MAXDRIVES] = { 1, 1 };
static int   drivesIsCdrom[MAXDRIVES];
static FILE* drives[MAXDRIVES];
static int   RdOnly[MAXDRIVES];
static char* ramImageBuffer[MAXDRIVES];
static int   ramImageSize[MAXDRIVES];
static int   sectorsPerTrack[MAXDRIVES];
static int   sectorSize[MAXDRIVES];
static int   fileSize[MAXDRIVES];
static int   sides[MAXDRIVES];
static int   tracks[MAXDRIVES];
static int   changed[MAXDRIVES];
static int   diskType[MAXDRIVES];
static int   maxSector[MAXDRIVES];
static char* drivesErrors[MAXDRIVES];

enum { MSX_DISK, SVI328_DISK, IDEHD_DISK } diskTypes;

static const UInt8 hdIdentifyBlock[512] = {
    0x5a,0x0c,0xba,0x09,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x3f,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x31,0x20,0x00,0x00,0x00,0x01,0x04,0x00,0x31,0x56,
    0x30,0x2e,0x20,0x20,0x20,0x20,0x6c,0x62,0x65,0x75,0x53,0x4d,0x00,0x58,0x48,0x20,
    0x52,0x41,0x20,0x44,0x49,0x44,0x4b,0x53,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x10,0x80,
    0x00,0x00,0x00,0x0b,0x00,0x00,0x00,0x02,0x00,0x02,0x03,0x00,0xba,0x09,0x10,0x00,
    0x3f,0x00,0x60,0x4c,0x26,0x00,0x00,0x00,0xe0,0x53,0x26,0x00,0x07,0x00,0x07,0x04,
    0x03,0x00,0x78,0x00,0x78,0x00,0xf0,0x00,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const UInt8 svi328Cpm80track[] = "CP/M-80";

static void diskReadHdIdentifySector(int driveId, UInt8* buffer)
{
    UInt32 totalSectors = fileSize[driveId] / 512;
    UInt16 heads = 16;
    UInt16 sectors = 32;
    UInt16 cylinders = (UInt16)(totalSectors / (heads * sectors));
    
    memcpy(buffer, hdIdentifyBlock, 512);

    buffer[0x02] = cylinders & 0xff;
    buffer[0x03] = cylinders >> 8;
    buffer[0x06] = heads & 0xff;
    buffer[0x07] = heads >> 8;
    buffer[0x0c] = sectors & 0xff;
    buffer[0x0d] = sectors >> 8;
    buffer[0x78] = (UInt8)((totalSectors & 0x000000ff) >>  0);
    buffer[0x79] = (UInt8)((totalSectors & 0x0000ff00) >>  8);
    buffer[0x7a] = (UInt8)((totalSectors & 0x00ff0000) >> 16);
    buffer[0x7b] = (UInt8)((totalSectors & 0xff000000) >> 24);
}


UInt8 diskEnabled(int driveId)
{
    return driveId >= 0 && driveId < MAXDRIVES && drivesEnabled[driveId];
}

int diskIsCdrom(int driveId)
{
    return driveId >= 0 && driveId < MAXDRIVES && drivesIsCdrom[driveId];
}


UInt8 diskReadOnly(int driveId)
{
    if (!diskPresent(driveId)) {
        return 0;
    }
    return RdOnly[driveId];
}

void  diskEnable(int driveId, int enable)
{
    if (driveId >= 0 && driveId < MAXDRIVES)
        drivesEnabled[driveId] = enable;
}

UInt8 diskPresent(int driveId)
{
    return driveId >= 0 && driveId < MAXDRIVES && 
        (drives[driveId] != NULL || ramImageBuffer[driveId] != NULL);
}

int diskGetSectorsPerTrack(int driveId)
{
    if (driveId < MAXDRIVES)
        return sectorsPerTrack[driveId];

    return 0;
}

int diskGetSides(int driveId)
{
    if (driveId < MAXDRIVES)
        return sides[driveId];

    return 0;
}

int diskGetSectorSize(int driveId, int side, int track, int density)
{
    int secSize;

    if (driveId >= MAXDRIVES)
        return 0;

    if (diskType[driveId] == SVI328_DISK) {
        secSize = (track==0 && side==0 && density==1) ? 128 : 256;
    }
    else {
        secSize = sectorSize[driveId];
    }

    return secSize;
}

static int diskGetSectorOffset(int driveId, int sector, int side, int track, int density)
{
    int offset;
    int secSize;

    if (driveId >= MAXDRIVES)
        return 0;

    secSize = diskGetSectorSize(driveId, side, track, density);

    if (diskType[driveId] == SVI328_DISK) {
        if (track==0 && side==0 && density==1)
            offset = (sector-1)*128; 
        else
            offset = ((track*sides[driveId]+side)*17+sector-1)*256-2048;
    }
    else {
        offset =  sector - 1 + diskGetSectorsPerTrack(driveId) * (track * diskGetSides(driveId) + side);
        offset *= secSize;
    }
    return offset;
}

int diskChanged(int driveId)
{
    if (driveId < MAXDRIVES) {
        int isChanged = changed[driveId];
        changed[driveId] = 0;

        return isChanged;
    }

    return 0;
}

static DSKE diskReadError(int driveId, int sector)
{
    if( drivesErrors[driveId] == NULL ) {
        return DSKE_OK;
    }else{
        return (drivesErrors[driveId][sector >> 3] & (0x80 >> (sector & 7)))?
               DSKE_CRC_ERROR : DSKE_OK;
    }
}

DSKE diskRead(int driveId, UInt8* buffer, int sector)
{
    if (!diskPresent(driveId))
        return DSKE_NO_DATA;

    if (ramImageBuffer[driveId] != NULL) {
        int offset = sector * sectorSize[driveId];

        if (ramImageSize[driveId] < offset + sectorSize[driveId]) {
            return DSKE_NO_DATA;
        }

        memcpy(buffer, ramImageBuffer[driveId] + offset, sectorSize[driveId]);
        return DSKE_OK;
    }
    else {
        if ((drives[driveId] != NULL)) {
            if (0 == fseek(drives[driveId], sector * sectorSize[driveId], SEEK_SET)) {
                UInt8 success = fread(buffer, 1, sectorSize[driveId], drives[driveId]) == sectorSize[driveId];
                return success? diskReadError(driveId, sector) : DSKE_NO_DATA;
            }
        }
    }
    return DSKE_NO_DATA;
}

DSKE diskReadSector(int driveId, UInt8* buffer, int sector, int side, int track, int density, int *sectorSize)
{
    int secSize;
    int offset;

    if (!diskPresent(driveId))
        return DSKE_NO_DATA;

    if (diskType[driveId] == IDEHD_DISK && sector == -1) {
        diskReadHdIdentifySector(driveId, buffer);
        return DSKE_OK;
    }

    offset = diskGetSectorOffset(driveId, sector, side, track, density);
    secSize = diskGetSectorSize(driveId, side, track, density);

    if (sectorSize != NULL) {
        *sectorSize = secSize;
    }

    if (ramImageBuffer[driveId] != NULL) {
        int sectornum;
        if (ramImageSize[driveId] < offset + secSize) {
            return DSKE_NO_DATA;
        }

        memcpy(buffer, ramImageBuffer[driveId] + offset, secSize);
        sectornum = sector - 1 + diskGetSectorsPerTrack(driveId) * (track * diskGetSides(driveId) + side);
        return diskReadError(driveId, sectornum);
    }
    else {
        if ((drives[driveId] != NULL)) {
            if (0 == fseek(drives[driveId], offset, SEEK_SET)) {
                UInt8 success = fread(buffer, 1, secSize, drives[driveId]) == secSize;
                int sectornum = sector - 1 + diskGetSectorsPerTrack(driveId) * (track * diskGetSides(driveId) + side);
                return success? diskReadError(driveId, sectornum) : DSKE_NO_DATA;
            }
        }
    }

    return DSKE_NO_DATA;
}

static int isSectorSize256(const UInt8* buf)
{
    // This implementation is quite rough, but it assmues that a disk with
    // 256 sectors have content in sector 1, while a 512 sector disk has
    // no data in the second half of the boot sector.
    UInt8 rv = 0;
    int cnt = 0xc0;
    buf += 0x120;

    while (cnt--) {
        rv |= *buf++;
    }
    return rv != 0;
}

static void diskUpdateInfo(int driveId) 
{
	UInt8 buf[512];
    int secSize;
    DSKE rv;

    sectorsPerTrack[driveId] = 9;
    sides[driveId]           = 2;
    tracks[driveId]          = 80;
    changed[driveId]         = 1;
    sectorSize[driveId]      = 512;
    diskType[driveId]        = MSX_DISK;
    maxSector[driveId]       = MAXSECTOR;

    if (fileSize[driveId] >= 1024 * 1024) {
        // HD image
        sectorsPerTrack[driveId] = fileSize[driveId] / 512;
        tracks[driveId]          = 1;
        changed[driveId]         = 1;
        sides[driveId]           = 1;
        diskType[driveId]        = IDEHD_DISK;
        maxSector[driveId]       = 99999999;
        return;
    }

    if (fileSize[driveId] / 512 == 1440) {
        return;
    }

    rv = diskReadSector(driveId, buf, 1, 0, 0, 512, &secSize);
    if (rv != DSKE_OK) {
        return;
    }

    switch (fileSize[driveId]) {
        case 163840:
            if (isSectorSize256(buf)) {
                sectorSize[driveId]      = 256;
	            sectorsPerTrack[driveId] = 16;
                tracks[driveId]          = 40;
	            sides[driveId]           = 1;
            }
            break;
        case 172032:  /* SVI-328 40 SS */
            sides[driveId] = 1;
            tracks[driveId] = 40;
            sectorsPerTrack[driveId] = 17;
            diskType[driveId] = SVI328_DISK;
            return;
        case 184320:  /* BW 12 SSDD */
            if (isSectorSize256(buf)) {
                sectorSize[driveId] = 256;
                sectorsPerTrack[driveId] = 18;
                tracks[driveId] = 40;
                sides[driveId] = 1;
            }
            return;
        case 204800:  /* Kaypro II SSDD */
            sectorSize[driveId] = 512;
            sectorsPerTrack[driveId] = 10;
            tracks[driveId] = 40;
            sides[driveId] = 1;
            return;
        case 346112:  /* SVI-328 40 DS/80 SS */
            sides[driveId] = 1;
            tracks[driveId] = 80;
            sectorsPerTrack[driveId] = 17;
            diskType[driveId] = SVI328_DISK;
            rv = diskReadSector(driveId, buf, 15, 0, 40, 0, &secSize);
            if (rv != DSKE_OK) {
                return;
            }
            // Is it formatted for 80 track Disk BASIC?
            if (buf[0] == 0xfe && buf[1] == 0xfe && buf[2] == 0xfe && buf[20] != 0xfe && buf[40] == 0xfe) {
            	return;
            }
            rv = diskReadSector(driveId, buf, 1, 0, 1, 0, &secSize);
            if (rv != DSKE_OK) {
                return;
            }
            // Is it sysgend for 80 track CP/M?
            if (memcmp(&buf[176], &svi328Cpm80track[0], strlen(svi328Cpm80track)) == 0) {
                rv = diskReadSector(driveId, buf, 2, 0, 0, 1, &secSize);
                if (rv != DSKE_OK) {
                    return;
                }
                if (buf[115] == 0x50 || buf[116] == 0x50) {
                    return;
                }
            }
            sides[driveId] = 2;
            tracks[driveId] = 40;
            return;
        case 348160:  /* SVI-728 DSDD (CP/M) */
            if (isSectorSize256(buf)) {
                sectorSize[driveId] = 256;
                sectorsPerTrack[driveId] = 17;
                tracks[driveId] = 40;
                sides[driveId] = 2;
            }
            return;
	}

    if (buf[0] ==0xeb) {
        switch (buf[0x15]) {
        case 0xf8:
	        sides[driveId]           = 1;
            tracks[driveId]          = 80;
	        sectorsPerTrack[driveId] = 9;
            return;
        case 0xf9:
	        sides[driveId]           = 2;
            tracks[driveId]          = 80;
	        sectorsPerTrack[driveId] = 9;
            // This check is needed to get the SVI-738 MSX-DOS disks to work
            // Maybe it should be applied to other cases as well
            rv = diskReadSector(driveId, buf, 2, 0, 0, 512, &secSize);
            if (rv == DSKE_OK && buf[0] == 0xf8) {
	            sides[driveId] = 1;
            }
            return;
        case 0xfa:
	        sides[driveId]           = 1;
            tracks[driveId]          = 80;
	        sectorsPerTrack[driveId] = 8;
            if (fileSize[driveId] == 368640) {
	            sectorsPerTrack[driveId] = 9;
            }
            return;
        case 0xfb:
	        sides[driveId]           = 2;
            tracks[driveId]          = 80;
	        sectorsPerTrack[driveId] = 8;
            return;
        case 0xfc:
	        sides[driveId]           = 1;
            tracks[driveId]          = 40;
	        sectorsPerTrack[driveId] = 9;
            return;
        case 0xfd:
	        sides[driveId]           = 2;
            tracks[driveId]          = 40;
	        sectorsPerTrack[driveId] = 9;
            return;
        case 0xfe:
	        sides[driveId]           = 1;
            tracks[driveId]          = 40;
	        sectorsPerTrack[driveId] = 8;
            return;
        case 0xff:
	        sides[driveId]           = 2;
            tracks[driveId]          = 40;
	        sectorsPerTrack[driveId] = 8;
            return;
        }
    }
    
    if ((buf[0] == 0xe9) || (buf[0] ==0xeb)) {
	    sectorsPerTrack[driveId] = buf[0x18] + 256 * buf[0x19];
	    sides[driveId]           = buf[0x1a] + 256 * buf[0x1b];
    }
    else {
        rv = diskReadSector(driveId, buf, 2, 0, 0, 512, &secSize);
        if (rv != DSKE_OK) {
            return;
        }
		if (buf[0] >= 0xF8) {
			sectorsPerTrack[driveId] = (buf[0] & 2) ? 8 : 9;
			sides[driveId]           = (buf[0] & 1) ? 2 : 1;
		}
    }

    if (sectorsPerTrack[driveId] == 0  || sides[driveId] == 0 || 
        sectorsPerTrack[driveId] > 255 || sides[driveId] > 2) 
    {
    	switch (fileSize[driveId]) {
        case 163840:
            sectorSize[driveId]      = 256;
	        sectorsPerTrack[driveId] = 16;
            tracks[driveId]          = 40;
	        sides[driveId]           = 1;
            break;
        case 327680:  /* 80 tracks, 1 side, 8 sectors/track */
	        sectorsPerTrack[driveId] = 8;
	        sides[driveId] = 1;
            break;
        case 368640:  /* 80 tracks, 1 side, 9 sectors/track */
	        sectorsPerTrack[driveId] = 9;
	        sides[driveId] = 1;
            break;
        case 655360:  /* 80 tracks, 2 side, 8 sectors/track */
	        sectorsPerTrack[driveId] = 8;
	        sides[driveId] = 2;
            break;
        default:
            sectorsPerTrack[driveId] = 9;
            sides[driveId]           = 2;
        }
    }
}

UInt8 diskWrite(int driveId, UInt8 *buffer, int sector)
{
    if (!diskPresent(driveId)) {
        return 0;
    }

    if (sector >= maxSector[driveId]) {
        return 0;
    }

    if (ramImageBuffer[driveId] != NULL) {
        int offset = sector * sectorSize[driveId];

        if (ramImageSize[driveId] < offset + sectorSize[driveId]) {
            return 0;
        }

        memcpy(ramImageBuffer[driveId] + offset, buffer, sectorSize[driveId]);
        return 1;
    }
    else {
        if (drives[driveId] != NULL && !RdOnly[driveId]) {
            if (0 == fseek(drives[driveId], sector * sectorSize[driveId], SEEK_SET)) {
                UInt8 success = fwrite(buffer, 1, sectorSize[driveId], drives[driveId]) == sectorSize[driveId];
                if (success && sector == 0) {
                    diskUpdateInfo(driveId);
                }
                return success;
            }
        }
    }
    return 0;
}

UInt8 diskWriteSector(int driveId, UInt8 *buffer, int sector, int side, int track, int density)
{
    int secSize;
    int offset;

    if (!diskPresent(driveId))
        return 0;

    if (sector >= maxSector[driveId])
        return 0;

    if (density == 0) {
        density = sectorSize[driveId];
    }

    offset = diskGetSectorOffset(driveId, sector, side, track, density);
    secSize = diskGetSectorSize(driveId, side, track, density);

    if (ramImageBuffer[driveId] != NULL) {
        if (ramImageSize[driveId] < offset + secSize) {
            return 0;
        }

        memcpy(ramImageBuffer[driveId] + offset, buffer, secSize);
        return 1;
    }
    else {
        if (drives[driveId] != NULL && !RdOnly[driveId]) {
            if (0 == fseek(drives[driveId], offset, SEEK_SET)) {
                UInt8 success = fwrite(buffer, 1, secSize, drives[driveId]) == secSize;
                return success;
            }
        }
    }
    return 0;
}

void diskSetInfo(int driveId, char* fileName, const char* fileInZipFile)
{
    drivesIsCdrom[driveId] = fileName && strcmp(fileName, DISK_CDROM) == 0;
}

static char *makeErrorsFileName(const char *fileName)
{
    char *p, *fname = (char*)malloc(strlen(fileName)+4);
    strcpy(fname, fileName);
    p = &fname[strlen(fname)-1];
    while(*p != '.' && p != fname) p--;
    if (p != fname) {
        strcpy(p, ".der");
        return fname;
    }else{
        free(p);
        return NULL;
    }
}

UInt8 diskChange(int driveId, const char* fileName, const char* fileInZipFile)
{
    struct stat s;
    int rv;
    char *fname;

    if (driveId >= MAXDRIVES)
        return 0;

    drivesIsCdrom[driveId] = 0;

    /* Close previous disk image */
    if(drives[driveId] != NULL) { 
        fclose(drives[driveId]);
        drives[driveId] = NULL; 
    }

    if (ramImageBuffer[driveId] != NULL) {
        // Flush to file??
        free(ramImageBuffer[driveId]);
        ramImageBuffer[driveId] = NULL;
    }

    if (drivesErrors[driveId] != NULL) {
        free(drivesErrors[driveId]);
        drivesErrors[driveId] = NULL;
    }

    if(!fileName) {
        return 1;
    }

    if (strcmp(fileName, DISK_CDROM) == 0) {
        drivesIsCdrom[driveId] = 1;
        return 1;
    }

    rv = stat(fileName, &s);
    if (rv == 0) {
        if (s.st_mode & S_IFDIR) {
            ramImageBuffer[driveId] = dirLoadFile(DDT_MSX, fileName, &ramImageSize[driveId]);
            fileSize[driveId] = ramImageSize[driveId];
            diskUpdateInfo(driveId);
            return ramImageBuffer[driveId] != NULL;
        }
    }

    if (fileInZipFile != NULL) {
        ramImageBuffer[driveId] = zipLoadFile(fileName, fileInZipFile, &ramImageSize[driveId]);
        fileSize[driveId] = ramImageSize[driveId];

        fname = makeErrorsFileName(fileInZipFile);
        if( fname != NULL ) {
            int size=0;
            drivesErrors[driveId] = zipLoadFile(fileName, fname, &size);
            if( drivesErrors[driveId] != NULL && size > DISK_ERRORS_HEADER_SIZE &&
                strcmp(drivesErrors[driveId], DISK_ERRORS_HEADER)==0 ) {
                memcpy(drivesErrors[driveId],
                       drivesErrors[driveId] + DISK_ERRORS_HEADER_SIZE,
                       size - DISK_ERRORS_HEADER_SIZE);
            }
            free(fname);
        }

        diskUpdateInfo(driveId);
        return ramImageBuffer[driveId] != NULL;
    }

    drives[driveId] = fopenHelper(fileName, "r+b");
    RdOnly[driveId] = 0;

    if (drives[driveId] == NULL) {
        drives[driveId] = fopenHelper(fileName, "rb");
        RdOnly[driveId] = 1;
    }

    if (drives[driveId] == NULL) {
        return 0;
    }

    fname = makeErrorsFileName(fileName);
    if( fname != NULL ) {
        FILE *f = fopenHelper(fname, "rb");
        if( f != NULL ) {
            char *p = (char*)malloc(DISK_ERRORS_SIZE);
            if( fread(p, 1, DISK_ERRORS_HEADER_SIZE, f) == DISK_ERRORS_HEADER_SIZE ) {
                if( strcmp(p, DISK_ERRORS_HEADER) == 0 ) {
                    fread(p, 1, DISK_ERRORS_SIZE, f);
                    drivesErrors[driveId] = p;
                    p = NULL;
                }
            }
                        if( p != NULL ) {
                free(p);
            }
            fclose(f);
        }
        free(fname);
    }

    fseek(drives[driveId],0,SEEK_END);
    fileSize[driveId] = ftell(drives[driveId]);

    diskUpdateInfo(driveId);

    return 1;
}

/*
    optimized routine for ScsiDevice.c
*/
int _diskRead2(int driveId, UInt8* buffer, int sector, int numSectors)
{
    int length  = numSectors * 512;
    if (!diskPresent(driveId))
        return 0;

    if (ramImageBuffer[driveId] == NULL) {
        if ((drives[driveId] != NULL)) {
            if (0 == fseek(drives[driveId], sector * 512, SEEK_SET))
                return (fread(buffer, 1, length, drives[driveId]) == length);
        }
        return 0;
    }

    memcpy(buffer, ramImageBuffer[driveId] + sector * 512, numSectors * 512);
    return 1;
}

/*
    optimized routine for ScsiDevice.c
*/
int _diskWrite2(int driveId, UInt8* buffer, int sector, int numSectors)
{
    int length  = numSectors * 512;
    if (!diskPresent(driveId))
        return 0;

    if (ramImageBuffer[driveId] == NULL) {
        if ((drives[driveId] != NULL)) {
            if (0 == fseek(drives[driveId], sector * 512, SEEK_SET))
                return (fwrite(buffer, 1, length, drives[driveId]) == length);
        }
        return 0;
    }

    memcpy(ramImageBuffer[driveId] + sector * 512, buffer, length);
    return 1;
}

/*
    for ScsiDevice.c
    corresponds to harddisk and floppy disk
*/
int _diskGetTotalSectors(int driveId)
{
    if ((diskPresent(driveId)) && (driveId < MAXDRIVES))
        return fileSize[driveId] / 512;
    return 0;
}
