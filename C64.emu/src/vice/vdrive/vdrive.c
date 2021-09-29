/*
 * vdrive.c - Virtual disk-drive implementation.
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

#ifdef DEBUG_DRIVE
#define DBG(x) log_debug x
#else
#define DBG(x)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "archdep.h"
#include "attach.h"
#include "cbmdos.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "fsdevice.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive-internal.h"
#include "vdrive-rel.h"
#include "vdrive-snapshot.h"
#include "vdrive.h"

static log_t vdrive_log = LOG_ERR;

void vdrive_init(void)
{
    vdrive_log = log_open("VDrive");

    vdrive_command_init();
    vdrive_dir_init();
    vdrive_iec_init();
    vdrive_internal_init();
    vdrive_rel_init();
    vdrive_snapshot_init();
}

/* ------------------------------------------------------------------------- */
/*
    allocate/free buffers. these functions should somewhat mimic the behaviour
    of a real drive to increase compatibility with some hacks a bit. see
    testprogs/vdrive/disk/dircheck.bas

    FIXME: find out the *exact* behaviour of the various drives and implement
           this function accordingly.
           - did this for 41/71/81/1001/9090 but not REL (rmusced)
           - data sector should be allocated before directory entry is made
           -  or else dir slots will have T/S of 0/0 for new files on full disks
    FIXME: REL files do not use this logic yet (vdrive-rel.c)
    FIXME: ideally this logic should allocate memory from a common drive memory
           array, which then can be used properly for M-R too
*/
void vdrive_alloc_buffer(bufferinfo_t *p, int mode)
{
    size_t size = 256;

    if (p->buffer == NULL) {
        /* first time actually allocate memory, and clear it */
        p->buffer = lib_malloc(size);
        memset(p->buffer, 0, size);
    } else {
        /* any other time, just adjust the size of the buffer */
        p->buffer = lib_realloc(p->buffer, size);
    }
    p->mode = mode;
}

void vdrive_free_buffer(bufferinfo_t *p)
{
    p->mode = BUFFER_NOT_IN_USE;
/*
    do NOT actually free here. once allocated, buffers should get reused and
    their content stay untouched. vdrive_device_shutdown will free the buffers
    at shutdown time.

    lib_free((char *)p->buffer);
    p->buffer = NULL;
*/
}

/* ------------------------------------------------------------------------- */

int vdrive_device_setup(vdrive_t *vdrive, unsigned int unit, unsigned int drive)
{
    unsigned int i;

    /* Check if the vdrive_t teleports to another unit. Should never happen. */
    if (vdrive->unit && vdrive->unit != unit) {
        log_error(vdrive_log, "**** vdrive_device_setup: vdrive changes from %u:%u to %u:%u!",
                vdrive->unit, vdrive->drive, unit, drive);
    }

    vdrive->unit = unit;
    vdrive->drive = drive;

    /* init buffers */
    for (i = 0; i < 15; i++) {
        vdrive->buffers[i].mode = BUFFER_NOT_IN_USE;
        lib_free(vdrive->buffers[i].buffer);
        vdrive->buffers[i].buffer = NULL;
    }

    /* init command channel */
    vdrive_alloc_buffer(&(vdrive->buffers[15]), BUFFER_COMMAND_CHANNEL);
    vdrive_command_set_error(vdrive, CBMDOS_IPE_DOS_VERSION, 0, 0);

    vdrive->d90toggle = 0;
    vdrive->last_code = CBMDOS_IPE_OK;

    return 0;
}

void vdrive_device_shutdown(vdrive_t *vdrive)
{
    unsigned int i;
    bufferinfo_t *p;

    if (vdrive != NULL) {
        /* de-init buffers */
        for (i = 0; i < 16; i++) {
            p = &(vdrive->buffers[i]);
            vdrive_free_buffer(p);
            lib_free(p->buffer);
        }
    }
}

/* ------------------------------------------------------------------------- */

/*
 * Close all channels. This happens on 'I' -command and on command-
 * channel close.
 */

void vdrive_close_all_channels(vdrive_t *vdrive)
{
    unsigned int i;
    bufferinfo_t *p;

    for (i = 0; i <= 15; i++) {
        p = &(vdrive->buffers[i]);
        if (p->mode != BUFFER_NOT_IN_USE && p->mode != BUFFER_COMMAND_CHANNEL) {
            vdrive_iec_close(vdrive, i);
        }
    }
}

/* ------------------------------------------------------------------------- */

/*
    get number of sectors for given track
 */
int vdrive_get_max_sectors(vdrive_t *vdrive, unsigned int track)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            return disk_image_sector_per_track(DISK_IMAGE_TYPE_D64, track);
        case VDRIVE_IMAGE_FORMAT_2040:
            return disk_image_sector_per_track(DISK_IMAGE_TYPE_D67, track);
        case VDRIVE_IMAGE_FORMAT_1571:
            return disk_image_sector_per_track(DISK_IMAGE_TYPE_D71, track);
        case VDRIVE_IMAGE_FORMAT_8050:
            return disk_image_sector_per_track(DISK_IMAGE_TYPE_D80, track);
        case VDRIVE_IMAGE_FORMAT_1581:
            return 40;
        case VDRIVE_IMAGE_FORMAT_8250:
            if (track <= NUM_TRACKS_8250 / 2) {
                return disk_image_sector_per_track(DISK_IMAGE_TYPE_D80, track);
            } else {
                return disk_image_sector_per_track(DISK_IMAGE_TYPE_D80, track
                                                   - (NUM_TRACKS_8250 / 2));
            }
        case VDRIVE_IMAGE_FORMAT_4000:
            return 256;
        case VDRIVE_IMAGE_FORMAT_9000:
            return vdrive->image->sectors;
        default:
            log_message(vdrive_log,
                        "Unknown disk type %u.  Cannot calculate max sectors",
                        vdrive->image_format);
    }
    return -1;
}

/*
    get number of sectors per head for given track (for D9090/60)
 */
int vdrive_get_max_sectors_per_head(vdrive_t *vdrive, unsigned int track)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_9000:
            return 32;
        default:
            return vdrive_get_max_sectors(vdrive, track);
    }
}

/* ------------------------------------------------------------------------- */

/*
 * Functions to attach the disk image files.
 */

void vdrive_detach_image(disk_image_t *image, unsigned int unit,
                         unsigned int drive, vdrive_t *vdrive)
{
    if (image == NULL) {
        return;
    }
    /*
     * TODO: This doesn't really need the parameters unit and drive,
     * since they are remembered in the vdrive. For now, check the consistency.
     */
    if (vdrive->unit != unit || vdrive->drive != drive) {
        log_error(vdrive_log, "**** vdrive_detach_image: vdrive->unit %u:%u != unit %u:%u",
                vdrive->unit, vdrive->drive, unit, drive);
    }

    disk_image_detach_log(image, vdrive_log, unit, drive);
    vdrive_close_all_channels(vdrive);
    lib_free(vdrive->bam);
    vdrive->bam = NULL;
    vdrive->image = NULL;
}

int vdrive_attach_image(disk_image_t *image, unsigned int unit,
                        unsigned int drive, vdrive_t *vdrive)
{
    /*
     * TODO: This doesn't really need the parameters unit and drive,
     * since they are remembered in the vdrive.
     */
    if (vdrive->unit != unit || vdrive->drive != drive) {
        log_error(vdrive_log, "**** vdrive_attach_image: vdrive->unit %u:%u != unit %u:%u",
                vdrive->unit, vdrive->drive, unit, drive);
    }
    vdrive->unit = unit;
    vdrive->drive = drive;

    disk_image_attach_log(image, vdrive_log, unit, drive);

    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            break;
        case DISK_IMAGE_TYPE_D67:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_2040;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            break;
        case DISK_IMAGE_TYPE_D71:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x200;
            break;
        case DISK_IMAGE_TYPE_D81:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1581;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x300;
            break;
        case DISK_IMAGE_TYPE_D80:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_8050;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x500;
            break;
        case DISK_IMAGE_TYPE_D82:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_8250;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x500;
            break;
        case DISK_IMAGE_TYPE_G64:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            break;
        case DISK_IMAGE_TYPE_G71:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x200;
            break;
        case DISK_IMAGE_TYPE_P64:
            /* FIXME: extra checks might be needed for supporting other drives */
            if (image->tracks > 42) {
                vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
                vdrive->num_tracks = image->tracks;
                vdrive->bam_size = 0x200;
            } else {
                vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
                vdrive->num_tracks = image->tracks;
                vdrive->bam_size = 0x100;
            }
            break;
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
            /* FIXME: x64 format can be any drive! */
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            break;
#endif
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_4000;
            vdrive->num_tracks = image->tracks - 1;
            vdrive->bam_size = 0x2100;
            break;
        case DISK_IMAGE_TYPE_D90:
            {
                uint32_t tmp;

                vdrive->image_format = VDRIVE_IMAGE_FORMAT_9000;
                vdrive->num_tracks = image->tracks;
                /* D9090/60 has track 0, include it below */
                tmp = ((image->tracks + 1) * image->sectors * 5);
                vdrive->bam_size = tmp / (32 * 240) + 1;
                tmp = tmp % (32 * 240);
                if (tmp) {
                    vdrive->bam_size++;
                }
                vdrive->bam_size *= 0x100;
                break;
            }
        default:
            return -1;
    }
    DBG(("vdrive_attach_image image type:%u vdrive format:%u num_tracks:%u bam_size:%u",
         image->type, vdrive->image_format, vdrive->num_tracks, vdrive->bam_size));

    /* Need an image associated for D9090/60 and vdrive_set_disk_geometry */
    vdrive->image = image;

    /* Initialise format constants */
    vdrive_set_disk_geometry(vdrive);

    vdrive->bam = lib_malloc(vdrive->bam_size);

    if (vdrive_bam_read_bam(vdrive)) {
        log_error(vdrive_log, "Cannot access BAM.");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

/*
 * Initialise format constants
 */

void vdrive_set_disk_geometry(vdrive_t *vdrive)
{
    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            vdrive->Bam_Track = BAM_TRACK_1541;
            vdrive->Bam_Sector = BAM_SECTOR_1541;
            vdrive->Header_Track = BAM_TRACK_1541;
            vdrive->Header_Sector = BAM_SECTOR_1541;
            vdrive->bam_name = BAM_NAME_1541;
            vdrive->bam_id = BAM_ID_1541;
            vdrive->Dir_Track = DIR_TRACK_1541;
            vdrive->Dir_Sector = DIR_SECTOR_1541;
            break;
        case VDRIVE_IMAGE_FORMAT_2040:
            vdrive->Bam_Track = BAM_TRACK_2040;
            vdrive->Bam_Sector = BAM_SECTOR_2040;
            vdrive->Header_Track = BAM_TRACK_2040;
            vdrive->Header_Sector = BAM_SECTOR_2040;
            vdrive->bam_name = BAM_NAME_2040;
            vdrive->bam_id = BAM_ID_2040;
            vdrive->Dir_Track = DIR_TRACK_2040;
            vdrive->Dir_Sector = DIR_SECTOR_2040;
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            vdrive->Bam_Track = BAM_TRACK_1571;
            vdrive->Bam_Sector = BAM_SECTOR_1571;
            vdrive->Header_Track = BAM_TRACK_1571;
            vdrive->Header_Sector = BAM_SECTOR_1571;
            vdrive->bam_name = BAM_NAME_1571;
            vdrive->bam_id = BAM_ID_1571;
            vdrive->Dir_Track = DIR_TRACK_1571;
            vdrive->Dir_Sector = DIR_SECTOR_1571;
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            vdrive->Bam_Track = BAM_TRACK_1581;
            vdrive->Bam_Sector = BAM_SECTOR_1581;
            vdrive->Header_Track = BAM_TRACK_1581;
            vdrive->Header_Sector = BAM_SECTOR_1581;
            vdrive->bam_name = BAM_NAME_1581;
            vdrive->bam_id = BAM_ID_1581;
            vdrive->Dir_Track = DIR_TRACK_1581;
            vdrive->Dir_Sector = DIR_SECTOR_1581;
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
            vdrive->Bam_Track = BAM_TRACK_8050;
            vdrive->Bam_Sector = BAM_SECTOR_8050;
            vdrive->Header_Track = HDR_TRACK_8050;
            vdrive->Header_Sector = HDR_SECTOR_8050;
            vdrive->bam_name = BAM_NAME_8050;
            vdrive->bam_id = BAM_ID_8050;
            vdrive->Dir_Track = DIR_TRACK_8050;
            vdrive->Dir_Sector = DIR_SECTOR_8050;
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            vdrive->Bam_Track = BAM_TRACK_8250;
            vdrive->Bam_Sector = BAM_SECTOR_8250;
            vdrive->Header_Track = HDR_TRACK_8250;
            vdrive->Header_Sector = HDR_SECTOR_8250;
            vdrive->bam_name = BAM_NAME_8250;
            vdrive->bam_id = BAM_ID_8250;
            vdrive->Dir_Track = DIR_TRACK_8250;
            vdrive->Dir_Sector = DIR_SECTOR_8250;
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            vdrive->Bam_Track = BAM_TRACK_4000;
            vdrive->Bam_Sector = BAM_SECTOR_4000;
            vdrive->Header_Track = BAM_TRACK_4000;
            vdrive->Header_Sector = BAM_SECTOR_4000;
            vdrive->bam_name = BAM_NAME_4000;
            vdrive->bam_id = BAM_ID_4000;
            vdrive->Dir_Track = DIR_TRACK_4000;
            vdrive->Dir_Sector = DIR_SECTOR_4000;
            break;
        case VDRIVE_IMAGE_FORMAT_9000:
            {
                uint8_t tmp[256];
                unsigned int i;

                i = vdrive->num_tracks / 2;

                /* information is in track 0, sector 0 */
                vdrive_read_sector(vdrive, tmp, 0, 0);
                vdrive->Bam_Track = tmp[8];
                vdrive->Bam_Sector = tmp[9];
                vdrive->Header_Track = tmp[6];
                vdrive->Header_Sector = tmp[7];
                vdrive->bam_name = BAM_NAME_9000;
                vdrive->bam_id = BAM_ID_9000;
                vdrive->Dir_Track = tmp[4];
                vdrive->Dir_Sector = tmp[5];
                /* lets try to honor it, but empty images will cause problems */
                if (vdrive->Dir_Track != i
                    || vdrive->Header_Track != i
                    || vdrive->Bam_Track != 1
                    || vdrive->Dir_Sector != 10
                    || vdrive->Header_Sector != 20 ) {
                    vdrive->Bam_Track = 1;
                    vdrive->Bam_Sector = 0;
                    vdrive->Header_Track = i;
                    vdrive->Header_Sector = 20;
                    vdrive->Dir_Track = i;
                    vdrive->Dir_Sector = 10;
                }
                break;
            }
        default:
            log_error(vdrive_log,
                      "Unknown disk type %u.  Cannot set disk geometry.",
                      vdrive->image_format);
    }

    /* set area for active root partition */
    vdrive->Part_Start = 1;
    vdrive->Part_End = vdrive->num_tracks;
}

/* ------------------------------------------------------------------------- */

static unsigned int last_read_track, last_read_sector;
static uint8_t last_read_buffer[256];

void vdrive_get_last_read(unsigned int *track, unsigned int *sector, uint8_t **buffer)
{
    *track = last_read_track;
    *sector = last_read_sector;
    *buffer = last_read_buffer;
}

void vdrive_set_last_read(unsigned int track, unsigned int sector, uint8_t *buffer)
{
    last_read_track = track;
    last_read_sector = sector;
    memcpy(last_read_buffer, buffer, 256);
}

/* ------------------------------------------------------------------------- */
/* This is where logical sectors are turned to physical. Not yet, but soon. */
int vdrive_read_sector(vdrive_t *vdrive, uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    dadr.track = track;
    dadr.sector = sector;
    return disk_image_read_sector(vdrive->image, buf, &dadr);
}

int vdrive_write_sector(vdrive_t *vdrive, const uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    dadr.track = track;
    dadr.sector = sector;
    return disk_image_write_sector(vdrive->image, buf, &dadr);
}
