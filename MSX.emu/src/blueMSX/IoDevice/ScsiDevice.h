/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/ScsiDevice.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2007-03-22 10:55:08 $
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
#ifndef SCSIDEVICE_H
#define SCSIDEVICE_H

#include "MsxTypes.h"
#include "ArchCdrom.h"

#define BIT_SCSI2           0x0001
#define BIT_SCSI2_ONLY      0x0002
#define BIT_SCSI3           0x0004

#define MODE_SCSI1          0x0000
#define MODE_SCSI2          0x0003
#define MODE_SCSI3          0x0005
#define MODE_UNITATTENTION  0x0008  // report unit attention
#define MODE_MEGASCSI       0x0010  // report disk change when call of
                                    // 'test unit ready'
#define MODE_FDS120         0x0020  // change of inquiry when inserted floppy image
#define MODE_CHECK2         0x0040  // mask to diskchange when load state
#define MODE_REMOVABLE      0x0080
#define MODE_NOVAXIS        0x0100

#define BUFFER_SIZE         0x10000                 // 64KB
#define BUFFER_BLOCK_SIZE   (BUFFER_SIZE / 512)

typedef enum {
    BusFree     =  0,
    Arbitration =  1,
    Selection   =  2,
    Reselection =  3,
    Command     =  4,
    Execute     =  5,
    DataIn      =  6,
    DataOut     =  7,
    Status      =  8,
    MsgOut      =  9,
    MsgIn       = 10
} SCSI_PHASE;

typedef struct SCSIDEVICE SCSIDEVICE;

SCSIDEVICE* scsiDeviceCreate(int scsiId, int diskId, UInt8* buf, char* name,
                  int type, int mode, CdromXferCompCb xferCompCb, void* ref);
void scsiDeviceDestroy(SCSIDEVICE* scsi);
void scsiDeviceReset(SCSIDEVICE* scsi);
void scsiDeviceSaveState(SCSIDEVICE* scsi);
void scsiDeviceLoadState(SCSIDEVICE* scsi);

int scsiDeviceSelection(SCSIDEVICE* scsi);
int scsiDeviceExecuteCmd(SCSIDEVICE* scsi, UInt8* cdb, SCSI_PHASE* phase, int* blocks);
int scsiDeviceExecutingCmd(SCSIDEVICE* scsi, SCSI_PHASE* phase, int* blocks);
UInt8 scsiDeviceGetStatusCode(SCSIDEVICE* scsi);
int scsiDeviceMsgOut(SCSIDEVICE* scsi, UInt8 value);
UInt8 scsiDeviceMsgIn(SCSIDEVICE* scsi);
void scsiDeviceDisconnect(SCSIDEVICE* scsi);
void scsiDeviceBusReset(SCSIDEVICE* scsi);

int scsiDeviceDataIn(SCSIDEVICE* scsi, int* blocks);
int scsiDeviceDataOut(SCSIDEVICE* scsi, int* blocks);
void scsiDeviceEnable(SCSIDEVICE* scsi, int enable);

/*
    debug routine
*/
//#define SCSIDEBUG "scsilog.txt"

#ifdef SCSIDEBUG
#include <stdio.h>

FILE* scsiDeviceLogCreate();
void scsiDeviceLogFlush();
void scsiDeviceLogClose();
#define SCSILOG(fmt) fprintf(scsiLog, fmt)
#define SCSILOG1(fmt, arg1) fprintf(scsiLog, fmt, arg1)
#define SCSILOG2(fmt, arg1, arg2) fprintf(scsiLog, fmt, arg1, arg2)
#define SCSILOG3(fmt, arg1, arg2, arg3) fprintf(scsiLog, fmt, arg1, arg2, arg3)

#else
#define scsiDeviceLogCreate() NULL
#define scsiDeviceLogFlush()
#define scsiDeviceLogClose()
#define SCSILOG(fmt)
#define SCSILOG1(fmt, arg1)
#define SCSILOG2(fmt, arg1, arg2)
#define SCSILOG3(fmt, arg1, arg2, arg3)
#endif

#endif
