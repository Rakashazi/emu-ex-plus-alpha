/*
 * vdrive-iec.c - Virtual disk-drive IEC implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on old code by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
 *  Jouko Valta <jopi@stekt.oulu.fi>
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  pottendo <pottendo@gmx.net>
 *
 * Patches by
 *  Dan Miner <dminer@nyx10.cs.du.edu>
 *  Germano Caronni <caronni@tik.ethz.ch>
 *  Daniel Fandrich <dan@fch.wimsey.bc.ca>
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

#include "cbmdos.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "machine-bus.h"
#include "types.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive-rel.h"
#include "vdrive.h"


static log_t vdrive_iec_log = LOG_ERR;

#define OFFSET_SUPER_POINTER 3
#define SIDE_SECTORS_MAX 6
#define OFFSET_RECORD_LEN  3

void vdrive_iec_init(void)
{
    vdrive_iec_log = log_open("VDriveIEC");
}

/* ------------------------------------------------------------------------- */

static int iec_open_read_sequential(vdrive_t *vdrive, unsigned int secondary, unsigned int track, unsigned int sector)
{
    int status;
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    vdrive_alloc_buffer(p, BUFFER_SEQUENTIAL);
    p->bufptr = 2;

    status = vdrive_read_sector(vdrive, p->buffer, track, sector);
    p->length = p->buffer[0] ? 0 : p->buffer[1];

    vdrive_set_last_read(track, sector, p->buffer);

    if (status != 0) {
        vdrive_iec_close(vdrive, secondary);
        return SERIAL_ERROR;
    }
    return SERIAL_OK;
}

static int iec_open_read(vdrive_t *vdrive, unsigned int secondary)
{
    int type;
    unsigned int track, sector;
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    BYTE *slot = p->slot;

    if (!slot) {
        vdrive_iec_close(vdrive, secondary);
        vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_FOUND, 0, 0);
        return SERIAL_ERROR;
    }

    type = slot[SLOT_TYPE_OFFSET] & 0x07;
    track = (unsigned int)slot[SLOT_FIRST_TRACK];
    sector = (unsigned int)slot[SLOT_FIRST_SECTOR];

    /* Del, Seq, Prg, Usr (Rel not supported here).  */
    if (type != CBMDOS_FT_REL) {
        return iec_open_read_sequential(vdrive, secondary, track, sector);
    }

    return SERIAL_ERROR;
}

static int iec_open_read_directory(vdrive_t *vdrive, unsigned int secondary,
                                   cbmdos_cmd_parse_t *cmd_parse)
{
    int retlen;
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    if (secondary > 0) {
        return iec_open_read_sequential(vdrive, secondary, vdrive->Header_Track, vdrive->Header_Sector);
    }

    vdrive_alloc_buffer(p, BUFFER_DIRECTORY_READ);

    retlen = vdrive_dir_first_directory(vdrive, cmd_parse->parsecmd, cmd_parse->parselength, CBMDOS_FT_DEL, p);

    p->length = (unsigned int)retlen;
    p->bufptr = 0;

    return SERIAL_OK;
}

static int iec_open_write(vdrive_t *vdrive, unsigned int secondary,
                          cbmdos_cmd_parse_t *cmd_parse, const BYTE *name)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    unsigned int track, sector;
    BYTE *slot = p->slot, *e;

    if (vdrive->image->read_only || VDRIVE_IMAGE_FORMAT_4000_TEST) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_WRITE_PROTECT_ON, 0, 0);
        return SERIAL_ERROR;
    }

    /* set flag for overwrite mode */
    p->needsupdate = 0;

    if (slot) {
        /* file exists */
        if (*name == '@') {
            /* replace mode: we don't want the dirent updated at all until
                close */
            /* allocate buffers */
            vdrive_alloc_buffer(p, BUFFER_SEQUENTIAL);
            p->bufptr = 2;

            /* Create our own slot, since the one passed is static */
            p->slot = lib_calloc(1, 32);

            /* Copy the static on to the new one. */
            memcpy(p->slot, slot, 32);
            slot = p->slot;

            /* set flag for replace mode */
            p->needsupdate = 1;

            /* find a new track and sector when writing */
            p->track = p->sector = 0;
        } else {
            if (p->readmode == CBMDOS_FAM_APPEND) {
                /* append mode */
                /* allocate buffers */
                vdrive_alloc_buffer(p, BUFFER_SEQUENTIAL);

                /* Create our own slot, since the one passed is static */
                p->slot = lib_calloc(1, 32);

                /* Copy the static on to the new one. */
                memcpy(p->slot, slot, 32);
                slot = p->slot;

                /* set file unclosed */
                p->slot[SLOT_TYPE_OFFSET] &= 0x7f;

                /* get the starting track and sector */
                p->track = track = slot[SLOT_FIRST_TRACK];
                p->sector = sector = slot[SLOT_FIRST_SECTOR];

                /* update block count as we find the end of the file */
                /* the real drives actually don't do this, so each time you
                    append to a file, the block count increases by 1.  I think it
                    is safer to correct the block count. */
                slot[SLOT_NR_BLOCKS] = 255;
                slot[SLOT_NR_BLOCKS + 1] = 255;

                /* scan to the end of the file */
                while (track) {
                    p->track = track;
                    p->sector = sector;
                    if (vdrive_read_sector(vdrive, p->buffer, p->track, p->sector)) {
                        /* couldn't read sector, report error and leave */
                        vdrive_free_buffer(p);
                        vdrive_command_set_error(vdrive, CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR, p->track, p->sector);
                        return SERIAL_ERROR;
                    }
                    /* setup next link */
                    track = p->buffer[0];
                    sector = p->buffer[1];

                    /* Increment block count. */
                    if (!(++slot[SLOT_NR_BLOCKS])) {
                        ++slot[SLOT_NR_BLOCKS + 1];
                    }
                }
                /* compensate if the dir link is 0 (rare possibility) */
                if (!p->track) {
                    /* Our loop didn't even execute once, set the block
                       size to 0 */
                    slot[SLOT_NR_BLOCKS] = 0;
                    slot[SLOT_NR_BLOCKS + 1] = 0;
                    /* set buffer pointer to 2 */
                    sector = 1;
                }
                /* set the buffer pointer */
                p->bufptr = sector + 1;
            } else {
                /* can't overwrite an existing file */
                vdrive_iec_close(vdrive, secondary);
                vdrive_command_set_error(vdrive, CBMDOS_IPE_FILE_EXISTS, 0, 0);
                return SERIAL_ERROR;
            }
        }
    } else {
        /* new file... */
        /* create a slot based on the opening name */
        vdrive_dir_create_slot(p, cmd_parse->parsecmd, cmd_parse->parselength,
                               cmd_parse->filetype);

        /* Write the directory entry to disk as an UNCLOSED file. */

        vdrive_dir_find_first_slot(vdrive, NULL, -1, 0, &p->dir);
        e = vdrive_dir_find_next_slot(&p->dir);

        /* If there is not space for the slot, disk is full */
        if (!e) {
            vdrive_free_buffer(p);
            vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
            return SERIAL_ERROR;
        }

        /* find a new track and sector when writing */
        p->track = p->sector = 0;
    }

    if (!p->needsupdate) {
        /* copy the slot information into the sector. */
        memcpy(&p->dir.buffer[p->dir.slot * 32 + 2],
               p->slot + 2, 30);

        /* Write the sector. */
        vdrive_write_sector(vdrive, p->dir.buffer, p->dir.track, p->dir.sector);
    }

    return SERIAL_OK;
}


/*
 * Open a file on the disk image, and store the information on the
 * directory slot.
 */

int vdrive_iec_open(vdrive_t *vdrive, const BYTE *name, unsigned int length,
                    unsigned int secondary, cbmdos_cmd_parse_t *cmd_parse_ext)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    BYTE *slot; /* Current directory entry */
    int rc, status = SERIAL_OK;
    /* FIXME: This should probably be set to zero */
    cbmdos_cmd_parse_t cmd_parse_stat;
    cbmdos_cmd_parse_t *cmd_parse;
    BYTE name_stat[17];
    unsigned int opentype;

    if (cmd_parse_ext != NULL) {
        cmd_parse = cmd_parse_ext;
        memset(name_stat, 0, sizeof(name_stat));
        strncpy((char *)name_stat, cmd_parse->parsecmd, sizeof(name_stat) - 1);
        name = name_stat;
        length = (unsigned int)strlen((char *)name);
        secondary = cmd_parse->secondary;
    } else {
        cmd_parse = &cmd_parse_stat;
    }

    if (cmd_parse_ext == NULL
        && (!name || !*name) && p->mode != BUFFER_COMMAND_CHANNEL) {
        return SERIAL_NO_DEVICE;
    }

    /* No floppy in drive?   */
    if (vdrive->image == NULL
        && p->mode != BUFFER_COMMAND_CHANNEL
        && secondary != 15
        && *name != '#') {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_READY, 18, 0);
        log_message(vdrive_iec_log, "Drive not ready.");
        return SERIAL_ERROR;
    }

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE#%i: OPEN: Name '%s' (%d) on ch %d.",
              vdrive->unit, name, length, secondary);
#endif

    /*
     * If channel is command channel, name will be used as write. Return only
     * status of last write ...
     */
    if (p->mode == BUFFER_COMMAND_CHANNEL) {
        unsigned int n;

        for (n = 0; n < length; n++) {
            status = vdrive_iec_write(vdrive, name[n], secondary);
        }

        if (length) {
            p->readmode = CBMDOS_FAM_WRITE;
        } else {
            p->readmode = CBMDOS_FAM_READ;
        }
        return status;
    }

    /*
     * Clear error flag
     */
    vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);

    /*
     * In use ?
     */
    if (p->mode != BUFFER_NOT_IN_USE) {
#ifdef DEBUG_DRIVE
        log_debug("Cannot open channel %d. Mode is %d.", secondary, p->mode);
#endif
        vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_CHANNEL, 0, 0);
        return SERIAL_ERROR;
    }

    if (cmd_parse_ext == NULL) {
        cmd_parse->cmd = name;
        cmd_parse->cmdlength = length;
        cmd_parse->secondary = secondary;
        /* make sure this is zero, since it isn't set below */
        cmd_parse->recordlength = 0;
        cmd_parse->drive = -1;

        rc = cbmdos_command_parse(cmd_parse);

        if (rc != CBMDOS_IPE_OK) {
            status = SERIAL_ERROR;
            goto out;
        }
#ifdef DEBUG_DRIVE
        log_debug("Raw file name: `%s', length: %i.", name, length);
        log_debug("Parsed file name: `%s', reallength: %i. drive: %i",
                  cmd_parse->parsecmd, cmd_parse->parselength, cmd_parse->drive);
#endif
        if (cmd_parse->drive != -1) {
            /* a drive number was specified in the filename */
            if ((vdrive->image_format == VDRIVE_IMAGE_FORMAT_8050) ||
                (vdrive->image_format == VDRIVE_IMAGE_FORMAT_8250) ||
                (vdrive->image_format == VDRIVE_IMAGE_FORMAT_2040)) {
                /* FIXME: dual disk drives not supported */
                if (cmd_parse->drive == 0) {
                    /* FIXME: use drive 0 */
                } else if (cmd_parse->drive == 1) {
                    /* FIXME: use drive 1 */
                    /*
                        since some software gets confused when it sees the same disk in
                        both drives, we bail out with an error instead.
                    */
                    log_warning(LOG_DEFAULT, "second drive of dual disk drive is not supported");
                    vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_READY, 18, 0);
                    status = SERIAL_ERROR;
                    goto out;
                } else {
                    /* FIXME: what exactly does the drive do if drivenumber is > 1, look
                              up the file on both drives perhaps ?
                    */
                }
            } else {
                /* single disk drives seem to ignore the drive number, *except* if it
                   is 1, which will result in an error. */
                if (cmd_parse->drive == 1) {
                    vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_READY, 18, 0);
                    status = SERIAL_ERROR;
                    goto out;
                }
            }
        }
    }

    /* Limit file name to 16 chars.  */
    if (cmd_parse->parselength > 16) {
        cmd_parse->parselength = 16;
    }

    /*
     * Internal buffer ?
     */
    if (*name == '#') {
        vdrive_alloc_buffer(p, BUFFER_MEMORY_BUFFER);

        /* the pointer is actually 1 on the real drives. */
        /* this probably relates to the B-R and B-W commands. */
        /* 1541 firmware: $cb84 - open channel, $cc0f bp = 1 */
        p->bufptr = 1;
        /* we need a length to support the original B-R and B-W
           commands. */
        p->length = 256;
        status = SERIAL_OK;
        goto out;
    }

    /* Clear update flag */
    p->needsupdate = 0;

    /*
     * Directory read
     * A little-known feature of the 1541: open 1,8,2,"$" (or even 1,8,1).
     * It gives you the BAM+DIR as a sequential file, containing the data
     * just as it appears on disk.  -Olaf Seibert
     */

    if (*name == '$') {
        p->readmode = CBMDOS_FAM_READ;
        status = iec_open_read_directory(vdrive, secondary, cmd_parse);
        goto out;
    }

    /*
     * Check that there is room on directory.
     */
    if (cmd_parse->readmode == CBMDOS_FAM_READ || cmd_parse->readmode == CBMDOS_FAM_APPEND) {
        opentype = cmd_parse->filetype;
    } else {
        opentype = CBMDOS_FT_DEL;
    }

    vdrive_dir_find_first_slot(vdrive, cmd_parse->parsecmd, cmd_parse->parselength, opentype, &p->dir);

    /*
     * Find the first non-DEL entry in the directory (if it exists).
     */
    do {
        slot = vdrive_dir_find_next_slot(&p->dir);
    } while (slot && ((slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_DEL));

    p->readmode = cmd_parse->readmode;
    p->slot = slot;

    /* Call REL function if we are creating OR opening one */
    if (cmd_parse->filetype == CBMDOS_FT_REL ||
        ( slot && (slot[SLOT_TYPE_OFFSET] & 0x07) == CBMDOS_FT_REL)) {
        /* Make sure the record length of the opening command is the same as
           the record length in the directory slot, if not DOS ERROR 50 */
        if (slot && cmd_parse->recordlength > 0 &&
            slot[SLOT_RECORD_LENGTH] != cmd_parse->recordlength) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NO_RECORD, 0, 0);
            status = SERIAL_ERROR;
            goto out;
        }
        /* At this point the record lengths are the same (or will be), so set
            them equal. */
        if (slot) {
            cmd_parse->recordlength = slot[SLOT_RECORD_LENGTH];
        }
        status = vdrive_rel_open(vdrive, secondary, cmd_parse, name);
        goto out;
    }

    if (cmd_parse->readmode == CBMDOS_FAM_READ) {
        status = iec_open_read(vdrive, secondary);
    } else {
        status = iec_open_write(vdrive, secondary, cmd_parse, name);
    }

out:
    lib_free(cmd_parse->parsecmd);
    return status;
}

/* ------------------------------------------------------------------------- */

static int iec_write_sequential(vdrive_t *vdrive, bufferinfo_t *bi, int length)
{
    unsigned int t_new, s_new;
    int retval;
    BYTE *buf = bi->buffer;
    BYTE *slot = bi->slot;

    /*
     * First block of a file ?
     */
    if (bi->track == 0) {
        /* allocate the first sector */
        retval = vdrive_bam_alloc_first_free_sector(vdrive, &t_new, &s_new);
        if (retval < 0) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
            return -1;
        }
        /* remember track and sector */
        bi->track = t_new;
        bi->sector = s_new;
        /* use update flag to indicate replace mode */
        if (bi->needsupdate) {
            /* save and replace */
            slot[SLOT_REPLACE_TRACK] = t_new;
            slot[SLOT_REPLACE_SECTOR] = s_new;
        } else {
            /* new file */
            slot[SLOT_FIRST_TRACK] = t_new;
            slot[SLOT_FIRST_SECTOR] = s_new;
        }
        /* reset block counter */
        slot[SLOT_NR_BLOCKS] = 0;
        slot[SLOT_NR_BLOCKS + 1] = 0;
    }

    if (length == WRITE_BLOCK) {
        /*
         * Write current sector and allocate next
         */
        t_new = bi->track;
        s_new = bi->sector;
        retval = vdrive_bam_alloc_next_free_sector(vdrive, &t_new, &s_new);
        if (retval < 0) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_DISK_FULL, 0, 0);
            return -1;
        }
        buf[0] = t_new;
        buf[1] = s_new;

        vdrive_write_sector(vdrive, buf, bi->track, bi->sector);

        bi->track = t_new;
        bi->sector = s_new;
    } else {
        /*
         * Write last block
         */
        buf[0] = 0;
        buf[1] = length - 1;

        vdrive_write_sector(vdrive, buf, bi->track, bi->sector);
    }

    /* Increment block count. */
    if (!(++slot[SLOT_NR_BLOCKS])) {
        ++slot[SLOT_NR_BLOCKS + 1];
    }

    return 0;
}

static int iec_close_sequential(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    unsigned int track = 0, sector = 0;

    if (p->readmode & (CBMDOS_FAM_WRITE | CBMDOS_FAM_APPEND)) {
        /*
         * Flush bytes and write slot to directory
         */

        if (vdrive->image->read_only || VDRIVE_IMAGE_FORMAT_4000_TEST) {
            vdrive_command_set_error(vdrive, CBMDOS_IPE_WRITE_PROTECT_ON, 0, 0);
            return SERIAL_ERROR;
        }

#ifdef DEBUG_DRIVE
        log_debug("DEBUG: flush.");
#endif
        /* Flush remained of file */
        iec_write_sequential(vdrive, p, p->bufptr);

        /* Set the file as closed */
        p->slot[SLOT_TYPE_OFFSET] |= 0x80; /* Closed */

        /* is this a save and replace? */
        if (p->needsupdate) {
            /* remember the original track and sector */
            track = p->slot[SLOT_FIRST_TRACK];
            sector = p->slot[SLOT_FIRST_SECTOR];
            /* move over the replacement track and sector */
            p->slot[SLOT_FIRST_TRACK] = p->slot[SLOT_REPLACE_TRACK];
            p->slot[SLOT_FIRST_SECTOR] = p->slot[SLOT_REPLACE_SECTOR];
            /* set replacement track and sector to 0 */
            p->slot[SLOT_REPLACE_TRACK] = 0;
            p->slot[SLOT_REPLACE_SECTOR] = 0;
        }

        /* Update the directory entry (block count, closed) */
        vdrive_iec_update_dirent(vdrive, secondary);

        /* if we have a track and sector saved */
        if (track) {
            /* remove the original file */
            vdrive_dir_free_chain(vdrive, track, sector);
        }

        /* Update BAM */
        vdrive_bam_write_bam(vdrive);

        /* Free up the slot */
        lib_free(p->slot);
    }
    /* Release buffers */
    vdrive_free_buffer(p);

    return SERIAL_OK;
}

int vdrive_iec_close(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    int status = SERIAL_OK;

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            return SERIAL_OK; /* FIXME: Is this correct? */

        case BUFFER_MEMORY_BUFFER:
        case BUFFER_DIRECTORY_READ:
            vdrive_free_buffer(p);
            p->slot = NULL;
            break;
        case BUFFER_SEQUENTIAL:
            status = iec_close_sequential(vdrive, secondary);
            break;
        case BUFFER_RELATIVE:
            status = vdrive_rel_close(vdrive, secondary);
            break;
        case BUFFER_COMMAND_CHANNEL:
            /* I'm not sure if this is correct, but really closing the buffer
               should reset the read pointer to the beginning for the next
               write! */
            vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);
            /* this breaks any rel file access if the command channel is closed
                when the record is changed. Removed for now.*/
/*        vdrive_close_all_channels(vdrive); */
            break;
        default:
            log_error(vdrive_iec_log, "Fatal: unknown floppy-close-mode: %i.", p->mode);
    }

    return status;
}

/* ------------------------------------------------------------------------- */

static int iec_read_sequential(vdrive_t *vdrive, BYTE *data,
                               unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    int status;
    unsigned int track, sector;

    if (p->readmode != CBMDOS_FAM_READ) {
        *data = 0xc7;
        return SERIAL_ERROR;
    }

    *data = p->buffer[p->bufptr];
    if (p->length != 0) {
        if (p->bufptr == p->length) {
            p->bufptr = 0xff;
        }
    }
    p->bufptr = (p->bufptr + 1) & 0xff;
    if (p->bufptr) {
        return SERIAL_OK;
    }
    if (p->length) {
        p->readmode = CBMDOS_FAM_EOF;
        return SERIAL_EOF;
    }

    switch (p->mode) {
        case BUFFER_SEQUENTIAL:

            track = (unsigned int)p->buffer[0];
            sector = (unsigned int)p->buffer[1];

            status = vdrive_read_sector(vdrive, p->buffer, track, sector);
            p->length = p->buffer[0] ? 0 : p->buffer[1];
            vdrive_set_last_read(track, sector, p->buffer);

            if (status == 0) {
                p->bufptr = 2;
            } else {
                p->readmode = CBMDOS_FAM_EOF;
            }
            break;
        case BUFFER_DIRECTORY_READ:
            p->length = vdrive_dir_next_directory(vdrive, p);
            p->bufptr = 0;
            break;
    }
    return SERIAL_OK;
}

int vdrive_iec_read(vdrive_t *vdrive, BYTE *data, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);
    int status = SERIAL_OK;

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_OPEN, 0, 0);
            return SERIAL_ERROR;

        case BUFFER_MEMORY_BUFFER:
            *data = p->buffer[p->bufptr];
            p->bufptr++;
            if (p->bufptr >= p->length) {
                /* Buffer pointer resets to 1, not 0. */
                p->bufptr = 1;
                status = SERIAL_EOF;
            }
            break;

        case BUFFER_DIRECTORY_READ:
        case BUFFER_SEQUENTIAL:
            status = iec_read_sequential(vdrive, data, secondary);
            break;

        case BUFFER_COMMAND_CHANNEL:
            if (p->bufptr > p->length) {
                vdrive_command_set_error(vdrive, CBMDOS_IPE_OK, 0, 0);
#if 0
#ifdef DEBUG_DRIVE
                log_debug("End of buffer in command channel.");
#endif
                *data = 0xc7;
#ifdef DEBUG_DRIVE
                if (p->mode == BUFFER_COMMAND_CHANNEL) {
                    log_debug("Disk read  %d [%02d %02d] data %02x (%c).",
                              p->mode, 0, 0, *data, (isprint(*data) ? *data : '.'));
                }
#endif
                return SERIAL_EOF;
#endif
            }
            *data = p->buffer[p->bufptr];
            p->bufptr++;
            if (p->bufptr > p->length) {
                status = SERIAL_EOF;
            }
            break;

        case BUFFER_RELATIVE:
            status = vdrive_rel_read(vdrive, data, secondary);
            break;

        default:
            log_error(vdrive_iec_log, "Fatal: unknown buffermode on floppy-read.");
    }

#ifdef DEBUG_DRIVE
    if (p->mode == BUFFER_COMMAND_CHANNEL) {
        log_debug("Disk read  %d [%02d %02d] data %02x (%c).",
                  p->mode, 0, 0, *data, (isprint(*data) ? *data : '.'));
    }
#endif
    return status;
}

/* ------------------------------------------------------------------------- */

int vdrive_iec_write(vdrive_t *vdrive, BYTE data, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    if ((vdrive->image->read_only || VDRIVE_IMAGE_FORMAT_4000_TEST) && p->mode != BUFFER_COMMAND_CHANNEL) {
        vdrive_command_set_error(vdrive, CBMDOS_IPE_WRITE_PROTECT_ON, 0, 0);
        return SERIAL_ERROR;
    }

#ifdef DEBUG_DRIVE
    if (p->mode == BUFFER_COMMAND_CHANNEL) {
        log_debug("Disk write %d [%02d %02d] data %02x (%c).",
                  p->mode, 0, 0, data, (isprint(data) ? data : '.'));
    }
#endif

    switch (p->mode) {
        case BUFFER_NOT_IN_USE:
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_OPEN, 0, 0);
            return SERIAL_ERROR;
        case BUFFER_DIRECTORY_READ:
            vdrive_command_set_error(vdrive, CBMDOS_IPE_NOT_WRITE, 0, 0);
            return SERIAL_ERROR;
        case BUFFER_MEMORY_BUFFER:
            p->buffer[p->bufptr] = data;
            p->bufptr++;
            if (p->bufptr >= p->length) {
                /* On writes, buffer pointer resets to 0. */
                p->bufptr = 0;
            }
            return SERIAL_OK;
        case BUFFER_SEQUENTIAL:
            if (p->readmode == CBMDOS_FAM_READ) {
                return SERIAL_ERROR;
            }

            if (p->bufptr >= 256) {
                p->bufptr = 2;
                if (iec_write_sequential(vdrive, p, WRITE_BLOCK) < 0) {
                    return SERIAL_ERROR;
                }
            }
            p->buffer[p->bufptr] = data;
            p->bufptr++;
            break;
        case BUFFER_COMMAND_CHANNEL:
            if (p->readmode == CBMDOS_FAM_READ) {
                p->bufptr = 0;
                p->readmode = CBMDOS_FAM_WRITE;
            }
            if (p->bufptr >= 256) { /* Limits checked later */
                return SERIAL_ERROR;
            }
            p->buffer[p->bufptr] = data;
            p->bufptr++;
            break;
        case BUFFER_RELATIVE:
            return vdrive_rel_write(vdrive, data, secondary);
            break;
        default:
            log_error(vdrive_iec_log, "Fatal: Unknown write mode.");
            exit(-1);
    }
    return SERIAL_OK;
}

/* ------------------------------------------------------------------------- */

void vdrive_iec_flush(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

#ifdef DEBUG_DRIVE
    log_debug("FLUSH:, secondary = %d, buffer=%s\n "
              "  bufptr=%d, length=%d, read?=%d.", secondary, p->buffer,
              p->bufptr, p->length, p->readmode == CBMDOS_FAM_READ);
#endif

    if (p->mode != BUFFER_COMMAND_CHANNEL) {
        return;
    }

#ifdef DEBUG_DRIVE
    log_debug("FLUSH: COMMAND CHANNEL");
#endif

    if (p->readmode == CBMDOS_FAM_READ) {
        return;
    }

#ifdef DEBUG_DRIVE
    log_debug("FLUSH: READ MODE");
#endif

    if (p->length) {
        /* If no command, do nothing - keep error code.  */
        vdrive_command_execute(vdrive, p->buffer, p->bufptr);
        p->bufptr = 0;
    }
}

/* ------------------------------------------------------------------------- */

int vdrive_iec_attach(unsigned int unit, const char *name)
{
    return machine_bus_device_attach(unit, name, vdrive_iec_read,
                                     vdrive_iec_write, vdrive_iec_open,
                                     vdrive_iec_close, vdrive_iec_flush,
                                     vdrive_iec_listen);
}

void vdrive_iec_listen(vdrive_t *vdrive, unsigned int secondary)
{
    bufferinfo_t *p = &(vdrive->buffers[secondary]);

    /* Only move to next record if the sector is dirty (indicates
        we just wrote something) and if this is a REL file. */
    /* All "overflows" are handled in the write routine. */
    if (p->mode == BUFFER_RELATIVE) {
        vdrive_rel_listen(vdrive, secondary);
    }

    return;
}

int vdrive_iec_update_dirent(vdrive_t *vdrive, unsigned int channel)
{
    bufferinfo_t *p = &(vdrive->buffers[channel]);

    /* Read in the track/sector where the directory entry lies. */
    vdrive_read_sector(vdrive, p->dir.buffer, p->dir.track, p->dir.sector);

    /* Copy over our new slot. */
    memcpy(&(p->dir.buffer[p->dir.slot * 32 + 2]), p->slot + 2, 30);

    /* Write it back. */
    vdrive_write_sector(vdrive, p->dir.buffer, p->dir.track, p->dir.sector);

    return 0;
}
