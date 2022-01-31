/*
 * scsi.h - SCSI disk emulation
 *
 * Written by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_SCSI_H
#define VICE_SCSI_H

#include "types.h"

struct scsi_context_s;

typedef struct scsi_context_s {
    char *myname;
    uint8_t state;
    uint8_t target;
    uint8_t databus;
    uint8_t ack;
    uint8_t req;
    uint8_t bsyi;
    uint8_t bsyo;
    uint8_t sel;
    uint8_t rst;
    uint8_t atn;
    uint8_t cd;
    uint8_t io;
    uint8_t msg;
    uint32_t seq;
    uint32_t cmd_size;
    uint32_t address;
    uint32_t blocks;
    uint32_t data_max;
    uint8_t link;
    uint8_t status;
    uint8_t lun;
    uint8_t command;
    uint8_t sensekey;
    uint8_t asc;
    uint8_t cmd_buf[256];
    uint8_t data_buf[512];
    uint8_t msg_after_status;
    uint32_t max_imagesize; /* in 512 byte sectors */
    uint32_t limit_imagesize; /* in 512 byte sectors */
    uint32_t log;
    FILE *file[56];
    void *p;
    void (*user_format)(struct scsi_context_s *);
    void (*user_read)(struct scsi_context_s *);
    void (*user_write)(struct scsi_context_s *);
} scsi_context_t;

/* From SCSI-1 standard:

==============================================================================
   Signal
-----------
MSG C/D I/O   Phase Name          Direction Of Transfer         Comment
------------------------------------------------------------------------------
 0   0   0    DATA OUT            Initiator to target     \     Data
 0   0   1    DATA IN             Initiator from target   /     Phase
 0   1   0    COMMAND             Initiator to target
 0   1   1    STATUS              Initiator from target
 1   0   0    *
 1   0   1    *
 1   1   0    MESSAGE OUT         Initiator to target     \     Message
 1   1   1    MESSAGE IN          Initiator from target   /     Phase
==============================================================================

*/

#define SCSI_STATE_DATAOUT    0x00
#define SCSI_STATE_DATAIN     0x01
#define SCSI_STATE_COMMAND    0x02
#define SCSI_STATE_STATUS     0x03
#define SCSI_STATE_MESSAGEOUT 0x04
#define SCSI_STATE_MESSAGEIN  0x05
#define SCSI_STATE_BUSFREE    0x10
#define SCSI_STATE_SELECTION  0x20

/* will be bitshifted left by 1 before sent: see table 14-2 of scsi spec */
#define SCSI_STATUS_GOOD                     0x00
#define SCSI_STATUS_CHECKCONDITION           0x01
#define SCSI_STATUS_CONDITIONMET             0x02
#define SCSI_STATUS_BUSY                     0x04
#define SCSI_STATUS_INTERMEDIATE             0x08
#define SCSI_STATUS_INTERMEDIATECONDITIONMET 0x0A
#define SCSI_STATUS_RESERVATIONCONFLICT      0x0C
#define SCSI_STATUS_COMMANDTERMINATED        0x11
#define SCSI_STATUS_QUEUEFULL                0x14

#define SCSI_SENSEKEY_NOSENSE        0x00
#define SCSI_SENSEKEY_RECOVEREDERROR 0x01
#define SCSI_SENSEKEY_NOTREADY       0x02
#define SCSI_SENSEKEY_MEDIUMERROR    0x03
#define SCSI_SENSEKEY_HARDWAREERROR  0x04
#define SCSI_SENSEKEY_ILLEGALREQUEST 0x05
#define SCSI_SENSEKEY_UNITATTENTION  0x06
#define SCSI_SENSEKEY_DATAPROTECT    0x07
#define SCSI_SENSEKEY_BLANKCHECK     0x08
#define SCSI_SENSEKEY_VENDORSPECIFIC 0x09
#define SCSI_SENSEKEY_COPYABORTED    0x0a
#define SCSI_SENSEKEY_ABORTEDCOMMAND 0x0b
#define SCSI_SENSEKEY_EQUAL          0x0c
#define SCSI_SENSEKEY_VOLUMEOVERFLOW 0x0d
#define SCSI_SENSEKEY_MISCOMPARE     0x0e
#define SCSI_SENSEKEY_COMPLETED      0x0f

#define SCSI_SASC_LOGICALBLOCKADDRESSOUTOFRANGE 0x21
#define SCSI_SASC_INVALIDFIELDINCDB             0x24
#define SCSI_SASC_INVALIDFIELDINPARAMETERLIST   0x26

#define SCSI_LOG_NODISK0 0x01

/* commands that have full or limited implementation known
    to be used by LTK and CMD HDs */
#define SCSI_COMMAND_TEST_UNIT_READY       0x00
#define SCSI_COMMAND_REZERO_UNIT           0x01
#define SCSI_COMMAND_REQUEST_SENSE         0x03
#define SCSI_COMMAND_FORMAT_UNIT           0x04
#define SCSI_COMMAND_REASSIGN_BLOCKS       0x07
#define SCSI_COMMAND_READ_6                0x08
#define SCSI_COMMAND_WRITE_6               0x0a
#define SCSI_COMMAND_INQUIRY               0x12
#define SCSI_COMMAND_MODE_SENSE            0x1a
#define SCSI_COMMAND_START_STOP            0x1b
#define SCSI_COMMAND_SEND_DIAGNOSTIC       0x1d
#define SCSI_COMMAND_READ_CAPACITY         0x25
#define SCSI_COMMAND_READ_10               0x28
#define SCSI_COMMAND_WRITE_10              0x2a
#define SCSI_COMMAND_VERIFY                0x2f
#define SCSI_COMMAND_MODE_SENSE_10         0x5a

/* commands users have requested to be implemented */
#define SCSI_COMMAND_WRITE_VERIFY          0x2e

/* unimplemented commands */
#define SCSI_COMMAND_READ_BLOCK_LIMITS     0x05
#define SCSI_COMMAND_SEEK_6                0x0b
#define SCSI_COMMAND_READ_REVERSE          0x0f
#define SCSI_COMMAND_WRITE_FILEMARKS       0x10
#define SCSI_COMMAND_SPACE                 0x11
#define SCSI_COMMAND_RECOVER_BUFFERED_DATA 0x14
#define SCSI_COMMAND_MODE_SELECT           0x15
#define SCSI_COMMAND_RESERVE               0x16
#define SCSI_COMMAND_RELEASE               0x17
#define SCSI_COMMAND_COPY                  0x18
#define SCSI_COMMAND_ERASE                 0x19
#define SCSI_COMMAND_RECEIVE_DIAGNOSTIC    0x1c
#define SCSI_COMMAND_ALLOW_MEDIUM_REMOVAL  0x1e
#define SCSI_COMMAND_SET_WINDOW            0x24
#define SCSI_COMMAND_SEEK_10               0x2b
#define SCSI_COMMAND_SEARCH_HIGH           0x30
#define SCSI_COMMAND_SEARCH_EQUAL          0x31
#define SCSI_COMMAND_SEARCH_LOW            0x32
#define SCSI_COMMAND_SET_LIMITS            0x33
#define SCSI_COMMAND_PRE_FETCH             0x34
#define SCSI_COMMAND_READ_POSITION         0x34
#define SCSI_COMMAND_SYNCHRONIZE_CACHE     0x35
#define SCSI_COMMAND_LOCK_UNLOCK_CACHE     0x36
#define SCSI_COMMAND_READ_DEFECT_DATA      0x37
#define SCSI_COMMAND_MEDIUM_SCAN           0x38
#define SCSI_COMMAND_COMPARE               0x39
#define SCSI_COMMAND_COPY_VERIFY           0x3a
#define SCSI_COMMAND_WRITE_BUFFER          0x3b
#define SCSI_COMMAND_READ_BUFFER           0x3c
#define SCSI_COMMAND_UPDATE_BLOCK          0x3d
#define SCSI_COMMAND_READ_LONG             0x3e
#define SCSI_COMMAND_WRITE_LONG            0x3f
#define SCSI_COMMAND_CHANGE_DEFINITION     0x40
#define SCSI_COMMAND_WRITE_SAME            0x41
#define SCSI_COMMAND_READ_TOC              0x43
#define SCSI_COMMAND_LOG_SELECT            0x4c
#define SCSI_COMMAND_LOG_SENSE             0x4d
#define SCSI_COMMAND_MODE_SELECT_10        0x55
#define SCSI_COMMAND_RESERVE_10            0x56
#define SCSI_COMMAND_RELEASE_10            0x57
#define SCSI_COMMAND_PERSISTENT_RESERVE_IN 0x5e
#define SCSI_COMMAND_PERSISTENT_RESERVE_OUT 0x5f
#define SCSI_COMMAND_MOVE_MEDIUM           0xa5
#define SCSI_COMMAND_READ_12               0xa8
#define SCSI_COMMAND_WRITE_12              0xaa
#define SCSI_COMMAND_WRITE_VERIFY_12       0xae
#define SCSI_COMMAND_SEARCH_HIGH_12        0xb0
#define SCSI_COMMAND_SEARCH_EQUAL_12       0xb1
#define SCSI_COMMAND_SEARCH_LOW_12         0xb2
#define SCSI_COMMAND_READ_ELEMENT_STATUS   0xb8
#define SCSI_COMMAND_SEND_VOLUME_TAG       0xb6
#define SCSI_COMMAND_WRITE_LONG_2          0xea

extern int scsi_image_detach(struct scsi_context_s *context, int disk);
extern void scsi_image_detach_all(struct scsi_context_s *context);
extern int scsi_image_attach(struct scsi_context_s *context, int disk,
    char *filename);
extern int32_t scsi_image_read(struct scsi_context_s *context);
extern int32_t scsi_image_write(struct scsi_context_s *context);
extern uint8_t scsi_get_bus(struct scsi_context_s *context);
extern int scsi_set_bus(struct scsi_context_s *context, uint8_t value);
extern void scsi_process_noack(struct scsi_context_s *context);
extern void scsi_process_ack(struct scsi_context_s *context);
extern void scsi_reset(struct scsi_context_s *context);
extern int scsi_snapshot_write_module(struct scsi_context_s *context,
    snapshot_t *s);
extern int scsi_snapshot_read_module(struct scsi_context_s *context,
    snapshot_t *s);

#endif
