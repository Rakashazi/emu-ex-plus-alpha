/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/TC8566AF.c,v $
**
** $Revision: 1.16 $
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
#include "TC8566AF.h"
#include "Board.h"
#include "SaveState.h"
#include "Disk.h"
#include "Led.h"
#include "FdcAudio.h"
#include <stdlib.h>
#include <string.h>


extern int diskOffset;

struct TC8566AF {
    UInt8 drive;

    UInt8 mainStatus;
    UInt8 status0;
    UInt8 status1;
    UInt8 status2;
    UInt8 status3;
    UInt8 commandCode;

    int command;
    int phase;
    int phaseStep;
    
    UInt8 fillerByte;

    UInt8 cylinderNumber;
    UInt8 side;
    UInt8 sectorNumber;
    UInt8 number;
    UInt8 currentTrack;
    UInt8 sectorsPerCylinder;

    int    sectorOffset;
    UInt32 dataTransferTime;

    UInt8 sectorBuf[512];

    FdcAudio* fdcAudio;
};

#define CMD_UNKNOWN                 0
#define CMD_READ_DATA               1
#define CMD_WRITE_DATA              2
#define CMD_WRITE_DELETED_DATA      3
#define CMD_READ_DELETED_DATA       4
#define CMD_READ_DIAGNOSTIC         5
#define CMD_READ_ID                 6
#define CMD_FORMAT                  7
#define CMD_SCAN_EQUAL              8
#define CMD_SCAN_LOW_OR_EQUAL       9
#define CMD_SCAN_HIGH_OR_EQUAL      10
#define CMD_SEEK                    11
#define CMD_RECALIBRATE             12
#define CMD_SENSE_INTERRUPT_STATUS  13
#define CMD_SPECIFY                 14
#define CMD_SENSE_DEVICE_STATUS     15

#define PHASE_IDLE                  0
#define PHASE_COMMAND               1
#define PHASE_DATATRANSFER          2
#define PHASE_RESULT                3

#define STM_DB0    0x01
#define STM_DB1    0x02
#define STM_DB2    0x04
#define STM_DB3    0x08
#define STM_CB     0x10
#define STM_NDM    0x20
#define STM_DIO    0x40
#define STM_RQM    0x80

#define ST0_DS0    0x01
#define ST0_DS1    0x02
#define ST0_HD     0x04
#define ST0_NR     0x08
#define ST0_EC     0x10
#define ST0_SE     0x20
#define ST0_IC0    0x40
#define ST0_IC1    0x80

#define ST1_MA     0x01
#define ST1_NW     0x02
#define ST1_ND     0x04
#define ST1_OR     0x10
#define ST1_DE     0x20
#define ST1_EN     0x80

#define ST2_MD     0x01
#define ST2_BC     0x02
#define ST2_SN     0x04
#define ST2_SH     0x08
#define ST2_NC     0x10
#define ST2_DD     0x20
#define ST2_CM     0x40

#define ST3_DS0    0x01
#define ST3_DS1    0x02
#define ST3_HD     0x04
#define ST3_2S     0x08
#define ST3_TK0    0x10
#define ST3_RDY    0x20
#define ST3_WP     0x40
#define ST3_FLT    0x80

static UInt8 tc8566afExecutionPhasePeek(TC8566AF* tc)
{
	switch (tc->command) {
	case CMD_READ_DATA:
		if (tc->sectorOffset < 512) {
			return tc->sectorBuf[tc->sectorOffset];
        }
        break;
    }
    return 0xff;
}


static UInt8 tc8566afExecutionPhaseRead(TC8566AF* tc)
{
	switch (tc->command) {
	case CMD_READ_DATA:
		if (tc->sectorOffset < 512) {
			UInt8 value = tc->sectorBuf[tc->sectorOffset++];
    		if (tc->sectorOffset == 512) {
                tc->phase = PHASE_RESULT;
                tc->phaseStep = 0;
            }
            return value;
        }
        break;
    }

    return 0xff;
}

static UInt8 tc8566afResultsPhasePeek(TC8566AF* tc)
{
	switch (tc->command) {
	case CMD_READ_DATA:
	case CMD_WRITE_DATA:
    case CMD_FORMAT:
		switch(tc->phaseStep) {
		case 0:
            return tc->status0;
		case 1:
            return tc->status1;
		case 2:
            return tc->status2;
		case 3:
            return tc->cylinderNumber;
		case 4:
            return tc->side;
		case 5:
            return tc->sectorNumber;
		case 6:
            return tc->number;
		}
		break;

    case CMD_SENSE_INTERRUPT_STATUS:
		switch (tc->phaseStep) {
		case 0:
            return tc->status0;
		case 1:
            return tc->currentTrack;
		}
		break;

    case CMD_SENSE_DEVICE_STATUS:
		switch (tc->phaseStep) {
		case 0:
            return tc->status3;
		}
		break;
    }

    return 0xff;
}

static UInt8 tc8566afResultsPhaseRead(TC8566AF* tc)
{
	switch (tc->command) {
	case CMD_READ_DATA:
	case CMD_WRITE_DATA:
    case CMD_FORMAT:
		switch	(tc->phaseStep++) {
		case 0:
            return tc->status0;
		case 1:
            return tc->status1;
		case 2:
            return tc->status2;
		case 3:
            return tc->cylinderNumber;
		case 4:
            return tc->side;
		case 5:
            return tc->sectorNumber;
		case 6:
			tc->phase       = PHASE_IDLE;
            tc->mainStatus &= ~STM_CB;
            tc->mainStatus &= ~STM_DIO;

            return tc->number;
		}
		break;

    case CMD_SENSE_INTERRUPT_STATUS:
		switch (tc->phaseStep++) {
		case 0:
            return tc->status0;
		case 1:
			tc->phase       = PHASE_IDLE;
            tc->mainStatus &= ~(STM_CB | STM_DIO);

            return tc->currentTrack;
		}
		break;

    case CMD_SENSE_DEVICE_STATUS:
		switch (tc->phaseStep++) {
		case 0:
			tc->phase       = PHASE_IDLE;
            tc->mainStatus &= ~(STM_CB | STM_DIO);
            
            return tc->status3;
		}
		break;
    }

    return 0xff;
}

void tc8566afIdlePhaseWrite(TC8566AF* tc, UInt8 value)
{
	tc->command = CMD_UNKNOWN;
	if ((value & 0x1f) == 0x06) tc->command = CMD_READ_DATA;
	if ((value & 0x3f) == 0x05) tc->command = CMD_WRITE_DATA;
	if ((value & 0x3f) == 0x09) tc->command = CMD_WRITE_DELETED_DATA;
	if ((value & 0x1f) == 0x0c) tc->command = CMD_READ_DELETED_DATA;
	if ((value & 0xbf) == 0x02) tc->command = CMD_READ_DIAGNOSTIC;
	if ((value & 0xbf) == 0x0a) tc->command = CMD_READ_ID;
	if ((value & 0xbf) == 0x0d) tc->command = CMD_FORMAT;
	if ((value & 0x1f) == 0x11) tc->command = CMD_SCAN_EQUAL;
	if ((value & 0x1f) == 0x19) tc->command = CMD_SCAN_LOW_OR_EQUAL;
	if ((value & 0x1f) == 0x1d) tc->command = CMD_SCAN_HIGH_OR_EQUAL;
	if ((value & 0xff) == 0x0f) tc->command = CMD_SEEK;
	if ((value & 0xff) == 0x07) tc->command = CMD_RECALIBRATE;
	if ((value & 0xff) == 0x08) tc->command = CMD_SENSE_INTERRUPT_STATUS;
	if ((value & 0xff) == 0x03) tc->command = CMD_SPECIFY;
	if ((value & 0xff) == 0x04) tc->command = CMD_SENSE_DEVICE_STATUS;

    tc->commandCode = value;

	tc->phase       = PHASE_COMMAND;
	tc->phaseStep   = 0;
    tc->mainStatus |= STM_CB;
    
    switch (tc->command) {
	case CMD_READ_DATA:
	case CMD_WRITE_DATA:
	case CMD_FORMAT:
        tc->status0 &= ~(ST0_IC0 | ST0_IC1);
        tc->status1 &= ~(ST1_ND | ST1_NW);
        tc->status2 &= ~ST2_DD;
		break;

	case CMD_RECALIBRATE:
        tc->status0 &= ~ST0_SE;
		break;

	case CMD_SENSE_INTERRUPT_STATUS:
		tc->phase       = PHASE_RESULT;
        tc->mainStatus |= STM_DIO;
        break;

    case CMD_SEEK:
	case CMD_SPECIFY:
	case CMD_SENSE_DEVICE_STATUS:
        break;

    default:
        tc->mainStatus &= ~STM_CB;
		tc->phase       = PHASE_IDLE;
    }
}

static void tc8566afCommandPhaseWrite(TC8566AF* tc, UInt8 value)
{
    switch (tc->command) {
	case CMD_READ_DATA:
	case CMD_WRITE_DATA:
		switch (tc->phaseStep++) {
		case 0:
            tc->status0 &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
            tc->status0 |= (diskPresent(tc->drive) ? 0 : ST0_DS0) | (value & (ST0_DS0 | ST0_DS1)) |
                           (diskEnabled(tc->drive) ? 0 : ST0_IC1);
            tc->status3  = (value & (ST3_DS0 | ST3_DS1)) | 
                           (tc->currentTrack == 0        ? ST3_TK0 : 0) | 
                           (diskGetSides(tc->drive) == 2 ? ST3_HD  : 0) | 
                           (diskReadOnly(tc->drive)      ? ST3_WP  : 0) |
                           (diskPresent(tc->drive)       ? ST3_RDY : 0);
			break;
		case 1:
            tc->cylinderNumber = value;
			break;
		case 2:
            tc->side = value & 1;
			break;
		case 3:
            tc->sectorNumber = value;
			break;
		case 4:
            tc->number = value;
            tc->sectorOffset = (value == 2 && (tc->commandCode & 0xc0) == 0x40) ? 0 : 512;
			break;
		case 7:
            if (tc->command == CMD_READ_DATA) {
                int sectorSize;
        		DSKE rv = diskReadSector(tc->drive, tc->sectorBuf, tc->sectorNumber, tc->side, 
                                         tc->currentTrack, 0, &sectorSize);
                fdcAudioSetReadWrite(tc->fdcAudio);
                boardSetFdcActive();
                if (rv == DSKE_NO_DATA) {
                    tc->status0 |= ST0_IC0;
                    tc->status1 |= ST1_ND;
                }
                if (rv == DSKE_CRC_ERROR) {
                    tc->status0 |= ST0_IC0;
                    tc->status1 |= ST1_DE;
                    tc->status2 |= ST2_DD;
                }
                tc->mainStatus |= STM_DIO;
            }
            else {
                tc->mainStatus &= ~STM_DIO;
            }
			tc->phase = PHASE_DATATRANSFER;
			tc->phaseStep = 0;
			break;
		}
		break;

	case CMD_FORMAT:
		switch (tc->phaseStep++) {
		case 0:
            tc->status0 &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
            tc->status0 |= (diskPresent(tc->drive) ? 0 : ST0_DS0) | (value & (ST0_DS0 | ST0_DS1)) |
                           (diskEnabled(tc->drive) ? 0 : ST0_IC1);
            tc->status3  = (value & (ST3_DS0 | ST3_DS1)) | 
                           (tc->currentTrack == 0        ? ST3_TK0 : 0) | 
                           (diskGetSides(tc->drive) == 2 ? ST3_HD  : 0) |  
                           (diskReadOnly(tc->drive)      ? ST3_WP  : 0) |
                           (diskPresent(tc->drive)       ? ST3_RDY : 0);
			break;
		case 1:
            tc->number = value;
			break;
		case 2:
            tc->sectorsPerCylinder = value;
            tc->sectorNumber       = value;
			break;
		case 4:
            tc->fillerByte   = value;
            tc->sectorOffset = 0;
            tc->mainStatus  &= ~STM_DIO;
			tc->phase        = PHASE_DATATRANSFER;
			tc->phaseStep    = 0;
			break;
        }
        break;

	case CMD_SEEK:
		switch (tc->phaseStep++) {
		case 0:
            tc->status0 &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
            tc->status0 |= (diskPresent(tc->drive) ? 0 : ST0_DS0) | (value & (ST0_DS0 | ST0_DS1)) |
                           (diskEnabled(tc->drive) ? 0 : ST0_IC1);
            tc->status3  = (value & (ST3_DS0 | ST3_DS1)) | 
                           (tc->currentTrack == 0        ? ST3_TK0 : 0) | 
                           (diskGetSides(tc->drive) == 2 ? ST3_HD  : 0) |  
                           (diskReadOnly(tc->drive)      ? ST3_WP  : 0) |
                           (diskPresent(tc->drive)       ? ST3_RDY : 0);
			break;
		case 1: 
            tc->currentTrack = value;
            tc->status0     |= ST0_SE;
            tc->mainStatus  &= ~STM_CB;
			tc->phase        = PHASE_IDLE;
			break;
		}
		break;

	case CMD_RECALIBRATE:
		switch (tc->phaseStep++) {
		case 0: 
            tc->status0 &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
            tc->status0 |= (diskPresent(tc->drive) ? 0 : ST0_DS0) | (value & (ST0_DS0 | ST0_DS1)) |
                           (diskEnabled(tc->drive) ? 0 : ST0_IC1);
            tc->status3  = (value & (ST3_DS0 | ST3_DS1)) | 
                           (tc->currentTrack == 0        ? ST3_TK0 : 0) | 
                           (diskGetSides(tc->drive) == 2 ? ST3_HD  : 0) |  
                           (diskReadOnly(tc->drive)      ? ST3_WP  : 0) |
                           (diskPresent(tc->drive)       ? ST3_RDY : 0);

            tc->currentTrack = 0;
            tc->status0     |= ST0_SE;
            tc->mainStatus  &= ~STM_CB;
			tc->phase        = PHASE_IDLE;
			break;
		}
		break;

	case CMD_SPECIFY:
		switch (tc->phaseStep++) {
		case 1:
            tc->mainStatus &= ~STM_CB;
			tc->phase       = PHASE_IDLE;
			break;
		}
		break;

	case CMD_SENSE_DEVICE_STATUS:
		switch (tc->phaseStep++) {
		case 0:
            tc->status0 &= ~(ST0_DS0 | ST0_DS1 | ST0_IC0 | ST0_IC1);
            tc->status0 |= (diskPresent(tc->drive) ? 0 : ST0_DS0) | (value & (ST0_DS0 | ST0_DS1)) |
                           (diskEnabled(tc->drive) ? 0 : ST0_IC1);
            tc->status3  = (value & (ST3_DS0 | ST3_DS1)) | 
                           (tc->currentTrack == 0        ? ST3_TK0 : 0) | 
                           (diskGetSides(tc->drive) == 2 ? ST3_HD  : 0) |  
                           (diskReadOnly(tc->drive)      ? ST3_WP  : 0) |
                           (diskPresent(tc->drive)       ? ST3_RDY : 0);
            
		    tc->phase       = PHASE_RESULT;
            tc->phaseStep   = 0;
            tc->mainStatus |= STM_DIO;
			break;
		}
		break;
	}
}

static void tc8566afExecutionPhaseWrite(TC8566AF* tc, UInt8 value)
{
    int rv;

	switch (tc->command) {
	case CMD_WRITE_DATA:
		if (tc->sectorOffset < 512) {
			tc->sectorBuf[tc->sectorOffset++] = value;
            
    		if (tc->sectorOffset == 512) {
                rv = diskWriteSector(tc->drive, tc->sectorBuf, tc->sectorNumber, tc->side, 
                                     tc->currentTrack, 0);
                if (!rv) {
                    tc->status1 |= ST1_NW;
                }

                fdcAudioSetReadWrite(tc->fdcAudio);

                boardSetFdcActive();

                tc->phase       = PHASE_RESULT;
                tc->phaseStep   = 0;
                tc->mainStatus |= STM_DIO;
            }
        }
		break;

    case CMD_FORMAT:
        switch(tc->phaseStep & 3) {
        case 0:
            tc->currentTrack = value;
            break;
        case 1:
            memset(tc->sectorBuf, tc->fillerByte, 512);
            rv = diskWrite(tc->drive, tc->sectorBuf, tc->sectorNumber - 1 +
                      diskGetSectorsPerTrack(tc->drive) * (tc->currentTrack * diskGetSides(tc->drive) + value));
            if (!rv) {
                tc->status1 |= ST1_NW;
            }
            boardSetFdcActive();
            break;
        case 2:
            tc->sectorNumber = value;
            break;
        }

        if (++tc->phaseStep == 4 * tc->sectorsPerCylinder - 2) {
            tc->phase       = PHASE_RESULT;
            tc->phaseStep   = 0;
            tc->mainStatus |= STM_DIO;
        }
        break;
	}
}

TC8566AF* tc8566afCreate()
{
    TC8566AF* tc = malloc(sizeof(TC8566AF));

    tc->fdcAudio = fdcAudioCreate(FA_PANASONIC);

    tc8566afReset(tc);

    return tc;
}

void tc8566afDestroy(TC8566AF* tc)
{
    fdcAudioDestroy(tc->fdcAudio);
    free(tc);
}

void tc8566afReset(TC8566AF* tc)
{
    FdcAudio* fdcAudio = tc->fdcAudio;
    memset(tc, 0, sizeof(TC8566AF));
    tc->fdcAudio = fdcAudio;

    tc->mainStatus = STM_NDM | STM_RQM;

    ledSetFdd1(0); /* 10:10 2004/10/09 FDD LED PATCH */ 
    ledSetFdd2(0); /* 10:10 2004/10/09 FDD LED PATCH */ 

    fdcAudioReset(tc->fdcAudio);
}

UInt8 tc8566afReadRegister(TC8566AF* tc, UInt8 reg)
{
    switch (reg) {
    case 4: 
        if (~tc->mainStatus & STM_RQM) {
            UInt32 elapsed = boardSystemTime() - tc->dataTransferTime;
            if (elapsed > boardFrequency() * 60 / 1000000) {
                tc->mainStatus |= STM_RQM;
            } 
        }
//        return tc->mainStatus;
  return (tc->mainStatus & ~ STM_NDM) | (tc->phase == PHASE_DATATRANSFER ? STM_NDM : 0);

	case 5:
        switch (tc->phase) {            
		case PHASE_DATATRANSFER:
            reg = tc8566afExecutionPhaseRead(tc);
            tc->dataTransferTime = boardSystemTime();
            tc->mainStatus &= ~STM_RQM;
            return reg;

		case PHASE_RESULT:
            return tc8566afResultsPhaseRead(tc);
        }
    }

    return 0x00;
}

UInt8 tc8566afPeekRegister(TC8566AF* tc, UInt8 reg)
{
    switch (reg) {
    case 4: 
        return tc->mainStatus;
	case 5:
        switch (tc->phase) {            
		case PHASE_DATATRANSFER:
            return tc8566afExecutionPhasePeek(tc);
		case PHASE_RESULT:
            return tc8566afResultsPhasePeek(tc);
        }
    }
    return 0xff;
}

void tc8566afWriteRegister(TC8566AF* tc, UInt8 reg, UInt8 value)
{
    switch (reg) {
	case 2:
        fdcAudioSetMotor(tc->fdcAudio, ((value & 0x10) && diskEnabled(0)) || ((value & 0x20) && diskEnabled(1)));

        ledSetFdd1((value & 0x10) && diskEnabled(0)); /* 10:10 2004/10/09 FDD LED PATCH */ 
        ledSetFdd2((value & 0x20) && diskEnabled(1)); /* 10:10 2004/10/09 FDD LED PATCH */ 

        tc->drive = value & 0x03;
        break;

	case 5:
        switch (tc->phase) {
	    case PHASE_IDLE:
            tc8566afIdlePhaseWrite(tc, value);
            break;

        case PHASE_COMMAND:
            tc8566afCommandPhaseWrite(tc, value);
            break;
            
		case PHASE_DATATRANSFER:
            tc8566afExecutionPhaseWrite(tc, value);
            tc->dataTransferTime = boardSystemTime();
            tc->mainStatus &= ~STM_RQM;
        	break;
        }
		break;
    }
}

int tc8566afDiskChanged(TC8566AF* tc, int drive)
{
    return diskChanged(drive);
}

void tc8566afLoadState(TC8566AF* tc)
{
    SaveState* state = saveStateOpenForRead("tc8566af");

    tc->drive               = (UInt8) saveStateGet(state, "drive",              0);
    tc->mainStatus          = (UInt8) saveStateGet(state, "mainStatus",         STM_NDM | STM_RQM);
    tc->status0             = (UInt8) saveStateGet(state, "status0",            0);
    tc->status1             = (UInt8) saveStateGet(state, "status1",            0);
    tc->status2             = (UInt8) saveStateGet(state, "status2",            0);
    tc->status3             = (UInt8) saveStateGet(state, "status3",            0);
    tc->commandCode         = (UInt8) saveStateGet(state, "commandCode",        0);
    tc->command             =         saveStateGet(state, "command",            0);
    tc->phase               =         saveStateGet(state, "phase",              0);
    tc->phaseStep           =         saveStateGet(state, "phaseStep",          0);
    tc->cylinderNumber      = (UInt8) saveStateGet(state, "cylinderNumber",     0);
    tc->side                = (UInt8) saveStateGet(state, "side",               0);
    tc->sectorNumber        = (UInt8) saveStateGet(state, "sectorNumber",       0);
    tc->number              = (UInt8) saveStateGet(state, "number",             0);
    tc->currentTrack        = (UInt8) saveStateGet(state, "currentTrack",       0);
    tc->sectorsPerCylinder  = (UInt8) saveStateGet(state, "sectorsPerCylinder", 0);
    tc->sectorOffset        =         saveStateGet(state, "sectorOffset",       0);
    tc->dataTransferTime    =         saveStateGet(state, "dataTransferTime",   0);
    
    saveStateGetBuffer(state, "sectorBuf", tc->sectorBuf, 512);

    saveStateClose(state);
}

void tc8566afSaveState(TC8566AF* tc)
{
    SaveState* state = saveStateOpenForWrite("tc8566af");

    saveStateSet(state, "drive",              tc->drive);
    saveStateSet(state, "mainStatus",         tc->mainStatus);
    saveStateSet(state, "status0",            tc->status0);
    saveStateSet(state, "status1",            tc->status1);
    saveStateSet(state, "status2",            tc->status2);
    saveStateSet(state, "status3",            tc->status3);
    saveStateSet(state, "commandCode",        tc->commandCode);
    saveStateSet(state, "command",            tc->command);
    saveStateSet(state, "phase",              tc->phase);
    saveStateSet(state, "phaseStep",          tc->phaseStep);
    saveStateSet(state, "cylinderNumber",     tc->cylinderNumber);
    saveStateSet(state, "side",               tc->side);
    saveStateSet(state, "sectorNumber",       tc->sectorNumber);
    saveStateSet(state, "number",             tc->number);
    saveStateSet(state, "currentTrack",       tc->currentTrack);
    saveStateSet(state, "sectorsPerCylinder", tc->sectorsPerCylinder);
    saveStateSet(state, "sectorOffset",       tc->sectorOffset);
    saveStateSet(state, "dataTransferTime",   tc->dataTransferTime);
    
    saveStateSetBuffer(state, "sectorBuf", tc->sectorBuf, 512);

    saveStateClose(state);
}
