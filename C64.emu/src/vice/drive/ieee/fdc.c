/*
 * fdc.c - 1001/8x50 FDC emulation
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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
#include "clkguard.h"
#include "diskimage.h"
#include "drive-check.h"
#include "drive.h"
#include "drivetypes.h"
#include "fdc.h"
#include "lib.h"
#include "log.h"
#include "snapshot.h"
#include "types.h"


#undef FDC_DEBUG

#define DOS_IS_80(type)  (type == DRIVE_TYPE_8050 || type == DRIVE_TYPE_8250 || type == DRIVE_TYPE_1001)
#define DOS_IS_40(type)  (type == DRIVE_TYPE_4040)
#define DOS_IS_30(type)  (type == DRIVE_TYPE_3040)
#define DOS_IS_20(type)  (type == DRIVE_TYPE_2040)

/************************************************************************/

#define NUM_FDC DRIVE_NUM

static log_t fdc_log = LOG_ERR;

typedef struct fdc_t {
    int          fdc_state;
    alarm_t      *fdc_alarm;
    CLOCK        alarm_clk;
    BYTE         *buffer;
    BYTE         *iprom;
    unsigned int drive_type;
    unsigned int num_drives;
    unsigned int last_track;
    unsigned int last_sector;
    int          wps_change;     /* if not zero, toggle wps and decrement */
    disk_image_t *image;
    disk_image_t *realimage;
} fdc_t;

static fdc_t fdc[NUM_FDC];

void fdc_reset(unsigned int fnum, unsigned int drive_type)
{
    fdc_t *thefdc = &fdc[fnum];
    int drive1 = mk_drive1(fnum);
    disk_image_t *saved_image0, *saved_image1;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_reset: drive %d type=%d\n", fnum, drive_type);
#endif

    saved_image0 = fdc[fnum].realimage;
    saved_image1 = NULL;

    /* detach disk images */
    if (thefdc->image) {
        thefdc->wps_change = 0;
        fdc_detach_image(thefdc->image, fnum + 8);
    }
    if (thefdc->num_drives == 2) {
        saved_image1 = fdc[drive1].realimage;
        if (fdc[drive1].image) {
            fdc[drive1].wps_change = 0;
            fdc_detach_image(fdc[drive1].image, drive1 + 8);
        }
    }

    if (drive_check_old(drive_type)) {
        thefdc->drive_type = drive_type;
        thefdc->num_drives = is_drive1(fnum) ? 1 :
                             drive_check_dual(drive_type) ? 2 : 1;
        thefdc->fdc_state = FDC_RESET0;
        alarm_set(thefdc->fdc_alarm, drive_clk[fnum] + 20);
    } else {
        thefdc->drive_type = DRIVE_TYPE_NONE;
        alarm_unset(thefdc->fdc_alarm);
        thefdc->fdc_state = FDC_UNUSED;
        thefdc->num_drives = 0;
    }

    /* re-attach disk images */
    if (saved_image0) {
#ifdef FDC_DEBUG
        printf("ieee/fdc.c:fdc_reset dev %d type %d drive 0 re-attach image %p (drive: %p)\n", fnum + 8, drive_type, saved_image0, drive_context[fnum]->drive->image);
#endif
        fdc_attach_image(saved_image0, fnum + 8);
    }
    if (saved_image1) {
#ifdef FDC_DEBUG
        printf("ieee/fdc.c:fdc_reset dev %d type %d drive 1 re-attach image %p (drive: %p)\n", fnum + 8, drive_type, saved_image0, drive_context[drive1]->drive->image);
#endif
        fdc_attach_image(saved_image1, drive1 + 8);
    }
}

/*****************************************************************************
 * Format a disk in DOS1 format
 */

static BYTE fdc_do_format_D20(fdc_t *fdc, unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, BYTE *header)
{
    int i;
    int ret;
    BYTE rc = 0;
    disk_addr_t dadr;
    BYTE sector_data[256];

    if (!memcmp(fdc[fnum].iprom + 0x2040, &fdc[fnum].buffer[0x100], 0x200)) {
        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        static const unsigned int nsecs[] = { 21, 20, 18, 17 };
        unsigned int ntracks, nsectors = 0;

        /*
        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        int ntracks, nsectors = 0;
        */
#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   track=%d, sector=%d", track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
#endif
        if (fdc[dnr].image->read_only) {
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
            log_message(fdc_log, "   track %d, -> %d sectors",
                        dadr.track, nsectors);
#endif
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(fdc[dnr].image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%d S:%d on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(dnr + 8, header);
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * Format a disk in DOS2 format
 */

static BYTE fdc_do_format_D40(fdc_t *fdc, unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, BYTE *header)
{
    int i;
    int ret;
    BYTE rc = 0;
    disk_addr_t dadr;
    BYTE sector_data[256];

    if (!memcmp(fdc[fnum].iprom + 0x1000, &fdc[fnum].buffer[0x100], 0x200)) {
        static const unsigned int sectorchangeat[4] = { 0, 17, 24, 30 };
        unsigned int ntracks, nsectors = 0;

#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   secs per track: %d %d %d %d",
                    fdc[fnum].buffer[0x99], fdc[fnum].buffer[0x9a],
                    fdc[fnum].buffer[0x9b], fdc[fnum].buffer[0x9c]);
        log_message(fdc_log, "   track=%d, sector=%d",
                    track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
#endif
        if (fdc[dnr].image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = 35;

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            for (i = 3; i >= 0; i--) {
                if (dadr.track > sectorchangeat[i]) {
                    nsectors = fdc[fnum].buffer[0x99 + 3 - i];
                    break;
                }
            }
#ifdef FDC_DEBUG
            log_message(fdc_log, "   track %d, -> %d sectors",
                        dadr.track, nsectors);
#endif
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(fdc[dnr].image, sector_data, &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%d S:%d on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(dnr + 8, header);
    }
    if (!rc) {
        rc = FDC_ERR_OK;
    }

    return rc;
}

/*****************************************************************************
 * Format a disk in DOS2/80 track format
 */

static BYTE fdc_do_format_D80(fdc_t *fdc, unsigned int fnum, unsigned int dnr,
                              unsigned int track, unsigned int sector,
                              int buf, BYTE *header)
{
    int i;
    int ret;
    BYTE rc = 0;
    disk_addr_t dadr;
    BYTE sector_data[256];

    if (!memcmp(fdc[fnum].iprom, &fdc[fnum].buffer[0x100], 0x300)) {
        unsigned int ntracks, nsectors = 0;
        /* detected format code */
#ifdef FDC_DEBUG
        log_message(fdc_log, "format code: ");
        log_message(fdc_log, "   track for zones side 0: %d %d %d %d",
                    fdc[fnum].buffer[0xb0], fdc[fnum].buffer[0xb1],
                    fdc[fnum].buffer[0xb2], fdc[fnum].buffer[0xb3]);
        log_message(fdc_log, "   track for zones side 1: %d %d %d %d",
                    fdc[fnum].buffer[0xb4], fdc[fnum].buffer[0xb5],
                    fdc[fnum].buffer[0xb6], fdc[fnum].buffer[0xb7]);
        log_message(fdc_log, "   secs per track: %d %d %d %d",
                    fdc[fnum].buffer[0x99], fdc[fnum].buffer[0x9a],
                    fdc[fnum].buffer[0x9b], fdc[fnum].buffer[0x9c]);
        log_message(fdc_log, "   vars: 870=%d 873=%d 875=%d",
                    fdc[fnum].buffer[0x470], fdc[fnum].buffer[0x473],
                    fdc[fnum].buffer[0x475]);
        log_message(fdc_log, "   track=%d, sector=%d",
                    track, sector);
        log_message(fdc_log, "   id=%02x,%02x (%c%c)",
                    header[0], header[1], header[0], header[1]);
        log_message(fdc_log, "   sides=%d",
                    fdc[fnum].buffer[0xac]);
#endif
        if (fdc[dnr].image->read_only) {
            rc = FDC_ERR_WPROT;
            return rc;
        }
        ntracks = (fdc[fnum].buffer[0xac] > 1) ? 154 : 77;

        memset(sector_data, 0, 256);

        for (ret = 0, dadr.track = 1; ret == 0 && dadr.track <= ntracks; dadr.track++) {
            if (dadr.track < 78) {
                for (i = 3; i >= 0; i--) {
                    if (dadr.track < fdc[fnum].buffer[0xb0 + i]) {
                        nsectors = fdc[fnum].buffer[0x99 + i];
                        break;
                    }
                }
            } else {
                for (i = 3; i >= 0; i--) {
                    if (dadr.track < fdc[fnum].buffer[0xb4 + i]) {
                        nsectors = fdc[fnum].buffer[0x99 + i];
                        break;
                    }
                }
            }
            for (dadr.sector = 0; dadr.sector < nsectors; dadr.sector++) {
                ret = disk_image_write_sector(fdc[dnr].image, sector_data,
                                              &dadr);
                if (ret < 0) {
                    log_error(LOG_DEFAULT,
                              "Could not update T:%d S:%d on disk image.",
                              dadr.track, dadr.sector);
                    rc = FDC_ERR_DCHECK;
                    break;
                }
            }
        }

        file_system_bam_set_disk_id(dnr + 8, header);
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
static BYTE fdc_do_job_(unsigned int fnum, int buf,
                        unsigned int drv, BYTE job, BYTE *header);
#endif

static BYTE fdc_do_job(unsigned int fnum, int buf,
                       unsigned int drv, BYTE job, BYTE *header)
{
#ifdef FDC_DEBUG
    BYTE retval = fdc_do_job_(fnum, buf, drv, job, header);
    const char *jobs[] =
        { "Read", "Write", "Verify", "Seek", "Bump", "Jump",
          "ExecWhenRdy", "--" };
    const char *errors[] =
        { "--", "OK", "HEADER", "SYNC", "NOBLOCK",
          "DCHECK", "???", "VERIFY", "WPROT", "HCHECK",
          "BLENGTH", "ID", "FSPEED", "DRIVE",
          "DECODE" };

    log_message(fdc_log, "  fdc_do_job (%s %02x) -> %02x (%s)\n",
                jobs[(job >> 4) & 7], job, retval,
                (retval <= 16) ? errors[retval] : "Unknown");
    return retval;
}

static BYTE fdc_do_job_(unsigned int fnum, int buf,
                        unsigned int drv, BYTE job, BYTE *header)
{
#endif
    unsigned int dnr;
    BYTE rc;
    int ret;
    int i;
    disk_addr_t dadr;
    BYTE *base;
    BYTE sector_data[256];
    BYTE disk_id[2];
    drive_t *drive;

    dadr.track = header[2];
    dadr.sector = header[3];

    /* determine drive/disk image to use */
    if (drv < fdc[fnum].num_drives) {
        dnr = fnum + drv;
    } else {
        /* drive 1 on a single disk drive */
        return FDC_ERR_SYNC;
    }

    rc = 0;
    base = &(fdc[fnum].buffer[(buf + 1) << 8]);

#ifdef FDC_DEBUG
    log_message(fdc_log, "do job %02x, buffer %d ($%04x): d%d t%d s%d, "
                "image=%p, type=%04d",
                job, buf, (buf + 1) << 8, dnr, dadr.track, dadr.sector,
                fdc[dnr].image,
                fdc[dnr].image ? fdc[dnr].image->type : 0);
#endif

    if (fdc[dnr].image == NULL && job != 0xd0) {
#ifdef FDC_DEBUG
        log_message(fdc_log, "dnr=%d, image=NULL -> no disk!", dnr);
#endif
        return FDC_ERR_SYNC;
    }

    file_system_bam_get_disk_id(dnr + 8, disk_id);

    switch (job) {
        case 0x80:        /* read */
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
                rc = FDC_ERR_ID;
                break;
            }
            ret = disk_image_read_sector(fdc[dnr].image, sector_data, &dadr);
            if (ret < 0) {
                log_error(LOG_DEFAULT,
                          "Cannot read T:%d S:%d from disk image.",
                          dadr.track, dadr.sector);
                rc = FDC_ERR_DRIVE;
            } else {
                memcpy(base, sector_data, 256);
                rc = FDC_ERR_OK;
            }
            break;
        case 0x90:        /* write */
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
                rc = FDC_ERR_ID;
                break;
            }
            if (fdc[dnr].image->read_only) {
                rc = FDC_ERR_WPROT;
                break;
            }
            memcpy(sector_data, base, 256);
            ret = disk_image_write_sector(fdc[dnr].image, sector_data, &dadr);
            if (ret < 0) {
                log_error(LOG_DEFAULT,
                          "Could not update T:%d S:%d on disk image.",
                          dadr.track, dadr.sector);
                rc = FDC_ERR_DRIVE;
            } else {
                rc = FDC_ERR_OK;
            }
            break;
        case 0xA0:        /* verify */
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
                rc = FDC_ERR_ID;
                break;
            }
            ret = disk_image_read_sector(fdc[dnr].image, sector_data, &dadr);
            if (ret < 0) {
                log_error(LOG_DEFAULT,
                          "Cannot read T:%d S:%d from disk image.",
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
            header[0] = disk_id[0];
            header[1] = disk_id[1];
            /* header[2] = fdc[dnr].last_track; */
            dadr.track = header[2];
            header[3] = 1;
            rc = FDC_ERR_OK;
            break;
        case 0xC0:        /* bump (to track 0 and back to 18?) */
            dadr.track = 1;
            if (DOS_IS_20(fdc[fnum].drive_type)) {
                header[2] = 18;
            }
            rc = FDC_ERR_OK;
            break;
        case 0xD0:        /* jump to buffer - but we do not emulate FDC CPU */
#ifdef FDC_DEBUG
            log_message(fdc_log, "exec buffer %d ($%04x): %02x %02x %02x %02x %02x",
                        buf, (buf + 1) << 8,
                        base[0], base[1], base[2], base[3]
                        );
#endif
            if (DOS_IS_40(fdc[fnum].drive_type)
                || DOS_IS_30(fdc[fnum].drive_type)) {
                if (!memcmp(fdc[fnum].iprom + 0x12f8, &fdc[fnum].buffer[0x100],
                            0x100)) {
                    fdc[fnum].fdc_state = FDC_RESET2;
                    return 0;
                }
            }
            if (DOS_IS_80(fdc[fnum].drive_type)) {
                static const BYTE jumpseq[] = {
                    0x78, 0x6c, 0xfc, 0xff
                };
                if (!memcmp(jumpseq, &fdc[fnum].buffer[0x100], 4)) {
                    fdc[fnum].fdc_state = FDC_RESET0;
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
            if (DOS_IS_80(fdc[fnum].drive_type)) {
                rc = fdc_do_format_D80(fdc, fnum, dnr, dadr.track, dadr.sector, buf, header);
            } else
            if (DOS_IS_40(fdc[fnum].drive_type)
                || DOS_IS_30(fdc[fnum].drive_type)) {
                rc = fdc_do_format_D40(fdc, fnum, dnr, dadr.track, dadr.sector, buf, header);
            } else
            if (DOS_IS_20(fdc[fnum].drive_type)) {
                rc = fdc_do_format_D20(fdc, fnum, dnr, dadr.track, dadr.sector, buf, header);
            } else {
                rc = FDC_ERR_DRIVE;
            }
            break;
        case 0xF0:
            if (header[0] != disk_id[0] || header[1] != disk_id[1]) {
                rc = FDC_ERR_ID;
                break;
            }
            /* try to read block header from disk */
            rc = FDC_ERR_OK;
            break;
    }

    drive = drive_context[dnr]->drive;
    drive->current_half_track = 2 * dadr.track;
    fdc[dnr].last_track = dadr.track;
    fdc[dnr].last_sector = dadr.sector;

    return rc;
}


static void int_fdc(CLOCK offset, void *data)
{
    CLOCK rclk;
    int i, j;
    drive_t *drive;
    unsigned int fnum;
    drive_context_t *drv = (drive_context_t *)data;

    fnum = drv->mynumber;
    rclk = drive_clk[fnum] - offset;

#ifdef FDC_DEBUG
    if (fdc[fnum].fdc_state < FDC_RUN) {
        static int old_state[NUM_FDC] = { -1, -1 };
        if (fdc[fnum].fdc_state != old_state[fnum])
            log_message(fdc_log, "int_fdc%d %d: state=%d\n",
                        fnum, rclk, fdc[fnum].fdc_state);
        }
        old_state[fnum] = fdc[fnum].fdc_state;
    }
#endif

    switch (fdc[fnum].fdc_state) {
        case FDC_RESET0:
            drive = drive_context[fnum]->drive;
            if (DOS_IS_80(fdc[fnum].drive_type)) {
                drive->current_half_track = 2 * 38;
                fdc[fnum].buffer[0] = 2;
            } else {
                drive->current_half_track = 2 * 18;
                fdc[fnum].buffer[0] = 0x3f;
            }

            if (DOS_IS_20(fdc[fnum].drive_type)) {
                fdc[fnum].fdc_state = FDC_RUN;
            } else {
                fdc[fnum].fdc_state++;
            }
            fdc[fnum].alarm_clk = rclk + 2000;
            alarm_set(fdc[fnum].fdc_alarm, fdc[fnum].alarm_clk);
            break;
        case FDC_RESET1:
            if (DOS_IS_80(fdc[fnum].drive_type)) {
                if (fdc[fnum].buffer[0] == 0) {
                    fdc[fnum].buffer[0] = 1;
                    fdc[fnum].fdc_state++;
                }
            } else {
                if (fdc[fnum].buffer[3] == 0xd0) {
                    fdc[fnum].buffer[3] = 0;
                    fdc[fnum].fdc_state++;
                }
            }
            fdc[fnum].alarm_clk = rclk + 2000;
            alarm_set(fdc[fnum].fdc_alarm, fdc[fnum].alarm_clk);
            break;
        case FDC_RESET2:
            if (DOS_IS_80(fdc[fnum].drive_type)) {
                if (fdc[fnum].buffer[0] == 0) {
                    /* emulate routine written to buffer RAM */
                    fdc[fnum].buffer[1] = 0x0e;
                    fdc[fnum].buffer[2] = 0x2d;
                    /* number of sides on disk drive */
                    fdc[fnum].buffer[0xac] =
                        (fdc[fnum].drive_type == DRIVE_TYPE_8050) ? 1 : 2;
                    /* 0 = 4040 (2A), 1 = 8x80 (2C) drive type */
                    fdc[fnum].buffer[0xea] = 1;
                    fdc[fnum].buffer[0xee] = 5; /* 3 for 4040, 5 for 8x50 */
                    fdc[fnum].buffer[0] = 3;    /* 5 for 4040, 3 for 8x50 */

                    fdc[fnum].fdc_state = FDC_RUN;
                    fdc[fnum].alarm_clk = rclk + 10000;
                } else {
                    fdc[fnum].alarm_clk = rclk + 2000;
                }
            } else
            if (DOS_IS_40(fdc[fnum].drive_type)
                || DOS_IS_30(fdc[fnum].drive_type)
                ) {
                if (fdc[fnum].buffer[0] == 0) {
                    fdc[fnum].buffer[0] = 0x0f;
                    fdc[fnum].fdc_state = FDC_RUN;
                    fdc[fnum].alarm_clk = rclk + 10000;
                } else {
                    fdc[fnum].alarm_clk = rclk + 2000;
                }
            }
            alarm_set(fdc[fnum].fdc_alarm, fdc[fnum].alarm_clk);
            break;
        case FDC_RUN:
            /* check write protect switch */
            if (fdc[fnum].wps_change) {
                fdc[fnum].buffer[0xA6] = 1;
                fdc[fnum].wps_change--;
#ifdef FDC_DEBUG
                log_message(fdc_log, "Detect Unit %d Drive %d wps change",
                            fnum + 8, fnum);
#endif
            }
            if (fdc[fnum].num_drives == 2) {
                if (fdc[mk_drive1(fnum)].wps_change) {
                    fdc[fnum].buffer[0xA6 + 1] = 1;
                    fdc[mk_drive1(fnum)].wps_change--;
#ifdef FDC_DEBUG
                    log_message(fdc_log, "Detect Unit %d Drive 1 wps change",
                                fnum + 8);
#endif
                }
            }

            /* check buffers */
            for (i = 14; i >= 0; i--) {
                /* job there? */
                if (fdc[fnum].buffer[i + 3] > 127) {
                    /* pointer to buffer/block header:
                        +0 = ID1
                        +1 = ID2
                        +2 = Track
                        +3 = Sector
                    */
                    j = 0x21 + (i << 3);
#ifdef FDC_DEBUG
                    log_message(fdc_log, "D/Buf %d/%x: Job code %02x t:%02d s:%02d", fnum, i, fdc[fnum].buffer[i + 3],
                                fdc[fnum].buffer[j + 2], fdc[fnum].buffer[j + 3]);
#endif
                    fdc[fnum].buffer[i + 3] =
                        fdc_do_job(fnum,                        /* FDC# */
                                   i,                           /* buffer# */
                                   (unsigned int)fdc[fnum].buffer[i + 3] & 1,
                                   /* drive */
                                   (BYTE)(fdc[fnum].buffer[i + 3] & 0xfe),
                                   /* job code */
                                   &(fdc[fnum].buffer[j])       /* header */
                                   );
                }
            }
            /* check "move head", by half tracks I guess... */
            for (i = 0; i < 2; i++) {
                if (fdc[fnum].buffer[i + 0xa1]) {
#ifdef FDC_DEBUG
                    log_message(fdc_log, "D %d: move head %d",
                                fnum, fdc[fnum].buffer[i + 0xa1]);
#endif
                    fdc[fnum].buffer[i + 0xa1] = 0;
                }
            }
            fdc[fnum].alarm_clk = rclk + 30000;
            alarm_set(fdc[fnum].fdc_alarm, fdc[fnum].alarm_clk);
            /* job loop */
            break;
    }
}

static void clk_overflow_callback(CLOCK sub, void *data)
{
    unsigned int fnum;

    fnum = vice_ptr_to_uint(data);

    if (fdc[fnum].fdc_state != FDC_UNUSED) {
        if (fdc[fnum].alarm_clk > sub) {
            fdc[fnum].alarm_clk -= sub;
        } else {
            fdc[fnum].alarm_clk = 0;
        }
    }
}

/* FIXME: hack, because 0x4000 is only ok for 1001/8050/8250.
   fdc.c:fdc_do_job() adds an offset for 2040/3040/4040 by itself :-(
   Why donlly get a table for that...! */
void fdc_init(drive_context_t *drv)
{
    unsigned int fnum = drv->mynumber;
    BYTE *buffermem = drv->drive->drive_ram + 0x100;
    BYTE *ipromp = &(drv->drive->rom[0x4000]);
    char *buffer;

    fdc[fnum].buffer = buffermem;
    fdc[fnum].iprom = ipromp;

    if (fdc_log == LOG_ERR) {
        fdc_log = log_open("fdc");
    }

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_init(drive %d)", fnum);
#endif

    buffer = lib_msprintf("fdc%i", drv->mynumber);
    fdc[fnum].fdc_alarm = alarm_new(drv->cpu->alarm_context, buffer, int_fdc,
                                    drv);
    lib_free(buffer);

    clk_guard_add_callback(drv->cpu->clk_guard, clk_overflow_callback,
                           uint_to_void_ptr(drv->mynumber));
}

/************************************************************************/

int fdc_attach_image(disk_image_t *image, unsigned int unit)
{
    int drive_no, imgno;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_attach_image(image=%p, unit=%d)",
                image, unit);
#endif

    if (unit < 8 || unit >= 8 + DRIVE_NUM) {
        return -1;
    }

    {
        int drive0 = mk_drive0(unit - 8);

        if (fdc[drive0].num_drives == 2) {
            drive_no = drive0;
        } else {
            drive_no = unit - 8;
        }
    }

    imgno = unit - 8;

    /* FIXME: hack - we need to save the image to be able to re-attach
       when the disk drive type changes, in particular from the initial
       DRIVE_TYPE_NONE to a proper drive. */
    fdc[imgno].realimage = image;

    if (fdc[drive_no].drive_type == DRIVE_TYPE_NONE) {
        return -1;
    }

    if (fdc[drive_no].drive_type == DRIVE_TYPE_8050
        || fdc[drive_no].drive_type == DRIVE_TYPE_8250
        || fdc[drive_no].drive_type == DRIVE_TYPE_1001) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D80:
            case DISK_IMAGE_TYPE_D82:
                disk_image_attach_log(image, fdc_log, unit);
                break;
            default:
#ifdef FDC_DEBUG
                log_message(fdc_log, "Could not attach image type %d to disk %d.",
                            image->type, fdc[drive_no].drive_type);
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
            case DISK_IMAGE_TYPE_X64:
                disk_image_attach_log(image, fdc_log, unit);
                break;
            default:
#ifdef FDC_DEBUG
                log_message(fdc_log, "Could not attach image type %d to disk %d.",
                            image->type, fdc[drive_no].drive_type);
#endif
                return -1;
        }
    }

    fdc[imgno].wps_change += 2;
    fdc[imgno].image = image;
    return 0;
}

int fdc_detach_image(disk_image_t *image, unsigned int unit)
{
    int drive_no, imgno;

#ifdef FDC_DEBUG
    log_message(fdc_log, "fdc_detach_image(image=%p, unit=%d)",
                image, unit);
#endif

    if (image == NULL || unit < 8 || unit >= (8 + DRIVE_NUM)) {
        return -1;
    }

    {
        int drive0 = mk_drive0(unit - 8);

        if (fdc[drive0].num_drives == 2) {
            drive_no = drive0;
        } else {
            drive_no = unit - 8;
        }
    }

    imgno = unit - 8;

    fdc[imgno].realimage = NULL;

    if (fdc[drive_no].drive_type == DRIVE_TYPE_8050
        || fdc[drive_no].drive_type == DRIVE_TYPE_8250
        || fdc[drive_no].drive_type == DRIVE_TYPE_1001) {
        switch (image->type) {
            case DISK_IMAGE_TYPE_D80:
            case DISK_IMAGE_TYPE_D82:
                disk_image_detach_log(image, fdc_log, unit);
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
            case DISK_IMAGE_TYPE_X64:
                disk_image_detach_log(image, fdc_log, unit);
                break;
            default:
                return -1;
        }
    }

    fdc[imgno].wps_change += 2;
    fdc[imgno].image = NULL;
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

    if (fdc[fnum].fdc_state == FDC_UNUSED) {
        return 0;
    }

    name = lib_msprintf("FDC%i", fnum);

    m = snapshot_module_create(p, name, SNAP_MAJOR, SNAP_MINOR);

    lib_free(name);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)(fdc[fnum].fdc_state)) < 0
        /* clk till next invocation */
        || SMW_DW(m, (DWORD)(fdc[fnum].alarm_clk - drive_clk[fnum])) < 0
        /* number of drives - so far 1 only */
        || SMW_B(m, 1) < 0
        /* last accessed track/sector */
        || SMW_B(m, ((BYTE)(fdc[fnum].last_track))) < 0
        || SMW_B(m, ((BYTE)(fdc[fnum].last_sector))) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int fdc_snapshot_read_module(snapshot_t *p, int fnum)
{
    BYTE vmajor, vminor;
    BYTE byte, ndrv;
    DWORD dword;
    snapshot_module_t *m;
    char *name;
    BYTE ltrack, lsector;

    name = lib_msprintf("FDC%d", fnum);

    m = snapshot_module_open(p, name, &vmajor, &vminor);
    lib_free(name);

    if (m == NULL) {
        log_message(fdc_log, "Could not find snapshot module %s", name);
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > SNAP_MAJOR || vminor > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

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
    fdc[fnum].fdc_state = byte;

    fdc[fnum].alarm_clk = drive_clk[fnum] + dword;
    alarm_set(fdc[fnum].fdc_alarm, fdc[fnum].alarm_clk);

    /* last accessed track/sector */
    fdc[fnum].last_track = ltrack;
    fdc[fnum].last_sector = lsector;

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
