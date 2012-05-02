/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Disk.h,v $
**
** $Revision: 1.20 $
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
#ifndef DISK_H
#define DISK_H

#include "MsxTypes.h"

#define MAX_FDC_COUNT            2

#define MAX_HD_COUNT             4
#define FIRST_INTERNAL_HD_INDEX  2

#define MAX_DRIVES_PER_HD        8

#define DISK_CDROM               "CD-Rom"

#define MAXDRIVES (MAX_FDC_COUNT + MAX_DRIVES_PER_HD * MAX_HD_COUNT)

typedef enum {
    DSKE_OK,
    DSKE_NO_DATA,
    DSKE_CRC_ERROR
} DSKE;

UInt8 diskChange(int driveId, const char* fileName, const char* fileInZipFile);
void diskSetInfo(int driveId, char* fileName, const char* fileInZipFile);
void  diskEnable(int driveId, int enable);
UInt8 diskEnabled(int driveId);
UInt8 diskReadOnly(int driveId);
UInt8 diskPresent(int driveId);
DSKE  diskRead(int driveId, UInt8* buffer, int sector);
DSKE  diskReadSector(int driveId, UInt8* buffer, int sector, int side, int track, int density, int *sectorSize);
UInt8 diskWrite(int driveId, UInt8* buffer, int sector);
UInt8 diskWriteSector(int driveId, UInt8 *buffer, int sector, int side, int track, int density);
int   diskGetSectorsPerTrack(int driveId);
int   diskGetSectorSize(int driveId, int side, int track, int density);
int   diskIsCdrom(int driveId);
int   diskGetSides(int driveId);
int   diskChanged(int driveId);
int   _diskRead2(int driveId, UInt8* buffer, int sector, int numSectors);
int   _diskWrite2(int driveId, UInt8* buffer, int sector, int numSectors);
int   _diskGetTotalSectors(int driveId);
static int diskGetHdDriveId(int hdId, int driveNo) {
    return MAX_FDC_COUNT + MAX_DRIVES_PER_HD * hdId + driveNo;
}
static int diskGetUsbDriveId(int driveId, int driveNo) { 
    return MAX_FDC_COUNT + MAX_DRIVES_PER_HD * driveId + driveNo;
}

#endif

