/*
 * vdrive.c - Virtual disk-drive implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Multi-drive and DHD enhancements by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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
#if 0
#include "uiapi.h"
#endif

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

int vdrive_device_setup(vdrive_t *vdrive, unsigned int unit)
{
    unsigned int i;

    vdrive->unit = unit;

    vdrive->current_offset = 0;
    vdrive->sys_offset = UINT32_MAX;
    vdrive->image_format = VDRIVE_IMAGE_FORMAT_NONE;
    vdrive->image = NULL;
    vdrive->image_mode = -1;
    vdrive->current_part = -1;

    for (i = 0; i < NUM_DRIVES; i++ ) {
        vdrive->images[i] = NULL;
    }

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
    vdrive->dir_part = 0;
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

/* called to start attached images from new */
void vdrive_refresh(unsigned int unit)
{
    int i;
    vdrive_t *vdrive;

#ifdef DEBUG_DRIVE
log_debug("VDRIVE: refresh unit %u", unit);
#endif

    vdrive = file_system_get_vdrive(unit);

    if (!vdrive) {
        return;
    }

    vdrive_bam_setup_bam(vdrive);

    vdrive->current_offset = 0;
    vdrive->sys_offset = UINT32_MAX;
    vdrive->image = NULL;
    vdrive->image_mode = -1;
    vdrive->current_part = -1;
    vdrive->dir_part = 0;

    /* clear out any default directory data */
    for (i = 0; i < 256; i++) {
        vdrive->cheadertrack[i] = 0;
        vdrive->cheadersector[i] = 0;
        vdrive->cdirtrack[i] = 0;
        vdrive->cdirsector[i] = 0;
        vdrive->cpartstart[i] = 0;
        vdrive->cpartend[i] = 0;
    }

}

/* called to flush out attached images */
void vdrive_flush(unsigned int unit)
{
    vdrive_t *vdrive;

#ifdef DEBUG_DRIVE
log_debug("VDRIVE: flush unit %u", unit);
#endif

    vdrive = file_system_get_vdrive(unit);

    if (!vdrive) {
        return;
    }

    /* flush current bam */
    if (vdrive->bam) {
        vdrive_bam_write_bam(vdrive);
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
 * Close all channels on a partition. This happens on 'I' -command and on command-
 * channel close.
 */

void vdrive_close_all_channels_partition(vdrive_t *vdrive, int part)
{
    unsigned int i;
    bufferinfo_t *p;

    for (i = 0; i <= 15; i++) {
        p = &(vdrive->buffers[i]);
        if (p->mode != BUFFER_NOT_IN_USE && p->mode != BUFFER_COMMAND_CHANNEL
            && p->partition == part) {
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
        case VDRIVE_IMAGE_FORMAT_NP:
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

    /* Make sure drive is in range */
    if (drive >= NUM_DRIVES) {
        return;
    }

    disk_image_detach_log(image, vdrive_log, unit, drive);

    /* shutdown everything on that drive */
    if (vdrive->haspt) {
        vdrive_close_all_channels(vdrive);
        lib_free(vdrive->bam);
        vdrive->bam = NULL;
        vdrive->image = NULL;
        vdrive->image_mode = -1;
        vdrive->current_part = -1;
        vdrive->selected_part = -1;
    } else {
        vdrive_close_all_channels_partition(vdrive, drive);
        if (vdrive->current_part == drive) {
            lib_free(vdrive->bam);
            vdrive->bam = NULL;
            vdrive->image = NULL;
            vdrive->image_mode = -1;
            vdrive->current_part = -1;
            vdrive->selected_part = -1;
        }
    }
    vdrive->images[drive] = NULL;
}

struct disk_image_s *vdrive_get_image(vdrive_t *vdrive, unsigned int drive)
{
    if (!vdrive) {
        return NULL;
    }

    return vdrive->images[drive];
}

int vdrive_attach_image(disk_image_t *image, unsigned int unit,
                        unsigned int drive, vdrive_t *vdrive)
{
    int i;
    uint32_t tmp;
    int haspt = 0;

    /* Make sure image is good */
    if (!image) {
        return -1;
    }

    /* Make sure drive is in range */
    if (drive >= NUM_DRIVES) {
        log_error(vdrive_log, "unit %u >= %d (MAX SUPPORTED DRIVES)",
            drive, NUM_DRIVES);
        return -1;
    }

    /* Make sure units match */
    if (vdrive->unit != unit) {
        log_error(vdrive_log, "vdrive->unit %u != unit %u",
            vdrive->unit, unit);
        return -1;
    }

    /* Make sure all the drives have the same as the one being requested */
    for (i = 0; i < NUM_DRIVES ; i++ ) {
        if (i == drive) {
            continue;
        }
        if (vdrive->images[i] && vdrive->images[i]->type != image->type) {
            log_error(vdrive_log, "All images attached to unit %u must be the "
                "same type. %p %u %u", unit, (void *)(vdrive->images[i]),
                vdrive->images[i]->type, image->type);
            return -1;
        }
    }

    disk_image_attach_log(image, vdrive_log, unit, drive);

    /* fix the number of tracks here as extended tracks aren't supported */
    switch (image->type) {
        case DISK_IMAGE_TYPE_D64:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = NUM_TRACKS_1541;
            vdrive->bam_size = 0x100;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_D67:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_2040;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_D71:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
            vdrive->num_tracks = NUM_TRACKS_1571;
            vdrive->bam_size = 0x200;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_D81:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1581;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x300;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_D80:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_8050;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x300;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_D82:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_8250;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x500;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_G64:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = NUM_TRACKS_1541;
            vdrive->bam_size = 0x100;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_G71:
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
            vdrive->num_tracks = NUM_TRACKS_1571;
            vdrive->bam_size = 0x200;
            vdrive->current_offset = 0;
            break;
        case DISK_IMAGE_TYPE_P64:
            /* FIXME: extra checks might be needed for supporting other drives */
            if (image->tracks > 42) {
                vdrive->image_format = VDRIVE_IMAGE_FORMAT_1571;
                vdrive->num_tracks = NUM_TRACKS_1571;
                vdrive->bam_size = 0x200;
            } else {
                vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
                vdrive->num_tracks = NUM_TRACKS_1541;
                vdrive->bam_size = 0x100;
            }
            vdrive->current_offset = 0;
            break;
#ifdef HAVE_X64_IMAGE
        case DISK_IMAGE_TYPE_X64:
            /* FIXME: x64 format can be any drive! */
            vdrive->image_format = VDRIVE_IMAGE_FORMAT_1541;
            vdrive->num_tracks = image->tracks;
            vdrive->bam_size = 0x100;
            vdrive->current_offset = 0;
            break;
#endif
        case DISK_IMAGE_TYPE_D1M:
        case DISK_IMAGE_TYPE_D2M:
        case DISK_IMAGE_TYPE_D4M:
        case DISK_IMAGE_TYPE_DHD:
            /* Can not attach multiple drives of DHD or D?M */
            if (drive > 0) {
                log_error(vdrive_log, "Can not attach image multiple DHD or "
                    "D?M images to one unit.");
                return -1;
            }
            haspt = 1;
            break;
        case DISK_IMAGE_TYPE_D90:
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
            vdrive->current_offset = 0;
            break;
        default:
            vdrive->current_offset = UINT32_MAX;
            return -1;
    }
#ifdef DEBUG_DRIVE
    log_debug("vdrive_attach_image image type:%u vdrive format:%u num_tracks:%u bam_size:%u",
         image->type, vdrive->image_format, vdrive->num_tracks, vdrive->bam_size);
#endif

    /* commit exist BAM possibly from another drive */
    vdrive_bam_write_bam(vdrive);

    /* Need an image associated for D9090/60 and vdrive_set_disk_geometry */
    vdrive->images[drive] = image;

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: Image attached to unit %u, drive %u as %u", unit, drive, image->type);
#endif

    vdrive->haspt = haspt;
/*    vdrive->dir_part = drive; */

    if (haspt) {
        vdrive->current_offset = UINT32_MAX;
        vdrive->sys_offset = UINT32_MAX;
        if (vdrive_read_partition_table(vdrive)) {
            vdrive->current_offset = UINT32_MAX;
            /* report error on bad DHD image for now */
            if (VDRIVE_IS_HD(vdrive)) {
                goto bad;
            }
            /* allow unformatted D?M images */
            vdrive->default_part = 1;
        }
        drive = vdrive->default_part;
    }

    /* clear out any default directory data */
    for (i = 0; i < 256; i++) {
        vdrive->cheadertrack[i] = 0;
        vdrive->cheadersector[i] = 0;
        vdrive->cdirtrack[i] = 0;
        vdrive->cdirsector[i] = 0;
        vdrive->cpartstart[i] = 0;
        vdrive->cpartend[i] = 0;
    }

    /* Initialise format constants */
    vdrive->current_part = -1;
    if (vdrive_switch(vdrive, drive)) {
        /* didn't work, set selected part anyways */
        vdrive->selected_part = drive;
    } else {
        /* all good, set selected partition */
        vdrive->selected_part = vdrive->current_part;
    }

#if 0
    /* read whole bam to ensure image is good */
    if (vdrive_bam_read_bam(vdrive)) {
        log_error(vdrive_log, "Error accessing BAM.");
        return -1;
    }
#endif
    return 0;

bad:
    vdrive->images[drive] = NULL;
    vdrive->image_mode = -1;
    vdrive->haspt = 0;
    vdrive->current_part = -1;

    return -1;
}

/* ------------------------------------------------------------------------- */

/*
 * Initialise format constants
 */

void vdrive_set_disk_geometry(vdrive_t *vdrive)
{
    vdrive->Part_Start = 1;

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
        case VDRIVE_IMAGE_FORMAT_NP:
            vdrive->Bam_Track = BAM_TRACK_NP;
            vdrive->Bam_Sector = BAM_SECTOR_NP;
            vdrive->Header_Track = BAM_TRACK_NP;
            vdrive->Header_Sector = BAM_SECTOR_NP;
            vdrive->bam_name = BAM_NAME_NP;
            vdrive->bam_id = BAM_ID_NP;
            vdrive->Dir_Track = DIR_TRACK_NP;
            vdrive->Dir_Sector = DIR_SECTOR_NP;
            break;
        case VDRIVE_IMAGE_FORMAT_SYS:
            vdrive->Bam_Track = 0;
            vdrive->Bam_Sector = 0;
            vdrive->Header_Track = 0;
            vdrive->Header_Sector = 0;
            vdrive->bam_name = 0;
            vdrive->bam_id = 0;
            vdrive->Dir_Track = 1;
            vdrive->Dir_Sector = 0;
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
                /* drive has track 0 */
                vdrive->Part_Start = 0;
                break;
            }
        default:
            log_error(vdrive_log,
                      "Unknown disk type %u.  Cannot set disk geometry.",
                      vdrive->image_format);
    }

    /* set area for active root partition */
    vdrive->Part_End = vdrive->num_tracks;

    /* update first time use for subdirectories */
    if (!vdrive->cheadertrack[vdrive->current_part]) {
        vdrive->cheadertrack[vdrive->current_part] = vdrive->Header_Track;
        vdrive->cheadersector[vdrive->current_part] = vdrive->Header_Sector;
        vdrive->cdirtrack[vdrive->current_part] = vdrive->Dir_Track;
        vdrive->cdirsector[vdrive->current_part] = vdrive->Dir_Sector;
        vdrive->cpartstart[vdrive->current_part] = 1;
        vdrive->cpartend[vdrive->current_part] = vdrive->num_tracks;
    } else { /* otherwise use the current settings */
        vdrive->Header_Track = vdrive->cheadertrack[vdrive->current_part];
        vdrive->Header_Sector = vdrive->cheadersector[vdrive->current_part];
        vdrive->Dir_Track = vdrive->cdirtrack[vdrive->current_part];
        vdrive->Dir_Sector = vdrive->cdirsector[vdrive->current_part];
        vdrive->Part_Start = vdrive->cpartstart[vdrive->current_part];
        vdrive->Part_End = vdrive->cpartend[vdrive->current_part];
        /* 1581's have a different BAM in each partition */
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581) {
            vdrive->Bam_Track = vdrive->Header_Track;
            vdrive->Bam_Sector = vdrive->Header_Sector;
        }
    }
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

static const signed int tosec4171[71] = {
      -1,
       0,   21,   42,   63,   84,  105,  126,  147,  168,  189, /* 1 */
     210,  231,  252,  273,  294,  315,  336,  357,  376,  395,
     414,  433,  452,  471,  490,  508,  526,  544,  562,  580,
     598,  615,  632,  649,  666,
     683,  704,  725,  746,  767,  788,  809,  830,  851,  872, /* 36 */
     893,  914,  935,  956,  977,  998, 1019, 1040, 1059, 1078,
    1097, 1116, 1135, 1154, 1173, 1191, 1209, 1227, 1245, 1263,
    1281, 1298, 1315, 1332, 1349
};

static const signed int max4171[71] = {
    -1,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, /* 1 */
    21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
    19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
    17, 17, 17, 17, 17,
    21, 21, 21, 21, 21, 21, 21, 21, 21, 21, /* 36 */
    21, 21, 21, 21, 21, 21, 21, 19, 19, 19,
    19, 19, 19, 19, 18, 18, 18, 18, 18, 18,
    17, 17, 17, 17, 17
};

static int vdrive_log_to_phy(vdrive_t *vdrive, disk_addr_t *dadr, unsigned int track, unsigned int sector)
{
    /* allows us to access CMD partitions without a lot of code changes */
    unsigned int offset;

    /* if no partition set, return -1, eventually becomes a drive not ready */
    if (vdrive->current_offset == UINT32_MAX || !vdrive->image) {
        return -1;
    }

    if (vdrive->haspt) {
        if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_SYS && track < 1) {
            return -1;
        }

        switch (vdrive->image_format) {
            case VDRIVE_IMAGE_FORMAT_1541:
                if (track > 35) {
                    return -1;
                }
                if (sector >= max4171[track]) {
                    return -1;
                }
                offset = tosec4171[track] + sector;
                break;
            case VDRIVE_IMAGE_FORMAT_1571:
                if (track > 70) {
                    return -1;
                }
                if (sector >= max4171[track]) {
                    return -1;
                }
                offset = tosec4171[track] + sector;
                break;
            case VDRIVE_IMAGE_FORMAT_1581:
                if (track > 80) {
                    return -1;
                }
                if (sector >= 40) {
                    return -1;
                }
                offset = (track - 1) * 40 + sector;
                break;
            case VDRIVE_IMAGE_FORMAT_NP:
                if (track > vdrive->num_tracks) {
                    return -1;
                }
                if (sector > 255) {
                    return -1;
                }
                offset = ((track - 1) << 8) + sector;
                break;
            case VDRIVE_IMAGE_FORMAT_SYS:
                /* system partition has 2 tracks: 0 and 1 */
                if (track > 1) {
                    return -1;
                }
                if (vdrive->image->type == DISK_IMAGE_TYPE_DHD) {
                    if (sector > 255) {
                        return -1;
                    }
                    offset = (track << 8) + sector;
                } else if (vdrive->image->type == DISK_IMAGE_TYPE_D1M) {
                    /* for FD series, track 0, is only 8 sectors */
                    /* track 1 can be variable; this is all logical, it works out well physically */
                    if (track == 0 && sector > 7) {
                        return -1;
                    }
                    if (sector >= 32) {
                        return -1;
                    }
                    offset = (track << 3) + sector;
                } else if (vdrive->image->type == DISK_IMAGE_TYPE_D2M) {
                    if (track == 0 && sector > 7) {
                        return -1;
                    }
                    if (sector >= 72) {
                        return -1;
                    }
                    offset = (track << 3) + sector;
                } else if (vdrive->image->type == DISK_IMAGE_TYPE_D4M) {
                    if (track == 0 && sector > 7) {
                        return -1;
                    }
                    if (sector >= 152) {
                        return -1;
                    }
                    offset = (track << 3) + sector;
                } else {
                    return -1;
                }
                break;
        default:
            log_error(vdrive_log,
                      "Unknown disk type %u.  Cannot set disk geometry.",
                      vdrive->image_format);
            return -1;
        }

        /* "current_offset" is 512-byte blocks to accomodate for DHD images of 4GB and to avoid overflow on 32-bit builds */
        /* CMD HD uses 16 bits for both track and sector to also accomodate for 4GB images */
        if (VDRIVE_IS_HD(vdrive)) {
            /* Do the math in steps to ensure we never overlow on 32-bit builds */
            dadr->track = (vdrive->current_offset >> 15) + 1;
            dadr->sector = (vdrive->current_offset << 1) & 65535;
            dadr->sector += offset;
            if (dadr->sector & 0xffff0000) {
                dadr->sector &= 65535;
                dadr->track++;
            }
        }
        else { /* FD series */
            /* no chance for overflow here */
            offset += vdrive->current_offset * 2;
            dadr->track = (offset >> 8) + 1;
            dadr->sector = offset & 255;
        }
    } else if (vdrive->image->type == DISK_IMAGE_TYPE_G71) {
        /* can't support extra tracks on G71 */
        if (track > 70) {
            return -1;
        }
        dadr->track = track;
        dadr->sector = sector;
        /* have to map logical track 36+ to 43+ */
        if (track > 35) {
            dadr->track += 7;
        }
    } else {
        dadr->track = track;
        dadr->sector = sector;
    }

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: log-to-phys %u %u %u to %u %u", track, sector, vdrive->current_offset, dadr->track, dadr->sector);
#endif

    return 0;
}

/* ------------------------------------------------------------------------- */
/* This is where logical sectors are turned to physical. Not yet, but soon. */
int vdrive_read_sector(vdrive_t *vdrive, uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    int ret;

    /* update image mode if disk is attached */
    if (vdrive->image) {
        vdrive->image_mode = vdrive->image->read_only;
    }
    /* check image mode */
    if (vdrive->image_mode < 0) {
        return CBMDOS_IPE_NOT_READY;
    }
    ret = vdrive_log_to_phy(vdrive, &dadr, track, sector);
    if (ret < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

#if 0
    ui_display_drive_track(vdrive->unit - 8, 0, dadr.track * 2);
#endif
    ret = disk_image_read_sector(vdrive->image, buf, &dadr);
#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: read_sector %u %u = %d", dadr.track, dadr.sector, ret);
#endif

    return ret;
}

int vdrive_write_sector(vdrive_t *vdrive, const uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    int ret;

    /* update image mode if disk is attached */
    if (vdrive->image) {
        vdrive->image_mode = vdrive->image->read_only;
    }
    /* check image mode */
    if (vdrive->image_mode > 0) {
        return CBMDOS_IPE_WRITE_PROTECT_ON;
    } else if (vdrive->image_mode < 0) {
        return CBMDOS_IPE_NOT_READY;
    }
    ret = vdrive_log_to_phy(vdrive, &dadr, track, sector);
    if (ret < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

#if 0
    ui_display_drive_track(vdrive->unit - 8, 0, dadr.track * 2);
#endif
    ret = disk_image_write_sector(vdrive->image, buf, &dadr);

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: write_sector %u %u = %d", dadr.track, dadr.sector, ret);
#endif

    return ret;
}

int vdrive_read_sector_physical(vdrive_t *vdrive, uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    dadr.track = track;
    dadr.sector = sector;
    return disk_image_read_sector(vdrive->image, buf, &dadr);
}

int vdrive_write_sector_physical(vdrive_t *vdrive, const uint8_t *buf, unsigned int track, unsigned int sector)
{
    disk_addr_t dadr;
    dadr.track = track;
    dadr.sector = sector;
    return disk_image_write_sector(vdrive->image, buf, &dadr);
}

/* For external access to individual drives */
int vdrive_ext_read_sector(vdrive_t *vdrive, int drive, uint8_t *buf,
    unsigned int track, unsigned int sector)
{
    if (vdrive_switch(vdrive, drive)) {
        return CBMDOS_IPE_NOT_READY;
    }

    return vdrive_read_sector(vdrive, buf, track, sector);
}

int vdrive_ext_write_sector(vdrive_t *vdrive, int drive, const uint8_t *buf,
    unsigned int track, unsigned int sector)
{
    if (vdrive_switch(vdrive, drive)) {
        return CBMDOS_IPE_NOT_READY;
    }

    return vdrive_write_sector(vdrive, buf, track, sector);
}

/* 0 = found, anything else DOS ERROR or not found */
int vdrive_find_sys(vdrive_t *vdrive)
{
    unsigned int i, old_co, old_if;
    int ret = CBMDOS_IPE_OK;
    unsigned char buf[256];
    unsigned char hdmagic[16] = {0x43, 0x4d, 0x44, 0x20, 0x48, 0x44, 0x20, 0x20,
                                 0x8d, 0x03, 0x88, 0x8e, 0x02, 0x88, 0xea, 0x60};
    unsigned char fdmagic[16] = {0x43, 0x4d, 0x44, 0x20, 0x46, 0x44, 0x20, 0x53,
                                 0x45, 0x52, 0x49, 0x45, 0x53, 0x20, 0x20, 0x20};

    vdrive->image = vdrive->images[0];
    vdrive->sys_offset = UINT32_MAX;
    old_co = vdrive->current_offset;
    old_if = vdrive->image_format;
    vdrive->current_offset = 0;
    vdrive->image_format = VDRIVE_IMAGE_FORMAT_SYS;
    switch (vdrive->image->type) {
        case DISK_IMAGE_TYPE_D1M:
            vdrive->current_offset = 0x640;
            /* fall through */
        case DISK_IMAGE_TYPE_D2M:
            if (!vdrive->current_offset) {
                vdrive->current_offset = 0xc80;
            }
            /* fall through */
        case DISK_IMAGE_TYPE_D4M:
            if (!vdrive->current_offset) {
                vdrive->current_offset = 0x1900;
            }
            ret = vdrive_read_sector(vdrive, buf, 0, 5);
            if (ret) {
                break;
            }
            if (memcmp(&(buf[0xf0]), fdmagic, 16) == 0) {
                vdrive->sys_offset = vdrive->current_offset;
                vdrive->default_part = buf[0xe2]; /* or 0xe3 */
            } else {
                ret = -1;
            }
            break;
        case DISK_IMAGE_TYPE_DHD:
            i = 0;
            while (1) {
                vdrive->current_offset = i << 7;
                ret = vdrive_read_sector(vdrive, buf, 0, 5);
                if (ret) {
                    break;
                }
                if (memcmp(&(buf[0xf0]), hdmagic, 16) == 0) {
                    vdrive->sys_offset = vdrive->current_offset;
                    vdrive->default_part = buf[0xe2]; /* or 0xe3 */
                    break;
                }
                ret = -1;
                i++;
/* stop for now short */
                if (i > 8) {
                    break;
                }
            }
            break;
        default:
            ret = -1;
    }

    vdrive->current_offset = old_co;
    vdrive->image_format = old_if;
    return ret;
}

static uint32_t vdrive_buf3_to_word(uint8_t *buf)
{
    return (buf[0] << 16) | (buf[1] << 8) | buf[2];
}

static void vdrive_word_to_buf3(uint8_t *buf, uint32_t value)
{
    buf[0] = ( value >> 16 ) & 255;
    buf[1] = ( value >> 8 ) & 255;
    buf[2] = value & 255;
}

/* 0 = okay, anything else, dos error */
int vdrive_read_partition_table(vdrive_t *vdrive)
{
    unsigned int old_co, old_if, maxpart;
    int i, j, k, s, olds;
    int ret = CBMDOS_IPE_OK;
    unsigned char buf[256];

    /* make sure this is a CMDHD or FD */
    if (!vdrive->haspt) {
        vdrive->sys_offset = UINT32_MAX;
        return -1;
    }

    /* we can only do this if we know where the system partition is */
    if (vdrive->sys_offset == UINT32_MAX) {
        ret = vdrive_find_sys(vdrive);
        if (ret) {
            for (i = 0; i <= 254; i++) {
                vdrive->ptype[i] = 0;
                vdrive->poffset[i] = 0;
                vdrive->psize[i] = 0;
                vdrive->cheadertrack[i] = 0;
            }
            return ret;
        }
    }

    /* CMD HDs can access 254 partitions (255 is system), FD2000/40000 can only do 31 */
    maxpart = VDRIVE_IS_HD(vdrive) ? 254 : 31;

    /* save old offsets/type for now */
    old_co = vdrive->current_offset;
    old_if = vdrive->image_format;
    /* setup access to system partition */
    vdrive->current_offset = vdrive->sys_offset;
    vdrive->image_format = VDRIVE_IMAGE_FORMAT_SYS;

    /* assume the paritions t/s links are correct, we won't follow them */
    olds = -1;
    for (i = 0; i <= maxpart; i++) {
        j = i << 5;
        s = j >> 8;
        j = j & 255;
        ret = CBMDOS_IPE_OK;
        if (s != olds) {
            ret = vdrive_read_sector(vdrive, buf, 1, s);
        }
        if (!ret) {
            olds = s;
            /* store physical partition #0 info (system) into 255 slot */;
            k = (i == 0) ? 255 : i;
            /* type */
            vdrive->ptype[k] = buf[j+2];
            /* offset in lba */
            vdrive->poffset[k] = vdrive_buf3_to_word(&(buf[j+0x15]));
            /* size in lba */
            vdrive->psize[k] = vdrive_buf3_to_word(&(buf[j+0x1d]));
            vdrive->cheadertrack[k] = 0;
            /* for FD series, the offset in the part table is 0;
                here we make it what is should be */
            if (i == 0) {
                vdrive->poffset[k] = vdrive->sys_offset;
            }
       } else {
            /* TODO: check for D?M without partition table */
            vdrive->sys_offset = UINT32_MAX;
            break;
       }
    }
    vdrive->ptype[0] = 0;
    vdrive->poffset[0] = UINT32_MAX;
    vdrive->psize[0] = UINT32_MAX;

    vdrive->current_offset = old_co;
    vdrive->image_format = old_if;
    return ret;
}

/* return DOS error */
int vdrive_write_partition_table(vdrive_t *vdrive)
{
    unsigned int old_co, old_if, maxpart;
    int i, j, s, olds;
    int ret = CBMDOS_IPE_OK;
    unsigned char buf[256];

    /* make sure this is a CMDHD or FD */
    if (!vdrive->haspt) {
        vdrive->sys_offset = UINT32_MAX;
        return -1;
    }

    /* we can only do this if we know where the system partition is; that is
        we read it first! */
    if (vdrive->sys_offset == UINT32_MAX) {
        return -1;
    }

    /* save old offsets/type for now */
    old_co = vdrive->current_offset;
    old_if = vdrive->image_format;
    /* setup access to system partition */
    vdrive->current_offset = vdrive->sys_offset;
    vdrive->image_format = VDRIVE_IMAGE_FORMAT_SYS;

    /* CMD HDs can access 254 partitions (255 is system), FD2000/40000 can only do 31 */
    maxpart = VDRIVE_IS_HD(vdrive) ? 254 : 31;

    /* assume the paritions t/s links are correct, we won't follow them */
    olds = -1;
    for (i = 0; i <= maxpart; i++) {
        j = i << 5;
        s = j >> 8;
        j = j & 255;
        ret = CBMDOS_IPE_OK;
        if (s != olds) {
            if (olds >= 0) {
                ret = vdrive_write_sector(vdrive, buf, 1, olds);
            }
            if (!ret) {
                ret = vdrive_read_sector(vdrive, buf, 1, s);
            }
        }
        if (!ret) {
            olds = s;
            /* don't touch system partition */
            if (i == 0) {
                continue;
            }
            buf[j + 2] = vdrive->ptype[i]; /* type */
            /* offset in lba */
            vdrive_word_to_buf3(&(buf[j + 0x15]), vdrive->poffset[i]);
            /* size in lba */
            vdrive_word_to_buf3(&(buf[j + 0x1d]), vdrive->psize[i]);
       } else {
            /* TODO: check for D?M without partition table */
            vdrive->sys_offset = UINT32_MAX;
            break;
       }
    }
    if (olds >= 0) {
        if (!ret) {
            ret = vdrive_write_sector(vdrive, buf, 1, olds);
        }
    }

    vdrive->current_offset = old_co;
    vdrive->image_format = old_if;
    return ret;
}

/* 0 if okay, anything else, dos error */
static int vdrive_change_part(vdrive_t *vdrive, int part)
{
    unsigned int old_co, old_if;
    int ret = CBMDOS_IPE_OK;
    unsigned int types[5] = {VDRIVE_IMAGE_FORMAT_NONE, VDRIVE_IMAGE_FORMAT_NP,
                             VDRIVE_IMAGE_FORMAT_1541, VDRIVE_IMAGE_FORMAT_1571,
                             VDRIVE_IMAGE_FORMAT_1581};
    unsigned int tracks[5] = {0, 255, NUM_TRACKS_1541, NUM_TRACKS_1571, NUM_TRACKS_1581};
    unsigned int bamsize[5] = {0, 0x2100, 0x100, 0x200, 0x300};

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: Change part to %d", part);
#endif
    /* make sure parition/drive is in a valid range */
    if (part < 0
        || (vdrive->haspt && part > 255)
        || (!vdrive->haspt && part >= NUM_DRIVES) ) {
        return CBMDOS_IPE_NOT_READY;
    }

    /* if it has a partition table and the part is 0, make it whatever the
        selected partition is */
    if (!part && vdrive->haspt) {
        part = vdrive->selected_part;
    }

    /* we can only do this if we know where the system partition is */
    if (vdrive->haspt && vdrive->sys_offset == UINT32_MAX) {
        ret = vdrive_read_partition_table(vdrive);
        if (ret) {
            return ret;
        }
    }

    /* can only do this for CMD images; shouldn't even have to check this, but we do anyways */
    if (vdrive->haspt) {
        /* save old offsets/type for now */
        old_co = vdrive->current_offset;
        old_if = vdrive->image_format;

        /* don't support anything above standard CBM */
        if ((vdrive->ptype[part] > 4 && vdrive->ptype[part] < 255)
            || vdrive->ptype[part] == 0) {
            ret = CBMDOS_IPE_NOT_READY;
        } else if (vdrive->ptype[part] <= 4) {
            old_if = types[vdrive->ptype[part]];
            old_co = vdrive->poffset[part];
            vdrive->current_part = part;
            vdrive->bam_size = bamsize[vdrive->ptype[part]];
            vdrive->num_tracks = tracks[vdrive->ptype[part]];
            if (vdrive->ptype[part] == 1) {
                vdrive->num_tracks = vdrive->psize[part] >> 7;
            }
            vdrive->image_mode = vdrive->image->read_only;
        /* only allow switches to system partition if part# is 255 and type is 255 */
        } else if (part == 255 && vdrive->ptype[part] == 255) {
            old_if = VDRIVE_IMAGE_FORMAT_SYS;
            old_co = vdrive->poffset[part];
            vdrive->current_part = part;
            vdrive->bam_size = 0;
            vdrive->num_tracks = 1;
        } else {
            ret = CBMDOS_IPE_NOT_READY;
        }
        /* restore original values or update changes */
        vdrive->current_offset = old_co;
        vdrive->image_format = old_if;
    } else {
        if (vdrive->images[part]) {
            vdrive->image = vdrive->images[part];
            vdrive->current_offset = 0;
            vdrive->current_part = part;
            vdrive->image_mode = vdrive->image->read_only;
        } else {
            ret = CBMDOS_IPE_NOT_READY;
        }
    }

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: Change part result is %d",ret);
#endif
    return ret;
}

/* 1 = yes, something is there
 * 0 = no, nothing */
int vdrive_ispartvalid(vdrive_t *vdrive, int part)
{
    int ret;

    /* make sure parition/drive is in a valid range */
    if (part < 0
        || (vdrive->haspt && part > 255)
        || (!vdrive->haspt && part >= NUM_DRIVES) ) {
        return 0;
    }

    /* dual drive, just check if image is there */
    if (!vdrive->haspt) {
        if (vdrive->images[part]) {
            return 1;
        } else {
            return 0;
        }
    }

    /* we can only do this if we don't know where the system partition is */
    if (vdrive->haspt && vdrive->sys_offset == UINT32_MAX) {
        ret = vdrive_read_partition_table(vdrive);
        if (ret) {
            return 0;
        }
    }

    /* if it has a partition table and the part is 0, make it whatever the
        selected partition is */
    if (!part && vdrive->haspt) {
        part = vdrive->selected_part;
    }

    /* can only do this for CMD images; shouldn't even have to check this, but we do anyways */
    if (vdrive->haspt) {
        /* don't support anything above standard CBM */
        if ((vdrive->ptype[part] > 4 && vdrive->ptype[part] < 255)
            || vdrive->ptype[part] == 0) {
            return 0;
        } else if (vdrive->ptype[part] <= 4) {
        /* it is listed as allocated, but maybe not formatted */
            return 1;
        } else if (vdrive->ptype[part] == 255) {
        /* or is a system part */
            return 1;
        } else {
        /* anything else = no */
            return 0;
        }
    }

    return 0;
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* this functions examines the partition table and packs everything to the
   beginning as much as possible. This is used for when a parition is deleted and data
   has to be packed so room can be made for newer partitions at the end of used space. */
/* do not set MOVE_LBA_CHUNKS to more than 128 */
#define MOVE_LBA_CHUNKS 32
int vdrive_pack_parts(vdrive_t *vdrive)
{
    unsigned int old_co, old_if, maxpart;
    int ret = CBMDOS_IPE_OK;
    int lowpos, bestpos, length;
    int i, j, k, best, block;
    int src, dest;
    uint8_t *buf = NULL;

    /* CMD HDs can access 254 partitions (255 is system), FD2000/40000 can only do 31 */
    maxpart = VDRIVE_IS_HD(vdrive) ? 254 : 31;

    /* save old offsets/type for now */
    old_co = vdrive->current_offset;
    old_if = vdrive->image_format;

    /* find lowest starting partition */
    lowpos = -1;
    length = 1;
    while (1) {
        /* scan for closest partition (physically) to the current one */
        best = 0;
        bestpos = INT32_MAX;
        block = 0;
        for (i = 1; i <= maxpart; i++) {
            /* skip deleted, foriegn, or anything else */
            if (vdrive->ptype[i] > 0 && vdrive->ptype[i] < 7
               && ( vdrive->poffset[i] >= lowpos + length )
               && vdrive->poffset[i] < bestpos) {
                /* make sure a move won't go into a foriegn partition */
                for (j = 1; j <= maxpart; j++) {
                    if (vdrive->ptype[j] == 7 &&
                        ( ( (vdrive->poffset[j] >= lowpos + length && vdrive->poffset[j] < lowpos + length + vdrive->psize[i]) ) ||
                        ( (vdrive->poffset[j] + vdrive->psize[j] >= lowpos + length && vdrive->poffset[j] + vdrive->psize[j] < lowpos + length + vdrive->psize[i]) ) ) ) {
                        /* remember which foriegn partation is blocking */
                        block = j;
                        break;
                    }
                }
                /* if no problems, mark this as the one to use, otherwise, look for a better one */
                if (!block) {
                    best = i;
                    bestpos = vdrive->poffset[i];
                } else {
                    break; /* otherwise, stop checking the foriegn conditions */
                }
            }
        }
        /* if not found, we are done */
        if (bestpos == INT32_MAX && !block) {
            break;
        } else if (bestpos == INT32_MAX && block) {
            /* we found something to move, but can't because there isn't enough space before a foriegn partition */
            /* so we will start searching after the foriegn partition */
            lowpos = vdrive->poffset[block];
            length = vdrive->psize[block];
            /* look again */
            continue;
        } /* else just follows thru */
        /* if it is right beside it, go look for another one */
        if (lowpos + length == bestpos) {
            lowpos = bestpos;
            length = vdrive->psize[best];
            /* look again */
            continue;
        }
        /* move it from bestpos to lowpos+length for psize[best] LBAs (512 byte sectors) */
        /* moving a lot of data is pretty serious, so we must take care that
            there will be no disk errors. */
        /* we will scan the whole source partition and destiation area to make
           sure there are no errors, if so, we will stop the process. */
        /* we set the drive type to NP since it is linear addressing */
        buf = lib_malloc(MOVE_LBA_CHUNKS * 2 * 256);
        vdrive->image_format = VDRIVE_IMAGE_FORMAT_NP;

        src = vdrive->current_offset = vdrive->poffset[best];
        j = vdrive->psize[best];

        /* read source and destination areas to ensure there are no errors first */
        /* read source partition area */
        while (j) {
            k = min(j, MOVE_LBA_CHUNKS);
            for (i = 0; i < k * 2; i++) {
                ret = vdrive_read_sector(vdrive, &(buf[i << 8]), 1, i);
                if (ret) {
                    break;
                }
            }
            vdrive->current_offset += k;
            j -= k;
        }
        if (ret) {
            goto out;
        }

        dest = vdrive->current_offset = lowpos + length;
        j = vdrive->psize[best];

        /* read destination partition area */
        while (j) {
            k = min(j, MOVE_LBA_CHUNKS);
            for (i = 0; i < k * 2; i++) {
                ret = vdrive_read_sector(vdrive, &(buf[i << 8]), 1, i);
                if (ret) {
                    break;
                }
            }
            vdrive->current_offset += k;
            j -= k;
        }
        if (ret) {
            goto out;
        }

        /* all good, do the copy */
        while (j) {
            k = min(j, MOVE_LBA_CHUNKS);
            vdrive->current_offset = src;
            for (i = 0; i < k * 2; i++) {
                vdrive_read_sector(vdrive, &(buf[i << 8]), 1, i);
            }
            vdrive->current_offset = dest;
            for (i = 0; i < k * 2; i++) {
                vdrive_write_sector(vdrive, &(buf[i << 8]), 1, i);
            }
            src += k;
            dest += k;
            j -= k;
        }

        lib_free(buf);
        buf = NULL;

        /* update table in RAM */
        vdrive->poffset[best] = lowpos + length;

        lowpos = vdrive->poffset[best];
        length = vdrive->psize[best];
        /* look again */
    }

out:
    if (buf) {
        lib_free(buf);
    }

    ret = vdrive_write_partition_table(vdrive);

    /* restore original values or update changes */
    vdrive->current_offset = old_co;
    vdrive->image_format = old_if;

    return ret;
}

/* 0 okay, anything else, bad */
int vdrive_switch(vdrive_t *vdrive, int part)
{
    int ret = CBMDOS_IPE_OK;

    /* make sure parition/drive is in a valid range */
    if (part < 0) {
        return CBMDOS_IPE_NOT_READY;
    }

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: request switch to %d",part);
#endif
    /* find the real partition/drive */
    part = vdrive_realpart(vdrive, part);

    /* try to switch only if the current paritition isn't the current one */
    if (part != vdrive->current_part && vdrive->haspt) {
        ret = 1;
    } else if (part != vdrive->current_part && !vdrive->haspt) {
    /* other wise it is a different drive */
        ret = 1;
    }

    if (ret) {
        /* flush current bam */
        vdrive_bam_write_bam(vdrive);
        /* try to change to selected partition */
        ret = vdrive_change_part(vdrive, part);
        if (!ret) {
            /* it worked */
            vdrive_set_disk_geometry(vdrive);
            vdrive_bam_setup_bam(vdrive);
        } else  {
            /* no, not good, make this an unaccessable part */
            if (vdrive->haspt) {
                vdrive->image_format = VDRIVE_IMAGE_FORMAT_NONE;
                vdrive->num_tracks = 0;
                vdrive->bam_size = 0;
            }
            vdrive->current_offset = UINT32_MAX;
            vdrive->current_part = -1;
        }
    }

    /* make sure to restore to "selected" internal 1581 partition regardless if
        the partition is switched */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_1581 &&
        (vdrive->Part_Start != vdrive->cpartstart[vdrive->current_part]
            || vdrive->Part_End != vdrive->cpartend[vdrive->current_part]) ) {
        /* flush current bam */
        vdrive_bam_write_bam(vdrive);
        vdrive_set_disk_geometry(vdrive);
        vdrive_bam_setup_bam(vdrive);
    }

#ifdef DEBUG_DRIVE
    log_debug("VDRIVE: result is %d %d %u %u",ret, vdrive->current_part, vdrive->current_offset, vdrive->image_format);
#endif
    return ret;
}

/* return the "real" partition when passed a 0 for CMD */
/* otherwiese return 0 if we exceed the number of drives */
int vdrive_realpart(vdrive_t *vdrive, int part)
{
    /* for dual drive systems, parts >= NUM_DRIVES is always 0 */
    if (!vdrive->haspt) {
        if (part >= NUM_DRIVES) {
            return 0;
        }
        return part;
    }

    /* otherwise only interested when the value is 0 */
    if (part) {
        return part;
    }

    /* return selected partition when a CMD HD or FD */
    return vdrive->selected_part;
}
