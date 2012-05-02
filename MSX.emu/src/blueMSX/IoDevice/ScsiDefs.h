/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/ScsiDefs.h,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:40 $
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
#ifndef SCSIDEFS_H
#define SCSIDEFS_H

#include "MsxTypes.h"

// Group 0: 6bytes cdb
#define SCSIOP_TEST_UNIT_READY      0x00
#define SCSIOP_REZERO_UNIT          0x01
#define SCSIOP_REQUEST_SENSE        0x03
#define SCSIOP_FORMAT_UNIT          0x04
#define SCSIOP_REASSIGN_BLOCKS      0x07
#define SCSIOP_READ6                0x08
#define SCSIOP_WRITE6               0x0A
#define SCSIOP_SEEK6                0x0B
#define SCSIOP_INQUIRY              0x12
#define SCSIOP_RESERVE_UNIT         0x16
#define SCSIOP_RELEASE_UNIT         0x17
#define SCSIOP_MODE_SENSE           0x1A
#define SCSIOP_START_STOP_UNIT      0x1B
#define SCSIOP_SEND_DIAGNOSTIC      0x1D

// Group 1: 10bytes cdb
#define SCSIOP_GROUP1               0x20
#define SCSIOP_BLUE_MSX             0x20    // special command (vendor option)
#define SCSIOP_READ_CAPACITY        0x25
#define SCSIOP_READ10               0x28
#define SCSIOP_WRITE10              0x2A
#define SCSIOP_SEEK10               0x2B

#define SCSIOP_GROUP2               0x40
#define SCSIOP_CHANGE_DEFINITION    0x40
#define SCSIOP_READ_SUB_CHANNEL     0x42
#define SCSIOP_READ_TOC             0x43
#define SCSIOP_READ_HEADER          0x44
#define SCSIOP_PLAY_AUDIO           0x45
#define SCSIOP_PLAY_AUDIO_MSF       0x47
#define SCSIOP_PLAY_TRACK_INDEX     0x48
#define SCSIOP_PLAY_TRACK_RELATIVE  0x49
#define SCSIOP_PAUSE_RESUME         0x4B

#define SCSIOP_PLAY_AUDIO12         0xA5
#define SCSIOP_READ12               0xA8
#define SCSIOP_PLAY_TRACK_RELATIVE12 0xA9
#define SCSIOP_READ_CD_MSF          0xB9
#define SCSIOP_READ_CD              0xBE

// Sense data                    KEY | ASC | ASCQ
#define SENSE_NO_SENSE               0x000000
#define SENSE_NOT_READY              0x020400
#define SENSE_MEDIUM_NOT_PRESENT     0x023a00
#define SENSE_UNRECOVERED_READ_ERROR 0x031100
#define SENSE_WRITE_FAULT            0x040300
#define SENSE_INVALID_COMMAND_CODE   0x052000
#define SENSE_ILLEGAL_BLOCK_ADDRESS  0x052100
#define SENSE_INVALID_LUN            0x052500
#define SENSE_POWER_ON               0x062900
#define SENSE_WRITE_PROTECT          0x072700
#define SENSE_MESSAGE_REJECT_ERROR   0x0b4300
#define SENSE_INITIATOR_DETECTED_ERR 0x0b4800
#define SENSE_ILLEGAL_MESSAGE        0x0b4900

// Message
#define MSG_COMMAND_COMPLETE        0x00
#define MSG_INITIATOR_DETECT_ERROR  0x05
#define MSG_ABORT                   0x06
#define MSG_REJECT                  0x07
#define MSG_NO_OPERATION            0x08
#define MSG_PARITY_ERROR            0x09
#define MSG_BUS_DEVICE_RESET        0x0c

// Status
#define SCSIST_GOOD             0
#define SCSIST_CHECK_CONDITION  2
#define SCSIST_BUSY             8

// SCSI device type
#define SDT_DirectAccess        0x00
#define SDT_SequencialAccess    0x01
#define SDT_Printer             0x02
#define SDT_Processor           0x03
#define SDT_WriteOnce           0x04
#define SDT_CDROM               0x05
#define SDT_Scanner             0x06
#define SDT_OpticalMemory       0x07
#define SDT_MediaChanger        0x08
#define SDT_Communications      0x09
#define SDT_Undefined           0x1f

#define cdbLength(cmd) (cmd < 0x20) ?  6 : (cmd < 0xa0) ? 10 : 12

#endif
