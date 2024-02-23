/*
 * fdc.c - 1001/8x50/90x0 FDC emulation
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *
 * D9090/D9060 portions by
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

#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "attach.h"
#include "diskimage.h"
#include "drive-check.h"
#include "drive.h"
#include "drivetypes.h"
#include "fdc.h"
#include "lib.h"
#include "log.h"
#include "snapshot.h"
#include "types.h"


/* #define FDC_DEBUG */

#define DOS_IS_90(type)  (type == DRIVE_TYPE_9000)
#define DOS_IS_80(type)  (type == DRIVE_TYPE_8050 || type == DRIVE_TYPE_8250 || type == DRIVE_TYPE_1001)
#define DOS_IS_40(type)  (type == DRIVE_TYPE_4040)
#define DOS_IS_30(type)  (type == DRIVE_TYPE_3040)
#define DOS_IS_20(type)  (type == DRIVE_TYPE_2040)

/************************************************************************/

#define NUM_FDC NUM_DISK_UNITS   /* dual disk drives */

static log_t fdc_log = LOG_ERR;

typedef struct fdc_t {
    int          fdc_state;
    alarm_t      *fdc_alarm;
    CLOCK        alarm_clk;
    uint8_t         *buffer;
    uint8_t         *iprom;
    unsigned int drive_type;
    unsigned int num_drives;
    unsigned int last_track;
    unsigned int last_sector;
    int          wps_change;     /* if not zero, toggle wps and decrement */
    disk_image_t *image;
    disk_image_t *realimage;
} fdc_t;

/*
 * The fdc[] array contains an fdc_t struct for every drive, i.e. two structs
 * for every unit, as each unit could be dual drive.
 */
static fdc_t fdc[NUM_FDC][2];

void fdc_reset(unsigned int fnum, unsigned int drive_type)
{
    fdc_t *thefdc0 = &fdc[fnum][0];
    fdc_t *thefdc1 = &fdc[fnum][1];
    disk_image_t *saved_image0, *saved_image1;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_reset: drive %u type=%u", fnum, drive_type);
#endif

    saved_image0 = thefdc0->realimage;
    saved_image1 = thefdc1->realimage;

    /* detach disk images */
    if (thefdc0->image) {
        thefdc0->wps_change = 0;
        fdc_detach_image(thefdc0->image, fnum + 8, 0);
    }
    if (thefdc1->image) {
        thefdc1->wps_change = 0;
        fdc_detach_image(thefdc1->image, fnum + 8, 1);
    }

    if (drive_check_old(drive_type)) {
        thefdc0->drive_type = drive_type;
        thefdc0->num_drives = drive_check_dual(drive_type) ? 2 : 1;
        thefdc0->fdc_state = FDC_RESET0;
        alarm_set(thefdc0->fdc_alarm, diskunit_clk[fnum] + 20);
    } else {
        thefdc0->drive_type = DRIVE_TYPE_NONE;
        alarm_unset(thefdc0->fdc_alarm);
        thefdc0->fdc_state = FDC_UNUSED;
        thefdc0->num_drives = 0;
    }

    /* re-attach disk images */
    if (saved_image0) {
#ifdef FDC_DEBUG
        log_message(fdc_log, "ieee/fdc.c:fdc_reset dev %u type %u drive 0 re-attach image %p (drive: %p)", fnum + 8, drive_type, saved_image0, diskunit_context[fnum]->drives[0]->image);
#endif
        fdc_attach_image(saved_image0, fnum + 8, 0);
    }
    if (saved_image1) {
#ifdef FDC_DEBUG
        log_message(fdc_log, "ieee/fdc.c:fdc_reset dev %u type %u drive 1 re-attach image %p (drive: %p)", fnum + 8, drive_type, saved_image0, diskunit_context[fnum+1]->drives[1]->image);
#endif
        fdc_attach_image(saved_image1, fnum + 8, 1);
    }
}

/*****************************************************************************
 * Format a disk in DOS1 format
 */

static uint8_t fdc_do_format_D20(unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, uint8_t *header)
{
    int i;
    int ret;
    uint8_t rc = 0;
    disk_addr_t dadr;
    uint8_t sector_data[256];

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][dnr];

    if (!memcmp(sysfdc->iprom + 0x2040, &sysfdc->buffer[0x100], 0x200)) {

        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        static const unsigned int nsecs[] = { 21, 20, 18, 17 };
        unsigned int ntracks, nsectors = 0;

        /*
        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        int ntracks, nsectors = 0;
        */
#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   track=%u, sector=%u", track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
#endif
        if (imgfdc->image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = 35;

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            for (i = 3; i >= 0; i--) {
                if (dadr.track > sectorchangeat[i]) {
                    nsectors = nsecs[i];
                    break;
                }
            }
#ifdef FDC_DEBUG
            log_message(fdc_log, "   track %u, -> %u sectors",
                        dadr.track, nsectors);
#endif
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(imgfdc->image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%u S:%u on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(fnum + 8, dnr, header);
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * Format a disk in DOS2 format
 */

static uint8_t fdc_do_format_D40(unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, uint8_t *header)
{
    int i;
    int ret;
    uint8_t rc = 0;
    disk_addr_t dadr;
    uint8_t sector_data[256];

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][dnr];

    if (!memcmp(sysfdc->iprom + 0x1000, &sysfdc->buffer[0x100], 0x200)) {

        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        unsigned int ntracks, nsectors = 0;

#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   secs per track: %d %d %d %d",
                    sysfdc->buffer[0x99], sysfdc->buffer[0x9a],
                    sysfdc->buffer[0x9b], sysfdc->buffer[0x9c]);
        log_message(fdc_log, "   track=%u, sector=%u",
                    track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
#endif
        if (imgfdc->image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = 35;

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            for (i = 3; i >= 0; i--) {
                if (dadr.track > sectorchangeat[i]) {
                    nsectors = sysfdc->buffer[0x99 + 3 - i];
                    break;
                }
            }
#ifdef FDC_DEBUG
            log_message(fdc_log, "   track %u, -> %u sectors",
                        dadr.track, nsectors);
#endif
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(imgfdc->image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%u S:%u on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(fnum + 8, dnr, header);
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * Format a disk in DOS2/80 track format
 */

static uint8_t fdc_do_format_D80(unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, uint8_t *header)
{
    int i;
    int ret;
    uint8_t rc = 0;
    disk_addr_t dadr;
    uint8_t sector_data[256];

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][dnr];

    if (!memcmp(sysfdc->iprom, &sysfdc->buffer[0x100], 0x300)) {

        unsigned int ntracks, nsectors = 0;
        /* detected format code */
#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   track for zones side 0: %d %d %d %d",
                    sysfdc->buffer[0xb0], sysfdc->buffer[0xb1],
                    sysfdc->buffer[0xb2], sysfdc->buffer[0xb3]);
        log_message(fdc_log, "   track for zones side 1: %d %d %d %d",
                    sysfdc->buffer[0xb4], sysfdc->buffer[0xb5],
                    sysfdc->buffer[0xb6], sysfdc->buffer[0xb7]);
        log_message(fdc_log, "   secs per track: %d %d %d %d",
                    sysfdc->buffer[0x99], sysfdc->buffer[0x9a],
                    sysfdc->buffer[0x9b], sysfdc->buffer[0x9c]);
        log_message(fdc_log, "   vars: 870=%d 873=%d 875=%d",
                    sysfdc->buffer[0x470], sysfdc->buffer[0x473],
                    sysfdc->buffer[0x475]);
        log_message(fdc_log, "   track=%u, sector=%u",
                    track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
        log_message(fdc_log, "   sides=%d",
                    sysfdc->buffer[0xac]);
#endif
        if (imgfdc->image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = (sysfdc->buffer[0xac] > 1) ? 154 : 77;

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            if (dadr.track < 78) {
                for (i = 3; i >= 0; i--) {
                    if (dadr.track < sysfdc->buffer[0xb0 + i]) {
                        nsectors = sysfdc->buffer[0x99 + i];
                        break;
                    }
                }
            } else {
                for (i = 3; i >= 0; i--) {
                    if (dadr.track < sysfdc->buffer[0xb4 + i]) {
                        nsectors = sysfdc->buffer[0x99 + i];
                        break;
                    }
                }
            }
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(imgfdc->image, sector_data,
                                              &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%u S:%u on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(fnum + 8, dnr, header);
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * Format a hard disk in DOS3/90 track format
 */

static uint8_t fdc_do_format_D90(unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, uint8_t *header)
{
    int ret;
    uint8_t rc = 0;
    disk_addr_t dadr;
    uint8_t sector_data[256];

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][dnr];

    if (1) {
        unsigned int ntracks, nsectors = 0;
        /* detected format code */
        if (imgfdc->image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = sysfdc->buffer[0x9a];
        nsectors = sysfdc->buffer[0x9d] << 5;

#ifdef FDC_DEBUG
        log_message(fdc_log, "format command: ");
        log_message(fdc_log, "   tracks=%u, sectors=%u",
                    ntracks + 1, nsectors);
#endif

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(imgfdc->image, sector_data,
                                              &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%u S:%u on disk image.",
                              dadr.track, dadr.sector);
                    /* save back the track/sector where the issue happened */
                    header[2] = dadr.track;
                    header[3] = dadr.sector;
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * execute an FDC job sent by the main CPU
 */
#ifdef FDC_DEBUG
static uint8_t fdc_do_job_(unsigned int fnum, int buf,
                        unsigned int drv, uint8_t job, uint8_t *header);
#endif

static uint8_t fdc_do_job(unsigned int fnum, int buf,
                       unsigned int drv, uint8_t job, uint8_t *header)
{
#ifdef FDC_DEBUG
    uint8_t retval = fdc_do_job_(fnum, buf, drv, job, header);
    const char *jobs[] =
        { "Read", "Write", "Verify", "Seek", "Bump", "Jump",
          "ExecWhenRdy", "--" };
    const char *errors[] =
        { "--", "OK", "HEADER", "SYNC", "NOBLOCK",
          "DCHECK", "???", "VERIFY", "WPROT", "HCHECK",
          "BLENGTH", "ID", "FSPEED", "DRIVE",
          "DECODE" };

    log_message(fdc_log, "  fdc_do_job (%s %02x) -> %02x (%s)",
                jobs[(job >> 4) & 7], job, retval,
                (retval <= 14) ? errors[retval] : "Unknown");
    return retval;
}

static uint8_t fdc_do_job_(unsigned int fnum, int buf,
                        unsigned int drv, uint8_t job, uint8_t *header)
{
#endif
    uint8_t rc;
    int ret;
    int i;
    disk_addr_t dadr;
    uint8_t *base;
    uint8_t sector_data[256];
    uint8_t disk_id[2];
    drive_t *drive;

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][drv];

    dadr.track = header[2];
    dadr.sector = header[3];

    rc = 0;
    base = &(sysfdc->buffer[(buf + 1) << 8]);

#ifdef FDC_DEBUG
    log_message(fdc_log, "do job %02x, buffer %d ($%04x): d%u t%u s%u, "
                "image=%p, type=%04u",
                job, buf, (unsigned int)(buf + 1) << 8, drv, dadr.track, dadr.sector,
                imgfdc->image,
                imgfdc->image ? imgfdc->image->type : 0);
#endif

    if (imgfdc->image == NULL && job != 0xd0) {
#ifdef FDC_DEBUG
        log_message(fdc_log, "dnr=%u, image=NULL -> no disk!", drv);
#endif
        return FDC_ERR_SYNC;
    }

    file_system_bam_get_disk_id(fnum + 8, drv, disk_id);
#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_do_job_: header '%c%c', disk_id '%c%c'",
        header[0], header[1], disk_id[0], disk_id[1]);
#endif

    switch (job) {
        case 0x80:        /* read */
            if (DOS_IS_90(sysfdc->drive_type)) {
                rc = FDC_ERR_OK;
                /* the HD fdc can transfer more than one block */
                for (i = sysfdc->buffer[0xa0]; i>0; i--) {
                    if (dadr.track > imgfdc->image->tracks) {
                        /* save back the track/sector where the issue happened */
                        header[2] = dadr.track;
                        header[3] = dadr.sector;
                        rc = FDC_ERR_DRIVE;
                        break;
                    }
                    ret = disk_image_read_sector(imgfdc->image, sector_data, &dadr);
                    if (ret < 0) {
                        log_error(LOG_DEFAULT,
                                  "Cannot read T:%u S:%u from disk image.",
                                  dadr.track, dadr.sector);
                        /* save back the track/sector where the issue happened */
                        header[2] = dadr.track;
                        header[3] = dadr.sector;
                        rc = FDC_ERR_DRIVE;
                        break;
                    } else {
                        memcpy(base, sector_data, 256);
                    }
                    dadr.sector++;
                    if (dadr.sector >= imgfdc->image->sectors) {
                        dadr.sector = 0;
                        dadr.track++;
                    }
                    /* check cycle buffer flag */
                    if (sysfdc->buffer[0xa3]) {
                        buf++;
                        if (buf == 15) {
                            buf = 0;
                        }
                        base = &(sysfdc->buffer[(buf + 1) << 8]);
                    }
                }
            } else {
                if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
#ifdef FDC_DEBUG
                    log_message(fdc_log, "do job read: header '%c%c' != disk_id '%c%c'",
                        header[0], header[1], disk_id[0], disk_id[1]);
#endif
                    rc = FDC_ERR_ID;
                    break;
                }
                ret = disk_image_read_sector(imgfdc->image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Cannot read T:%u S:%u from disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DRIVE;
                } else {
                    memcpy(base, sector_data, 256);
                    rc = FDC_ERR_OK;
                }
            }
            break;
        case 0x90:        /* write */
            if (DOS_IS_90(sysfdc->drive_type)) {
                if (imgfdc->image->read_only) {
                    rc = FDC_ERR_WPROT;
                    break;
                }
                rc = FDC_ERR_OK;
                /* the HD fdc can transfer more than one block */
                for (i = sysfdc->buffer[0xa0]; i>0; i--) {
                    if (dadr.track > imgfdc->image->tracks) {
                        /* save back the track/sector where the issue happened */
                        header[2] = dadr.track;
                        header[3] = dadr.sector;
                        rc = FDC_ERR_DRIVE;
                        break;
                    }
                    memcpy(sector_data, base, 256);
                    ret = disk_image_write_sector(imgfdc->image, sector_data, &dadr);
                    if (ret < 0) {
                        log_error(LOG_DEFAULT,
                                  "Could not update T:%u S:%u on disk image.",
                                  dadr.track, dadr.sector);
                        /* save back the track/sector where the issue happened */
                        header[2] = dadr.track;
                        header[3] = dadr.sector;
                        rc = FDC_ERR_DRIVE;
                        break;
                    }
                    dadr.sector++;
                    if (dadr.sector >= imgfdc->image->sectors) {
                        dadr.sector = 0;
                        dadr.track++;
                    }
                    /* check cycle buffer flag */
                    if (sysfdc->buffer[0xa3]) {
                        buf++;
                        if (buf == 15) {
                            buf = 0;
                        }
                        base = &(sysfdc->buffer[(buf + 1) << 8]);
                    }
                }
            } else {
                if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
#ifdef FDC_DEBUG
                    log_message(fdc_log, "do job write: header '%c%c' != disk_id '%c%c'",
                        header[0], header[1], disk_id[0], disk_id[1]);
#endif
                    rc = FDC_ERR_ID;
                    break;
                }
                if (imgfdc->image->read_only) {
                    rc = FDC_ERR_WPROT;
                    break;
                }
                memcpy(sector_data, base, 256);
                ret = disk_image_write_sector(imgfdc->image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%u S:%u on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DRIVE;
                } else {
                    rc = FDC_ERR_OK;
                }
            }
            break;
        case 0xA0:        /* verify */
            if (DOS_IS_90(sysfdc->drive_type)) {
                /* the fdc just does a read, doesn't compare anything */
                rc = FDC_ERR_OK;
                break;
            }
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
#ifdef FDC_DEBUG
                log_message(fdc_log, "do job verify: header '%c%c' != disk_id '%c%c'",
                    header[0], header[1], disk_id[0], disk_id[1]);
#endif
                rc = FDC_ERR_ID;
                break;
            }
            ret = disk_image_read_sector(imgfdc->image, sector_data, &dadr);
            if (ret < 0) {
                log_error(LOG_DEFAULT,
                          "Cannot read T:%u S:%u from disk image.",
                          dadr.track, dadr.sector);
                rc = FDC_ERR_DRIVE;
            } else {
                rc = FDC_ERR_OK;
                for (i = 0; i < 256; i++) {
                    if (fnum) {
                        if (sector_data[i] != base[i]) {
                            rc = FDC_ERR_VERIFY;
                        }
                    } else {
                        if (sector_data[i] != base[i]) {
                            rc = FDC_ERR_VERIFY;
                        }
                    }
                }
            }
            break;
        case 0xB0:        /* seek - move to track and read ID(?) */
#ifdef FDC_DEBUG
                log_message(fdc_log, "do job seek: header was '%c%c' becomes disk_id '%c%c'",
                    header[0], header[1], disk_id[0], disk_id[1]);
#endif
            header[0] = disk_id[0];
            header[1] = disk_id[1];
            /* header[2] = fdc[dnri].last_track; */
            dadr.track = header[2];
            header[3] = 1;
            rc = FDC_ERR_OK;
            break;
        case 0xC0:        /* bump (to track 0 and back to 18?) */
            dadr.track = 1;
            if (DOS_IS_20(sysfdc->drive_type)) {
                header[2] = 18;
            }
            rc = FDC_ERR_OK;
            break;
        case 0xD0:        /* jump to buffer - but we do not emulate FDC CPU */
#ifdef FDC_DEBUG
            log_message(fdc_log, "exec buffer %d ($%04x): %02x %02x %02x %02x",
                        buf, (unsigned int)(buf + 1) << 8,
                        base[0], base[1], base[2], base[3]
                        );
#endif
            if (DOS_IS_40(sysfdc->drive_type)
                || DOS_IS_30(sysfdc->drive_type)) {
                if (!memcmp(sysfdc->iprom + 0x12f8, &sysfdc->buffer[0x100],
                            0x100)) {
                    sysfdc->fdc_state = FDC_RESET2;
                    return 0;
                }
            }
            if (DOS_IS_80(sysfdc->drive_type) || DOS_IS_90(sysfdc->drive_type)) {
                static const uint8_t jumpseq[] = {
                    0x78, 0x6c, 0xfc, 0xff
                };
                if (!memcmp(jumpseq, &sysfdc->buffer[0x100], 4)) {
                    sysfdc->fdc_state = FDC_RESET0;
                    return 0;
                }
            }
            rc = FDC_ERR_DRIVE;
            break;
        case 0xE0:        /* execute when drive/head ready. We do not emulate
                             FDC CPU, but we handle the case when a disk is
                             formatted */
            /* we have to check for standard format code that is copied
               to buffers 0-3 */
            if (DOS_IS_80(sysfdc->drive_type)) {
                rc = fdc_do_format_D80(fnum, drv, dadr.track, dadr.sector, buf, header);
            } else
            if (DOS_IS_40(sysfdc->drive_type)
                || DOS_IS_30(sysfdc->drive_type)) {
                rc = fdc_do_format_D40(fnum, drv, dadr.track, dadr.sector, buf, header);
            } else
            if (DOS_IS_20(sysfdc->drive_type)) {
                rc = fdc_do_format_D20(fnum, drv, dadr.track, dadr.sector, buf, header);
            } else {
                rc = FDC_ERR_DRIVE;
            }
            break;
        case 0xF0:
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
#ifdef FDC_DEBUG
                log_message(fdc_log, "do job F0: header '%c%c' != disk_id '%c%c'",
                    header[0], header[1], disk_id[0], disk_id[1]);
#endif
                rc = FDC_ERR_ID;
                break;
            }
            /* try to read block header from disk */
            rc = FDC_ERR_OK;
            break;
        case 0xC4:
            /* HD low-level format, not DOS data at all */
            if (DOS_IS_90(sysfdc->drive_type)) {
                rc = fdc_do_format_D90(fnum, drv, dadr.track, dadr.sector, buf, header);
            }
            break;
        case 0xC8: /* SASI bus reset */
        case 0xB8: /* Unknown vendor command */
            rc = FDC_ERR_OK;
            break;
    }

    drive = diskunit_context[fnum]->drives[drv];
    drive->current_half_track = 2 * dadr.track;
    imgfdc->last_track = dadr.track;
    imgfdc->last_sector = dadr.sector;

    return rc;
}


static void int_fdc(CLOCK offset, void *data)
{
    CLOCK rclk;
    int i, j;
    drive_t *drive;
    diskunit_context_t *drv = (diskunit_context_t *)data;
    unsigned int fnum = drv->mynumber;

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][1];

    rclk = diskunit_clk[fnum] - offset;

#ifdef FDC_DEBUG
    static int old_state[NUM_FDC] = { -1, -1 };
    if (sysfdc->fdc_state < FDC_RUN) {
        if (sysfdc->fdc_state != old_state[fnum]) {
            log_message(fdc_log, "int_fdc%u %u: state=%d",
                        fnum, rclk, sysfdc->fdc_state);
        }
        old_state[fnum] = sysfdc->fdc_state;
    }
#endif

    switch (sysfdc->fdc_state) {
        case FDC_RESET0:
            drive = diskunit_context[fnum]->drives[0];
            if (DOS_IS_80(sysfdc->drive_type)) {
                drive->current_half_track = 2 * 38;
                sysfdc->buffer[0] = 2;
            } else if (DOS_IS_90(sysfdc->drive_type)) {
                drive->current_half_track = 2 * (153/2);
                sysfdc->buffer[0] = 2;
            } else {
                drive->current_half_track = 2 * 18;
                sysfdc->buffer[0] = 0x3f;
            }

            if (DOS_IS_20(sysfdc->drive_type)) {
                sysfdc->fdc_state = FDC_RUN;
            } else {
                sysfdc->fdc_state++;
            }
            sysfdc->alarm_clk = rclk + 2000;
            alarm_set(sysfdc->fdc_alarm, sysfdc->alarm_clk);
            break;
        case FDC_RESET1:
            if (DOS_IS_80(sysfdc->drive_type) || DOS_IS_90(sysfdc->drive_type)) {
                if (sysfdc->buffer[0] == 0) {
                    sysfdc->buffer[0] = 1;
                    sysfdc->fdc_state++;
                }
            } else {
                if (sysfdc->buffer[3] == 0xd0) {
                    sysfdc->buffer[3] = 0;
                    sysfdc->fdc_state++;
                }
            }
            sysfdc->alarm_clk = rclk + 2000;
            alarm_set(sysfdc->fdc_alarm, sysfdc->alarm_clk);
            break;
        case FDC_RESET2:
            if (DOS_IS_80(sysfdc->drive_type)) {
                if (sysfdc->buffer[0] == 0) {
                    /* emulate routine written to buffer RAM */
                    sysfdc->buffer[1] = 0x0e;
                    sysfdc->buffer[2] = 0x2d;
                    /* number of sides on disk drive */
                    sysfdc->buffer[0xac] =
                        (sysfdc->drive_type == DRIVE_TYPE_8050) ? 1 : 2;
                    /* 0 = 4040 (2A), 1 = 8x80 (2C) drive type */
                    sysfdc->buffer[0xea] = 1;
                    sysfdc->buffer[0xee] = 5; /* 3 for 4040, 5 for 8x50 */
                    sysfdc->buffer[0] = 3;    /* 5 for 4040, 3 for 8x50 */

                    sysfdc->fdc_state = FDC_RUN;
                    sysfdc->alarm_clk = rclk + 10000;
                } else {
                    sysfdc->alarm_clk = rclk + 2000;
                }
            } else if (DOS_IS_90(sysfdc->drive_type)) {
                if (sysfdc->buffer[0] == 0) {
                    /* emulate routine written to buffer RAM */
                    sysfdc->buffer[0xa1] = 0x00;
                    sysfdc->buffer[0xa0] = 0x01;
                    sysfdc->buffer[0xa2] = 0x01;
                    /* disk geometry */
                    /* heads */
                    if (sysfdc->image) {
                        sysfdc->buffer[0x9d] = sysfdc->image->sectors >> 5;
                    } else {
                        sysfdc->buffer[0x9d] = 1;
                    }
                    sysfdc->buffer[0x9b] = sysfdc->buffer[0x9d] - 1;
                    /* sectors */
                    sysfdc->buffer[0x9e] = 32;
                    sysfdc->buffer[0x9c] = 32 - 1;
                    /* tracks */
                    if (sysfdc->image) {
                        sysfdc->buffer[0x9a] = sysfdc->image->tracks;
                    } else {
                        sysfdc->buffer[0x9a] = 0;
                    }
                    /* other */
                    sysfdc->buffer[0x9f] = 4;
                    sysfdc->buffer[0] = 1;

                    sysfdc->fdc_state = FDC_RUN;
                    sysfdc->alarm_clk = rclk + 10000;
                } else {
                    sysfdc->alarm_clk = rclk + 2000;
                }
            } else if (DOS_IS_40(sysfdc->drive_type)
                || DOS_IS_30(sysfdc->drive_type)
                ) {
                if (sysfdc->buffer[0] == 0) {
                    sysfdc->buffer[0] = 0x0f;
                    sysfdc->fdc_state = FDC_RUN;
                    sysfdc->alarm_clk = rclk + 10000;
                } else {
                    sysfdc->alarm_clk = rclk + 2000;
                }
            }
            alarm_set(sysfdc->fdc_alarm, sysfdc->alarm_clk);
            break;
        case FDC_RUN:
            /* do not do this for D9090/60 */
            if (!DOS_IS_90(sysfdc->drive_type)) {
                /* check write protect switch */
                if (sysfdc->wps_change) {
                    sysfdc->buffer[0xA6] = 1;
                    sysfdc->wps_change--;
#ifdef FDC_DEBUG
                    log_message(fdc_log, "Detect Unit %u Drive 0 wps change",
                                fnum + 8);
#endif
                }
                if (sysfdc->num_drives == 2) {
                    if (imgfdc->wps_change) {
                        sysfdc->buffer[0xA6 + 1] = 1;
                        imgfdc->wps_change--;
#ifdef FDC_DEBUG
                        log_message(fdc_log, "Detect Unit %u Drive 1 wps change",
                                    fnum + 8);
#endif
                    }
                }
            }
            /* check buffers */
            for (i = 14; i >= 0; i--) {
                /* job there? */
                if (sysfdc->buffer[i + 3] > 127) {
                    /* pointer to buffer/block header:
                        +0 = ID1
                        +1 = ID2
                        +2 = Track
                        +3 = Sector
                    */
                    j = 0x21 + (i << 3);
#ifdef FDC_DEBUG
                    log_message(fdc_log, "D/Buf %u/%d: Job code %02x t:%02d s:%02d", fnum, i, sysfdc->buffer[i + 3],
                                sysfdc->buffer[j + 2], sysfdc->buffer[j + 3]);
#endif
                    sysfdc->buffer[i + 3] =
                        fdc_do_job(fnum,                        /* FDC# */
                                   i,                           /* buffer# */
                                   (unsigned int)sysfdc->buffer[i + 3] & 1,
                                   /* drive */
                                   (uint8_t)(sysfdc->buffer[i + 3] & 0xfe),
                                   /* job code */
                                   &(sysfdc->buffer[j])       /* header */
                                   );
                }
            }
            /* do not do this for D9090/60 */
            if (!DOS_IS_90(sysfdc->drive_type)) {
                /* check "move head", by half tracks I guess... */
                for (i = 0; i < 2; i++) {
                    if (sysfdc->buffer[i + 0xa1]) {
#ifdef FDC_DEBUG
                        log_message(fdc_log, "D %u: move head %d",
                                    fnum, sysfdc->buffer[i + 0xa1]);
#endif
                        sysfdc->buffer[i + 0xa1] = 0;
                    }
                }
                sysfdc->alarm_clk = rclk + 30000;
            } else {
                sysfdc->alarm_clk = rclk + 2000;
            }
            /* job loop */
            break;
    }
    alarm_set(sysfdc->fdc_alarm, sysfdc->alarm_clk);
}

/* FIXME: hack, because 0x4000 is only ok for 1001/8050/8250.
   fdc.c:fdc_do_job() adds an offset for 2040/3040/4040 by itself :-(
   Why donlly get a table for that...! */
void fdc_init(diskunit_context_t *drv)
{
    unsigned int fnum = drv->mynumber;
    uint8_t *buffermem = drv->drive_ram + 0x100;
    uint8_t *ipromp = &(drv->rom[0x4000]);
    char *buffer;

    fdc_t *sysfdc = &fdc[fnum][0];
    fdc_t *imgfdc = &fdc[fnum][1];

    sysfdc->buffer = buffermem;
    sysfdc->iprom = ipromp;

    /* defensive. should not be used so trigger segfault */
    imgfdc->buffer = NULL;
    imgfdc->iprom = NULL;

    if (fdc_log == LOG_ERR) {
        fdc_log = log_open("fdc");
    }

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_init(drive %u)", fnum);
#endif

    buffer = lib_msprintf("fdc%u", drv->mynumber);
    sysfdc->fdc_alarm = alarm_new(drv->cpu->alarm_context, buffer, int_fdc,
                                    drv);
    lib_free(buffer);
}

/************************************************************************/

int fdc_attach_image(disk_image_t *image, unsigned int unit, unsigned int drive)
{
    fdc_t *sysfdc, *imgfdc;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_attach_image(image=%p, unit=%u, drive=%u)",
                image, unit, drive);
#endif

    if (unit < 8 || unit >= 8 + NUM_DISK_UNITS) {
        return -1;
    }
    if (drive > 1) {
        return -1;
    }

    sysfdc = &fdc[unit - 8][0];
    imgfdc = &fdc[unit - 8][drive];

    /* FIXME: hack - we need to save the image to be able to re-attach
       when the disk drive type changes, in particular from the initial
       DRIVE_TYPE_NONE to a proper drive. */
    imgfdc->realimage = image;

    if (sysfdc->drive_type == DRIVE_TYPE_NONE) {
#ifdef FDC_DEBUG
        log_message(fdc_log, "Could not attach image type %u to disk #%u without type.",
                            image->type, unit);
#endif
        return -1;
    }

    if (sysfdc->drive_type == DRIVE_TYPE_8050
        || sysfdc->drive_type == DRIVE_TYPE_8250
        || sysfdc->drive_type == DRIVE_TYPE_1001) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D80:
            case DISK_IMAGE_TYPE_D82:
                disk_image_attach_log(image, fdc_log, unit, drive);
                break;
            default:
#ifdef FDC_DEBUG
                log_message(fdc_log, "Could not attach image type %u to disk %u.",
                            image->type, sysfdc->drive_type);
#endif
                return -1;
        }
    } else if (sysfdc->drive_type == DRIVE_TYPE_9000) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D90:
                disk_image_attach_log(image, fdc_log, unit, drive);
                break;
            default:
#ifdef FDC_DEBUG
                log_message(fdc_log, "Could not attach image type %u to disk %u.",
                            image->type, sysfdc->drive_type);
#endif
                return -1;
        }
    } else {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D64:
            case DISK_IMAGE_TYPE_D67:
            case DISK_IMAGE_TYPE_G64:
            case DISK_IMAGE_TYPE_G71:
            case DISK_IMAGE_TYPE_P64:
#ifdef HAVE_X64_IMAGE
            case DISK_IMAGE_TYPE_X64:
#endif
                disk_image_attach_log(image, fdc_log, unit, drive);
                break;
            default:
#ifdef FDC_DEBUG
                log_message(fdc_log, "Could not attach image type %u to disk %u.",
                            image->type, sysfdc->drive_type);
#endif
                return -1;
        }
    }

    imgfdc->wps_change += 2;
    imgfdc->image = image;
    return 0;
}

int fdc_detach_image(disk_image_t *image, unsigned int unit, unsigned int drive)
{
    fdc_t *sysfdc, *imgfdc;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_detach_image(image=%p, unit=%u, drive=%u)",
                image, unit, drive);
#endif

    if (image == NULL || unit < 8 || unit >= (8 + NUM_DISK_UNITS)) {
        return -1;
    }
    if (drive > 1) {
        return -1;
    }

    sysfdc = &fdc[unit - 8][0];
    imgfdc = &fdc[unit - 8][drive];

    imgfdc->realimage = NULL;

    if (sysfdc->drive_type == DRIVE_TYPE_8050
        || sysfdc->drive_type == DRIVE_TYPE_8250
        || sysfdc->drive_type == DRIVE_TYPE_1001) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D80:
            case DISK_IMAGE_TYPE_D82:
                disk_image_detach_log(image, fdc_log, unit, drive);
                break;
            default:
                return -1;
        }
    } else if (sysfdc->drive_type == DRIVE_TYPE_9000) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D90:
                disk_image_detach_log(image, fdc_log, unit, drive);
                break;
            default:
                return -1;
        }
    } else {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D64:
            case DISK_IMAGE_TYPE_D67:
            case DISK_IMAGE_TYPE_G64:
            case DISK_IMAGE_TYPE_G71:
            case DISK_IMAGE_TYPE_P64:
#ifdef HAVE_X64_IMAGE
            case DISK_IMAGE_TYPE_X64:
#endif
                disk_image_detach_log(image, fdc_log, unit, drive);
                break;
            default:
                return -1;
        }
    }

    imgfdc->wps_change += 2;
    imgfdc->image = NULL;
    return 0;
}

/************************************************************************/

/* FDC* snapshot module format:

   type  | name     | description
   ------------------------------
   BYTE  | STATE    | FDC state
   DWORD | CLK      | clk ticks till next fdc invocation
   BYTE  | NDRV     | number of drives (1 or 2)
   BYTE  | LTRACK0  | last track
   BYTE  | LSECTOR0 | last sector
   BYTE  | LTRACK1  | last track (if ndrv == 2)
   BYTE  | LSECTOR1 | last sector (if ndrv == 2)
 */

#define SNAP_MAJOR      0
#define SNAP_MINOR      0

int fdc_snapshot_write_module(snapshot_t *p, int fnum)
{
    snapshot_module_t *m;
    char *name;

    fdc_t *sysfdc = &fdc[fnum][0];
    /* fdc_t *imgfdc = &fdc[fnum][1]; */

    if (sysfdc->fdc_state == FDC_UNUSED) {
        return 0;
    }

    name = lib_msprintf("FDC%i", fnum);

    m = snapshot_module_create(p, name, SNAP_MAJOR, SNAP_MINOR);

    lib_free(name);

    if (m == NULL) {
        return -1;
    }

    /* TODO: drive 1 */
    if (0
        || SMW_B(m, (uint8_t)(sysfdc->fdc_state)) < 0
        /* clk till next invocation */
        || SMW_DW(m, (uint32_t)(sysfdc->alarm_clk - diskunit_clk[fnum])) < 0
        /* number of drives - so far 1 only */
        || SMW_B(m, 1) < 0
        /* last accessed track/sector */
        || SMW_B(m, ((uint8_t)(sysfdc->last_track))) < 0
        || SMW_B(m, ((uint8_t)(sysfdc->last_sector))) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int fdc_snapshot_read_module(snapshot_t *p, int fnum)
{
    uint8_t vmajor, vminor;
    uint8_t byte, ndrv;
    uint32_t dword;
    snapshot_module_t *m;
    char *name;
    uint8_t ltrack, lsector;

    fdc_t *sysfdc = &fdc[fnum][0];
    /*fdc_t *imgfdc = &fdc[fnum][1];*/

    name = lib_msprintf("FDC%d", fnum);

    m = snapshot_module_open(p, name, &vmajor, &vminor);
    lib_free(name);

    if (m == NULL) {
        log_message(fdc_log, "Could not find snapshot module %s", name);
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* TODO: drive 1 */
    if (0
        || SMR_B(m, &byte) < 0
        /* clk till next invocation */
        || SMR_DW(m, &dword) < 0
        /* number of drives - so far 1 only */
        || SMR_B(m, &ndrv) < 0
        || SMR_B(m, &ltrack) < 0
        || SMR_B(m, &lsector) < 0) {
        goto fail;
    }

    if (byte > FDC_LAST_STATE) {
        goto fail;
    }
    sysfdc->fdc_state = byte;

    sysfdc->alarm_clk = diskunit_clk[fnum] + dword;
    alarm_set(sysfdc->fdc_alarm, sysfdc->alarm_clk);

    /* last accessed track/sector */
    sysfdc->last_track = ltrack;
    sysfdc->last_sector = lsector;

    if (ndrv > 1) {
        /* ignore drv 0 values */
        SMR_B(m, &byte);
        SMR_B(m, &byte);
    }

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
