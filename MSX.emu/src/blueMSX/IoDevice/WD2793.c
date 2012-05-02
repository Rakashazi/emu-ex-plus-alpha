/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/WD2793.c,v $
**
** $Revision: 1.16 $
**
** $Date: 2009-07-18 15:08:04 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Ricardo Bittencourt
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
#include "WD2793.h"
#include "WDCRC.h"
#include "Board.h"
#include "SaveState.h"
#include "Disk.h"
#include "Led.h"
#include "FdcAudio.h"
#include <stdlib.h>
#include <string.h>

struct WD2793 {
	UInt8  regStatus;
	UInt8  regCommand;
	UInt8  regSector;
	UInt8  regTrack;
	UInt8  regData;
	int    immediateInt;
	int    intRequest;
	int    dataRequest;
	int    dataReady;
	int    stepDirection;
    int    step;
    UInt32 curStep;
    int    headLoaded;
    UInt32 dataRequsetTime;
    UInt32 stepTime;
    int    sectorOffset;
	int    dataAvailable;
    int    drive;
    UInt8  diskTrack;
    UInt8  diskTracks[4];
    int    diskSide;
	int    diskDensity;
    FdcAudio* fdcAudio;
    Wd2793FdcType type;
    UInt8  sectorBuf[512];
};

#define CMD_RESTORE         0x00
#define CMD_SEEK            0x01
#define CMD_STEP1           0x02
#define CMD_STEP2           0x03
#define CMD_STEP_IN1        0x04
#define CMD_STEP_IN2        0x05
#define CMD_STEP_OUT1       0x06
#define CMD_STEP_OUT2       0x07
#define CMD_READ_SECTOR     0x08
#define CMD_READ_SECTORS    0x09
#define CMD_WRITE_SECTOR    0x0a
#define CMD_WRITE_SECTORS   0x0b
#define CMD_READ_ADDRESS    0x0c
#define CMD_FORCE_INTERRUPT 0x0d
#define CMD_READ_TRACK      0x0e
#define CMD_WRITE_TRACK     0x0f

/* Type II commands status */
#define ST_BUSY             0x01
#define ST_INDEX            0x02
#define ST_DATA_REQUEST     0x02
#define ST_TRACK00          0x04
#define ST_LOST_DATA        0x04
#define ST_CRC_ERROR        0x08
#define ST_SEEK_ERROR       0x10
#define ST_RECORD_NOT_FOUND 0x10
#define ST_HEAD_LOADED      0x20
#define ST_RECORD_TYPE      0x20
#define ST_WRITE_PROTECTED  0x40
#define ST_NOT_READY        0x80

#define FLAG_HLD            0x08
#define FLAG_IMM            0x08
#define FLAG_DDM            0x10

static void wd2793ReadSector(WD2793* wd)
{
    DSKE rv = 0;
    int sectorSize = 0;

    if (wd->drive >= 0) {
		rv = diskReadSector(wd->drive, wd->sectorBuf, wd->regSector, wd->diskSide, wd->diskTrack, wd->diskDensity, &sectorSize);
        fdcAudioSetReadWrite(wd->fdcAudio);
        boardSetFdcActive();
    }
    if (rv == DSKE_NO_DATA || wd->diskTrack != wd->regTrack) {
	    wd->regStatus |= ST_RECORD_NOT_FOUND;
	    wd->intRequest = 1;
	    wd->regStatus &= ~ST_BUSY;
    }
    else {
        if (rv == DSKE_CRC_ERROR) {
            wd->regStatus |= ST_CRC_ERROR;
        }
		wd->sectorOffset    = 0;
		wd->dataRequest     = 0;
        wd->dataReady       = 0;
        wd->dataRequsetTime = boardSystemTime();
        wd->dataAvailable = sectorSize;
    }
}

static void sync(WD2793* wd)
{
    if (wd->step) {
        const UInt64 timePerStep[4] = { 200, 100, 66, 50 };

        UInt32 steps = (UInt32)(timePerStep[wd->regCommand & 3] * (boardSystemTime() - wd->stepTime) / boardFrequency());

        while (wd->curStep < steps) {
            wd->curStep++;
	        if ((wd->regCommand & 0x10) || ((wd->regCommand & 0xe0) == 0x00)) {
		        wd->regTrack += wd->stepDirection;
            }

            if (diskEnabled(wd->drive) && 
                ((wd->stepDirection == -1 && wd->diskTrack > 0) || wd->stepDirection == 1)) {
		        wd->diskTrack += wd->stepDirection;
            }
            if (wd->regCommand & 0xe0) {
	            wd->intRequest = 1;
	            wd->regStatus &= ~ST_BUSY;
                wd->step       = 0;
                break;
            } 
            if (wd->stepDirection == -1 && diskEnabled(wd->drive) && wd->diskTrack == 0) {
                wd->regTrack   = 0;
	            wd->intRequest = 1;
	            wd->regStatus &= ~ST_BUSY;
                wd->step       = 0;
                break;
            }
            
            if (wd->regTrack == wd->regData) {
	            wd->intRequest = 1;
	            wd->regStatus &= ~ST_BUSY;
                wd->step       = 0;
                break;
            }
        }
    }
}

static void commandType4(WD2793* wd)
{
	UInt8 flags = wd->regCommand & 0x0f;
	if (flags == 0x00) {
		wd->immediateInt = 0;
	}
	if (flags & FLAG_IMM) {
		wd->immediateInt = 1;
	}
	
	wd->dataRequest = 0;
	wd->regStatus  &= ~ST_BUSY;
}

static void commandType2(WD2793* wd)
{
	wd->regStatus  &= ~(ST_LOST_DATA   | ST_RECORD_NOT_FOUND | ST_RECORD_TYPE | ST_WRITE_PROTECTED);
	wd->regStatus  |= ST_BUSY;
    wd->headLoaded  = 1;
	wd->dataRequest = 0;

	if (!diskPresent(wd->drive)) {
	    wd->intRequest  = 1;
	    wd->regStatus &= ~ST_BUSY;
        return;
	} 

    switch (wd->regCommand >> 4) {
	case CMD_READ_SECTOR:
	case CMD_READ_SECTORS:
		wd2793ReadSector(wd);
		break;

	case CMD_WRITE_SECTOR:
	case CMD_WRITE_SECTORS:
		wd->sectorOffset  = 0;
		wd->dataRequest   = 1;
        wd->dataAvailable = diskGetSectorSize(wd->drive, wd->diskSide, wd->diskTrack, wd->diskDensity);
		break;
	}
}

static void commandType3(WD2793* wd)
{
	wd->regStatus  &= ~(ST_LOST_DATA | ST_RECORD_NOT_FOUND | ST_RECORD_TYPE);
	wd->regStatus  |= ST_BUSY;
    wd->headLoaded  = 1;
	wd->dataRequest = 0;
	wd->dataReady  = 0;

	if (!diskPresent(wd->drive)) {
	    wd->intRequest = 1;
	    wd->regStatus &= ~ST_BUSY;
        return;
	} 

	switch (wd->regCommand >> 4) {
	case CMD_READ_ADDRESS:
	case CMD_READ_TRACK:
	    wd->intRequest = 1;
	    wd->regStatus &= ~ST_BUSY;

	case CMD_WRITE_TRACK:
	    wd->dataRequest = 1;
		break;
	}
}

static void commandType1(WD2793* wd)
{
	wd->regStatus  &= ~(ST_SEEK_ERROR | ST_CRC_ERROR);
    wd->headLoaded  = wd->regCommand & FLAG_HLD;
	wd->regStatus  |= ST_BUSY;
	wd->dataRequest = 0;

	switch (wd->regCommand >> 4) {
	case CMD_RESTORE:
        wd->regTrack = 0xff;
		wd->regData  = 0x00;
        wd->stepDirection = -1;
		break;
		
	case CMD_SEEK:
        if (wd->regTrack == wd->regData) {
	        wd->intRequest = 1;
	        wd->regStatus &= ~ST_BUSY;
            return;
        }
        wd->stepDirection = wd->regTrack > wd->regData ? -1 : 1;
		break;
		
	case CMD_STEP1:
	case CMD_STEP2:
		break;
		
	case CMD_STEP_IN1:
	case CMD_STEP_IN2:
		wd->stepDirection = 1;
		break;
		
	case CMD_STEP_OUT1:
	case CMD_STEP_OUT2:
		wd->stepDirection = -1;
		break;
	}

    wd->step     = 1;
    wd->curStep  = 0;
    wd->stepTime = boardSystemTime();
}

int wd2793GetSide(WD2793* wd)
{
    return wd->diskSide;
}

void wd2793SetSide(WD2793* wd, int side)
{
    sync(wd);
    if (wd->type != FDC_TYPE_WD1772) {
        wd->diskSide = side;
    }
}

void wd2793SetDensity(WD2793* wd, int density)
{
	wd->diskDensity = density;
}

int wd2793GetDrive(WD2793* wd)
{
    return wd->drive;
}

void wd2793SetDrive(WD2793* wd, int drive)
{
    sync(wd);
    if (wd->drive != -1) {
        wd->diskTracks[wd->drive] = wd->diskTrack;
    }
    if (drive != -1) {
        wd->diskTrack = wd->diskTracks[drive];
    }
    wd->drive = drive;
}

void wd2793SetMotor(WD2793* wd, int motorOn)
{
    switch (wd->drive) {
    case 0:
        ledSetFdd1(motorOn);
        ledSetFdd2(0);
        break;
    case 1:
        ledSetFdd1(0);
        ledSetFdd2(motorOn);
        break;
    default:
        ledSetFdd1(0);
        ledSetFdd2(0);
        break;
    }

    fdcAudioSetMotor(wd->fdcAudio, diskEnabled(wd->drive));
}

int wd2793DiskChanged(WD2793* wd, int drive)
{
    return diskChanged(drive);
}

int wd2793PeekDataRequest(WD2793* wd)
{
    int dataRequest = wd->dataRequest;

	if (((wd->regCommand & 0xF0) == 0xF0) && ((wd->regStatus & ST_BUSY) || wd->dataReady)) {
        UInt32 pulses = (boardSystemTime() - wd->dataRequsetTime) / (boardFrequency() / 5);
		if (wd->dataReady) {
			dataRequest = 1;
		} 
		if (pulses > 1) {
			dataRequest   = 0;
		}
	}

    if ((wd->regCommand & 0xe0) == 0x80 && (wd->regStatus & ST_BUSY)) {
        UInt32 pulses = (boardSystemTime() - wd->dataRequsetTime) / (boardFrequency() / 25);
		if (wd->dataReady) {
			dataRequest = 1;
		}
    }

    return dataRequest;
}

int wd2793GetDataRequest(WD2793* wd)
{
    sync(wd);
	if (((wd->regCommand & 0xF0) == 0xF0) && ((wd->regStatus & ST_BUSY) || wd->dataReady)) {
        UInt32 pulses = (boardSystemTime() - wd->dataRequsetTime) / (boardFrequency() / 5);
		if (wd->dataReady) {
			wd->dataRequest = 1;
		} 
		if (pulses > 0) {
			wd->dataReady = 1;
		}
		if (pulses > 1) {
			wd->dataAvailable = 0;
			wd->sectorOffset  = 0;
			wd->dataRequest   = 0;
	        wd->intRequest    = 1;
	        wd->regStatus    &= ~ST_BUSY;
		}
	}

    if ((wd->regCommand & 0xe0) == 0x80 && (wd->regStatus & ST_BUSY)) {
        UInt32 pulses = (boardSystemTime() - wd->dataRequsetTime) / (boardFrequency() / 25);
		if (wd->dataReady) {
			wd->dataRequest = 1;
		}
		if (pulses > 0) {
            wd->dataReady = 1;
        }
    }

    return wd->dataRequest;
}

int wd2793PeekIrq(WD2793* wd)
{
	return wd->intRequest;
}

int wd2793GetIrq(WD2793* wd)
{
    sync(wd);
	return wd->intRequest;
}

void wd2793SetTrackReg(WD2793* wd, UInt8 value)
{
    sync(wd);
	wd->regTrack = value;
}

UInt8 wd2793PeekTrackReg(WD2793* wd)
{
    sync(wd);
	return wd->regTrack;
}

UInt8 wd2793GetTrackReg(WD2793* wd)
{
    sync(wd);
	return wd->regTrack;
}

void wd2793SetSectorReg(WD2793* wd, UInt8 value)
{
    sync(wd);
	wd->regSector = value;
}

UInt8 wd2793PeekSectorReg(WD2793* wd)
{
    sync(wd);
	return wd->regSector;
}

UInt8 wd2793GetSectorReg(WD2793* wd)
{
    sync(wd);
	return wd->regSector;
}

UInt8 wd2793PeekDataReg(WD2793* wd)
{
    UInt8 regData;

    sync(wd);
	if (((wd->regCommand & 0xe0) == 0x80) && (wd->regStatus & ST_BUSY)) {
		regData = wd->sectorBuf[wd->sectorOffset];
	}
	return wd->regData;
}

UInt8 wd2793GetDataReg(WD2793* wd)
{
    sync(wd);
	if (((wd->regCommand & 0xe0) == 0x80) && (wd->regStatus & ST_BUSY)) {
		wd->regData = wd->sectorBuf[wd->sectorOffset];
		wd->sectorOffset++;
        if (wd->dataAvailable) {
    		wd->dataAvailable--;
        }
		if (wd->dataAvailable == 0) {
			if (!(wd->regCommand & FLAG_DDM)) {
				wd->regStatus &= ~(ST_BUSY | ST_DATA_REQUEST);
				wd->dataRequest = 0;
	            wd->intRequest  = 1;
			} else {
				wd->regSector++;
				wd2793ReadSector(wd);
			}
		}
	}
	return wd->regData;
}

void wd2793SetDataReg(WD2793* wd, UInt8 value)
{
    sync(wd);
	wd->regData = value;
	if ((wd->regCommand & 0xE0) == 0xA0) {
		wd->sectorBuf[wd->sectorOffset] = value;
		wd->sectorOffset++;
        if (wd->dataAvailable) {
		    wd->dataAvailable--;
        }
		if (wd->dataAvailable == 0) {
            int rv = 0;
            if (wd->drive >= 0) {
                wd->dataRequsetTime = boardSystemTime();
                rv = diskWriteSector(wd->drive, wd->sectorBuf, wd->regSector, wd->diskSide, wd->diskTrack, wd->diskDensity);
                fdcAudioSetReadWrite(wd->fdcAudio);
                boardSetFdcActive();
            }
			wd->sectorOffset  = 0;
            wd->dataAvailable = diskGetSectorSize(wd->drive, wd->diskSide, wd->diskTrack, wd->diskDensity);
			if (!rv || wd->diskTrack != wd->regTrack) {
				wd->regStatus |= ST_RECORD_NOT_FOUND;
	            wd->intRequest = 1;
	            wd->regStatus &= ~ST_BUSY;
				return;
			}
			wd->regStatus &= ~(ST_BUSY | ST_DATA_REQUEST);
			if (!(wd->regCommand & FLAG_DDM)) {
	            wd->intRequest  = 1;
				wd->dataRequest = 0;
			}
		}
	}
}

UInt8 wd2793PeekStatusReg(WD2793* wd)
{
    UInt8 regStatus;

    sync(wd);

    regStatus = wd->regStatus;

	if (((wd->regCommand & 0x80) == 0) || ((wd->regCommand & 0xf0) == 0xd0)) {
		regStatus &= ~(ST_INDEX | ST_TRACK00 | ST_HEAD_LOADED | ST_WRITE_PROTECTED);
    	if (diskEnabled(wd->drive)) {
            if (diskPresent(wd->drive)) {
                if ((UInt64)160 * boardSystemTime() / boardFrequency() & 0x1e) {
			        regStatus |= ST_INDEX;
		        }
            }
		    if (wd->diskTrack == 0) {
			    regStatus |=  ST_TRACK00;
		    }
            if (wd->headLoaded) {
    		    regStatus |=  ST_HEAD_LOADED;
            }
        }
        else {
    		regStatus |= ST_WRITE_PROTECTED;
        }
	} 
    else {
		if (wd2793PeekDataRequest(wd)) {
			regStatus |=  ST_DATA_REQUEST;
		} 
        else {
			regStatus &= ~ST_DATA_REQUEST;
		}
	}

	if (diskPresent(wd->drive)) {
		regStatus &= ~ST_NOT_READY;
	} 
    else {
		regStatus |=  ST_NOT_READY;
	}

	return regStatus;
}

UInt8 wd2793GetStatusReg(WD2793* wd)
{
    sync(wd);
	if (((wd->regCommand & 0x80) == 0) || ((wd->regCommand & 0xf0) == 0xd0)) {
		wd->regStatus &= ~(ST_INDEX | ST_TRACK00 | ST_HEAD_LOADED | ST_WRITE_PROTECTED);
    	if (diskEnabled(wd->drive)) {
            if (diskPresent(wd->drive)) {
                if ((UInt64)160 * boardSystemTime() / boardFrequency() & 0x1e) {
			        wd->regStatus |= ST_INDEX;
		        }
            }
		    if (wd->diskTrack == 0) {
			    wd->regStatus |=  ST_TRACK00;
		    }
            if (wd->headLoaded) {
    		    wd->regStatus |=  ST_HEAD_LOADED;
            }
        }
        else {
    		wd->regStatus |= ST_WRITE_PROTECTED;
        }
	} 
    else {
		if (wd2793GetDataRequest(wd)) {
			wd->regStatus |=  ST_DATA_REQUEST;
		} 
        else {
			wd->regStatus &= ~ST_DATA_REQUEST;
		}
	}

	if (diskPresent(wd->drive)) {
		wd->regStatus &= ~ST_NOT_READY;
	} 
    else {
		wd->regStatus |=  ST_NOT_READY;
	}

    wd->intRequest = wd->immediateInt;

	return wd->regStatus;
}

void wd2793SetCommandReg(WD2793* wd, UInt8 value)
{
    sync(wd);
	wd->regCommand = value;
	wd->intRequest = wd->immediateInt;

//    printf("WD2793 Cmd : %d\n", wd->regCommand >> 4);
	switch (wd->regCommand >> 4) {
	case CMD_RESTORE:
	case CMD_SEEK:
	case CMD_STEP1:
	case CMD_STEP2:
	case CMD_STEP_IN1:
	case CMD_STEP_IN2:
	case CMD_STEP_OUT1:
	case CMD_STEP_OUT2:
		commandType1(wd);
		break;

	case CMD_READ_SECTOR:
	case CMD_READ_SECTORS:
	case CMD_WRITE_SECTOR:
	case CMD_WRITE_SECTORS:
		commandType2(wd);
		break;
		
	case CMD_READ_ADDRESS:
	case CMD_READ_TRACK:
	case CMD_WRITE_TRACK:
		commandType3(wd);
		break;
	
	case CMD_FORCE_INTERRUPT:
		commandType4(wd);
		break;
	}
}

void wd2793Reset(WD2793* wd)
{
    wd->dataRequsetTime = 0;
    wd->dataReady       = 0;
    wd->curStep         = 0;
    wd->step            = 0;
    wd->stepTime        = 0;
	wd->sectorOffset    = 0;
	wd->dataAvailable   = 0;
    wd->regStatus       = 0;
    wd->regTrack        = 0;
	wd->regData         = 0;
	wd->stepDirection   = 1;
	wd->regCommand      = 0x03;
	wd->regSector       = 0x01;
	wd->dataRequest     = 0;
	wd->intRequest      = 0;
	wd->immediateInt    = 0;
    wd->headLoaded      = 0;
    wd->drive           = 0;
    wd->diskTrack       = 0;
    wd->diskTracks[0]   = 0;
    wd->diskTracks[1]   = 0;
    wd->diskTracks[2]   = 0;
    wd->diskTracks[3]   = 0;
    wd->diskSide        = 0;
    memset(wd->sectorBuf, 0, sizeof(wd->sectorBuf));

    ledSetFdd1(0);
    ledSetFdd2(0);

    fdcAudioReset(wd->fdcAudio);
}

WD2793* wd2793Create(Wd2793FdcType type)
{
    WD2793* wd = malloc(sizeof(WD2793));

    wd->fdcAudio = fdcAudioCreate(FA_WESTERN_DIGITAL);

    wd->type = type;

    wd2793Reset(wd);

    return wd;
}

void wd2793Destroy(WD2793* wd)
{
    fdcAudioDestroy(wd->fdcAudio);
    free(wd);
}

void wd2793LoadState(WD2793* wd)
{
    SaveState* state = saveStateOpenForRead("wd2793");

    wd->step            =         saveStateGet(state, "step",            0);
    wd->curStep         =         saveStateGet(state, "curStep",         0);
    wd->stepTime        =         saveStateGet(state, "stepTime",        0);
    wd->regStatus       = (UInt8) saveStateGet(state, "regStatus",       0);
    wd->regCommand      = (UInt8) saveStateGet(state, "regCommand",      0);
    wd->regSector       = (UInt8) saveStateGet(state, "regSector",       0);
    wd->regTrack        = (UInt8) saveStateGet(state, "regTrack",        0);
    wd->regData         = (UInt8) saveStateGet(state, "regData",         0);
    wd->immediateInt    =         saveStateGet(state, "immediateInt",    0);
    wd->intRequest      =         saveStateGet(state, "intRequest",      0);
    wd->dataRequest     =         saveStateGet(state, "dataRequest",     0);
    wd->dataReady       =         saveStateGet(state, "dataReady",       0);
    wd->stepDirection   =         saveStateGet(state, "stepDirection",   0);
    wd->headLoaded      =         saveStateGet(state, "headLoaded",      0);
    wd->dataRequsetTime =         saveStateGet(state, "dataRequsetTime", 0);
    wd->drive           =         saveStateGet(state, "drive",           0);
    wd->sectorOffset    =         saveStateGet(state, "sectorOffset",    0);
    wd->dataAvailable   =         saveStateGet(state, "dataAvailable",   0);
    wd->diskTrack       = (UInt8) saveStateGet(state, "diskTrack",       0);
    wd->diskTracks[0]   = (UInt8) saveStateGet(state, "diskTracks0",     0);
    wd->diskTracks[1]   = (UInt8) saveStateGet(state, "diskTracks1",     0);
    wd->diskTracks[2]   = (UInt8) saveStateGet(state, "diskTracks2",     0);
    wd->diskTracks[3]   = (UInt8) saveStateGet(state, "diskTracks3",     0);
    wd->diskSide        =         saveStateGet(state, "diskSide",        0);
    wd->diskDensity     =         saveStateGet(state, "diskDensity",     0);

    saveStateGetBuffer(state, "sectorBuf", wd->sectorBuf, sizeof(wd->sectorBuf));

    saveStateClose(state);
}

void wd2793SaveState(WD2793* wd)
{
    SaveState* state = saveStateOpenForWrite("wd2793");

    saveStateSet(state, "step",            wd->step);
    saveStateSet(state, "curStep",         wd->curStep);
    saveStateSet(state, "stepTime",        wd->stepTime);
    saveStateSet(state, "regStatus",       wd->regStatus);
    saveStateSet(state, "regCommand",      wd->regCommand);
    saveStateSet(state, "regSector",       wd->regSector);
    saveStateSet(state, "regTrack",        wd->regTrack);
    saveStateSet(state, "regData",         wd->regData);
    saveStateSet(state, "immediateInt",    wd->immediateInt);
    saveStateSet(state, "intRequest",      wd->intRequest);
    saveStateSet(state, "dataRequest",     wd->dataRequest);
    saveStateSet(state, "dataReady",       wd->dataReady);
    saveStateSet(state, "stepDirection",   wd->stepDirection);
    saveStateSet(state, "headLoaded",      wd->headLoaded);
    saveStateSet(state, "dataRequsetTime", wd->dataRequsetTime);
    saveStateSet(state, "drive",           wd->drive);
    saveStateSet(state, "sectorOffset",    wd->sectorOffset);
    saveStateSet(state, "dataAvailable",   wd->dataAvailable);
    saveStateSet(state, "diskTrack",       wd->diskTrack);
    saveStateSet(state, "diskTracks[0]",   wd->diskTracks[0]);
    saveStateSet(state, "diskTracks[1]",   wd->diskTracks[1]);
    saveStateSet(state, "diskTracks[2]",   wd->diskTracks[2]);
    saveStateSet(state, "diskTracks[3]",   wd->diskTracks[3]);
    saveStateSet(state, "diskSide",        wd->diskSide);
    saveStateSet(state, "diskDensity",     wd->diskDensity);

    saveStateSetBuffer(state, "sectorBuf", wd->sectorBuf, 512);

    saveStateClose(state);
}

