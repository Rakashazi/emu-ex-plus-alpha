/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/HarddiskIDE.c,v $
**
** $Revision: 1.11 $
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
#include "HarddiskIDE.h"
#include "Board.h"
#include "SaveState.h"
#include "Disk.h"
#include <stdlib.h>
#include <string.h>

#define STATUS_ERR  0x01
#define STATUS_DRQ  0x08
#define STATUS_DSC  0x10
#define STATUS_DRDY 0x40



struct HarddiskIde {
    UInt8 errorReg;
    UInt8 sectorCountReg;
    UInt8 sectorNumReg;
    UInt8 cylinderLowReg;
    UInt8 cylinderHighReg;
    UInt8 devHeadReg;
    UInt8 statusReg;
    UInt8 featureReg;
    
    int transferRead;
    int transferWrite;
    UInt32 transferCount;
    UInt32 transferSectorNumber;

    int   sectorDataOffset;
    UInt8 sectorData[512 * 256];

    int diskId;
};


static UInt32 getSectorNumber(HarddiskIde* hd)
{
    return hd->sectorNumReg | (hd->cylinderLowReg << 8) |
        (hd->cylinderHighReg << 16) | ((hd->devHeadReg & 0x0f) << 24);
}

static void setError(HarddiskIde* hd, UInt8 error)
{
    hd->errorReg = error;
    hd->statusReg |= STATUS_ERR;
    hd->statusReg &= ~STATUS_DRQ;
    hd->transferWrite = 0;
    hd->transferRead = 0;
}

static UInt32 getNumSectors(HarddiskIde* hd)
{
    return hd->sectorCountReg == 0 ? 256 : hd->sectorCountReg;
}

static void executeCommand(HarddiskIde* hd, UInt8 cmd)
{
    hd->statusReg &= ~(STATUS_DRQ | STATUS_ERR);
    hd->transferRead = 0;
    hd->transferWrite = 0;
    switch (cmd) {
    case 0xef: // Set Feature
        if (hd->featureReg != 0x03) {
            setError(hd, 0x04);
        }
        break;

    case 0xec: // ATA Identify Device
        if (diskReadSector(hd->diskId, hd->sectorData, -1, 0, 0, 0, NULL) != DSKE_OK) {
            setError(hd, 0x44);
            break;
        }
        hd->transferCount = 512/2;
        hd->sectorDataOffset = 0;
        hd->transferRead = 1;
        hd->statusReg |= STATUS_DRQ;
        break;

    case 0x91:
        break;

    case 0xf8: {
        UInt32 sectorCount = diskGetSectorsPerTrack(hd->diskId);
	    hd->sectorNumReg    = (UInt8)((sectorCount >>  0) & 0xff);
	    hd->cylinderLowReg  = (UInt8)((sectorCount >>  8) & 0xff);
	    hd->cylinderHighReg = (UInt8)((sectorCount >> 16) & 0xff);
	    hd->devHeadReg      = (UInt8)((sectorCount >> 24) & 0x0f);
        break;
    }
    case 0x30: { // Write Sector
        int sectorNumber = getSectorNumber(hd);
        int numSectors = getNumSectors(hd);
        if ((sectorNumber + numSectors) > diskGetSectorsPerTrack(hd->diskId)) {
            setError(hd, 0x14);
            break;
        }
        hd->transferSectorNumber = sectorNumber;
        hd->transferCount = 512/2 * numSectors;
        hd->sectorDataOffset = 0;
        hd->transferWrite = 1;
        hd->statusReg |= STATUS_DRQ;
        break;
    }
    case 0x20: { // Read Sector
        int sectorNumber = getSectorNumber(hd);
        int numSectors = getNumSectors(hd);
        int i;

        if ((sectorNumber + numSectors) > diskGetSectorsPerTrack(hd->diskId)) {
            setError(hd, 0x14);
            break;
        }
          
        for (i = 0; i < numSectors; i++) {
            if (diskReadSector(hd->diskId, hd->sectorData + i * 512, sectorNumber + i + 1, 0, 0, 0, NULL) != DSKE_OK) {
                break;
            }
        }
        if (i != numSectors) {
            setError(hd, 0x44);
            break;
        }

        hd->transferCount = 512/2 * numSectors;
        hd->sectorDataOffset = 0;
        hd->transferRead = 1;
        hd->statusReg |= STATUS_DRQ;
        break;
    }
    default: // all others
        setError(hd, 0x04);
    }
}

void harddiskIdeReset(HarddiskIde* hd)
{
    hd->errorReg = 0x01;
    hd->sectorCountReg = 0x01;
    hd->sectorNumReg = 0x01;
    hd->cylinderLowReg = 0x00;
    hd->cylinderHighReg = 0x00;
    hd->devHeadReg = 0x00;
    hd->statusReg = STATUS_DSC | STATUS_DRDY;
    hd->featureReg = 0x00;
    hd->transferRead = 0;
    hd->transferWrite = 0;
}

UInt16 harddiskIdeRead(HarddiskIde* hd)
{
    UInt16 value;

    if (!hd->transferRead || !diskPresent(hd->diskId)) {
        return 0x7f7f;
    }

    value  = hd->sectorData[hd->sectorDataOffset++];
    value |= hd->sectorData[hd->sectorDataOffset++] << 8;
    if (--hd->transferCount == 0) {
        hd->transferRead = 0;
        hd->statusReg &= ~STATUS_DRQ;
    }
    return value;
}

UInt16 harddiskIdePeek(HarddiskIde* hd)
{
    UInt16 value;

    if (!hd->transferRead || !diskPresent(hd->diskId)) {
        return 0x7f7f;
    }

    value  = hd->sectorData[hd->sectorDataOffset];
    value |= hd->sectorData[hd->sectorDataOffset + 1] << 8;

    return value;
}

void harddiskIdeWrite(HarddiskIde* hd, UInt16 value)
{
    if (!hd->transferWrite || !diskPresent(hd->diskId)) {
        return;
    }

    hd->sectorData[hd->sectorDataOffset++] = value & 0xff;
    hd->sectorData[hd->sectorDataOffset++] = value >> 8;
    hd->transferCount--;
    if ((hd->transferCount & 255) == 0) {
        if (!diskWriteSector(hd->diskId, hd->sectorData, hd->transferSectorNumber + 1, 0, 0, 0)) {
            setError(hd, 0x44);
            hd->transferWrite = 0;
            return;
        }
        hd->transferSectorNumber++;
        hd->sectorDataOffset = 0;
    }
    if (hd->transferCount == 0) {
        hd->transferWrite = 0;
        hd->statusReg &= ~STATUS_DRQ;
    }
}

UInt8 harddiskIdeReadRegister(HarddiskIde* hd, UInt8 reg)
{
    if (!diskPresent(hd->diskId)) {
        return 0x7f;
    }

    switch (reg) {
    case 1:
        return hd->errorReg;
    case 2: 
        return hd->sectorCountReg;
    case 3: 
        return hd->sectorNumReg;
    case 4: 
        return hd->cylinderLowReg;
    case 5: 
        return hd->cylinderHighReg;
    case 6: 
        return hd->devHeadReg;
    case 7: 
        return hd->statusReg;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 15:
        return 0x7f;
    }

    return 0x7f;
}

UInt8 harddiskIdePeekRegister(HarddiskIde* hd, UInt8 reg)
{
    return harddiskIdeReadRegister(hd, reg);
}

void harddiskIdeWriteRegister(HarddiskIde* hd, UInt8 reg, UInt8 value)
{
    if (!diskPresent(hd->diskId)) {
        return;
    }
    switch (reg) {
    case 1:
        hd->featureReg = value;
        break;
    case 2:
        hd->sectorCountReg = value;
        break;
    case 3:
        hd->sectorNumReg = value;
        break;
    case 4:
        hd->cylinderLowReg = value;
        break;
    case 5:
        hd->cylinderHighReg = value;
        break;
    case 6:
        hd->devHeadReg = value;
        break;
    case 7:
        executeCommand(hd, value);
        break;
    }
}

void harddiskIdeLoadState(HarddiskIde* ide)
{
    SaveState* state = saveStateOpenForRead("harddiskIde");

    ide->errorReg               = (UInt8)saveStateGet(state, "errorReg",        0);
    ide->sectorCountReg         = (UInt8)saveStateGet(state, "sectorCountReg",  0);
    ide->sectorNumReg           = (UInt8)saveStateGet(state, "sectorNumReg",    0);
    ide->cylinderLowReg         = (UInt8)saveStateGet(state, "cylinderLowReg",  0);
    ide->cylinderHighReg        = (UInt8)saveStateGet(state, "cylinderHighReg", 0);
    ide->devHeadReg             = (UInt8)saveStateGet(state, "devHeadReg",      0);
    ide->statusReg              = (UInt8)saveStateGet(state, "statusReg",       0);
    ide->featureReg             = (UInt8)saveStateGet(state, "featureReg",      0);
    ide->transferRead           = saveStateGet(state, "transferRead",           0);
    ide->transferWrite          = saveStateGet(state, "transferWrite",          0);
    ide->transferCount          = saveStateGet(state, "transferCount",          0);
    ide->transferSectorNumber   = saveStateGet(state, "transferSectorNumber",   0);

    saveStateClose(state);
}

void harddiskIdeSaveState(HarddiskIde* ide)
{
    SaveState* state = saveStateOpenForWrite("harddiskIde");

    saveStateSet(state, "errorReg",               ide->errorReg);
    saveStateSet(state, "sectorCountReg",         ide->sectorCountReg);
    saveStateSet(state, "sectorNumReg",           ide->sectorNumReg);
    saveStateSet(state, "cylinderLowReg",         ide->cylinderLowReg);
    saveStateSet(state, "cylinderHighReg",        ide->cylinderHighReg);
    saveStateSet(state, "devHeadReg",             ide->devHeadReg);
    saveStateSet(state, "statusReg",              ide->statusReg);
    saveStateSet(state, "featureReg",             ide->featureReg);
    saveStateSet(state, "transferRead",           ide->transferRead);
    saveStateSet(state, "transferWrite",          ide->transferWrite);
    saveStateSet(state, "transferCount",          ide->transferCount);
    saveStateSet(state, "transferSectorNumber",   ide->transferSectorNumber);

    saveStateClose(state);
}

HarddiskIde* harddiskIdeCreate(int diskId)
{
    HarddiskIde* hd = malloc(sizeof(HarddiskIde));

    hd->diskId = diskId;
    hd->transferRead = 0;
    hd->transferWrite = 0;

    harddiskIdeReset(hd);

    return hd;
}

void harddiskIdeDestroy(HarddiskIde* hd)
{
    free(hd);
}
