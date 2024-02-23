/*
 * vdrive-command.c - Virtual disk-drive implementation. Command interpreter.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Multi-drive and DHD enhancements by
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

#include "vice.h"

/* #define DEBUG_DRIVE */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cbmdos.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "machine-drive.h"
#include "types.h"
#include "util.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive-rel.h"
#include "vdrive.h"
#include "diskconstants.h"
#include "serial.h"

#define VDRIVE_IS_IMAGE(a) (serial_device_type_get(a->unit - 8) == SERIAL_DEVICE_VIRT)

#define IP_MAX_COMMAND_LEN 128 /* real 58 */


static log_t vdrive_command_log = LOG_ERR;

static int vdrive_command_format_internal(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_format_worker(struct vdrive_s *vdrive, uint8_t *disk_name, uint8_t *disk_id);
static int vdrive_command_validate_internal(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_block(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_memory(vdrive_t *vdrive, uint8_t *buffer, unsigned int length);
static int vdrive_command_initialize(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_copy(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_chdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_mkdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_rmdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_chpart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_chcmdpart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_rename(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_renameheader(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_renamepart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_scratch(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_position(vdrive_t *vdrive, uint8_t *buf, unsigned int length);
static int vdrive_command_u1a2b(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_lockunlock(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd);
static int vdrive_command_time(vdrive_t *vdrive, uint8_t *cmd, int length);
static int vdrive_command_getpartinfo(vdrive_t *vdrive, const uint8_t *cmd, int length);
static int vdrive_command_deletepart(vdrive_t *vdrive, const uint8_t *cmd, int length);

static void vdrive_cmdfree_full(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->full) {
        lib_free((void *)cmd->full);
        cmd->full = NULL;
    }
}

static void vdrive_cmdfree_abbrv(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->abbrv) {
        lib_free(cmd->abbrv);
        cmd->abbrv = NULL;
    }
}

static void vdrive_cmdfree_path(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->path) {
        lib_free(cmd->path);
        cmd->path = NULL;
    }
}

static void vdrive_cmdfree_file(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->file) {
        lib_free(cmd->file);
        cmd->file = NULL;
    }
}

static void vdrive_cmdfree_command(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->command) {
        lib_free(cmd->command);
        cmd->command = NULL;
    }
}

static void vdrive_cmdfree_more(cbmdos_cmd_parse_plus_t *cmd) {
    if (cmd->more) {
        lib_free(cmd->more);
        cmd->more = NULL;
    }
}

static int vdrive_haswildcard(uint8_t *name, int length)
{
    uint8_t *check;

    /* check for wildcards anywhere in the name */
    check = memchr(name, '*', length);
    if (check) {
        return 1;
    }
    check = memchr(name, '?', length);
    if (check) {
        return 1;
    }

    return 0;
}

static unsigned int vdrive_atoi(char *namein, char **nameout, unsigned int length)
{
    unsigned int i = 0, j = 0;

    while (j < length && namein[j] >= '0' && namein[j] <= '9') {
      i = i * 10 + (namein[j] - '0');
      j ++;
    }
    if (nameout) {
        *nameout = &(namein[j]);
    }

    return i;
}

void vdrive_command_init(void)
{
    vdrive_command_log = log_open("VDriveCommand");
}

/* called by vdrive-iec.c; return code not used */
int vdrive_command_execute(vdrive_t *vdrive, const uint8_t *buf,
                           unsigned int length)
{
    int status = CBMDOS_IPE_INVAL, statret1 = 0;
    uint8_t *p;
    cbmdos_cmd_parse_plus_t cmd;

    if (!length) {
        return CBMDOS_IPE_OK;
    }
    if (length > IP_MAX_COMMAND_LEN) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_LONG_LINE, 0, 0);
        return CBMDOS_IPE_LONG_LINE;
    }

    /* process commands which shouldn't have the CR stripped */
    /* they check the image context internally before proceeding */
    switch (buf[0]) {
        case 'G': /* get partition info */
            status = vdrive_command_getpartinfo(vdrive, buf, length);
            goto out3;
        case 'D': /* delete partition */
            status = vdrive_command_deletepart(vdrive, buf, length);
            goto out3;
        case 'M': /* Memory */
            if (length > 2 && buf[1] == '-') {
                status = vdrive_command_memory(vdrive, (uint8_t *)buf, length);
                goto out3;
            }
            break;
    }

#if 0
    /* if no image context leave as just about everything after this needs one */
    if (!vdrive->image) {
        return vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_READY, 0, 0);
    }
#endif

    /* status = CBMDOS_IPE_INVAL; */

    if (buf[length - 1] == 0x0d) {
        --length; /* chop CR character */
    }

    p = lib_malloc(length + 1);
    memcpy(p, buf, length);
    p[length] = 0;

#ifdef DEBUG_DRIVE
    log_debug("Command '%c' (%s).", *p, p);
#endif

    cmd.full = p;
    cmd.fulllength = length;

    /* process commands which are binary specific, no abbreviations, etc. */
    switch (*p) {
        case 'P': /* Position - update error channel */
            status = vdrive_command_position(vdrive, p, length);
            vdrive_command_set_error(vdrive, status, 0, 0);
            goto out2;
        case 'T': /* Time - don't update status - it does */
            status = vdrive_command_time(vdrive, p, length);
            goto out2;

    }

    cmd.secondary = 0;
    cmd.mode = 1;

    status = cbmdos_command_parse_plus(&cmd);

    /* leave if no command provided */
    if (cmd.commandlength == 0) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("Command '%s' (%u) %d.", cmd.command, cmd.commandlength, status);
    log_debug("File    '%s' (%u).", cmd.file, cmd.filelength);
    log_debug("Drive   '%d'.", cmd.drive);
#endif

    if (status != CBMDOS_IPE_OK) {
        goto out;
    }

/* check for unabbreviated commands */

    switch (cmd.command[0]) {
        case 'C':       /* CD, CP, copy moves to next case */
            if (cmd.commandlength > 1 && cmd.command[1] == 'D'
                && vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
                status = vdrive_command_chdir(vdrive, &cmd);
                goto out;
            } else if (cmd.commandlength > 1 && (cmd.command[1] & 0x7f) == 'P') {
                status = vdrive_command_chcmdpart(vdrive, &cmd);
                /* it sets the error */
                goto out1;
            }
            break;

        case 'U': /* User */
            if (cmd.commandlength > 1) {
                switch (cmd.command[1]) {
                    case 'A': /* UA */
                    case '1': /* U1 */
                    case 'B': /* UB */
                    case '2': /* U2 */
                        status = vdrive_command_u1a2b(vdrive, &cmd);
                        goto out1;

                    case 'C': /* Jumps */
                    case '3':
                    case 'D':
                    case '4':
                    case 'E':
                    case '5':
                    case 'F':
                    case '6':
                    case 'G':
                    case '7':
                    case 'H':
                    case '8':
                        if (cmd.commandlength == 2) {
                            status = CBMDOS_IPE_NOT_READY;
                        }
                        goto out;

                    case 'I': /* UI */
                    case '9': /* U9 */
                        if (cmd.commandlength == 3 && (cmd.command[2] == '-' || cmd.command[2] == '+')) {
                            status = CBMDOS_IPE_OK; /* Set IEC bus speed */
                            goto out;
                        } else {
                            if (!vdrive_command_initialize(vdrive, NULL)) {
                                vdrive_close_all_channels(vdrive);
                                status = CBMDOS_IPE_DOS_VERSION;
                            }
                        }
                        goto out;

                    case 'J': /* UJ */ /* CMDHD/FD2000/F4000 also have UJP to reload partition table */
                        if (cmd.commandlength > 2 && cmd.command[2] == 'P' &&
                            vdrive->haspt ) {
                            vdrive_close_all_channels(vdrive); /* Warm/Cold reset */
                            vdrive->sys_offset = UINT32_MAX;
                            vdrive->current_offset = UINT32_MAX;
                            if (!vdrive_read_partition_table(vdrive)) {
                                /* try to switch to default */
                                if (vdrive_command_switch(vdrive, vdrive->default_part)) {
                                    /* didn't work, make it the "selected" one anyways */
                                    vdrive->selected_part = vdrive->default_part;
                                }
                                /* error is dos version regardless of outcome */
                                status = CBMDOS_IPE_DOS_VERSION;
                                goto out;
                            }
                            status = CBMDOS_IPE_NOT_READY;
                            goto out;
                        }
                        status = CBMDOS_IPE_DOS_VERSION;
                        goto out;

                    case ':': /* U: */
                        vdrive_close_all_channels(vdrive); /* Warm/Cold reset */
                        status = CBMDOS_IPE_DOS_VERSION;
                        goto out;

                    case '0': /* U0 */
                        if (cmd.commandlength == 2 && cmd.command[1] == '0') {
                            status = CBMDOS_IPE_OK;
                            goto out;
                        }
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    default: /* any thing else */
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                }
            }
            goto out;

        case 'M':       /* MD - make sub dir */
            if (cmd.commandlength > 1 && cmd.command[1] == 'D' &&
                vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
                status = vdrive_command_mkdir(vdrive, &cmd);
                goto out;
            }
            break;

        case 'R':       /* RD */
            if (cmd.commandlength > 1 && cmd.command[1] == 'D' &&
                vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
                status = vdrive_command_rmdir(vdrive, &cmd);
                goto out1;
            }
            break;
    }


/* check for abbreviated commands */

    if (cmd.abbrvlength == 0) {
        status = CBMDOS_IPE_INVAL;
        goto out;
    }

    switch (cmd.abbrv[0]) {
        case 'C':       /* Copy */
            status = vdrive_command_copy(vdrive, &cmd);
            goto out;

        case '/':       /* change partition */
            if ((vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) ||
                (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP)) {
                status = vdrive_command_chpart(vdrive, &cmd);
                goto out1;
            }
            break;

#if 0
        case 'D':       /* Duplicate is unused */
            break;
#endif

        case 'R':       /* R-H, R-P, or R */
            if (cmd.abbrvlength > 2 && cmd.abbrv[1] == '-' && cmd.abbrv[2] == 'H') {
                status = vdrive_command_renameheader(vdrive, &cmd);
            } else if (cmd.abbrvlength > 2 && cmd.abbrv[1] == '-' && cmd.abbrv[2] == 'P') {
                status = vdrive_command_renamepart(vdrive, &cmd);
            } else {
                status = vdrive_command_rename(vdrive, &cmd);
            }
            break;

        case 'L':       /* Lock/Unlock */
            status = vdrive_command_lockunlock(vdrive, &cmd);
            break;

        case 'S':       /* Scratch */
            status = vdrive_command_scratch(vdrive, &cmd);
            goto out1; /* error message is set by function */

        case 'I':       /* initialize */
            status = vdrive_command_initialize(vdrive, &cmd);
            break;

        case 'N':       /* new */
            status = vdrive_command_format_internal(vdrive, &cmd);
            break;

        case 'V':       /* validate */
            status = vdrive_command_validate_internal(vdrive, &cmd);
            goto out1; /* error message is set by function */

        case 'B':       /* Block, Buffer */
            status = vdrive_command_block(vdrive, &cmd);
            goto out1; /* error message is set by function */

        default:
            break;
    } /* commands */

out:

    if (status == CBMDOS_IPE_INVAL) {
        log_error(vdrive_command_log, "Wrong command `%s'.", p);
    }

    vdrive_command_set_error(vdrive, status, statret1, 0);

out1:

    vdrive_cmdfree_abbrv(&cmd);
    vdrive_cmdfree_path(&cmd);
    vdrive_cmdfree_file(&cmd);
    vdrive_cmdfree_command(&cmd);
    vdrive_cmdfree_more(&cmd);

out2:
    vdrive_cmdfree_full(&cmd);

out3:
    return status;
}

static int vdrive_command_get_block_parameters(char *buf, int *p1, int *p2, int *p3,
                                               int *p4)
{
    int ip;
    char *bp, endsign;
    int *p[4];  /* This is a kludge */
    p[0] = p1;
    p[1] = p2;
    p[2] = p3;
    p[3] = p4;

    bp = buf;

    for (ip = 0; ip < 4; ip++) {
        /* 1541 firmware skips 0x20, 0x2c, and 0x1d */
        while (*bp == ' ' || *bp == ')' || *bp == ',' || *bp == '#' || *bp == 0x1d) {
            bp++;
        }
        if (*bp == 0) {
            break;
        }
        /* Convert and skip over decimal number.  */
        *p[ip] = (int)strtol(bp, &bp, 10);
    }
    endsign = *bp;
    if (isalnum((int)endsign) && (ip == 4)) {
        return CBMDOS_IPE_SYNTAX;
    }
    return -ip;                 /* negative of # arguments found */
}

static int vdrive_command_u1a2b(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    /* set all parameters to 0 by default */
    int channel = 0, drive = 0, track = 0, sector = 0;
    int rc;
    int origpart = -1;

#ifdef DEBUG_DRIVE
    log_debug("vdrive_command_ua1b2 command: %c.", cmd->command[1]);
#endif

    status = CBMDOS_IPE_SYNTAX;

    /* handle 1581 u-R and u-W cases */
    if (cmd->abbrvlength > 2 && cmd->abbrv[0] == 'U' && cmd->abbrv[1] == '-') {
        if (cmd->abbrv[2] == 0xd2) {
            /* u-R */
            cmd->command[1] = '1';
        } else if (cmd->abbrv[2] == 0xd7) {
            /* u-W */
            cmd->command[1] = '2';
        } else {
            status = CBMDOS_IPE_SYNTAX;
            goto out;
        }
    }

    if (cmd->commandlength > 1 && cmd->filelength > 5) {
        rc = vdrive_command_get_block_parameters((char*)cmd->file, &channel,
                                                 &drive, &track, &sector);
        if (rc < 0) {
#ifdef DEBUG_DRIVE
            log_debug("b-R/W parsed OK. (np=%d) channel %d mode %u, "
                      "drive=%d, track=%d sector=%d.", rc, channel,
                      vdrive->buffers[channel].mode, drive, track, sector);
#endif

            if (vdrive->buffers[channel].mode != BUFFER_MEMORY_BUFFER) {
                status = CBMDOS_IPE_NO_CHANNEL;
                track = 0;
                sector = 0;
                goto out;
            }

            if (vdrive->haspt) {
                /* CMD HD/FDs don't allow "drive" to be anything other than
                    0. The current partition is used when "#" is opened. */
                if (drive != 0 ) {
                    status = CBMDOS_IPE_NOT_READY;
                    track = 0;
                    sector = 0;
                    goto out;
                } else {
                    drive = vdrive->buffers[channel].partition;
                }
            }
            /* drive is honored when using dual drives; checked next */

            origpart = vdrive->current_part;
            status = CBMDOS_IPE_NOT_READY;
            /* if we can't switch to it, error out */
            if (vdrive_command_switch(vdrive, drive)) {
                track = 0;
                sector = 0;
                goto out;
            }

            if (cmd->command[1] == '2' || cmd->command[1] == 'B') {
                /* For write */
#if 0
                if (vdrive->image->read_only) {
                    status = CBMDOS_IPE_WRITE_PROTECT_ON;
                    goto out;
                }
#endif
                status = vdrive_write_sector(vdrive, vdrive->buffers[channel].buffer, track, sector);
                if (status > 0) {
                    goto out;
                } else if (status < 0) {
                    track = 0;
                    sector = 0;
                    status = CBMDOS_IPE_NOT_READY;
                    goto out;
                }
            } else if (cmd->command[1] == '1' || cmd->command[1] == 'A') {
                /* For read */
                status = vdrive_read_sector(vdrive, vdrive->buffers[channel].buffer, track, sector);
                if (status > 0) {
                    goto out;
                } else if (status < 0) {
                    track = 0;
                    sector = 0;
                    status = CBMDOS_IPE_NOT_READY;
                    goto out;
                }
            }
            vdrive->buffers[channel].bufptr = 0;
            track = 0;
            sector = 0;
        } else {
            log_error(vdrive_command_log, "U1/A/2/B invalid parameter "
                      "C:%i D:%i T:%i S:%i.", channel, drive, track, sector);
            status = rc;
            track = 0;
            sector = 0;
        }
    }
out:

    vdrive_command_return(vdrive, origpart);

    vdrive->last_code = CBMDOS_IPE_OK;

    return vdrive_command_set_error(vdrive, status, track, sector);
}


static int vdrive_command_block(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    /* set all parameters to 0 by default */
    int channel = 0, drive = 0, track = 0, sector = 0, position = 0;
    int t = 0, s = 0;
    int l;
    int origpart = -1;

#ifdef DEBUG_DRIVE
    log_debug("vdrive_command_block command: %c.", cmd->abbrv[2]);
#endif

    /* status = CBMDOS_IPE_SYNTAX; */

    switch (cmd->abbrv[2]) {
        /* Old-style B-R and B-W */
        case 'R':
        case 'W':
            l = vdrive_command_get_block_parameters((char*)cmd->file, &channel,
                                                    &drive, &track, &sector);

            if (l < 0) {
#ifdef DEBUG_DRIVE
                log_debug("b-r/w parsed OK. (l=%d) channel %d mode %u, "
                          "drive=%d, track=%d sector=%d.", l, channel,
                          vdrive->buffers[channel].mode, drive, track, sector);
#endif

                if (vdrive->buffers[channel].mode != BUFFER_MEMORY_BUFFER) {
                    status = CBMDOS_IPE_NO_CHANNEL;
                    goto out;
                }

                if (vdrive->haspt) {
                    /* CMD HD/FDs don't allow "drive" to be anything other than
                        0. The current partition is used when "#" is opened. */
                    if (drive != 0 ) {
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    } else {
                        drive = vdrive->buffers[channel].partition;
                    }
                }
                /* drive is honored when using dual drives; checked next */

                origpart = vdrive->current_part;
                status = CBMDOS_IPE_NOT_READY;
                if (vdrive_command_switch(vdrive, drive)) {
                    goto out;
                }

                if (cmd->abbrv[2] == 'W') {
                    /* For write */
#if 0
                    if (vdrive->image->read_only) {
                        status = CBMDOS_IPE_WRITE_PROTECT_ON;
                        t = track;
                        s = sector;
                        goto out;
                    }
#endif
                    /* Update length of block based on the buffer pointer. */
                    l = vdrive->buffers[channel].bufptr - 1;
                    vdrive->buffers[channel].buffer[0] = ( l < 1 ? 1 : l );
                    status = vdrive_write_sector(vdrive, vdrive->buffers[channel].buffer, track, sector);
                    if (status < 0) {
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    } else if (status > 0) {
                        goto out;
                    }
                    /* after write, buffer pointer is 1. */
                    vdrive->buffers[channel].bufptr = 1;
                } else {
                    /* For read */
                    status = vdrive_read_sector(vdrive, vdrive->buffers[channel].buffer, track, sector);
                    if (status > 0) {
                        t = track;
                        s = sector;
                        goto out;
                    } else if (status < 0) {
                        t = track;
                        s = sector;
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    }
                    /* set buffer length base on first value */
                    vdrive->buffers[channel].length =
                        vdrive->buffers[channel].buffer[0] + 1;
                    /* buffer pointer is 1, not 0. */
                    vdrive->buffers[channel].bufptr = 1;
                }
            } else {
                log_error(vdrive_command_log, "b-r/w invalid parameter "
                          "C:%i D:%i T:%i S:%i.", channel, drive, track, sector);
                status = l;
                goto out;
            }
            break;
        case 'A':
        case 'F':
            if (VDRIVE_IS_READONLY(vdrive)) {
                status = CBMDOS_IPE_WRITE_PROTECT_ON;
                goto out;
            }

            l = vdrive_command_get_block_parameters((char*)cmd->file, &drive,
                                                    &track, &sector, &channel);
            if (l > 0) { /* just 3 args used */
                status = l;
                goto out;
            }

            if (vdrive->haspt) {
                /* CMD HD/FDs don't allow "drive" to be anything other than
                    0. The current partition is used when "#" is opened. */
                if (drive != 0 ) {
                    status = CBMDOS_IPE_NOT_READY;
                    goto out;
                } else {
                    drive = vdrive->buffers[channel].partition;
                }
            }
            /* drive is honored when using dual drives; checked next */

            origpart = vdrive->current_part;
            status = CBMDOS_IPE_NOT_READY;
            if (vdrive_command_switch(vdrive, drive)) {
                goto out;
            }

            if (cmd->abbrv[2] == 'A') {
                if (!vdrive_bam_allocate_sector(vdrive, track, sector)) {
                    /*
                     * Desired sector not free. Suggest another. XXX The 1541
                     * uses an inferior search function that only looks on
                     * higher tracks and can return sectors in the directory
                     * track.
                     */
                    if (vdrive_bam_alloc_next_free_sector(vdrive,
                        (unsigned int*)&track, (unsigned int *)&sector) >= 0) {
                        /* Deallocate it and merely suggest it */
                        vdrive_bam_free_sector(vdrive, track, sector);
                    } else {
                        /* Found none */
                        track = 0;
                        sector = 0;
                    }
                    t = track;
                    s = sector;
                    status = CBMDOS_IPE_NO_BLOCK;
                    goto out;
                }
            } else {
                vdrive_bam_free_sector(vdrive, track, sector);
            }
            status = CBMDOS_IPE_OK;
            goto out;
            break;
        case 'P':
            l = vdrive_command_get_block_parameters((char*)cmd->file, &channel,
                                                    &position, &track, &sector);
            if (l > 0) { /* just 2 args used */
                status = l;
                goto out;
            }
            if (vdrive->buffers[channel].mode != BUFFER_MEMORY_BUFFER) {
                status = CBMDOS_IPE_NO_CHANNEL;
                goto out;
            }
            vdrive->buffers[channel].bufptr = position;
            status = CBMDOS_IPE_OK;
            goto out;
            break;
        case 'E':
            log_warning(vdrive_command_log, "B-E: %d %d %d %d (needs TDE)", channel, drive, track, sector);
            status = CBMDOS_IPE_OK;
            goto out;
            break;
        default:
            status = CBMDOS_IPE_INVAL;
            goto out;
    }

out:
    vdrive_command_return(vdrive, origpart);

    return vdrive_command_set_error(vdrive, status, t, s);
}

/*
    The buffer pointer passed to this function points to the character
    following '-' in the memory command.

    The payload, address, and length of payload is passed to the function.
*/
static int vdrive_command_memory(vdrive_t *vdrive, uint8_t *buffer, unsigned int length)
{
    uint16_t addr;

    if (length < 5) {
        return vdrive_command_set_error(vdrive, CBMDOS_IPE_SYNTAX, 0, 0);
    }

    addr = util_le_buf_to_word(&(buffer[3]));

    switch (buffer[2]) {
        case 'W':
            return vdrive_command_memory_write(vdrive, &(buffer[5]), addr, length);
        case 'R':
            return vdrive_command_memory_read(vdrive, &(buffer[5]), addr, length);
        case 'E':
            return vdrive_command_memory_exec(vdrive, NULL, addr, length);
    }

    return vdrive_command_set_error(vdrive, CBMDOS_IPE_INVAL, 0, 0);
}

/* CMD HD states you can concatenate upto 5 files, but it only checks to see if
    the first 4 are present. It first checks to see if the files exist, if
    not, it gives a file not found. It then checks to see if the destination
    does; if so it gives a file exists.  It seems to create the new file with
    the type of the first file being copied, but it will copy any other file
    type when concatenating.  We only allow copying of CLOSED PRG, SEQ, or USR
    files. REL is concatenated too if the record lengths are the same. */
#define COPYBUFFER 32
#define COPYMAX 6
static int vdrive_command_copy(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    int t[COPYMAX], s[COPYMAX], es = 0;
    uint8_t *slot;
    vdrive_dir_context_t dir;
    cbmdos_cmd_parse_plus_t dest, src[COPYMAX];
    uint8_t delimiter[COPYMAX];
    int newtype = 0, newlength = 0;
    int files = 0;
    uint8_t *wb = NULL, *rb = NULL;
    unsigned int wt, ws, nt, ns, rt, rs;
    int ssn, super = 0;
    unsigned int sst, sss, ssst, ssss;
    int wp, rn;
    int i, k, m, x;
    bufferinfo_t *p = &(vdrive->buffers[15]);

    /* backup current partition information */
    int origpart = vdrive->current_part;

    /* initialize the source registers */
    for (i = 0; i <COPYMAX; i++) {
        src[i].file = NULL;
        src[i].filelength = 0;
        src[i].path = NULL;
        src[i].pathlength = 0;
        src[i].drive = 0;
    }

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    status = CBMDOS_IPE_SYNTAX;

    /* bail on wildcards in destination */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }

    dest.file = cmd->file;
    dest.filelength = cmd->filelength;
    dest.path = cmd->path;
    dest.pathlength = cmd->pathlength;
    dest.drive = vdrive_realpart(vdrive, cmd->drive);

    /* use the parser plus to go through all the passed drive/part/names
        separated by , and = 's */
    for (i = 0; i < COPYMAX; i++) {
        if (cmd->morelength && cmd->more && (cmd->more[0] == '='
            || cmd->more[0] == ',')) {
            delimiter[i] = cmd->more[0];
            vdrive_cmdfree_full(cmd);
            vdrive_cmdfree_abbrv(cmd);
            vdrive_cmdfree_command(cmd);
            cmd->full = cmd->more;
            cmd->fulllength = cmd->morelength;
            rc = cbmdos_command_parse_plus(cmd);
            if (rc) {
                goto out;
            }

            src[i].file = cmd->file;
            src[i].filelength = cmd->filelength;
            src[i].path = cmd->path;
            src[i].pathlength = cmd->pathlength;
            src[i].drive = vdrive_realpart(vdrive, cmd->drive);
        } else {
            break;
        }
    }
    /* set these to NULL so caller doesn't free them; we will do this later
        on here */
    cmd->file = NULL;
    cmd->path = NULL;

    files = i;

#if 0
    printf("files=%d\n", files);
    for (i = 0; i < files; i++) {
        printf("files%d: file=%s, length=%u\n", i, src[i].file, src[i].filelength);
        printf("files%d: path=%s, plength=%u\n", i, src[i].path, src[i].pathlength);
        printf("files%d: drive=%d, delim=%c\n", i, src[i].drive, delimiter[i]);
    }
#endif

    /* bail if no additional files provided */
    if (!files) {
        goto out;
    }

    /* find the starting point of each source file */
    for (i = 0; i < files; i++) {
        /* bail on wildcards */
        if (vdrive_haswildcard(src[i].file, src[i].filelength)) {
            goto out;
        }

        /* bail if the delimiters between the files aren't correct */
        if ( (i == 0 && delimiter[i] != '=') || (i && delimiter[i] != ',') ) {
            goto out;
        }

        /* find the base dir entry */
        status = vdrive_command_switchtraverse(vdrive, &(src[i]));

        /* bail if we can't find the path */
        if (status) {
            goto out;
        }

        /* search for file */
        vdrive_dir_find_first_slot(vdrive, src[i].file, src[i].filelength,
            CBMDOS_FT_DEL, &dir);
        slot = vdrive_dir_find_next_slot(&dir);

        /* if found, and it is good, copy the starting points and estimate
            the final block size */
        if (slot) {
            /* error out on DIR, CBM, and REL entries */
            if ((slot[SLOT_TYPE_OFFSET] != (CBMDOS_FT_PRG | CBMDOS_FT_CLOSED) ) &&
                (slot[SLOT_TYPE_OFFSET] != (CBMDOS_FT_SEQ | CBMDOS_FT_CLOSED) ) &&
                (slot[SLOT_TYPE_OFFSET] != (CBMDOS_FT_USR | CBMDOS_FT_CLOSED) ) &&
                (slot[SLOT_TYPE_OFFSET] != (CBMDOS_FT_REL | CBMDOS_FT_CLOSED) )) {
                goto out;
            }
            if (i == 0) {
                newtype = slot[SLOT_TYPE_OFFSET];
                if (slot[SLOT_TYPE_OFFSET] == (CBMDOS_FT_REL | CBMDOS_FT_CLOSED)) {
                    newlength = slot[SLOT_RECORD_LENGTH];
                }
            }
            /* make sure lengths are the same, if not, syntax error? */
            if (newlength && newlength != slot[SLOT_RECORD_LENGTH]) {
                goto out;
            }
            t[i] = slot[SLOT_FIRST_TRACK];
            s[i] = slot[SLOT_FIRST_SECTOR];
            es += (slot[SLOT_NR_BLOCKS] | (slot[SLOT_NR_BLOCKS + 1] << 8 ));
        } else {
            status = CBMDOS_IPE_NOT_FOUND;
            goto out;
        }
    }

    /* make sure the destination partition is good */
    status = vdrive_command_switchtraverse(vdrive, &dest);
    if (status) {
        goto out;
    }

    /* roughly check to see if we have space */
    if (vdrive_bam_free_block_count(vdrive) < (es - files + 1)) {
        status = CBMDOS_IPE_DISK_FULL;
        goto out;
    }

    /* look for destination entry */
    vdrive_dir_find_first_slot(vdrive, dest.file, dest.filelength,
        CBMDOS_FT_DEL, &dir);

    slot = vdrive_dir_find_next_slot(&dir);

    /* error out if it exists */
    if (slot) {
        status = CBMDOS_IPE_FILE_EXISTS;
        goto out;
    }

    status = CBMDOS_IPE_DISK_FULL;

    /* if this is null, do not unallocate rel space on abnormal exit */
    p->side_sector = NULL;

    /* we can make it now */
    /* find empty entry */
    vdrive_dir_find_first_slot(vdrive, dest.file, -1, CBMDOS_FT_DEL, &dir);
    slot = vdrive_dir_find_next_slot(&dir);
    if (slot) {
        /* clear dir out entry */
        memset(&(dir.buffer[dir.slot * 32 + 2]), 0, 30);
        /* copy name from original search */
        memcpy(&(dir.buffer[dir.slot * 32 + SLOT_NAME_OFFSET]),
            dir.find_nslot, CBMDOS_SLOT_NAME_LENGTH);
        /* set up type, and possibly length */
        dir.buffer[dir.slot * 32 + SLOT_TYPE_OFFSET] = newtype;
        dir.buffer[dir.slot * 32 + SLOT_RECORD_LENGTH] = newlength;
        /* setup initial track/sectors, blocks, and write pointer */
        wt = 0;
        ws = 0;
        es = 0;
        wp = 2;

        /* setup buffers; use larger read ones to avoid switching between
            partitions often */
        wb = lib_malloc(256);
        rb = lib_malloc(COPYBUFFER * 256);

        /* for REL files, setup a buffer for a single side-sector group; we
            will reuse this group over and over as needed */
        if (newlength) {
            super = vdrive_rel_setup_ss_buffers(vdrive, 15);
            ssn = 0;
            k = 0;
            for (i = 0; i < SIDE_SECTORS_MAX; i++) {
                p->side_sector[k | 0x02] = i;
                p->side_sector[k | 0x03] = newlength;
                k += 0x100;
            }
        }

        /* loop through all the supplied files */
        for (i = 0; i < files; i++) {
            rt = t[i];
            rs = s[i];
            do {
                /* read an input chunk */
                rn = 0;
                vdrive_command_switch(vdrive, src[i].drive);
                while (rt && rn < (COPYBUFFER << 8)) {
                    rc = vdrive_read_sector(vdrive, &(rb[rn]), rt, rs);
                    if (rc > 0) {
                        status = rc;
                        goto out;
                    }
                    rt = rb[rn];
                    rs = rb[rn | 1];
                    rn += 0x100;
                }
                /* write it in blocks to destination */
                vdrive_command_switch(vdrive, dest.drive);
                for (k = 0; k < rn; k += 0x100) {
                    /* loop through the whole sector, or whatever is allocated
                        if on last block */
                    for (m = 2; m <= (rb[k] ? 255 : rb[k | 1]); m++) {
                        /* we only write when we know we have a full buffer
                            and have another byte on the way */
                        if (wp & 256) {
                            /* allocate next sector */
                            nt = wt;
                            ns = ws;
                            rc = vdrive_bam_alloc_next_free_sector(vdrive, &nt, &ns);
                            if (rc < 0) {
                                goto out;
                            }
                            /* setup links to next one */
                            wb[0] = nt;
                            wb[1] = ns;
                            /* write data */
                            if (vdrive_write_sector(vdrive, wb, wt, ws ) < 0 ) {
                                vdrive_bam_free_sector(vdrive, wt, ws);
                                status = CBMDOS_IPE_WRITE_ERROR_VER;
                                goto out;
                            }
                            /* get ready to fill in the next one */
                            wt = nt;
                            ws = ns;
                            wb[0] = 0;
                            wp = 2;
                            es++;
                            /* for REL files, record new track/sector into side sector */
                            if (newlength) {
                                /* are we at the end of the current side sector? */
                                if (p->side_sector[(ssn << 8) | 0x01] == 0xff) {
                                    /* yes, are we at then end of the current
                                        side sector group? */
                                    if (ssn == 5) {
                                        /* write out side sectors and purge out all old data */
                                        for (x = 0; x < SIDE_SECTORS_MAX; x++) {
                                            if (p->side_sector_track[x]) {
                                                if ( vdrive_write_sector(vdrive,
                                                    &(p->side_sector[(x << 8)]),
                                                    p->side_sector_track[x],
                                                    p->side_sector_sector[x] ) < 0 ) {
                                                    goto out;
                                                }
                                            }
                                            memset(&(p->side_sector[(x << 8) | 0x04]), 0, 252);
                                            p->side_sector_track[x] = 0;
                                            p->side_sector_sector[x] = 0;
                                        }
                                        ssn = -1;
                                    }
                                    /* allocate a new side sector */
                                    sst = wt;
                                    sss = ws;
                                    rc = vdrive_bam_alloc_next_free_sector(vdrive, &sst, &sss);
                                    if (rc < 0) {
                                        goto out;
                                    }
                                    es++;
                                    if (ssn >= 0) {
                                        /* setup link to new sector in current one */
                                        p->side_sector[(ssn << 8) | 0x00] = sst;
                                        p->side_sector[(ssn << 8) | 0x01] = sss;
                                    } else if (super) {
                                        /* update super side sector with first side sector link */
                                        super += 2;
                                        p->super_side_sector[super + 2] = sst;
                                        p->super_side_sector[super + 3] = sss;
                                    }
                                    /* update new structure */
                                    ssn++;
                                    p->side_sector_track[ssn] = sst;
                                    p->side_sector_sector[ssn] = sss;
                                    /* update all sector structures */
                                    for (x = 0; x < SIDE_SECTORS_MAX; x++) {
                                        p->side_sector[(x << 8) + 0x04 + (ssn << 1)] = sst;
                                        p->side_sector[(x << 8) + 0x05 + (ssn << 1)] = sss;
                                    }
                                    p->side_sector[(ssn << 8) | 0x00] = 0;
                                    p->side_sector[(ssn << 8) | 0x01] = 15;
                                }
                                p->side_sector[((ssn << 8) | p->side_sector[(ssn << 8) | 0x01]) + 1] = wt;
                                p->side_sector[((ssn << 8) | p->side_sector[(ssn << 8) | 0x01]) + 2] = ws;
                                p->side_sector[(ssn << 8) | 0x01] += 2;
                            }
                        }
                        /* transfer the byte, update end of block marker each time */
                        wb[wp] = rb[k | m];
                        wb[1] = wp;
                        wp++;
                        /* allocate first block only if new data is to be written */
                        if (wt == 0) {
                            rc = vdrive_bam_alloc_first_free_sector(vdrive, &wt, &ws);
                            if (rc < 0) {
                                goto out;
                            }
                            /* setup first sector in dir entry */
                            dir.buffer[dir.slot * 32 + SLOT_FIRST_TRACK] = wt;
                            dir.buffer[dir.slot * 32 + SLOT_FIRST_SECTOR] = ws;
                            wb[0] = 0;
                            es++;

                            /* for REL files, allocate first side sector */
                            if (newlength) {
                                /* allocate a new side sector */
                                sst = wt;
                                sss = ws;
                                rc = vdrive_bam_alloc_next_free_sector(vdrive, &sst, &sss);
                                if (rc < 0) {
                                    goto out;
                                }
                                es++;
                                p->side_sector_track[ssn] = sst;
                                p->side_sector_sector[ssn] = sss;
                                /* update all sector structures */
                                for (x = 0; x < SIDE_SECTORS_MAX; x++) {
                                    p->side_sector[(x << 8) + 0x04 + (ssn << 1)] = sst;
                                    p->side_sector[(x << 8) + 0x05 + (ssn << 1)] = sss;
                                }
                                p->side_sector[(ssn << 8) | 0x00] = 0;
                                p->side_sector[(ssn << 8) | 0x01] = 15;
                                p->side_sector[(ssn << 8) | 0x10] = wt;
                                p->side_sector[(ssn << 8) | 0x11] = ws;
                                p->side_sector[(ssn << 8) | 0x01] += 2;
                                /* update dir entry */
                                dir.buffer[dir.slot * 32 + SLOT_SIDE_TRACK] = sst;
                                dir.buffer[dir.slot * 32 + SLOT_SIDE_SECTOR] = sss;

                                if (super) {
                                    /* allocate a new side sector */
                                    ssst = sst;
                                    ssss = sss;
                                    rc = vdrive_bam_alloc_next_free_sector(vdrive, &ssst, &ssss);
                                    if (rc < 0) {
                                        goto out;
                                    }
                                    es++;
                                    p->super_side_sector_track = ssst;
                                    p->super_side_sector_sector = ssss;
                                    p->super_side_sector[0] = sst;
                                    p->super_side_sector[1] = sss;
                                    p->super_side_sector[2] = 0xfe;
                                    p->super_side_sector[3] = sst;
                                    p->super_side_sector[4] = sss;
                                    /* update dir entry */
                                    dir.buffer[dir.slot * 32 + SLOT_SIDE_TRACK] = ssst;
                                    dir.buffer[dir.slot * 32 + SLOT_SIDE_SECTOR] = ssss;
                                }
                            }
                        }
                    }
                }
            /* go back and read more if there are more links in the read chain */
            } while (rt);
        }
        /* flush the last block */
        vdrive_command_switch(vdrive, dest.drive);
        if (wt) {
            if (vdrive_write_sector(vdrive, wb, wt, ws ) < 0 ) {
                vdrive_bam_free_sector(vdrive, wt, ws);
                status = CBMDOS_IPE_WRITE_ERROR_VER;
                goto out;
            }
        }
        /* flush super and remaining side sector group */
        if (newlength) {
            if (p->super_side_sector_track) {
                if (vdrive_write_sector(vdrive, p->super_side_sector,
                    p->super_side_sector_track, p->super_side_sector_sector) < 0) {
                    goto out;
                }
            }
            for (x = 0; x < SIDE_SECTORS_MAX; x++) {
                if (p->side_sector_track[x]) {
                    if ( vdrive_write_sector(vdrive, &(p->side_sector[(x << 8)]),
                        p->side_sector_track[x], p->side_sector_sector[x] ) < 0 ) {
                        goto out;
                    }
                }
            }
        }

        /* complete entry */
        dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS] = es & 255;
        dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS + 1] = es >> 8;
        /* apply date and time on DHDs */
        if (vdrive->haspt) {
            vdrive_dir_updatetime(vdrive, &(dir.buffer[dir.slot * 32]));
        }
        /* update directory entry */
        vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector);

        /* update bam */
        vdrive_bam_write_bam(vdrive);
        /* all done! */
        status = CBMDOS_IPE_OK;
    }

out:
    /* for REL files, remove side sector buffer */
    if (newlength && p->side_sector) {
        vdrive_rel_shutdown_ss_buffers(vdrive, 15);
    }

    /* clean up */
    if (wb) {
        lib_free(wb);
    }
    if (rb) {
        lib_free(rb);
    }

    for (i = 0; i < COPYMAX; i++) {
        if (src[i].file) {
            lib_free(src[i].file);
        }
        if (src[i].path) {
            lib_free(src[i].path);
        }
    }

    vdrive_command_return(vdrive, origpart);

    return status;
}

static int vdrive_command_rename(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    uint8_t *slot;
    vdrive_dir_context_t dir;
/*    unsigned int filetype; */
    uint8_t *dest = NULL;
    int destlen;

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    status = vdrive_command_switchtraverse(vdrive, cmd);
    if (status) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("RENAME: dest name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    status = CBMDOS_IPE_SYNTAX;
    /* check to make sure there is an '=' in the command */
    if (!(cmd->morelength && cmd->more[0] == '=')) {
        goto out;
    }
    /* check for wildcards anywhere in the destination name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }
    /* check for wildcards anywhere in the source name */
    if (vdrive_haswildcard(cmd->more, cmd->morelength)) {
        goto out;
    }

    vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, 0, &dir);
    slot = vdrive_dir_find_next_slot(&dir);
    /* Check if the destination name is already in use.  */
    if (slot) {
        status = CBMDOS_IPE_FILE_EXISTS;
        goto out;
    }
    /* copy the destination name */
/*    filetype = cmd->filetype; */
    if (cmd->filelength) {
        destlen = cmd->filelength;
        dest = lib_calloc(1, destlen + 1);
        memcpy(dest, cmd->file, destlen);
    } else {
    /* BUG in CBM DOS: If the destination file is empty, it renames it to "=";
       not sure if we want to keep this behavior */
        destlen = 1;
        dest = lib_calloc(1, destlen + 1);
        dest[0] = '=';
    }
    dest[destlen] = 0;

    /* use parser to find source name */
    vdrive_cmdfree_full(cmd);
    vdrive_cmdfree_abbrv(cmd);
    vdrive_cmdfree_path(cmd);
    vdrive_cmdfree_file(cmd);
    vdrive_cmdfree_command(cmd);
    cmd->full = cmd->more;
    cmd->fulllength = cmd->morelength;
    rc = cbmdos_command_parse_plus(cmd);
    /* if there is some type of parsing error, just error out */
    if (rc) {
        goto out;
    }

    /* Find the file to rename */
    vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, 0, &dir);
    slot = vdrive_dir_find_next_slot(&dir);
    /* error out if it isn't there */
    if (!slot) {
        status = CBMDOS_IPE_NOT_FOUND;
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("RENAME: src name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    /* Now we can replace the old file name...  */
    /* We write directly to the Dir_buffer.  */

    slot = &dir.buffer[dir.slot * 32];
    memset(slot + SLOT_NAME_OFFSET, 0xa0, 16);
    memcpy(slot + SLOT_NAME_OFFSET, dest, destlen);

    /* FIXME: is this right? */

    /* Doesn't look like it, does the rename command even allow specifying
     * file types? And even if so, the closed bit should be set with '| 0x80'
     * (BW 2017-02-04) */
#if 0
    if (cmd->filetype) {
        slot[SLOT_TYPE_OFFSET] = filetype;
    }
#endif

    /* Update the directory.  */
    if (vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector) < 0) {
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }

    status = CBMDOS_IPE_OK;

out:
    if (dest) {
        lib_free(dest);
    }

    vdrive_command_return(vdrive, origpart);

    return status;
}

static int vdrive_command_renameheader(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    uint8_t tmp[256];
    int i;

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    /* find the right header location */
    status = vdrive_command_switchtraverse(vdrive, cmd);
    if (status) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("RENAMEHEADER: dest name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    status = CBMDOS_IPE_SYNTAX;
    /* check for wildcards anywhere in the new name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }

    status = CBMDOS_IPE_OK;

    /* flush out any pending changes */
    vdrive_bam_write_bam(vdrive);

    /* read header */
    rc = vdrive_read_sector(vdrive, tmp, vdrive->Header_Track, vdrive->Header_Sector);
    if (rc > 0) {
        status = rc;
        goto out;
    }
    if (rc < 0) {
        status = CBMDOS_IPE_NOT_READY;
        goto out;
    }

    /* copy over new header */
    for (i = 0; i < 16 && i < cmd->filelength; i++) {
        tmp[vdrive->bam_name + i] = cmd->file[i];
    }
    /* pad the rest */
    for (; i < 16; i++) {
        tmp[vdrive->bam_name + i] = 0xa0;
    }

    /* write header */
    rc = vdrive_write_sector(vdrive, tmp, vdrive->Header_Track, vdrive->Header_Sector);
    if (rc > 0) {
        status = rc;
        goto out;
    }
    if (rc < 0) {
        status = CBMDOS_IPE_NOT_READY;
        goto out;
    }

    /* force a bam re-read */
    vdrive_bam_setup_bam(vdrive);

out:
    vdrive_command_return(vdrive, origpart);

    return status;
}

static int vdrive_command_renamepart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    uint8_t *slot;
    vdrive_dir_context_t dir;
    uint8_t *dest = NULL;
    int destlen;

/* backup current partition information */
    int origpart = vdrive->current_part;

    status = CBMDOS_IPE_INVAL;
    /* can only do this on CMD HDs and FDs */
    if (!vdrive->haspt) {
        goto bad;
    }

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    /* try to change to selected partition */
    if (vdrive_switch(vdrive, 255)) {
        status = CBMDOS_IPE_INVAL;
        goto bad;
    }

#ifdef DEBUG_DRIVE
    log_debug("RENAMEPART: dest name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    status = CBMDOS_IPE_SYNTAX;
    /* check to make sure there is an '=' in the command */
    if (!(cmd->morelength && cmd->more[0] == '=')) {
        goto out;
    }
    /* check for wildcards anywhere in the destination name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }
    /* check for wildcards anywhere in the source name */
    if (vdrive_haswildcard(cmd->more, cmd->morelength)) {
        goto out;
    }

    vdrive_dir_part_find_first_slot(vdrive, cmd->file, cmd->filelength, 0, &dir);
    slot = vdrive_dir_part_find_next_slot(&dir);
    /* Check if the destination name is already in use.  */
    if (slot) {
        status = CBMDOS_IPE_FILE_EXISTS;
        goto out;
    }
    /* copy the destination name */
    if (cmd->filelength) {
        destlen = cmd->filelength;
        dest = lib_calloc(1, destlen + 1);
        memcpy(dest, cmd->file, destlen);
    } else {
    /* BUG in HD DOS: If the destination file is empty, it renames it to "=";
       not sure if we want to keep this behavior as there is no way to fix it */
        status = CBMDOS_IPE_SYNTAX;
        goto out;
    }
    dest[destlen] = 0;

    /* use parser to find source name */
    vdrive_cmdfree_full(cmd);
    vdrive_cmdfree_abbrv(cmd);
    vdrive_cmdfree_path(cmd);
    vdrive_cmdfree_file(cmd);
    vdrive_cmdfree_command(cmd);
    cmd->full = cmd->more;
    cmd->fulllength = cmd->morelength;
    rc = cbmdos_command_parse_plus(cmd);
    /* if there is some type of parsing error, just error out */
    if (rc) {
        goto out;
    }

    /* Find the file to rename */
    vdrive_dir_part_find_first_slot(vdrive, cmd->file, cmd->filelength, 0, &dir);
    slot = vdrive_dir_part_find_next_slot(&dir);
    /* error out if it isn't there */
    if (!slot) {
        status = CBMDOS_IPE_NOT_FOUND;
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("RENAMEPART: src name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    /* Now we can replace the old file name...  */
    /* We write directly to the Dir_buffer.  */

    slot = &dir.buffer[dir.slot * 32];
    memset(slot + PSLOT_NAME, 0xa0, 16);
    memcpy(slot + PSLOT_NAME, dest, destlen);

    /* Update the directory.  */
    if (vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector) < 0) {
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }

    status = CBMDOS_IPE_OK;

out:
    if (dest) {
        lib_free(dest);
    }

    vdrive_command_return(vdrive, origpart);

bad:
    return status;
}

static int vdrive_command_lockunlock(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    uint8_t *slot;
    vdrive_dir_context_t dir;

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    status = vdrive_command_switchtraverse(vdrive, cmd);
    if (status) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("LOCK/UNLOCK: dest name='%s', len=%u", cmd->file, cmd->filelength);
#endif

    status = CBMDOS_IPE_SYNTAX;
    /* check for wildcards anywhere in the file name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }
/*
    slot=memchr(cmd->file,'*',cmd->filelength);
    if (slot) goto out;
    slot=memchr(cmd->file,'?',cmd->filelength);
    if (slot) goto out;
*/

    vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, 0, &dir);
    slot = vdrive_dir_find_next_slot(&dir);

    /* Check if the file is found  */
    if (!slot) {
        status = CBMDOS_IPE_NOT_FOUND;
        goto out;
    }

    /* update the directory entry */

    slot = &dir.buffer[dir.slot * 32];
    slot[SLOT_TYPE_OFFSET] ^= CBMDOS_FT_LOCKED;

    if (vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector) < 0) {
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }

    status = CBMDOS_IPE_OK;

out:
    vdrive_command_return(vdrive, origpart);

    return status;
}

int vdrive_command_switchtraverse(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    uint8_t *slot, *next;
    vdrive_dir_context_t dir;
    int i, skip;
    uint8_t buffer[256];

    status = CBMDOS_IPE_NOT_READY;

    if (vdrive_command_switch(vdrive, cmd->drive)) {
        goto out;
    }

    /* traverse paths if on supported image, otherwise ignore it - that is what
        the CMD ROMs do */

     /* swap filename to pathname if command is CD to simplify coding */
    if (cmd->commandlength == 2 && cmd->command[0] == 'C'
        && cmd->command[1] == 'D' && cmd->pathlength == 0) {
        cmd->path = cmd->file;
        cmd->pathlength = cmd->filelength;
        cmd->file = NULL;
        cmd->filelength = 0;
    }

    status = CBMDOS_IPE_OK;

    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        i = 0;
        do {
            skip = 0;
            /* if no path, traverse nothing */
            if (!cmd->path || cmd->pathlength == 0) {
                break;
            }

/* for CD: "CD:DIR" "CD/DIR" "CD/DIR/" "CD_" (<-) are acceptable */
            /* if the file is NULL and the path is '_', go to previous directory */
            /* this really only applies to CD */
            if (i == 0 && cmd->commandlength == 2 && cmd->command[0] == 'C'
                && cmd->command[1] == 'D') {
                if (cmd->pathlength == 1 && cmd->path[0] == '_') {
                    /* read header */
                    rc = vdrive_read_sector(vdrive, buffer, vdrive->Header_Track,
                                            vdrive->Header_Sector);
                    if (rc > 0) {
                        status = rc;
                        goto out;
                    }
                    if (rc < 0) {
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    }
                    vdrive->Header_Track = buffer[34]; /* get parent info */
                    vdrive->Header_Sector = buffer[35];

                    /* read previous header to get dir links */
                    rc = vdrive_read_sector(vdrive, buffer, vdrive->Header_Track,
                                            vdrive->Header_Sector);
                    if (rc > 0) {
                        status = rc;
                        goto out;
                    }
                    if (rc < 0) {
                        status = CBMDOS_IPE_NOT_READY;
                        goto out;
                    }
                    vdrive->Dir_Track = buffer[0]; /* update dir info */
                    vdrive->Dir_Sector = buffer[1];
                    status = CBMDOS_IPE_OK;
                    goto out;
                } else {
                    if (i < cmd->pathlength && cmd->path[i] != '/') {
                        next = memchr(&(cmd->path[i]), '/', cmd->pathlength - i);
                        if (!next) {
                            next = &(cmd->path[cmd->pathlength]);
                        }
                        skip++;
                    }
                }
            }
            if (!skip) {
                skip = 1;
                /* if path has '//' move to root before searching; eg. //PATH1//PATH1/ is valid */
                if (i < cmd->pathlength - 1 && cmd->path[i] == '/'
                    && cmd->path[i + 1] == '/') {
                    vdrive->Header_Track = BAM_TRACK_NP;
                    vdrive->Header_Sector = BAM_SECTOR_NP;
                    i += 2;
                    if (cmd->pathlength == 2) {
                        skip = 0;
                    }
                } else if (i < cmd->pathlength && cmd->path[i] == '/') {
                    i++;
                }
                next = memchr(&(cmd->path[i]), '/', cmd->pathlength - i);
                /* If this is CD, allow the last '/' to be omitted */
                if (!next && cmd->commandlength == 2 && cmd->command[0] == 'C'
                    && cmd->command[1] == 'D') {
                    next = &(cmd->path[cmd->pathlength]);
                }
                /* paths must begin and end with '/' */
                if (!i || !next) {
                    status = CBMDOS_IPE_PATH_NOT_FOUND;
                    goto out;
                }
            }

            /* skip directory search if "CD//" */
            if (skip) {
                vdrive_dir_find_first_slot(vdrive, &(cmd->path[i]),
                            (int)(next - &(cmd->path[i])), CBMDOS_FT_DIR, &dir);

                slot = vdrive_dir_find_next_slot(&dir);
            }

            if (!skip) {
                /* just set root folder */
                vdrive->Dir_Track = DIR_TRACK_NP;
                vdrive->Dir_Sector = DIR_SECTOR_NP;
                status = CBMDOS_IPE_OK;
            } else if (slot) {
                /* search for folder in directory */
                slot = &dir.buffer[dir.slot * 32];
                rc = vdrive_read_sector(vdrive, buffer, slot[SLOT_FIRST_TRACK],
                                        slot[SLOT_FIRST_SECTOR]);
                if (rc > 0) {
                    status = rc;
                    goto out;
                }
                if (rc < 0) {
                    status = CBMDOS_IPE_NOT_READY;
                    goto out;
                }

                /* update vdrive settings */
                vdrive->Header_Track = slot[SLOT_FIRST_TRACK];
                vdrive->Header_Sector = slot[SLOT_FIRST_SECTOR];
                vdrive->Dir_Track = buffer[0];
                vdrive->Dir_Sector = buffer[1];
                status = CBMDOS_IPE_OK;
            } else {
                status = CBMDOS_IPE_PATH_NOT_FOUND;
                goto out;
            }
            i = (int)(next - (uint8_t*)cmd->path);
        } while (i < cmd->pathlength - 1);
    }

out:
    return status;
}

void vdrive_command_return(vdrive_t *vdrive, int origpart)
{
    /* rather than checking to see if we didn't switch partitions each time,
        just return if the given part is negative */
    if (origpart < 0) {
        return;
    }

    /* reset directory pointers, etc. if the current parition is the one we traversed */
    /* don't need to flush the bam here as that would apply to 1581 only and they
        can have their partitions changed in a command */
    if (origpart == vdrive->current_part) {
        vdrive_set_disk_geometry(vdrive);
    }

    return;
}

int vdrive_command_switch(vdrive_t *vdrive, int part)
{
    /* don't allow anything to the system partition */
    if (part == 255) {
        return CBMDOS_IPE_NOT_READY;
    }
    return vdrive_switch(vdrive, part);
}

static int vdrive_command_scratch(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, rc;
    int t, s, l;
    uint8_t *slot, *newmore;
    vdrive_dir_context_t dir;
    int deleted_files = 0, filetype = CBMDOS_FT_DEL;
    int geos;
    uint8_t tmp[256];

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    /* use the parser plus to go through all the passed drive/part/names separated by , and = 's */
    do {

        status = vdrive_command_switchtraverse(vdrive, cmd);
        if (status) {
            goto out;
        }

#ifdef DEBUG_DRIVE
        log_debug("remove name='%s', len=%u, type=%u.",
            cmd->file, cmd->filelength, cmd->filetype);
#endif

/* any =[filetype] is on the 'more' structure */
        if (cmd->morelength && cmd->more[0] == '=') {
            if (cmd->morelength > 1) {
                filetype = vdrive_dir_filetype(cmd->more, 2);
            }
            /* adjust cmd->more now that we read a little more of it */
            slot = memchr(cmd->more, ',', cmd->morelength);
            /* if nothine else, just get rid of it */
            if (!slot) {
                vdrive_cmdfree_more(cmd);
                cmd->more = NULL;
                cmd->morelength = 0;
            } else {
                /* re-adjust it */
                rc = cmd->morelength - (int)(slot - cmd->more);
                newmore = lib_calloc(1, rc + 1);
                memcpy(newmore, slot, rc);
                newmore[rc] = 0;
                vdrive_cmdfree_more(cmd);
                cmd->more = newmore;
                cmd->morelength = rc;
            }
        }

        /* check if this is a GEOS formatted partition */
/* FIXME: 1581 sub-directories? where is the geos signature? */
        geos = vdrive_bam_isgeos(vdrive);

        vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, filetype, &dir);

        while ((slot = vdrive_dir_find_next_slot(&dir))) {
            /* skip DIR entries, have to use the RD command for those */
            if ((slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_DIR) {
                continue;
            }
            /* skip any locked entries */
            if ((slot[SLOT_TYPE_OFFSET] & CBMDOS_FT_LOCKED)) {
                continue;
            }
            /* handle geos type */
            if (slot[SLOT_GEOS_TYPE] && geos) {
                /* remove info block */
                t = slot[SLOT_GEOS_ITRACK];
                s = slot[SLOT_GEOS_ISECTOR];
                vdrive_dir_free_chain(vdrive, t, s);
                /* check file type */
                if (slot[SLOT_GEOS_STRUCT] == 1) {
                    /* VLIR type, like REL */
                    /* read record pointer sector */
                    t = slot[SLOT_FIRST_TRACK];
                    s = slot[SLOT_FIRST_SECTOR];
                    vdrive_read_sector(vdrive, tmp, t, s);
                    /* cycle through all 127 records */
                    for (l = 0; l < 127; l++) {
                        t = tmp[2 + l * 2];
                        s = tmp[2 + l * 2 + 1];
                        /* 0,255 is empty */
                        if (t == 0 && s == 255) {
                            continue;
                        }
                        /* 0,0 is end */
                        if (t == 0 && s == 0) {
                            break;
                        }
                        /* anything else is effectively a normal file */
                        vdrive_dir_free_chain(vdrive, t, s);
                    }
                } /* other types and VLIR record block will be freed below */
                /* clear info block normal scratch (REL) won't do anything */
                dir.buffer[dir.slot * 32 + SLOT_GEOS_TYPE] = 0;
                dir.buffer[dir.slot * 32 + SLOT_GEOS_STRUCT] = 0;
                dir.buffer[dir.slot * 32 + SLOT_GEOS_ITRACK] = 0;
                dir.buffer[dir.slot * 32 + SLOT_GEOS_ISECTOR] = 0;
            }
            /* handle CBM type */
            if ((slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_CBM) {
                /* unallocate all the sectors in the partition */
                t = slot[SLOT_FIRST_TRACK];
                s = slot[SLOT_FIRST_SECTOR];
                l = slot[SLOT_NR_BLOCKS] | (slot[SLOT_NR_BLOCKS + 1] << 8);

                /* cycle through all the sectors; no error detection - just skip anything bad */
                while (l) {
                    /* stop if we are over track 80 */
                    if (t > 80) {
                        break;
                    }
                    /* skip 40 just in case */
                    if (t != 40) {
                        vdrive_bam_free_sector(vdrive, t, s);
                    }
                    s++;
                    if (s > 39) {
                        s = 0;
                        t++;
                    }
                    l--;
                }
                /* clear points and length so normal scratch won't do anything */
                dir.buffer[dir.slot * 32 + SLOT_FIRST_TRACK] = 0;
                dir.buffer[dir.slot * 32 + SLOT_FIRST_SECTOR] = 0;
                dir.buffer[dir.slot * 32 + SLOT_SIDE_TRACK] = 0;
                dir.buffer[dir.slot * 32 + SLOT_SIDE_SECTOR] = 0;
                dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS] = 0;
                dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS + 1] = 0;
                /* follow through with directory entry removal */
            }
            vdrive_dir_remove_slot(&dir);
            deleted_files++;
        }

        status = CBMDOS_IPE_DELETED; /* always, even if no files */

        /* process more of the command if a ',' or '=' appears */
        if (cmd->morelength) {
            vdrive_cmdfree_full(cmd);
            vdrive_cmdfree_abbrv(cmd);
            vdrive_cmdfree_path(cmd);
            vdrive_cmdfree_file(cmd);
            vdrive_cmdfree_command(cmd);
            cmd->full = cmd->more;
            cmd->fulllength = cmd->morelength;
            rc = cbmdos_command_parse_plus(cmd);
            if (rc) {
                goto out;
            }
            /* stop if it isn't a ',' */
            if (!((cmd->commandlength == 1 && cmd->command[0] == ','))) {
                break;
            }
        } else {
            cmd->filelength = 0;
        }

    } while (cmd->filelength);

out:

    vdrive_command_set_error(vdrive, status, deleted_files, 0);

    vdrive_command_return(vdrive, origpart);

    return status;
}

/*
    CMD change partition support
*/
static int vdrive_command_chcmdpart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status = CBMDOS_IPE_INVAL, statret1 = 0;
    int floppy, hd, np = 0;

    /* only works on D1M, D2M, D4M, and DHD images */
    if (!vdrive->haspt) {
        goto out;
    }

    floppy = VDRIVE_IS_FD(vdrive);
    hd = VDRIVE_IS_HD(vdrive);

    /* has to be CP or C(shift)P */
    if (!(cmd->commandlength > 1 && (cmd->command[1] & 0x7f) == 'P')) {
        goto out;
    }

    /* if no part table, leave with error */
    if (!vdrive_ispartvalid(vdrive, 255)) {
        status = CBMDOS_IPE_NOT_READY;
        goto out;
    }

    if (cmd->command[1] == 'P') {
        if (cmd->commandlength == 2) {
            /* CP command with no arguments returns current partition */
            if (cmd->filelength == 0) {
                status = CBMDOS_IPE_SEL_PARTN;
                statret1 = vdrive->selected_part;
                goto out;
            }
            np = vdrive_atoi((char*)cmd->file, NULL, cmd->filelength);
            /* FD4000 returns 77 on parts>=224, largest should be 254 */
            if ( (np < 1) || (np >= 224 && floppy) || (np > 254 && hd)) {
                status = CBMDOS_IPE_BAD_PARTN;
                statret1 = np;
                goto out;
            }
        }
    } else if (cmd->commandlength == 3 && (unsigned char)cmd->command[1] == ('P' | 0x80) ) {
        np = cmd->command[2];
        if (np == 0 && floppy) {
            /* switch to system parititon, only FD series right now */
            np = 255;
        } else if ( (np >= 224 && floppy) || (np > 254 && hd)) {
            status = CBMDOS_IPE_BAD_PARTN;
            statret1 = np;
            goto out;
        }
    } else {
        status = CBMDOS_IPE_BAD_PARTN;
        goto out;
    }

    /* at this point, np is good, try switching */
    if (!vdrive_switch(vdrive, np)) {
        /* can't really check for errors here, can only return 2 or 77  */
        vdrive->selected_part = np;
        statret1 = np;
        status = CBMDOS_IPE_SEL_PARTN;
    } else {
        /* if it is not a valid parition type, give an error */
        status = CBMDOS_IPE_BAD_PARTN;
        statret1 = np;
    }

out:
    return vdrive_command_set_error(vdrive, status, statret1, 0);
}

/*
    CMD style subdir support (using DIR filetype)
*/
static int vdrive_command_chdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status, p;

    /* backup current partition information */
    int origpart = vdrive->current_part;

    /* use the parser plus to go through all the passed drive/part/names separated by ,'s */
    status = vdrive_command_switchtraverse(vdrive, cmd);
    if (status) {
        goto out;
    }

    p = vdrive_realpart(vdrive, cmd->drive);
    vdrive->cheadertrack[p] = vdrive->Header_Track;
    vdrive->cheadersector[p] = vdrive->Header_Sector;
    vdrive->cdirtrack[p] = vdrive->Dir_Track;
    vdrive->cdirsector[p] = vdrive->Dir_Sector;

out:

    vdrive_command_return(vdrive, origpart);

    return status;
}

/*
    CBM style sub partition support (using CBM filetype)

    on 1581 dos command "/dirname" enters a partition, "i" will go back to root

    OK result is #2, with t=start track and s=end track, ie. 120 blocks at 1 would be 1,3
    when creating, anything goes, just no overlap (looks at bam only)
    when creating, existing parts gives #63, files exists
    when switching, rules are checked. if bad, #77 - illegal part
    unfound partitions give 62, file not found
*/
static int vdrive_command_chpart(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    int ts = 0, ss, te = 0, len;
    uint8_t *slot;
    vdrive_dir_context_t dir;

/* backup current partition information */
    int origpart = vdrive->current_part;

    status = CBMDOS_IPE_NOT_READY;
    if (vdrive_command_switch(vdrive, cmd->drive)) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("chpart name='%s', len=%u",
              cmd->file, cmd->filelength);
#endif

    /* if no filename after '/', reset to root paritition */
    if (!cmd->filelength) {
        vdrive_bam_write_bam(vdrive);
        /* force vdrive_set_disk_geometry to update presistent values */
        vdrive->cheadertrack[vdrive->current_part] = 0;
        vdrive_set_disk_geometry(vdrive);
        /* update the BAM in ram */
        vdrive_bam_setup_bam(vdrive);
        status = CBMDOS_IPE_SEL_PARTN;
        ts = 1;
        te = 80;
        goto out;
    }

    /* find entry */
    vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, CBMDOS_FT_CBM, &dir);

    slot = vdrive_dir_find_next_slot(&dir);

    status = CBMDOS_IPE_NOT_FOUND;

    /* check to see if we are in create mode */
    if (cmd->morelength >= 7 && cmd->more[0] == ',' && cmd->more[5] == ',' && cmd->more[6] == 'C') {
        /* read only mode? */
        if (VDRIVE_IS_READONLY(vdrive)) {
            status = CBMDOS_IPE_WRITE_PROTECT_ON;
            goto out;
        }
        /* error out if it exists */
        if (slot) {
            status = CBMDOS_IPE_FILE_EXISTS;
            goto out;
        }

        ts = cmd->more[1];
        ss = cmd->more[2];
        len = cmd->more[3] | (cmd->more[4] << 8);
        /* error out if length is 0 */
        if (!len) {
            status = CBMDOS_IPE_BAD_PARTN;
            goto out;
        }

        /* check to see if any of the sectors are allocated */
        while (len) {
            /* error out if we are on track 40 */
            if (ts == 40) {
                break;
            }
            /* error out if we are over track 80 */
            if (ts > 80) {
                break;
            }
            if (vdrive_bam_is_sector_allocated(vdrive, ts, ss)) {
                break;
            }
            ss++;
            if (ss > 39) {
                ss = 0;
                ts++;
            }
            len--;
        }

        if (len) {
            status = CBMDOS_IPE_ILLEGAL_SYSTEM_T_OR_S; /* special to 1581 */
            /* if out of range, switch from 67 to 66 */
            if (ts > 80) {
                status = CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR;
            }
            te = ss;
            goto out;
        }

        /* we can make it now */
        /* find empty entry */
        vdrive_dir_find_first_slot(vdrive, cmd->file, -1, CBMDOS_FT_DEL, &dir);
        slot = vdrive_dir_find_next_slot(&dir);
        if (slot) {
            /* clear out entry */
            memset(&(dir.buffer[dir.slot * 32 + 2]), 0, 30);
            /* copy name from original search */
            memcpy(&(dir.buffer[dir.slot * 32 + SLOT_NAME_OFFSET]), dir.find_nslot, CBMDOS_SLOT_NAME_LENGTH);
            /* build entry */
            dir.buffer[dir.slot * 32 + SLOT_FIRST_TRACK] = cmd->more[1];
            dir.buffer[dir.slot * 32 + SLOT_FIRST_SECTOR] = cmd->more[2];
            dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS] = cmd->more[3];
            dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS + 1] = cmd->more[4];
            dir.buffer[dir.slot * 32 + SLOT_TYPE_OFFSET] = CBMDOS_FT_CLOSED | CBMDOS_FT_CBM;
            /* update directory entry */
            vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector);
            /* allocate the sectors */
            ts = cmd->more[1];
            ss = cmd->more[2];
            len = cmd->more[3] | (cmd->more[4] << 8);
            /* should be no errors here */
            while (len) {
                vdrive_bam_allocate_sector(vdrive, ts, ss);
                ss++;
                if (ss > 39) {
                    ss = 0;
                    ts++;
                }
                len--;
            }
            /* update bam */
            vdrive_bam_write_bam(vdrive);
            /* all done! */
            status = CBMDOS_IPE_OK;
        } else {
           /* no directory entries left */
            status = CBMDOS_IPE_DISK_FULL;
        }
        ts = 0;
        te = 0;

    } else {
       /* change mode */
        if (slot) {
            slot = &dir.buffer[dir.slot * 32];
            /*
            In order to _change to_ a partition as a sub-directory, it  must  adhere  to
            the following four rules:

            1. It must start on sector 0
            2. It's size must be in multiples of 40 sectors (which means the
               last sector is 39)
            3. It must be a minimum of 120 sectors long (3 tracks)
            4. It must not start on or cross track 40
            */
            ts = slot[SLOT_FIRST_TRACK];
            ss = slot[SLOT_FIRST_SECTOR];
            len = slot[SLOT_NR_BLOCKS] | (slot[SLOT_NR_BLOCKS + 1] << 8);

            if ((ss == 0) && ((len % 40) == 0) && (len >= 120) && (ts != 40)) {
                te = ts * 40 + len;
                ss = ts * 40;

                if ( (ss <= 1600 && te > 1600) ||
                   ( ss < ((int)vdrive->Part_Start) * 40) ||
                   ( te >((int)vdrive->Part_End + 1) * 40) ) {
                    status = CBMDOS_IPE_BAD_PARTN;
                    ts = 0;
                    te = 0;
                    goto out;
                }
                te = ts + (len / 40) - 1;

                /* don't need to check the BAM or anything, just proceed */

#ifdef DEBUG_DRIVE
                log_debug("Partition Trk %d Sec %d - Trk %d len: %d", ts, ss, te, len);
#endif
                /* setup BAM location */
                vdrive->Header_Track = ts;
                vdrive->Header_Sector = 0;
                vdrive->Bam_Track = ts;
                vdrive->Bam_Sector = 0;
                /* start of directory */
                vdrive->Dir_Track = ts;
                vdrive->Dir_Sector = DIR_SECTOR_1581;
                /* set area for active partition */
                vdrive->Part_Start = ts;
                vdrive->Part_End = te;
                /* update persistent data */
                vdrive->cheadertrack[vdrive->current_part] =  vdrive->Header_Track;
                vdrive->cheadersector[vdrive->current_part] =  vdrive->Header_Sector;
                vdrive->cdirtrack[vdrive->current_part] =  vdrive->Dir_Track;
                vdrive->cdirsector[vdrive->current_part] =  vdrive->Dir_Sector;
                vdrive->cpartstart[vdrive->current_part] = ts;
                vdrive->cpartend[vdrive->current_part] = te;

                /* update the BAM in ram */
                vdrive_bam_setup_bam(vdrive);

                status = CBMDOS_IPE_SEL_PARTN;
            } else {
                status = CBMDOS_IPE_BAD_PARTN;
                ts = 0;
                te = 0;
                goto out;
            }
        }
    }
out:
    vdrive_command_return(vdrive, origpart);

    vdrive_command_set_error(vdrive, status, ts, te);

    return status;

}

static int vdrive_command_mkdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    uint8_t *slot;
    vdrive_dir_context_t dir;
    unsigned int ht, hs, dt, ds;
    uint8_t *hb = NULL, *db = NULL;

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    status = CBMDOS_IPE_SYNTAX;
    /* check for wildcards anywhere in the name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }
/*
    slot=memchr(cmd->file,'*',cmd->filelength);
    if (slot) goto out;
    slot=memchr(cmd->file,'?',cmd->filelength);
    if (slot) goto out;
*/

    status = vdrive_command_switchtraverse(vdrive, cmd);
    if (status) {
        goto out;
    }

#ifdef DEBUG_DRIVE
    log_debug("mkdir name='%s', len=%u.",
        cmd->file, cmd->filelength);
#endif

    /* find entry */
    vdrive_dir_find_first_slot(vdrive, cmd->file, cmd->filelength, CBMDOS_FT_DIR, &dir);

    slot = vdrive_dir_find_next_slot(&dir);

    /* status = CBMDOS_IPE_NOT_FOUND; */

    /* error out if it exists */
    if (slot) {
        status = CBMDOS_IPE_FILE_EXISTS;
        goto out;
    }

    /* default error is disk full now */
    status = CBMDOS_IPE_DISK_FULL;
    /* stop if only 3 blocks left, 1 for header, 1 for directory, and 1 for possible expansion of current directory */
    if (vdrive_bam_free_block_count(vdrive) < 4) {
        goto out;
    }

    /* use parent as a starting point */
    ht = vdrive->Dir_Track;
    hs = vdrive->Dir_Sector;
    /* if the parent is root and in track 1, make sure it is sector 64 or higher */
    if (ht == 1 && hs < 63 ) {
        hs = 63;
    }
    if (vdrive_bam_alloc_next_free_sector(vdrive, &ht, &hs) < 0 ) {
        /* no space? leave */
        goto out;
    }
    dt = ht;
    ds = hs;
    if (vdrive_bam_alloc_next_free_sector(vdrive, &dt, &ds) < 0 ) {
        /* no space? free header and leave */
        vdrive_bam_free_sector(vdrive, ht, hs);
        goto out;
    }

    /* we can make it now */
    /* find empty entry */
    vdrive_dir_find_first_slot(vdrive, cmd->file, -1, CBMDOS_FT_DEL, &dir);
    slot = vdrive_dir_find_next_slot(&dir);
    /* slot should never be NULL at this point, check anyways */
    if (!slot) {
        goto out;
    }

    /* create header sector */
    hb = lib_malloc(256);
    memset(hb, 0, 256);

    hb[0] = dt;
    hb[1] = ds;
    memset(&hb[vdrive->bam_name], 0xa0, 25);
    /* copy name from original search */
    memcpy(&hb[vdrive->bam_name], dir.find_nslot, CBMDOS_SLOT_NAME_LENGTH);
    /* copy id */
    hb[vdrive->bam_id] = vdrive->bam[vdrive->bam_id];
    hb[vdrive->bam_id + 1] = vdrive->bam[vdrive->bam_id + 1];

    hb[0x02] = 72;
    hb[BAM_VERSION_NP] = 49;
    hb[BAM_VERSION_NP + 1] = 72;
    hb[0x20] = ht;
    hb[0x21] = hs;
    hb[0x22] = vdrive->Header_Track;
    hb[0x23] = vdrive->Header_Sector;
    hb[0x24] = dir.track; /* vdrive->Dir_Track; */
    hb[0x25] = dir.sector; /* vdrive->Dir_Sector; */
    hb[0x26] = dir.slot * 32 + 2;

    /* create directory sector */
    db = lib_malloc(256);
    memset(db, 0, 256);
    db[1] = 0xff;

    /* write new sub-directory header */
    if (vdrive_write_sector(vdrive, hb, ht, hs ) < 0 ) {
        vdrive_bam_free_sector(vdrive, ht, hs);
        vdrive_bam_free_sector(vdrive, dt, ds);
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }

    /* write new sub-directory dir sector */
    if (vdrive_write_sector(vdrive, db, dt, ds ) < 0 ) {
        vdrive_bam_free_sector(vdrive, ht, hs);
        vdrive_bam_free_sector(vdrive, dt, ds);
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }

    /* clear dir out entry */
    memset(&(dir.buffer[dir.slot * 32 + 2]), 0, 30);
    /* copy name from original search */
    memcpy(&(dir.buffer[dir.slot * 32 + SLOT_NAME_OFFSET]), dir.find_nslot, CBMDOS_SLOT_NAME_LENGTH);
    /* build entry */
    dir.buffer[dir.slot * 32 + SLOT_FIRST_TRACK] = ht;
    dir.buffer[dir.slot * 32 + SLOT_FIRST_SECTOR] = hs;
    dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS] = 2;
    dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS + 1] = 0;
    dir.buffer[dir.slot * 32 + SLOT_TYPE_OFFSET] = CBMDOS_FT_CLOSED | CBMDOS_FT_DIR;
    /* apply date and time */
    vdrive_dir_updatetime(vdrive, &(dir.buffer[dir.slot * 32]));
    /* update directory entry */
    vdrive_write_sector(vdrive, dir.buffer, dir.track, dir.sector);

    /* update bam */
    vdrive_bam_write_bam(vdrive);
    /* all done! */
    status = CBMDOS_IPE_OK;

out:
    if (hb) {
        lib_free(hb);
    }
    if (db) {
        lib_free(db);
    }

    vdrive_command_return(vdrive, origpart);

    return status;

}

static int vdrive_command_rmdir(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    uint8_t *slot;
    vdrive_dir_context_t dir;
    int deleted_files = 0;
    uint8_t *file;
    int filelength;

/* backup current partition information */
    int origpart = vdrive->current_part;

    /* leave if write protected */
    status = CBMDOS_IPE_WRITE_PROTECT_ON;
    if (VDRIVE_IS_READONLY(vdrive)) {
        goto out;
    }

    status = CBMDOS_IPE_SYNTAX;
    /* check for wildcards anywhere in the name */
    if (vdrive_haswildcard(cmd->file, cmd->filelength)) {
        goto out;
    }

    /* remove path from parse structure */
    if (cmd->path) {
        vdrive_cmdfree_path(cmd);
        cmd->pathlength = 0;
    }

    /* call switchtraverse to "CD" to the folder */
    vdrive_cmdfree_command(cmd);
    cmd->command = (uint8_t*)lib_strdup("CD");
    cmd->commandlength = 2;

    /* backup file name as switchtraverse will delete it */
    file = (uint8_t*)lib_strdup((const char *)cmd->file);
    filelength = cmd->filelength;

    if (vdrive_command_switchtraverse(vdrive, cmd)) {
        /* error out if we can't CD to it */
        status = CBMDOS_IPE_NOT_FOUND;
        goto out;
    }

    /* status regardless of outcome */
    status = CBMDOS_IPE_DELETED;

    /* we are now in it; check if there are any files here */
    vdrive_dir_find_first_slot(vdrive, (uint8_t*)"*", 1, 0, &dir);

    while ((slot = vdrive_dir_find_next_slot(&dir))) {
        /* if DIR entrie is not deleted, leave */
        if (slot[SLOT_TYPE_OFFSET] != 0) {
            status = CBMDOS_IPE_DELETED;
            goto out;
        }
    }

    /* nothing in the folder; go back to previous directory */
    vdrive_set_disk_geometry(vdrive);

    /* find the entry */
    vdrive_dir_find_first_slot(vdrive, file, filelength, CBMDOS_FT_DIR, &dir);

    slot = vdrive_dir_find_next_slot(&dir);

    /* it shouldn't be null, but let's be safe anyways */
    if (slot) {
        /* remove the header/directory chain */
        vdrive_dir_remove_slot(&dir);
        deleted_files++;
    }

    if (file) {
        lib_free(file);
    }

out:
    vdrive_command_return(vdrive, origpart);

    return vdrive_command_set_error(vdrive, status, deleted_files, 0);
}

static int vdrive_command_initialize(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
/* backup current partition information */
    int origpart = vdrive->current_part;

    if (cmd) {
        status = CBMDOS_IPE_NOT_READY;
        if (vdrive_command_switch(vdrive, cmd->drive)) {
            goto out;
        }
    }

    vdrive_close_all_channels_partition(vdrive, vdrive->current_part);

    /* on 1581s, goto root partition */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) {
        vdrive_bam_write_bam(vdrive);
        /* force vdrive_set_disk_geometry to update presistent values */
        vdrive->cheadertrack[vdrive->current_part] = 0;
        vdrive_set_disk_geometry(vdrive);
        /* update the BAM in ram */
        vdrive_bam_setup_bam(vdrive);
    }

    /* Update BAM in memory.  */
    if (vdrive->image != NULL) {
        vdrive_bam_setup_bam(vdrive);
    }
    status = CBMDOS_IPE_OK;

out:
    if (cmd) {
        vdrive_command_return(vdrive, origpart);
    }

    return status;
}

static int vdrive_command_allocate_chain(vdrive_t *vdrive, unsigned int t, unsigned int s, unsigned int *c)
{
    uint8_t tmp[256];
    int rc;

    while (t) {
        /* Check for illegal track or sector.  */
        if (disk_image_check_sector(vdrive->image, t, s) < 0) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR,
                                     s, t);
            return CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR;
        }
        if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
            /* The real drive does not seem to catch this error.  */
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_BLOCK, s, t);
            return CBMDOS_IPE_NO_BLOCK;
        }
        rc = vdrive_read_sector(vdrive, tmp, t, s);
        if (rc > 0) {
            return rc;
        }
        if (rc < 0) {
            return CBMDOS_IPE_NOT_READY;
        }

        t = (int)tmp[0];
        s = (int)tmp[1];
        if (c) {
            *c = (*c) + 1;
        }
    }
    return CBMDOS_IPE_OK;
}

static int vdrive_command_validate_internal(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;

/* backup current partition information */
    int origpart = vdrive->current_part;

    if (VDRIVE_IS_READONLY(vdrive)) {
        return CBMDOS_IPE_WRITE_PROTECT_ON;
    }

    status = CBMDOS_IPE_NOT_READY;
    if (vdrive_command_switch(vdrive, cmd->drive)) {
        goto out;
    }

    status = vdrive_command_validate(vdrive);

out:
    vdrive_command_return(vdrive, origpart);

    return status;
}

static int vdrive_command_validate_worker(vdrive_t *vdrive, int geos, unsigned int *t_passed, unsigned int *s_passed)
{
    unsigned int l, sz, t = 0, s = 0;
    int status;
    uint8_t *b;
    vdrive_dir_context_t dir;
    uint8_t tmp[256];
    int i;
    unsigned int old_header_track, old_header_sector;

    status = CBMDOS_IPE_OK;
    old_header_track = vdrive->Header_Track;
    old_header_sector = vdrive->Header_Sector;
    vdrive->Header_Track = *t_passed;
    vdrive->Header_Sector = *s_passed;

    vdrive_dir_find_first_slot(vdrive, (uint8_t*)"*", 1, 0, &dir);

    while ((b = vdrive_dir_find_next_slot(&dir))) {
        char *filetype = (char *)&dir.buffer[dir.slot * 32 + SLOT_TYPE_OFFSET];

        if (*filetype & CBMDOS_FT_CLOSED) {
        /* If it is a closed/complete file ... */
            l = sz = 0;
            if ((*filetype & 0x07) == CBMDOS_FT_CBM) {
                /* CBM type on 1581 */
                if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_1581) {
                    /* throw error 71 if there is a CBM type on a non-1581 format */
                    status = CBMDOS_IPE_DIRECTORY_ERROR;
                    goto bad;
                }
                t = b[SLOT_FIRST_TRACK];
                s = b[SLOT_FIRST_SECTOR];
                l = b[SLOT_NR_BLOCKS] | (b[SLOT_NR_BLOCKS + 1] << 8);
                /* allocate sectors */
                while (l) {
                    /* Check for illegal track or sector.  */
                    if (t == 40 || disk_image_check_sector(vdrive->image, t, s) < 0) {
                        status = CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR;
                        break;
                    }
                    if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
                        status = CBMDOS_IPE_NO_BLOCK;
                        break;
                    }

                    s++;
                    if (s > 39) {
                        s = 0;
                        t++;
                    }
                    l--;
                }
                if (l) {
                    goto bad;
                }
                l = sz = 0;
            } else if ((*filetype & 0x07) == CBMDOS_FT_DIR) {
                /* NP DIR */
                if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_NP) {
                    /* throw error 71 if there is a DIR type on a non-NP format */
                    status = CBMDOS_IPE_DIRECTORY_ERROR;
                    goto bad;
                }
                t = b[SLOT_FIRST_TRACK];
                s = b[SLOT_FIRST_SECTOR];
                sz = b[SLOT_NR_BLOCKS] | (b[SLOT_NR_BLOCKS + 1] << 8);
                /* allocate diectory chain (header+dir) */
                status = vdrive_command_allocate_chain(vdrive, t, s, &l);
                if (status != CBMDOS_IPE_OK) {
                    goto bad;
                }
                /* code below long if will update blocks info */
                /* recursively call itself to process subdirectories */
                status = vdrive_command_validate_worker(vdrive, geos, &t, &s);
                if (status != CBMDOS_IPE_OK) {
                    goto bad;
                }
            } else if ((*filetype & 0x07) >= CBMDOS_FT_SEQ && (*filetype & 0x07) <= CBMDOS_FT_REL) {
                /* SEQ, PRG, USR */
                sz = b[SLOT_NR_BLOCKS] | (b[SLOT_NR_BLOCKS + 1] << 8);
                l = sz;
                if (b[SLOT_GEOS_TYPE] && geos && (*filetype & 0x07) != CBMDOS_FT_REL) {
                    /* GEOS disk, validate appropriately */
                    /* validate info block */
                    l = 0;
                    t = b[SLOT_GEOS_ITRACK];
                    s = b[SLOT_GEOS_ISECTOR];
                    if (t) {
                        if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
                            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_BLOCK, t, s);
                            goto bad;
                        }
                    }
                    l++;

                    /* check type */
                    if (b[SLOT_GEOS_STRUCT] == 0) {
                        /* sequential type, just like CBM */
                        t = b[SLOT_FIRST_TRACK];
                        s = b[SLOT_FIRST_SECTOR];
                        status = vdrive_command_allocate_chain(vdrive, t, s, &l);
                        if (status != CBMDOS_IPE_OK) {
                            goto bad;
                        }
                    } else if (b[SLOT_GEOS_STRUCT] == 1) {
                        /* VLIR type, like REL */
                        /* read record pointer sector */
                        t = b[SLOT_FIRST_TRACK];
                        s = b[SLOT_FIRST_SECTOR];
                        status = vdrive_read_sector(vdrive, tmp, t, s);
                        if (status > 0) {
                            goto bad;
                        }
                        if (status < 0) {
                            status = CBMDOS_IPE_NOT_READY;
                            goto bad;
                        }
                        if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
                            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_BLOCK, t, s);
                            goto bad;
                        }
                        l++;
                        /* cycle through all 127 records */
                        for (i = 0; i < 127; i++) {
                            t = tmp[2 + i * 2];
                            s = tmp[2 + i * 2 + 1];
                            /* 0,255 is empty */
                            if (t == 0 && s == 255) {
                                continue;
                            }
                            /* 0,0 is end */
                            if (t == 0 && s == 0) {
                                break;
                            }
                            /* anything else is effectively a normal file */
                            status = vdrive_command_allocate_chain(vdrive, t, s, &l);
                            if (status != CBMDOS_IPE_OK) {
                                goto bad;
                            }
                        }

                    } else {
                        /* don't know what this is */
                        status = CBMDOS_IPE_DIRECTORY_ERROR;
                        goto bad;
                    }
                } else {
                    /* Standard CBM files */
                    l = 0;
                    t = b[SLOT_FIRST_TRACK];
                    s = b[SLOT_FIRST_SECTOR];
                    status = vdrive_command_allocate_chain(vdrive, t, s, &l);
                    if (status != CBMDOS_IPE_OK) {
                        goto bad;
                    }

                    /* The real drive always validates side sectors even if the file
                       type is not REL.  */
                    if ((*filetype & 0x07) == CBMDOS_FT_REL) {
                        t = b[SLOT_SIDE_TRACK];
                        s = b[SLOT_SIDE_SECTOR];
                        status = vdrive_command_allocate_chain(vdrive, t, s, &l);
                        if (status != CBMDOS_IPE_OK) {
                            goto bad;
                        }
                    }
                }
            } else if ((*filetype & 0x07) == CBMDOS_FT_DIR) {
                 l = sz = 0;
            } /* anything else, skip it */
            /* check to see if block count matches up; if not, update entry */
            if ( sz != l ) {
                dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS] = l & 255;
                dir.buffer[dir.slot * 32 + SLOT_NR_BLOCKS + 1] = l >> 8;
                t = dir.track;
                s = dir.sector;
                if (vdrive_write_sector(vdrive, dir.buffer, t, s) < 0) {
                    status = CBMDOS_IPE_WRITE_ERROR_VER;
                    goto bad;
                }
            }
        } else {
            /* Delete an unclosed file. */
            *filetype = CBMDOS_FT_DEL;
            t = dir.track;
            s = dir.sector;
            if (vdrive_write_sector(vdrive, dir.buffer, t, s) < 0) {
                status = CBMDOS_IPE_WRITE_ERROR_VER;
                goto bad;
            }
        }
    }

bad:
    *t_passed = t;
    *s_passed = s;

    /* restore original dir track and sector */
    vdrive->Header_Track = old_header_track;
    vdrive->Header_Sector = old_header_sector;

    return status;
}

int vdrive_command_validate(vdrive_t *vdrive)
{
    unsigned int t = 0, s = 0;
    int status, max_sector;
    uint8_t *oldbam = NULL;
    uint8_t *oldbamstate = NULL;
    int geos;

    vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);

    if (VDRIVE_IS_READONLY(vdrive)) {
        status = CBMDOS_IPE_WRITE_PROTECT_ON;
        goto out;
    }

    /* read the whole bam so we can make a backup */
    status = vdrive_bam_read_bam(vdrive);
    if (status) {
        status = CBMDOS_IPE_NOT_READY;
        goto out;
    }

    /* don't initialize as it will reset the parition on 1581's */
    /* close all files first */
    vdrive_close_all_channels_partition(vdrive, vdrive->current_part);

    /* save BAM incase there is some kind of error */
    oldbam = lib_malloc(vdrive->bam_size);
    oldbamstate = lib_malloc(VDRIVE_BAM_MAX_STATES);
    memcpy(oldbam, vdrive->bam, vdrive->bam_size);
    memcpy(oldbamstate, vdrive->bam_state, VDRIVE_BAM_MAX_STATES);

    /* set all bits to "zero" which is allocated */
    vdrive_bam_clear_all(vdrive);

    /* "free" all known sectors in defined partition size for 1581 */
    /* works on all drives too */
    for (t = vdrive->Part_Start; t <= vdrive->Part_End; t++) {
        max_sector = vdrive_get_max_sectors(vdrive, t);
        for (s = 0; s < (unsigned int)max_sector; s++) {
            vdrive_bam_free_sector(vdrive, t, s);
        }
    }

    /* First, map out the header (BAM) and the directory, themselves. */
    if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_9000) {
        status = vdrive_command_allocate_chain(vdrive, vdrive->Bam_Track,
                                               vdrive->Bam_Sector, NULL);
        if (status != CBMDOS_IPE_OK) {
            goto bad;
        }
    }

    /* other specific drives */
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1571:
            /* Map the opposite side of the directory cylinder. */
            max_sector = vdrive_get_max_sectors(vdrive, 53);
            for (s = 0; s < (unsigned int)max_sector; s++) {
                vdrive_bam_allocate_sector(vdrive, 53, s);
            }
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            /* Map the BAM sectors. */
            vdrive_bam_allocate_sector(vdrive, vdrive->Bam_Track, vdrive->Bam_Sector + 1);
            vdrive_bam_allocate_sector(vdrive, vdrive->Bam_Track, vdrive->Bam_Sector + 2);
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
            /* BAM allocation above doesn't include track 39, sector 0 */
            vdrive_bam_allocate_sector(vdrive, vdrive->Header_Track, vdrive->Header_Sector);
            break;
        case VDRIVE_IMAGE_FORMAT_NP:
            /* Map the boot sector. */
            vdrive_bam_allocate_sector(vdrive, 1, 0);

            /* Map the BAM sectors. */
            for (s = 2; s < 34; s++) {
                vdrive_bam_allocate_sector(vdrive, 1, s);
            }
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            /* The D9090/60 bam ends with 255/255 not 0/x */
            vdrive_bam_allocate_chain_255(vdrive, vdrive->Bam_Track, vdrive->Bam_Sector);
            /* BAM allocation above doesn't include header or dir */
            /* header include dir */
            vdrive_bam_allocate_chain(vdrive, vdrive->Header_Track, vdrive->Header_Sector);
            /* Map the config sector and bad blocks sector */
            vdrive_bam_allocate_sector(vdrive, 0, 0);
            vdrive_bam_allocate_sector(vdrive, 0, 1);
            break;
    }

    /* check if this is a GEOS formatted disk */
    geos = vdrive_bam_isgeos(vdrive);
    if (geos) {
        /* if so, allocate the border sector */
        t = vdrive->bam[0xab];
        s = vdrive->bam[0xac];
        if (t) {
            if (!vdrive_bam_allocate_sector(vdrive, t, s)) {
                vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_BLOCK, t, s);
                goto out;
            }
        }
    }

    t = vdrive->Header_Track;
    s = vdrive->Header_Sector;

    /* NPs validate from any subfolder, but always comeback to that subfolder */
    /* The worker function will recover this, we just set the start point */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        t = BAM_TRACK_NP;
        s = BAM_SECTOR_NP;
    }

    status = vdrive_command_validate_worker(vdrive, geos, &t, &s);

    if (status == CBMDOS_IPE_OK) {
        /* Write back BAM only if validate was successful.  */
        vdrive_bam_write_bam(vdrive);
        goto out;
    }

bad:
    memcpy(vdrive->bam, oldbam, vdrive->bam_size);
    memcpy(vdrive->bam_state, oldbamstate, VDRIVE_BAM_MAX_STATES);

out:
    if (oldbam) {
        lib_free(oldbam);
        lib_free(oldbamstate);
    }

    if (vdrive->last_code != CBMDOS_IPE_OK ) {
        vdrive_command_set_error(vdrive, status, t, s);
    }

    return status;
}

static int vdrive_command_format_internal(vdrive_t *vdrive, cbmdos_cmd_parse_plus_t *cmd)
{
    int status;
    uint8_t *name = NULL, *id = NULL, *more = NULL, *longname = NULL, newid[3] = "  ";
    int morelength = 0;
    int i, j, k, ptype = -1, parts = -1, psize = 0, ssize = 0;
    uint8_t buf[256];
/* backup current partition information */
    int origpart = vdrive->current_part;

    if (VDRIVE_IS_READONLY(vdrive)) {
        return CBMDOS_IPE_WRITE_PROTECT_ON;
    }

    if (!cmd->file || !cmd->filelength) {
        return CBMDOS_IPE_SYNTAX;
    }

    /* split up name, id, and extra stuff */
    name = cmd->file;
    name[cmd->filelength] = 0;
    /* parser plus splits on the ',' */
    if (cmd->morelength && cmd->more && cmd->more[0] == ',') {
        /* id supplied, check to see if there is more */
        more = memchr(&(cmd->more[1]), ',', cmd->morelength - 1);
        id = newid;
        if (!more) {
            /* nothing extra, just get id */
            if (cmd->morelength > 1) {
                newid[0] = cmd->more[1];
            }
            if (cmd->morelength > 2) {
                newid[1] = cmd->more[2];
            }
            more = NULL;
        } else {
            /* something extra, get id, setup for later */
            if (more - &(cmd->more[1]) > 0) {
                newid[0] = cmd->more[1];
            }
            if (more - &(cmd->more[1]) > 1) {
                newid[1] = cmd->more[2];
            }
            more++;
            morelength = (int)(cmd->morelength - (more - cmd->more));
        }
    }

    /* CMD FDs allow partitioning with the NEW command, if more options are given */
    if (VDRIVE_IS_FD(vdrive) && more) {
        /* process parameters: DD8, DDN, HD8, HDN, ED8, EDN, S8, SN, but ignore 81 */
        if (morelength == 3 && more[1] == 'D') {
            if (more[2] == '8') {
                ptype = 4;
                if (more[0] == 'D' && vdrive->image->type == DISK_IMAGE_TYPE_D1M) {
                    parts = 1;
                }
                if (more[0] == 'H' && vdrive->image->type == DISK_IMAGE_TYPE_D2M) {
                    parts = 2;
                }
                if (more[0] == 'E' && vdrive->image->type == DISK_IMAGE_TYPE_D4M) {
                    parts = 4;
                }
            } else if (more[2] == 'N') {
                ptype = 1;
                parts = 1;
            }
        } else if (morelength == 2 && more[0] == 'S') {
            if (more[1] == '8') {
                ptype = 4;
                if (vdrive->image->type == DISK_IMAGE_TYPE_D1M) {
                    parts = 1;
                }
                if (vdrive->image->type == DISK_IMAGE_TYPE_D2M) {
                    parts = 2;
                }
                if (vdrive->image->type == DISK_IMAGE_TYPE_D4M) {
                    parts = 4;
                }
            } else if (more[1] == 'N') {
                ptype = 1;
                parts = 1;
            }
        }
        status = CBMDOS_IPE_FORMAT_ERROR;
        /* leave with error if parameters aren't set correctly */
        if (ptype < 0 || parts < 0) {
            goto out2;
        }
        /* set disk size */
        switch (vdrive->image->type) {
            case DISK_IMAGE_TYPE_D1M:
                psize = 0x640;
                ssize = 32;
                break;
            case DISK_IMAGE_TYPE_D2M:
                psize = 0xc80;
                ssize = 72;
                break;
            case DISK_IMAGE_TYPE_D4M:
                psize = 0x1900;
                ssize = 152;
                break;
        }

        vdrive_close_all_channels(vdrive);

        /* setup disk image access */
        vdrive->sys_offset = psize;
        vdrive->current_offset = psize;
        vdrive->image_format = VDRIVE_IMAGE_FORMAT_SYS;
        vdrive->current_part = 255;

        /* clear system area */
        memset(buf, 0, 256);
        for (i = 0; i < 8; i++) {
            if (i != 5) {
                if (vdrive_write_sector(vdrive, buf, 0, i)) {
                    goto out2;
                }
            }
        }
        for (i = 4; i < ssize; i++) {
            if (vdrive_write_sector(vdrive, buf, 1, i)) {
                goto out2;
            }
        }

        /* create partition(s) */
        buf[1] = 0xff;
        if (vdrive_write_sector(vdrive, buf, 1, 3)) {
            goto out2;
        }
        buf[0] = 0x01;
        buf[1] = 0x02;
        if (vdrive_write_sector(vdrive, buf, 1, 1)) {
            goto out2;
        }
        buf[1] = 0x03;
        if (vdrive_write_sector(vdrive, buf, 1, 2)) {
            goto out2;
        }
        /* do system partition */
        buf[1] = 0x01;
        buf[2] = 0xff;
        memset(&(buf[5]), 0xa0, 16);
        memcpy(&(buf[5]), "SYSTEM", 6);
        /* do whatever is left */
        j = 0;
        k = 0x20;
        for (i = 0; i < parts; i++) {
            buf[k | 0x02] = ptype;
            memset(&(buf[k | 0x05]), 0xa0, 16);
            memcpy(&(buf[k | 0x05]), "PARTITION ", 10);
            buf[k | 0x0f] = '1' + i;
            buf[k | 0x16] = (j >> 8) & 255;
            buf[k | 0x17] = j & 255;
            buf[k | 0x1e] = ((psize/parts) >> 8) & 255;
            buf[k | 0x1f] = (psize/parts) & 255;
            j += (psize/parts);
            k += 0x20;
        }
        if (vdrive_write_sector(vdrive, buf, 1, 0)) {
            goto out2;
        }

        /* setup system partition */
        memset(&(buf[0]), 255, 224);
        memset(&(buf[224]), 0, 32);
        buf[0x00] = 0;
        buf[0x38] = 0;
        buf[0x39] = 0;
        buf[0x70] = psize & 255;
        buf[0x71] = (psize >> 8) & 255;
        buf[0xa8] = 0;
        buf[0xa9] = 0;
        buf[0xe2] = 1;
        buf[0xe3] = 1;
        memcpy(&(buf[0xf0]), "CMD FD SERIES   ", 16);
        if (vdrive_write_sector(vdrive, buf, 0, 5)) {
            goto out2;
        }

        /* force re-read of system partition */
        vdrive->sys_offset = UINT32_MAX;
        vdrive->current_offset = UINT32_MAX;
        if (vdrive_read_partition_table(vdrive)) {
            /* this shouldn't happen */
            goto out2;
        }

        origpart = vdrive->default_part;

        if (parts > 1) {
             i = (int)strlen((char*)name);
             longname = lib_malloc(17);
             memset(longname, ' ', 16);
             memcpy(longname, name, i);
             longname[14] = ' ';
             longname[16] = 0;
        }

        /* go through each partition and format it using the label provided */
        /* "id" is provided so all data will be destroyed */
        for (i = 0; i < parts; i++) {
            /* reset geometry for 1581s */
            vdrive->cheadertrack[i + 1] = 0;
            if (vdrive_command_switch(vdrive, i + 1)) {
                goto out;
            }
            if (longname) {
                longname[15] = '1' + i;
            }
            status = vdrive_command_format_worker(vdrive, i == 0 ? name : longname, id);
            if (status) {
                goto out;
            }
        }
        if (longname) {
            lib_free(longname);
        }
        /* all done */
        status = CBMDOS_IPE_OK;
        goto out;
    }

    status = CBMDOS_IPE_NOT_READY;
    if (vdrive_command_switch(vdrive, cmd->drive)) {
        goto out;
    }

    status = vdrive_command_format_worker(vdrive, name, id);

out:
    vdrive_command_return(vdrive, origpart);

out2:
    return status;
}

int vdrive_command_format(struct vdrive_s *vdrive, const char *disk_name)
{
    int status;
    uint8_t *p, *po, *pc;
    int length;
    cbmdos_cmd_parse_plus_t cmd;

    if (!disk_name) {
        return CBMDOS_IPE_SYNTAX;
    }

    if (VDRIVE_IS_READONLY(vdrive)) {
        return CBMDOS_IPE_WRITE_PROTECT_ON;
    }

    if (vdrive->image->device == DISK_IMAGE_DEVICE_FS) {
        if (disk_image_fsimage_fd_get(vdrive->image) == NULL) {
            return CBMDOS_IPE_NOT_READY;
        }
    }

    /* TODO: the cmd structure should just be created and passed to the format function */

    /* create NEW command */
    length = (int)strlen(disk_name);
    po = p = lib_malloc(length + 5);
    *p = 'N';
    p++;
    /* add a ':' if none was given in the name */
    pc = memchr(disk_name, ':', length);
    if (!pc) {
        *p = ':';
        p++;
    }
    /* pad empty disk names from GUI create menu */
    if (!length || disk_name[0] == ',') {
        *p = ' ';
        p++;
    }
    memcpy(p, disk_name, length);
    p[length] = 0;

    cmd.full = po;
    cmd.fulllength = (unsigned int)strlen((char*)po);
    cmd.secondary = 0;
    cmd.mode = 1;

    status = cbmdos_command_parse_plus(&cmd);

    if (status != CBMDOS_IPE_OK) {
        goto out;
    }

    /* add an ID, if it isn't there, to force a full format */
    if (!cmd.more) {
        cmd.more = (uint8_t*)lib_strdup(",  ");
        cmd.morelength = 3;
    }

    status = vdrive_command_format_internal(vdrive, &cmd);

out:

    vdrive_cmdfree_abbrv(&cmd);
    vdrive_cmdfree_path(&cmd);
    vdrive_cmdfree_file(&cmd);
    vdrive_cmdfree_command(&cmd);
    vdrive_cmdfree_more(&cmd);
    lib_free(po);

    return status;
}

static int vdrive_command_format_worker(struct vdrive_s *vdrive, uint8_t *disk_name, uint8_t *disk_id)
{
    uint8_t tmp[256];
    int status;
    uint8_t id[2];
    int t, s, max_sector;

    /* close all files first */
    vdrive_close_all_channels_partition(vdrive, vdrive->current_part);

    memset(tmp, 0, 256);

    if (!disk_id) {
        /* take old ID first */
        if (!vdrive_bam_int_get_disk_id(vdrive, id)) {
            disk_id = id;
        }
        /* emulate behavior of 1581 for replacing header */
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) {
            /* It won't update the header unless the dos version is correct.
               If you try it just returns error 73.
               It also REALLY DOESN'T LIKE if the IDs on the header and BAMs
               don't match up; it sets them to NULL and the directory listing
               is later corrupted as this will terminate the basic lines.
               We won't emulate this. */
            if (vdrive->bam[0x02] != 68) {
                status = CBMDOS_IPE_DOS_VERSION;
                /* the status "track" should be the first track of the partition,
                   but this isn't necessary to execution */
                goto out;
            }
        }
    } else {
        /* FIXME: should we do this? */
        /* If ID provided, erase data */
        for (t = vdrive->Part_Start; t <= vdrive->Part_End; t++) {
            max_sector = vdrive_get_max_sectors(vdrive, t);
            for (s = 0; s < (unsigned int)max_sector; s++) {
                if (vdrive_write_sector(vdrive, tmp, t, s) < 0) {
                    status = CBMDOS_IPE_WRITE_ERROR_VER;
                    goto out;
                }
            }
        }
    }

    /* Make the first dir-entry.  */
    tmp[1] = 255;

    /* if NP type, reset to root directory */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP) {
        /* force vdrive_set_disk_geometry to update presistent values */
        vdrive->cheadertrack[vdrive->current_part] = 0;
        vdrive_set_disk_geometry(vdrive);
    }

    if (vdrive_write_sector(vdrive, tmp, vdrive->Dir_Track, vdrive->Dir_Sector) < 0) {
        status = CBMDOS_IPE_WRITE_ERROR_VER;
        goto out;
    }
    vdrive_bam_create_empty_bam(vdrive, (char*)disk_name, disk_id);
    vdrive_bam_write_bam(vdrive);

    /* Validate is called to clear the BAM.  */
    status = vdrive_command_validate(vdrive);

out:
    return status;
}

static int vdrive_command_position(vdrive_t *vdrive, uint8_t *buf,
                                   unsigned int length)
{
    unsigned int channel;
/* default the record number to 1 */
/* default the record number's high byte to 0 */
/* default the position to 1 */
    unsigned int rec_lo = 1, rec_hi = 0, position = 1;

    switch (length) {
        /* 5 or more */
        default:
            position = buf[4];
            /* fall through */
        case 4:
            rec_hi = buf[3];
            /* fall through */
        case 3:
            rec_lo = buf[2];
            /* fall through */
        case 2:
            /* Remove bits 5 & 6 from the channel number. */
            channel = buf[1] & 0x0f;
            break;
        case 1: /* no channel was specified; return NO CHANNEL */
        case 0: /* shouldn't happen, but just in case */
            return CBMDOS_IPE_NO_CHANNEL;
    }

    if (vdrive->buffers[channel].mode != BUFFER_RELATIVE) {
        return CBMDOS_IPE_NO_CHANNEL;
    }

    return vdrive_rel_position(vdrive, channel, rec_lo, rec_hi, position);
}


/* ------------------------------------------------------------------------- */

int vdrive_command_set_error(vdrive_t *vdrive, int code, unsigned int track,
                              unsigned int sector)
{
    const char *message = "";
    bufferinfo_t *p = &vdrive->buffers[15];

/*    last_code = CBMDOS_IPE_OK; */

#ifdef DEBUG_DRIVE
    log_debug("Set error channel: code =%d, last_code =%d, track =%u, "
              "sector =%u.", code, vdrive->last_code, track, sector);
#endif

#if 0
    /* Set an error only once per command. */
    if (code != CBMDOS_IPE_OK && vdrive->last_code != CBMDOS_IPE_OK) {
        return code;
    }
#endif
    vdrive->last_code = code;

    message = cbmdos_errortext(code);

    sprintf((char *)p->buffer, "%02d,%s,%02u,%02u\015", code,
            message, track, sector);

    /* Length points to the last byte, and doesn't give the length.  */
    p->length = (unsigned int)strlen((char *)p->buffer) - 1;

    if (code && code != CBMDOS_IPE_DOS_VERSION) {
        log_message(vdrive_command_log, "ERR = %02d, %s, %02u, %02u",
                    code, message, track, sector);
    }

    p->bufptr = 0;
    p->readmode = CBMDOS_FAM_READ;

    return code;
}

/* FIXME: incomplete */
int vdrive_command_memory_write(vdrive_t *vdrive, const uint8_t *buf, uint16_t addr, unsigned int length)
{
    unsigned int len = buf[0];
    int i, job;
    int jobs = 0, tracksector = 0, maxjobs = 0;
    unsigned int type = 0;

    /* make sure we have enough data */
    if (len + 1 > length - 5) {
        log_warning(vdrive_command_log,
            "M-W %04x %u (command ends prematurely, got %u bytes) (might need TDE)", addr, len, length);
        return vdrive_command_set_error(vdrive, CBMDOS_IPE_SYNTAX, 0, 0);
    }

    /* only to RAM */
    if (addr >= 0x8000) {
        goto out;
    }

    /* commit the data */
    for (i = 0; i < len; i++) {
        vdrive->ram[(addr + i) & 0x7fff] = buf[1 + i];
    }

    /* Since FSDEVICE uses this command, we need to make sure we are using
      an actual disc image before proceeding. */
    if (!VDRIVE_IS_IMAGE(vdrive)) {
        goto out;
    }

    /* support only FD series */
    if (!VDRIVE_IS_FD(vdrive)) {
        goto out;
    }

    /* grab a valid image type as there may be no context */
    if (vdrive->image) {
        type = vdrive->image->type;
    }

    /* emulate job queue */
    switch (type) {
        case DISK_IMAGE_TYPE_P64:
        case DISK_IMAGE_TYPE_G64:
        case DISK_IMAGE_TYPE_D64:
        case DISK_IMAGE_TYPE_D71:
        case DISK_IMAGE_TYPE_G71:
            jobs = 0;
            tracksector = 0x0006;
            maxjobs = 5;
            break;
        case DISK_IMAGE_TYPE_D81:
            jobs = 2;
            tracksector = 0x000b;
            maxjobs = 9;
            break;
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
            jobs = 0x28;
            tracksector = 0x2800;
            maxjobs = 32;
            break;
        case DISK_IMAGE_TYPE_DHD:
            jobs = 0x20;
            tracksector = 0x2800;
            maxjobs = 32;
            break;
        case DISK_IMAGE_TYPE_D67:
        case DISK_IMAGE_TYPE_D80:
        case DISK_IMAGE_TYPE_D82:
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
#endif
        default:
            maxjobs = 0;
            break;
    }

    /* leave if writes not near job queue */
    if (!((addr >= jobs && addr < jobs + maxjobs) || (addr + len >= jobs && addr + len < jobs + maxjobs))) {
        goto good;
    }

    /* check job queue */
    for (i = 0; i < maxjobs; i++) {
        job = vdrive->ram[jobs + i];
        if (job >= 0x80) {
            switch (job) {
                case 0x80:
#ifdef DEBUG_DRIVE
log_warning(LOG_DEFAULT,"job #%d read sector %u %u",i,(unsigned int)vdrive->ram[tracksector + (i << 1)], (unsigned int)vdrive->ram[tracksector + (i << 1) + 1]);
#endif
                    vdrive_switch(vdrive, vdrive->selected_part);
                    if (vdrive_read_sector(vdrive, &(vdrive->ram[0x300 + (i << 8)]), vdrive->ram[tracksector + (i << 1)], vdrive->ram[tracksector + (i << 1) + 1])) {
                        vdrive->ram[jobs + i] = 5;
                    } else {
                        vdrive->ram[jobs + i] = 0;
                    }
                    break;

                case 0x90:
#ifdef DEBUG_DRIVE
log_warning(LOG_DEFAULT,"job #%d write sector %u %u",i,(unsigned int)vdrive->ram[tracksector + (i << 1)], (unsigned int)vdrive->ram[tracksector + (i << 1) + 1]);
#endif
                    if (!VDRIVE_IS_READONLY(vdrive)) {
                        vdrive_switch(vdrive, vdrive->selected_part);
                        if (vdrive_write_sector(vdrive, &(vdrive->ram[0x300 + (i << 8)]), vdrive->ram[tracksector + (i << 1)], vdrive->ram[tracksector + (i << 1) + 1])) {
                            vdrive->ram[jobs + i] = 7;
                        } else {
                            vdrive->ram[jobs + i] = 0;
                        }
                    } else {
                        vdrive->ram[jobs + i] = 8;
                    }
                    break;

                case 0xa0:
                case 0xb0:
                case 0xb8:
                case 0x82:
                case 0x86:
                case 0x88:
                case 0x8a:
                case 0x8c:
                    vdrive->ram[jobs + i] = 0;
                    break;

                case 0x84:
                    vdrive->ram[jobs + i] = 1;
                    break;

                case 0xd0:
                case 0xe0:
                    log_warning(vdrive_command_log, "M-W %04x %u (+%u) (Job Queue Execute Function - needs TDE)", addr, len, length - 6);
                    break;

                default:
                    log_warning(vdrive_command_log, "Unknown job code: %02x\n", (unsigned int)job);
                    break;
            }
        }
    }

    goto good;

out:

    log_warning(vdrive_command_log,
            "M-W %04x %u (+%u) (might need TDE)",
            addr, len, length - 6);

good:

    return vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);
}

/* FIXME: This function doesn't need buf or length. */
int vdrive_command_memory_exec(vdrive_t *vdrive, const uint8_t *buf, uint16_t addr, unsigned int length)
{
    if (length < 5) {
        log_warning(vdrive_command_log,
            "M-E %04x (command ends prematurely, got %u bytes) (needs TDE)", addr, length);
        return vdrive_command_set_error(vdrive, CBMDOS_IPE_SYNTAX, 0, 0);
    }

    log_warning(vdrive_command_log, "M-E %04x (+%u) (needs TDE)", addr, length - 5);
    return vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);
}

/*
  Not a real drive, return some things for FD series
*/
/*
 CMD FD: 0xFEA0 = "CMD FD"
         0xFEF0 = "2" for 2000, "4" for 4000
* Job queue $28-$47 ($80,$90,...)
* MHDRS  $2800-$283f (t/s pairs)
* MHDRS2 $28c0-$28ff
* MSIDS  $2840-$285f
* JOBBUF  $300-$21ff
*
*/
int vdrive_command_memory_read(vdrive_t *vdrive, const uint8_t *buf, uint16_t addr, unsigned int length)
{
    unsigned int len = buf[0];
    int i;
    bufferinfo_t *p = &vdrive->buffers[15];

    if (length < 6) {
        log_warning(vdrive_command_log,
            "M-R %04x %u (command ends prematurely, got %u bytes) (might need TDE)", addr, len, length);
        if (length < 5) {
            return vdrive_command_set_error(vdrive, CBMDOS_IPE_SYNTAX, 0, 0);
        } else {
            len = 1; /* when no length byte is present, the length is 1 */
        }
    } else {
        log_warning(vdrive_command_log, "M-R %04x %u (+%u) (might need TDE)",
                addr, len, length - 6);
    }

    /* support only FD series for FD-TOOLS */
    if (VDRIVE_IS_FD(vdrive) && VDRIVE_IS_IMAGE(vdrive)) {
        if (addr == 0xfea0 && len == 6) {
            memcpy(p->buffer, "CMD FD", 6);
            goto out;
        } else if (addr == 0xfef0 && len == 1) {
            if (vdrive->image->type == DISK_IMAGE_TYPE_D1M
                || vdrive->image->type == DISK_IMAGE_TYPE_D2M) {
                i = '2';
            } else {
                i = '4';
            }
            p->buffer[0] = i;
            goto out;
        }
    }

    if (len == 0) {
        len = 256;
    }

    /* move the data */
    for (i = 0; i < len; i++) {
        p->buffer[i] = vdrive->ram[(addr + i) & 0x7fff];
    }
    /* add a CR at the end */
    p->buffer[i] = 13;

out:
    p->length = len;
    p->bufptr = 0;
    p->readmode = CBMDOS_FAM_READ;

    /* don't update the buffer as it has the return memory data */
    return CBMDOS_IPE_MEMORY_READ;
}

inline static int int_to_bcd(int dec)
{
    return ((dec / 10) << 4) + (dec % 10);
}

static int vdrive_command_time(vdrive_t *vdrive, uint8_t *cmd, int length)
{
/* only process T-RA, T-RD, T-RB, and ignore T-WA, T-WD, T-WB; error out on everything else */
    int dyear, dmon, ddate, dday, dhour, dhour2, dmin, dsec;
    bufferinfo_t *p = &vdrive->buffers[15];
    uint8_t ampm;
    time_t timep;
    struct tm *ts;

    static const char * const days[7] = {
        "SUN.", "MON.", "TUES", "WED.", "THUR", "FRI.", "SAT."
    };

    if (length < 4 || (length > 1 && cmd[0] != 'T') || (length > 2 && cmd[1] != '-')) {
        goto bad;
    }

    time(&timep);
    ts = localtime(&timep);
#ifdef DEBUG_DRIVE
    log_debug("current time: %s %02d/%02d/%04d %02d:%02d:%02d\n",
            days[ts->tm_wday], ts->tm_mon + 1, ts->tm_mday, ts->tm_year + 1900,
            ts->tm_hour, ts->tm_min, ts->tm_sec);
#endif

    if (cmd[2] == 'R') {
        dday = ts->tm_wday;
        dyear = ts->tm_year;
        dmon = ts->tm_mon + 1;
        ddate = ts->tm_mday;
        dhour = ts->tm_hour;
        dmin = ts->tm_min;
        dsec = ts->tm_sec;
        ampm = 'A';
        dhour2 = dhour;
        if (dhour == 0) {
            dhour2 = 12;
        } else if (dhour == 12) {
            ampm = 'P';
        } else if (dhour > 12) {
            dhour2 = dhour - 12;
            ampm = 'P';
        }
        if (cmd[3] == 'A') {
            sprintf((char *)p->buffer, "%4s %02d/%02d/%02d %02d:%02d:%02d %cM\015",
                days[dday], dmon, ddate, dyear, dhour2, dmin, dsec, ampm);
            p->length = (unsigned int)strlen((char *)p->buffer) - 1;
        } else if (cmd[3] == 'D') {
            p->buffer[0] = dday;
            p->buffer[1] = dyear;
            p->buffer[2] = dmon;
            p->buffer[3] = ddate;
            p->buffer[4] = dhour2;
            p->buffer[5] = dmin;
            p->buffer[6] = dsec;
            p->buffer[7] = (ampm == 'A') ? 0: 1 ;
            p->buffer[8] = 13;
            p->length = 8;
        } else if (cmd[3] == 'B') {
            p->buffer[0] = dday;
            p->buffer[1] = int_to_bcd(dyear);
            p->buffer[2] = int_to_bcd(dmon);
            p->buffer[3] = int_to_bcd(ddate);
            p->buffer[4] = int_to_bcd(dhour2);
            p->buffer[5] = int_to_bcd(dmin);
            p->buffer[6] = int_to_bcd(dsec);
            p->buffer[7] = (ampm == 'A') ? 0: 1 ;
            p->buffer[8] = 13;
            p->length = 8;
        } else {
            goto bad;
        }
        p->bufptr = 0;
        p->readmode = CBMDOS_FAM_READ;
        return CBMDOS_IPE_OK;
    } else if (cmd[2] == 'W') {
        if  (cmd[3] != 'A' || cmd[3] != 'D' || cmd[3] != 'B') {
            goto bad;
        }
        vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);
        return CBMDOS_IPE_OK;
    }

bad:
    vdrive_command_set_error(vdrive, CBMDOS_IPE_INVAL, 0, 0);
    return CBMDOS_IPE_INVAL;
}

static int vdrive_command_getpartinfo(vdrive_t *vdrive, const uint8_t *cmd, int length)
{
    bufferinfo_t *p = &vdrive->buffers[15];
    /* backup current partition information */
    int origpart = vdrive->current_part;
    int part, maxpart;
    int j, s, ret;
    uint8_t buf[256];

    /* can only do this on CMD HDs and FDs */
    if (!vdrive->haspt) {
        goto bad;
    }

    /* check command */
    if (length < 4 || length > 5 || cmd[0] != 'G' || cmd[1] != '-' || cmd[2] != 'P') {
        goto bad;
    }

    /* if the command is only 'G-P<CR>', use current part, otherwise, use what follows the 'P' */
    if (length == 4 && cmd[3] == 13) {
        part = origpart;
    } else if (length == 5 && cmd[4] == 13) {
        part = cmd[3];
    } else {
        goto bad;
    }

    /* asking for part 255 is the same as asking for the selected partition */
    if (part == 255) {
        part = vdrive->selected_part;
    }

    /* CMD HDs can access 254 partitions (255 is system), FD2000/40000 can only do 31 */
    maxpart = VDRIVE_IS_HD(vdrive) ? 254 : 31;
    /* if we access higher on FD series, just return all 0s */

    /* although we loaded the types, sizes, and offsets into the vdrive structure, we don't
       have the names, so we will read the sectors of the part table. */

    /* try to change to system partition */
    if (vdrive_switch(vdrive, 255)) {
        goto bad;
    }

    /* assume the paritions t/s links are correct, we won't follow them */
    j = part << 5;
    s = j >> 8;
    j = j & 255;
    ret = vdrive_read_sector(vdrive, buf, 1, s);
    memset(p->buffer, 0, 30);
    if (ret >= 0) {
        p->buffer[2] = part;
        if (part <= maxpart) {
            /* if the entry exists */
            p->buffer[0] = buf[j + 2]; /* type */
            memcpy((char*)&(p->buffer[3]), &(buf[j + 5]), 16);
            p->buffer[19] = buf[j + 0x15];
            p->buffer[20] = buf[j + 0x16];
            p->buffer[21] = buf[j + 0x17];
            p->buffer[27] = buf[j + 0x1d];
            p->buffer[28] = buf[j + 0x1e];
            p->buffer[29] = buf[j + 0x1f];
        }
    } else {
        if (part == 1) {
            /* image without partition table; lets make a single NP entry */
            strcpy((char*)&(p->buffer[3]), "NO PARTITIONS");
            p->buffer[2] = 1; /* only 1 partition */
            p->buffer[0] = 1;
            /* compute # of 512 sectors based on # of tracks */
            j = vdrive->image->tracks << 7;
            p->buffer[27] = (j >> 16) & 255;
            p->buffer[28] = (j >> 8) & 255;
            p->buffer[29] = j & 255;
        }
    }
    if (VDRIVE_IS_FD(vdrive)) {
        p->buffer[1] = 0x80 | 0x40 | 0x20;
        if (vdrive->image->type == DISK_IMAGE_TYPE_D1M) {
            p->buffer[1] |= 1;
        }
        if (vdrive->image->type == DISK_IMAGE_TYPE_D2M) {
            p->buffer[1] |= 2;
        }
        if (vdrive->image->type == DISK_IMAGE_TYPE_D4M) {
            p->buffer[1] |= 4;
        }
    }
    p->buffer[30] = 13;
    p->length = 30;
    p->bufptr = 0;
    p->readmode = CBMDOS_FAM_READ;

    return CBMDOS_IPE_OK;

bad:
    vdrive_command_set_error(vdrive, CBMDOS_IPE_INVAL, 0, 0);
    return CBMDOS_IPE_INVAL;

}
/*
* delete partitions: D-P(# parts)(list)($FF)($0d)
*/

static int vdrive_command_deletepart(vdrive_t *vdrive, const uint8_t *cmd, int length)
{
    int maxpart;
    int i;

    /* can only do this on CMD FDs */
    if (!VDRIVE_IS_FD(vdrive)) {
        goto bad;
    }

    /* check command */
    if (length < 7 || cmd[0] != 'D' || cmd[1] != '-' || cmd[2] != 'P') {
        goto bad;
    }

    /* the "# of parts" provided should line up with the command length */
    if (cmd[3] + 6 != length) {
        goto bad;
    }

    /* CMD HDs can access 254 partitions (255 is system), FD2000/40000 can only do 31 */
    maxpart = VDRIVE_IS_HD(vdrive) ? 254 : 31;

    /* check to see if passed list has 0 or 255 in it, or it is above device maximum */
    for (i = 0; i < cmd[3]; i++) {
        if (cmd[4 + i] == 0 || cmd[4 + i] == 255 || cmd[4 + i] > maxpart) {
            goto bad;
        }
    }

    /* all is good, clear the parition types */
    for (i = 0; i < cmd[3]; i++) {
        vdrive->ptype[cmd[4 + i]] = 0;
    }

    /* call routine to pack the partitions to the beginning of the disk */
    if (vdrive_pack_parts(vdrive)) {
        goto bad;
    }

    return vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);

bad:
    return vdrive_command_set_error(vdrive, CBMDOS_IPE_INVAL, 0, 0);
}
