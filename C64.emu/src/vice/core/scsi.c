/*
 * scsi.c - SCSI disk emulation
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

#ifdef _FILE_OFFSET_BITS
#undef _FILE_OFFSET_BITS
#endif

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "types.h"
#include "snapshot.h"
#include "scsi.h"
#include "util.h"

/* #define SCSILOG1 */
/* #define SCSILOG2 */
/* #define SCSIDBG */

#define LOG LOG_DEFAULT
#define ERR LOG_ERR

#ifdef SCSILOG1
#define LOG1(_x_) log_message _x_
#else
#define LOG1(_x_)
#endif

#ifdef SCSILOG2
#define LOG2(_x_) log_message _x_
#else
#define LOG2(_x_)
#endif

#ifdef SCSIDBG
#define SDBG(_x_) log_message _x_
#else
#define SDBG(_x_)
#endif

#define CRIT(_x_) log_message _x_

#define MAXIDS 7
#define MAXLUNS 8

static off_t scsi_getmaxsize(struct scsi_context_s *context)
{
    off_t work;

    /* make sure the target and lun are valid */
    if (context->target >= MAXIDS || context->lun >= MAXLUNS) {
        return 0;
    }

    /* make sure there is a file associated with the target and lun */
    if (context->file[(context->target << 3) | context->lun]) {
        if (!context->max_imagesize) {
            /* if the max length setting is zero, return the image length */
            work = util_file_length(context->file[(context->target << 3) | context->lun]);
            /* turn the file size into 512 byte sectors */
            work = (work >> 9) + (work & 511 ? 1 : 0);
            return work;
        } else {
            /* other wise, return the setting itself */
            return context->max_imagesize;
        }
    }

    /* not a valid file, return 0 */
    return 0;
}

static int32_t scsi_imagecheck(struct scsi_context_s *context)
{
    if (context->target >= MAXIDS || context->lun >= MAXLUNS) {
        return 1;
    }

    if (!context->file[(context->target << 3) | context->lun]) {
        if ( context->target == 0 && context->lun == 0 &&
            !(context->log & SCSI_LOG_NODISK0) ) {
            CRIT((ERR, "SCSI: no image attached to disk 0;"
                " expect unusual results and/or hangs" ));
            context->log |= SCSI_LOG_NODISK0;
        }
        return 2;
    }

    return 0;
}

int scsi_image_detach(struct scsi_context_s *context, int disk)
{
    if (disk < 0 || disk > 55) {
        return 2;
    }

    if (context->file[disk]) {
        fclose(context->file[disk]);
        context->file[disk] = NULL;
        return 0;
    }

    return 1;
}

void scsi_image_detach_all(struct scsi_context_s *context)
{
    int32_t i;

    for (i = 0; i < 56; i++) {
        scsi_image_detach(context, i);
    }

    return;
}

int scsi_image_attach(struct scsi_context_s *context, int disk, char *filename)
{
    if (disk < 0 || disk > 55) {
        return 2;
    }

    scsi_image_detach(context, disk);
    context->file[disk] = fopen(filename, "rb+");

    if (context->file[disk]) {
        setbuf(context->file[disk], NULL);
        return 0;
    } else {
        return 1;
    }
}

int32_t scsi_image_read(struct scsi_context_s *context)
{
    int32_t i;
    FILE *fhd;

    if (scsi_imagecheck(context)) {
        return -1;
    }

    fhd = context->file[(context->target << 3) | context->lun];

    if (fseeko(fhd, (off_t)context->address * 512, SEEK_SET) < 0) {
        CRIT((ERR, "SCSI: error seeking disk %d at sector 0x%x",
            context->target, context->address));
        return -3;
    }

    if (fread(context->data_buf, 512, 1, fhd) < 1) {
        if (!feof(fhd)) {
            CRIT((ERR, "SCSI: error reading disk %d at sector 0x%x",
                context->target, context->address));
            return -4;
        }

        /* if there is a read beyond the EOF, fill it with zeros and say it
            is good */
        for ( i = 0; i < 512; i++) {
            context->data_buf[i] = 0;
        }
    }

    LOG2((LOG, "SCSI: read disk %d at sector 0x%x", context->target,
        context->address));

    /* if the user provides it, call a function to modify the data before
       it goes back to the host */
    if (context->user_read) {
        context->user_read(context);
    }

    return 0;
}

int32_t scsi_image_write(struct scsi_context_s *context)
{
    FILE *fhd;

    if (scsi_imagecheck(context)) {
        return -1;
    }

    /* if the user provides it, call a function to modify the data before
       it writes to disk */
    if (context->user_write) {
        context->user_write(context);
    }

    fhd = context->file[(context->target << 3) | context->lun];

    if (fseeko(fhd, (off_t)context->address * 512, SEEK_SET) < 0) {
        CRIT((ERR, "SCSI: error seeking disk %d at sector 0x%x",
            context->target, context->address));
        return -3;
    }

    if (fwrite(context->data_buf, 512, 1, fhd) < 1) {
        CRIT((ERR, "SCSI: error writing disk %d at sector 0x%x",
            context->target, context->address));
        return -4;
    }
    fflush(fhd);

    LOG2((LOG, "SCSI: write disk %d at sector 0x%x", context->target,
        context->address));

    return 0;
}

/* default format handler */
/* We don't actually format the disk, we just zero out the first sector */
static void scsi_format_sector0(struct scsi_context_s *context)
{
    int32_t i;

    context->address = 0;

    for (i = 0; i < 512; i++) {
        context->data_buf[i] = 0;
    }

    scsi_image_write(context);
}

uint8_t scsi_get_bus(struct scsi_context_s *context)
{
    return context->databus;
}

int scsi_set_bus(struct scsi_context_s *context, uint8_t value)
{
    if (!context->io) {
        context->databus = value;
        return 0;
    } else {
        return 1;
    }
}

void scsi_process_noack(struct scsi_context_s *context)
{
    int32_t i, t, n;

    SDBG((LOG, "SCSI: process noack state=%02x %02x", context->state,
        context->databus));

    /* handle reset condition */
    if (context->rst) {
        context->cmd_size = 256;
        context->target = 255;
        context->bsyo = 0;
        context->req = 0;
        context->io = 0;
        context->cd = 0;
        context->msg = 0;
        context->seq = 0;
        context->link = 0;
        context->state = SCSI_STATE_BUSFREE;
        return;
    }

    if (context->state != SCSI_STATE_BUSFREE) {
        return;
    }

    /* handle SELECTION phase here */
    if (context->sel && !context->bsyo) {
        /* obtain target */
        context->target = 0;
        /* remove initiator from list */
        t = (context->databus ^ 0xff) & 0x7f;
        n = 0;
        i = 0;
        while (t) {
            if (t & 1) {
                context->target = i;
                n++;
            }
            t = t >> 1;
            i++;
        }
        if (n==1 && context->target < 7) {
            context->bsyo = 1;
            context->req = 0;
            context->seq = 0;
            SDBG((LOG, "SCSI: TARGET=%02x", context->target));
        } else {
            /* Not asking for single ID or < 7, don't respond */
            context->target = 255;
            context->cmd_size = 256;
        }
        return;
    } else if (context->sel && context->bsyo) {
        /* do nothing, we are asserting BSY waiting for SEL to drop */
        SDBG((LOG, "SCSI: waiting for SEL to drop"));
    } else if (!context->sel && context->bsyo &&
        context->state == SCSI_STATE_BUSFREE) {
        /* initiator has dropped SEL, we switch to command mode, keep
            BSY set */
        SDBG((LOG, "SCSI: ready for command"));
        context->state = SCSI_STATE_COMMAND;
        context->cmd_size = 256;
        context->req = 1;
    }

}

/* LTK DOS only seems to send out commands:
    TEST UNIT READY (0) - from boot rom
    REZERO UNIT (1) - from boot rom
    READ (8) - from boot rom and DOS
    WRITE (10) - DOS
   I'll keep other stuff since it is for CMD HD emulation.
   May also add to DOS in future.
*/

void scsi_process_ack(struct scsi_context_s *context)
{
    uint8_t data = context->databus ^ 0xff;
    uint8_t byte;
    off_t j;
    int32_t i;
    unsigned char VENDORID[9] = "VICEEMUL"; /* 8 chars */
    unsigned char PRODID[17] = "SCSI Image File "; /* 16 chars */
    char *REVISION; /* 4 chars */

    SDBG((LOG, "SCSI: process ack state=%02x", context->state));

    if (context->state == SCSI_STATE_BUSFREE) {
        return;
    }

    if (context->state == SCSI_STATE_STATUS) {
        context->cd = 1;
        if (context->link) {
            context->seq = 0;
            context->state = SCSI_STATE_COMMAND;
            context->io = 0;
        } else {
            if (context->msg_after_status) {
                /* LTK expects drive to go to MESSAGEIN after STATUS instead
                    of BUSFREE */
                context->state = SCSI_STATE_MESSAGEIN;
                context->io = 1;
                context->msg = 1;
                context->databus = 0 ^ 0xff;  /* just incase we set it 0 */
            } else {
                context->state = SCSI_STATE_BUSFREE;
                context->req = 0;
                context->io = 0;
                context->msg = 0;
                context->cd = 0;
            }
            context->bsyo = 0;
            goto out;
        }
    }

    if (context->state == SCSI_STATE_MESSAGEIN) {
        context->state = SCSI_STATE_BUSFREE;
        context->req = 0;
        context->bsyo = 0;
        context->io = 0;
        context->msg = 0;
        context->cd = 0;
        goto out;
    }

    if (context->state == SCSI_STATE_DATAOUT) {
        do {
            context->io = 0;
            context->cd = 0;
            context->data_buf[context->seq] = data;
            SDBG((LOG, "SCSI: DATAOUT[%02x]=%02x", context->seq,
                context->data_buf[context->seq]));
            context->seq++;
            if (context->seq >= context->data_max) {
                /* write if WRITE command */
                if (context->command == SCSI_COMMAND_WRITE_6 ||
                    context->command == SCSI_COMMAND_WRITE_10 ||
                    context->command == SCSI_COMMAND_WRITE_VERIFY) {
                    if (scsi_image_write(context)) {
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    context->seq = 0;
                    context->blocks--;
                    context->address++;
                    /* count down the number of blocks received, keep going
                        if necessary */
                    if (!context->blocks) {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    if (context->address >= scsi_getmaxsize(context)) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_LOGICALBLOCKADDRESSOUTOFRANGE;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                } else if (context->command == SCSI_COMMAND_FORMAT_UNIT) {
                    for (i = 0; i < 4; i++) {
                        if (context->data_buf[i]) {
                            break;
                        }
                    }
                    /* can read defect list header, but it has to be all
                        zeros */
                    if (i != 4) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_INVALIDFIELDINPARAMETERLIST;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                    } else {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                        context->user_format(context);
                    }
                } else if (context->command == SCSI_COMMAND_REASSIGN_BLOCKS) {
                    /* accept defect list, just don't do anything about it */
                    if (context->seq == 4) {
                        context->seq = ((context->data_buf[0] << 24)|
                            (context->data_buf[1] << 16)|
                            (context->data_buf[2] << 8)|
                            (context->data_buf[3])) + 4;
                        if (context->seq > 512) {
                            context->seq = 512;
                        }
                    } else {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                    }
                } else {
                /* otherwise, finish up */
                    context->status = SCSI_STATUS_GOOD;
                    context->state = SCSI_STATE_STATUS;
                }
            }
        } while (0);
    }

    if (context->state == SCSI_STATE_COMMAND) {
        do {
            context->io = 0;
            context->cd = 1;
            context->cmd_buf[context->seq] = data;
            SDBG((LOG, "SCSI: COMMAND[%u]=%02x", context->seq, data));

            if (context->seq == 0) {
                context->command = context->cmd_buf[0];
                switch (context->command) {
                case SCSI_COMMAND_TEST_UNIT_READY:
                    /* fall through */
                case SCSI_COMMAND_REZERO_UNIT:
                    /* fall through */
                case SCSI_COMMAND_REQUEST_SENSE:
                    /* fall through */
                case SCSI_COMMAND_FORMAT_UNIT:
                    /* fall through */
                case SCSI_COMMAND_REASSIGN_BLOCKS:
                    /* fall through */
                case SCSI_COMMAND_READ_6:
                    /* fall through */
                case SCSI_COMMAND_WRITE_6:
                    /* fall through */
                case SCSI_COMMAND_INQUIRY:
                    /* fall through */
                case SCSI_COMMAND_MODE_SENSE:
                    /* fall through */
                case SCSI_COMMAND_START_STOP:
                    /* fall through */
                case SCSI_COMMAND_SEND_DIAGNOSTIC:
                    context->cmd_size = 6;
                    break;
                case SCSI_COMMAND_READ_CAPACITY:
                    /* fall through */
                case SCSI_COMMAND_READ_10:
                    /* fall through */
                case SCSI_COMMAND_WRITE_10:
                    /* fall through */
                case SCSI_COMMAND_MODE_SENSE_10:
                    /* fall through */
                case SCSI_COMMAND_WRITE_VERIFY:
                    /* fall through */
                case SCSI_COMMAND_VERIFY:
                    context->cmd_size = 10;
                    break;
                default:
                    CRIT((ERR, "SCSI: Unknown Command=%0x",
                        context->cmd_buf[0]));
                    context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                    context->status = SCSI_STATUS_CHECKCONDITION;
                    context->state = SCSI_STATE_STATUS;
                    break;
                }
            }
            context->seq++;
            if (context->seq >= context->cmd_size) {
                switch (context->command) {
                case SCSI_COMMAND_TEST_UNIT_READY:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[5] & 1;
                    if (scsi_imagecheck(context)) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                    } else {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                    }
                    break;
                case SCSI_COMMAND_REQUEST_SENSE:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[5] & 1;
                    context->data_max = context->cmd_buf[4];
                    if (!context->data_max) {
                        context->data_max = 4;
                    }
                    if (context->data_max > 18) {
                        context->data_max = 18;
                    }
                    for (i = 0; i < 512; i++) {
                        context->data_buf[i] = 0;
                    }
                    context->data_buf[0] = 0x80 | 0x70;
                    context->data_buf[1] = 0x00;
                    context->data_buf[2] = context->sensekey;
                    context->data_buf[12] = context->asc;
                    context->status = SCSI_STATUS_GOOD;
                    context->state = SCSI_STATE_STATUS;
                    break;
                case SCSI_COMMAND_REASSIGN_BLOCKS:
                    context->state = SCSI_STATE_DATAOUT;
                    context->data_max = 4;
                    context->seq = 0;
                    break;
                case SCSI_COMMAND_READ_6:
                    /* fall through */
                case SCSI_COMMAND_READ_10:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    if (context->command == SCSI_COMMAND_READ_6) {
                        context->link = context->cmd_buf[5] & 1;
                        context->address = ((context->cmd_buf[1] & 0x1f) << 16)|
                            (context->cmd_buf[2] << 8)|(context->cmd_buf[3]);
                        context->blocks = context->cmd_buf[4];
                        /* if blocks=0, it really means 256 */
                        if (!context->blocks) {
                            context->blocks = 256;
                        }
                    } else {
                        context->link = context->cmd_buf[9] & 1;
                        context->address = (context->cmd_buf[2] << 24)|
                            (context->cmd_buf[3] << 16)|
                            (context->cmd_buf[4] << 8)|(context->cmd_buf[5]);
                        context->blocks = (context->cmd_buf[7] << 8)|
                            (context->cmd_buf[8]);
                        /* if blocks=0, it really means 0 */
                        if (!context->blocks) {
                            context->status = SCSI_STATUS_GOOD;
                            context->state = SCSI_STATE_STATUS;
                            break;
                        }
                    }
                    context->data_max = 512;
                    if (context->address >= scsi_getmaxsize(context)) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_LOGICALBLOCKADDRESSOUTOFRANGE;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    if (!scsi_image_read(context)) {
                        context->state = SCSI_STATE_DATAIN;
                        context->seq = 0;
                    } else {
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                    }
                    break;
                case SCSI_COMMAND_WRITE_6:
                    /* fall through */
                case SCSI_COMMAND_WRITE_10:
                    /* fall through */
                case SCSI_COMMAND_WRITE_VERIFY:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    if (context->command == SCSI_COMMAND_WRITE_6) {
                        context->link = context->cmd_buf[5] & 1;
                        context->address = ((context->cmd_buf[1] & 0x1f) << 16)|
                            (context->cmd_buf[2] << 8)|(context->cmd_buf[3]);
                        context->blocks = context->cmd_buf[4];
                        /* if blocks=0, it really means 256 */
                        if (!context->blocks) {
                            context->blocks = 256;
                        }
                    } else {
                        context->link = context->cmd_buf[9] & 1;
                        context->address = (context->cmd_buf[2] << 24)|
                            (context->cmd_buf[3] << 16)|
                            (context->cmd_buf[4] << 8)|(context->cmd_buf[5]);
                        context->blocks = (context->cmd_buf[7] << 8)|
                            (context->cmd_buf[8]);
                        /* if blocks=0, it really means 0 */
                        if (!context->blocks) {
                            context->status = SCSI_STATUS_GOOD;
                            context->state = SCSI_STATE_STATUS;
                            break;
                        }
                    }
                    context->data_max = 512;
                    if (context->address >= scsi_getmaxsize(context)) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_LOGICALBLOCKADDRESSOUTOFRANGE;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    if (!scsi_imagecheck(context)) {
                        context->state = SCSI_STATE_DATAOUT;
                        context->seq = 0;
                    } else {
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                    }
                    break;
                case SCSI_COMMAND_INQUIRY:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[5] & 1;
                    context->data_max = context->cmd_buf[4]; /* usually 96 */
                    for (i = 0; i < 512; i++) {
                        context->data_buf[i] = 0;
                    }
                    if (scsi_imagecheck(context)) {
                        context->data_buf[0] = 0x60;
                    } else {
                        context->data_buf[0] = 0x00;
                    }
                    context->data_buf[1] = 0x00; /* not removable */
                    context->data_buf[2] = 0x01; /* SCSI 1 */
                    context->data_buf[3] = 0x02;
                    context->data_buf[4] = 92; /* no additional data */
                    context->data_buf[5] = 0x00; /* res */
                    context->data_buf[6] = 0x00; /* res */
                    context->data_buf[7] = 0x00; /* no reladr, wbus32, wbus16,
                        sync, linked, cmdque, or sftre */
                    for (i = 0; i < 8; i++) {
                        context->data_buf[i + 8] = VENDORID[i];
                    }
                    for (i = 0; i < 16; i++) {
                        context->data_buf[i + 16] = PRODID[i];
                    }
                    REVISION = lib_msprintf("%s    ", VERSION);
                    for (i = 0; i < 4; i++) {
                        context->data_buf[i + 32] = REVISION[i];
                    }
                    lib_free(REVISION);
                    context->state = SCSI_STATE_DATAIN;
                    context->seq = 0;
                    break;
                case SCSI_COMMAND_READ_CAPACITY:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[9] & 1;
                    context->data_max = 8;
                    j = scsi_getmaxsize(context);
                    if (j == 0) {
                        i = 0;
                    } else {
/* Limit size returned as some tools can't handle large disks */
                        if (j > context->limit_imagesize) {
                            j = context->limit_imagesize;
                        }
                        i = 512;
/* SCSI spec says return last accessable block number */
                        j--;
                    }
                    context->data_buf[0] = (j >> 24) & 255;
                    context->data_buf[1] = (j >> 16) & 255;
                    context->data_buf[2] = (j >> 8) & 255;
                    context->data_buf[3] = j & 255;
                    context->data_buf[4] = (i >> 24) & 255;
                    context->data_buf[5] = (i >> 16) & 255;
                    context->data_buf[6] = (i >> 8) & 255;
                    context->data_buf[7] = i & 255;
                    context->state = SCSI_STATE_DATAIN;
                    context->seq = 0;
                    break;
                case SCSI_COMMAND_MODE_SENSE:
                    /* fall through */
                case SCSI_COMMAND_MODE_SENSE_10:
                    context->state = SCSI_STATE_DATAIN;
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->seq = 0;
                    if (context->command == SCSI_COMMAND_MODE_SENSE) {
                        context->data_max = context->cmd_buf[4];
                        context->link = context->cmd_buf[5] & 1;
                    } else {
                        context->data_max = (context->cmd_buf[7] << 8)|
                            (context->cmd_buf[8]);
                        if (context->data_max > 255) {
                            context->data_max = 255;
                        }
                        context->link = context->cmd_buf[9] & 1;
                    }
                    for (i = 0; i < 512; i++) {
                        context->data_buf[i] = 0;
                    }
/* CMD drive info tool looks at page 20h, so lets put that one in */
                    switch (context->cmd_buf[2]) {
                        case 0x00 | 0x20:
                            context->data_buf[3] = 1;
                            REVISION = lib_msprintf("%s-%s", VERSION, "SCSI");
                            for (i = 0; i < context->data_max - (3 + 1 + 3) &&
                                REVISION[i]; i++) {
                                context->data_buf[3 + 1 + 3 + i] = REVISION[i];
                            }
                            lib_free(REVISION);
                            context->data_buf[3 + 1 + 2] = i;
                            break;
                        default:
                            context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                            context->asc = SCSI_SASC_INVALIDFIELDINCDB;
                            context->status = SCSI_STATUS_CHECKCONDITION;
                            context->state = SCSI_STATE_STATUS;
                            break;
                    }
                    break;
                case SCSI_COMMAND_FORMAT_UNIT:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[5] & 1;
                    /* check if fmtdata is set, move on to dataout */
                    if ((context->cmd_buf[1] & 0x17) == 0x10) {
                        context->state = SCSI_STATE_DATAOUT;
                        context->seq = 0;
                        context->data_max = 4;
                    /* cmplist can be anything, but list format must be 0 */
                    } else if ((context->cmd_buf[1] & 0x17) == 0x00) {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                        context->user_format(context);
                    } else {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_INVALIDFIELDINPARAMETERLIST;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                    }
                    break;
                case SCSI_COMMAND_REZERO_UNIT:
                    /* fall through */
                case SCSI_COMMAND_START_STOP:
                    /* fall through */
                case SCSI_COMMAND_SEND_DIAGNOSTIC:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[5] & 1;
                    context->status = SCSI_STATUS_GOOD;
                    context->state = SCSI_STATE_STATUS;
                    break;
                case SCSI_COMMAND_VERIFY:
                    context->lun = (context->cmd_buf[1] >> 5) & 7;
                    context->link = context->cmd_buf[9] & 1;
                    context->status = SCSI_STATUS_GOOD;
                    context->state = SCSI_STATE_STATUS;
                    break;
                }
            }
        } while (0);
    }

    if (context->state == SCSI_STATE_DATAIN) {
        do {
            if (context->seq >= context->data_max) {
                /* reload more data if READ command */
                if (context->command == SCSI_COMMAND_READ_6 ||
                    context->command == SCSI_COMMAND_READ_10) {
                    context->seq = 0;
                    context->blocks--;
                    context->address++;
                    /* count down the number of blocks sent, keep going if
                        necessary */
                    if (!context->blocks) {
                        context->status = SCSI_STATUS_GOOD;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    /* if we are out of range, report an error */
                    if (context->address >= scsi_getmaxsize(context)) {
                        context->sensekey = SCSI_SENSEKEY_ILLEGALREQUEST;
                        context->asc = SCSI_SASC_LOGICALBLOCKADDRESSOUTOFRANGE;
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    /* read it, report error if a problem */
                    if (scsi_image_read(context)) {
                        context->status = SCSI_STATUS_CHECKCONDITION;
                        context->state = SCSI_STATE_STATUS;
                        break;
                    }
                    /* all is good, keep state the same */
                } else {
                    /* otherwise, finish up */
                    context->status = SCSI_STATUS_GOOD;
                    context->state = SCSI_STATE_STATUS;
                    break;
                }
                /* never gets here */
            }
            byte = context->data_buf[context->seq];
            SDBG((LOG, "SCSI: DATAIN[%02x]=%02x", context->seq,
                context->data_buf[context->seq]));
            context->seq++;
            context->databus = byte ^ 0xff;
            context->io = 1;
            context->cd = 0;
        } while (0);
    }

    if (context->state == SCSI_STATE_STATUS) {
        byte = (context->status << 1) | 0;
        SDBG((LOG, "SCSI: STATUS=%02x", context->status));
        context->io = 1;
        context->databus = byte ^ 0xff;
        context->cd = 1;
    }

    if (context->state == SCSI_STATE_DATAOUT) {
        context->cd = 0;
    }

out:
    return;
}

void scsi_reset(struct scsi_context_s *context)
{
    context->max_imagesize = 512 * 1024 * 1024 / 512;
    context->limit_imagesize = 512 * 1024 * 1024 / 512;
    context->rst = 1;
    scsi_process_noack(context);
    context->rst = 0;
    context->msg_after_status = 0;
    context->user_format = scsi_format_sector0;
    context->user_read = NULL;
    context->user_write = NULL;
    context->log = 0;

    return;
}

/* ---------------------------------------------------------------------*/

/* SCSI snapshot module format:

   type   | name               | description
   -----------------------------------------------------
   BYTE   | state              | state
   BYTE   | target             | target
   BYTE   | databus            | databus
   BYTE   | ack                | ack
   BYTE   | req                | req
   BYTE   | bsyi               | bsyi
   BYTE   | bsyo               | bsyo
   BYTE   | sel                | sel
   BYTE   | rst                | rst
   BYTE   | atn                | atn
   BYTE   | cd                 | cd
   bYTE   | io                 | io
   BYTE   | msg                | msg
   BYTE   | link               | link
   BYTE   | status             | status
   BYTE   | lun                | lun
   BYTE   | command            | command
   BYTE   | sensekey           | sensekey
   BYTE   | asc                | asc
   BYTE   | msg_after_status   | msg_after_status
   DWORD  | seq                | seq
   DWORD  | cmd_size           | cmd_size
   DWORD  | address            | address
   DWORD  | blocks             | blocks
   DWORD  | data_max           | data_max
   DWORD  | max_imagesize      | max_imagesize
   ARRAY  | cmd_buf            | 256 bytes of cmd_buf
   ARRAY  | data_buf           | 512 bytes of data_buf

*/

#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int scsi_snapshot_write_module(struct scsi_context_s *context, snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, context->myname, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_B(m, (uint8_t)context->state) < 0)
        || (SMW_B(m, (uint8_t)context->target) < 0)
        || (SMW_B(m, (uint8_t)context->databus) < 0)
        || (SMW_B(m, (uint8_t)context->ack) < 0)
        || (SMW_B(m, (uint8_t)context->req) < 0)
        || (SMW_B(m, (uint8_t)context->bsyi) < 0)
        || (SMW_B(m, (uint8_t)context->bsyo) < 0)
        || (SMW_B(m, (uint8_t)context->sel) < 0)
        || (SMW_B(m, (uint8_t)context->rst) < 0)
        || (SMW_B(m, (uint8_t)context->atn) < 0)
        || (SMW_B(m, (uint8_t)context->cd) < 0)
        || (SMW_B(m, (uint8_t)context->io) < 0)
        || (SMW_B(m, (uint8_t)context->msg) < 0)
        || (SMW_B(m, (uint8_t)context->link) < 0)
        || (SMW_B(m, (uint8_t)context->status) < 0)
        || (SMW_B(m, (uint8_t)context->lun) < 0)
        || (SMW_B(m, (uint8_t)context->command) < 0)
        || (SMW_B(m, (uint8_t)context->sensekey) < 0)
        || (SMW_B(m, (uint8_t)context->asc) < 0)
        || (SMW_B(m, (uint8_t)context->msg_after_status) < 0)
        || (SMW_DW(m, (uint32_t)context->seq) < 0)
        || (SMW_DW(m, (uint32_t)context->cmd_size) < 0)
        || (SMW_DW(m, (uint32_t)context->address) < 0)
        || (SMW_DW(m, (uint32_t)context->blocks) < 0)
        || (SMW_DW(m, (uint32_t)context->data_max) < 0)
        || (SMW_DW(m, (uint32_t)context->max_imagesize) < 0)
        || (SMW_BA(m, context->cmd_buf, 256) < 0)
        || (SMW_BA(m, context->data_buf, 512) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int scsi_snapshot_read_module(struct scsi_context_s *context, snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, context->myname, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_B(m, &context->state) < 0)
        || (SMR_B(m, &context->target) < 0)
        || (SMR_B(m, &context->databus) < 0)
        || (SMR_B(m, &context->ack) < 0)
        || (SMR_B(m, &context->req) < 0)
        || (SMR_B(m, &context->bsyi) < 0)
        || (SMR_B(m, &context->bsyo) < 0)
        || (SMR_B(m, &context->sel) < 0)
        || (SMR_B(m, &context->rst) < 0)
        || (SMR_B(m, &context->atn) < 0)
        || (SMR_B(m, &context->cd) < 0)
        || (SMR_B(m, &context->io) < 0)
        || (SMR_B(m, &context->msg) < 0)
        || (SMR_B(m, &context->link) < 0)
        || (SMR_B(m, &context->status) < 0)
        || (SMR_B(m, &context->lun) < 0)
        || (SMR_B(m, &context->command) < 0)
        || (SMR_B(m, &context->sensekey) < 0)
        || (SMR_B(m, &context->asc) < 0)
        || (SMR_B(m, &context->msg_after_status) < 0)
        || (SMR_DW(m, &context->seq) < 0)
        || (SMR_DW(m, &context->cmd_size) < 0)
        || (SMR_DW(m, &context->address) < 0)
        || (SMR_DW(m, &context->blocks) < 0)
        || (SMR_DW(m, &context->data_max) < 0)
        || (SMR_DW(m, &context->max_imagesize) < 0)
        || (SMR_BA(m, context->cmd_buf, 256) < 0)
        || (SMR_BA(m, context->data_buf, 512) < 0)) {
        goto fail;
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
