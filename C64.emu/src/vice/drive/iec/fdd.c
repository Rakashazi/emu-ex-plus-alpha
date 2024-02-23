/*
 * fdd.c - (M)FM floppy disk drive emulation
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
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

#include "lib.h"
#include "log.h"
#include "types.h"
#include "fdd.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "drive.h"
#include "snapshot.h"

/* 3.5" mechs are usually specified for 80 physical tracks. However, using track
   81 is usually no problem at all on any combination of mech and disk. Often
   also track 82 works. On good quality drives even track 83 is ok.
   Most importantly, track 81 is not uncommon "in the wild":
   - the FD2000/4000 ROMS write to track 81 with zeros when formatting for 1581
     disks (TODO: test and handle this without breaking the upper tracks)
   - Wheels "MakeSysDisk" puts its copyright info on track 81

   FIXME: images are always "extended" without asking

*/
#define FDD_MAX_TRACK   (80 + 3)
#define FDD_NUM_TRACKS  (80 + 3)

const int fdd_data_rates[4] = { 500, 300, 250, 1000 }; /* kbit/s */
#define INDEXLEN (16)
static void fdd_flush_raw(fd_drive_t *drv);
static uint16_t *crc1021 = NULL;

struct fd_drive_s {
    char *myname;
    int number;
    int disk_change; /* out signal */
    int write_protect; /* out signal */
    int track; /* physical head position */
    int tracks; /* total tracks in image */
    int head; /* head selected */
    int sectors; /* physical sectors per track */
    int motor; /* in signal */
    int rate; /* in signal */
    int sector_size; /* physical sector size */
    int iso;
    int gap2; /* size of gap 2 */
    int gap3; /* size of gap 3 */
    int head_invert;
    int disk_rate;
    int image_sectors;
    unsigned int index_count;
    /*int write_beyond;*/ /* flag to see if we writing D?M data to a D81 image */
    drive_t *drive;
    struct disk_image_s *image;
    struct {
        int head;
        int size;
        int track_head;
        int dirty;
        uint8_t *data;
        uint8_t *sync;
    } raw;
};

fd_drive_t *fdd_init(int num, drive_t *drive)
{
    fd_drive_t *drv = lib_malloc(sizeof(fd_drive_t));
    memset(drv, 0, sizeof(fd_drive_t));
    drv->myname = lib_msprintf("FDD%d", num);
    drv->image = NULL;
    drv->number = num & 3;
    drv->motor = 0;
    drv->track = 0;
    drv->tracks = FDD_NUM_TRACKS;
    drv->sectors = 10;
    drv->sector_size = 2;
    drv->head_invert = 1;
    drv->disk_change = 1;
    drv->write_protect = 1;
    drv->rate = 2;
    drv->image_sectors = 40;
    drv->drive = drive;
    /* TODO: What about the other fields in raw? */
    drv->raw.data = NULL;
    drv->raw.sync = NULL;
    /*drv->write_beyond = 0;*/
    return drv;
}

static void fdd_init_crc1021(void)
{
    unsigned int i, j;
    unsigned int w;

    crc1021 = lib_malloc(256 * sizeof(uint16_t));
    for (i = 0; i < 256; i++) {
        w = i << 8;
        for (j = 0; j < 8; j++) {
            if (w & 0x8000) {
                w <<= 1;
                w ^= 0x1021;
            } else {
                w <<= 1;
            }
        }
        crc1021[i] = (uint16_t)w;
    }
}

void fdd_shutdown(fd_drive_t *drv)
{
    if (!drv) {
        return;
    }
    lib_free(drv->myname);
    lib_free(drv);
}

void fdd_image_attach(fd_drive_t *drv, struct disk_image_s *image)
{
    if (!drv) {
        return;
    }
    drv->image = image;
    switch (image->type) {
        case DISK_IMAGE_TYPE_D1M:
            drv->tracks = 80 + 3; /* FIXME */
            drv->sectors = 10;
            drv->sector_size = 2;
            drv->head_invert = 1;
            drv->disk_rate = 2;
            drv->iso = 0;
            drv->gap2 = 22;
            drv->gap3 = 35;
            drv->image_sectors = 256;
            break;
        case DISK_IMAGE_TYPE_D2M:
            drv->tracks = 80 + 3; /* FIXME */
            drv->sectors = 10;
            drv->sector_size = 3;
            drv->head_invert = 1;
            drv->disk_rate = 0;
            drv->iso = 0;
            drv->gap2 = 22;
            drv->gap3 = 100;
            drv->image_sectors = 256;
            break;
        case DISK_IMAGE_TYPE_D4M:
            drv->tracks = 80 + 3; /* FIXME */
            drv->sectors = 20;
            drv->sector_size = 3;
            drv->head_invert = 1;
            drv->disk_rate = 3;
            drv->iso = 0;
            drv->gap2 = 41;
            drv->gap3 = 100;
            drv->image_sectors = 256;
            break;
        case DISK_IMAGE_TYPE_D81:
        default:
#if 0
            /* Normally, the value below would be 80, but the FD2K/4K ROMS
               write to sector 81 with zeros when formatting for 1581 disks.
               By setting it to 81, we later detect this and error out
               appropriately so the format command will work. */
#endif
            drv->tracks = MAX_TRACKS_1581;
            drv->sectors = 10;
            drv->sector_size = 2;
            drv->head_invert = 1;
            drv->disk_rate = 2;
            drv->iso = 1;
            drv->gap2 = 22;
            drv->gap3 = 35;
            drv->image_sectors = 40;
            break;
    }
    drv->raw.size = 25 * fdd_data_rates[drv->disk_rate];
    drv->raw.data = lib_malloc((size_t)(drv->raw.size));
    drv->raw.sync = lib_calloc(1, (size_t)((drv->raw.size + 7) >> 3));
    drv->raw.track_head = -1;
    drv->raw.dirty = 0;
    drv->raw.head = 0;
    /*drv->write_beyond = 0;*/

    drv->disk_change = 1;
    drv->write_protect = (int)(image->read_only);
}

void fdd_image_detach(fd_drive_t *drv)
{
    if (!drv) {
        return;
    }
    fdd_flush_raw(drv);
    drv->image = NULL;
    lib_free(drv->raw.data);
    drv->raw.data = NULL;
    lib_free(drv->raw.sync);
    drv->raw.sync = NULL;
    drv->disk_change = 1;
}

#define fdd_raw_write(b)                                    \
    {                                                       \
        drv->raw.data[p] = b;                \
        drv->raw.sync[p >> 3] &= (uint8_t)(0xff7f >> (p & 7)); \
        p++;                                                \
        if (p >= drv->raw.size) {                           \
            p = 0;                                          \
        }                                                   \
    }

#define fdd_raw_write_sync(b)                               \
    {                                                       \
        drv->raw.data[p] = b;                \
        drv->raw.sync[p >> 3] |= (uint8_t)(0x80 >> (p & 7));   \
        p++;                                                \
        if (p >= drv->raw.size) {                           \
            p = 0;                                          \
        }                                                   \
    }

inline uint16_t fdd_crc(uint16_t crc, uint8_t b)
{
    if (!crc1021) {
        fdd_init_crc1021();
    }
    return (uint16_t)(crc1021[(crc >> 8) ^ b] ^ (crc << 8));
}

static void fdd_flush_raw(fd_drive_t *drv)
{
    int i, j, s, p, step, d;
    uint8_t *data;
    uint16_t w;
    disk_addr_t dadr;

    if (!drv->raw.dirty) {
        return;
    }
    drv->raw.dirty = 0;

    if (drv->raw.track_head / 2 < drv->tracks && drv->image) {
#ifdef FDD_DEBUG
        for (i = 0; i < drv->raw.size; i++) {
            if (!(i & 15)) {
                printf("%04x: ", i);
            }
            printf("%02x ", drv->raw.data[i]);
            if ((i & 15) == 15) {
                printf("\n");
            }
        }
#endif
        data = lib_malloc((size_t)(128 << drv->sector_size));
        p = 0;
        for (s = 0; s < drv->sectors; s++) {
            step = 0;
            d = 0;
            for (i = 0; i < drv->raw.size * 2; i++) {
                w = drv->raw.data[p];
                if (drv->raw.sync[p >> 3] & (0x80 >> (p & 7))) {
                    w |= 0x100;
                }
                p++;
                if (p >= drv->raw.size) {
                    p = 0;
                }
                switch (step) {
                    case 0:
                        if (w == 0x00) {
                            step++;
                        }
                        continue;
                    case 1:
                        if (w == 0x00) {
                            continue;
                        }
                        if (w == 0x1a1) {
                            step++;
                            continue;
                        }
                        break;
                    case 2:
                        if (w == 0x1a1) {
                            continue;
                        }
                        if (w == 0xfe) {
                            step++;
                            continue;
                        }
                        break;
                    case 3:
                        if (w == drv->raw.track_head / 2) {
                            step++;
                            continue;
                        }
                        break;
                    case 4:
                        if (w == ((drv->raw.track_head & 1) ^ drv->head_invert)) {
                            step++;
                            continue;
                        }
                        break;
                    case 5:
                        if (w == s + 1) {
                            step++;
                            continue;
                        }
                        break;
                    case 6:
                        if (w == drv->sector_size) {
                            step++;
                            continue;
                        }
                        break;
                    case 7:
                        step++;
                        continue;
                    case 8:
                        step++;
                        continue;
                    case 9:
                        if (w == 0x00) {
                            step++;
                        }
                        continue;
                    case 10:
                        if (w == 0x00) {
                            continue;
                        }
                        if (w == 0x1a1) {
                            step++;
                            continue;
                        }
                        step = 9;
                        continue;
                    case 11:
                        if (w == 0x1a1) {
                            continue;
                        }
                        if (w == 0xfb) {
                            step++;
                            continue;
                        }
                        break;
                    case 12:
                        data[d++] = (uint8_t)w;
                        if (d >= (128 << drv->sector_size)) {
                            step++;
                        }
                        continue;
                    case 13:
                        step++;
                        continue;
                    case 14:

                        dadr.sector = (unsigned int)((drv->raw.track_head
                                    ^ drv->head_invert) * drv->sectors + s);
                        dadr.sector <<= drv->sector_size - 1;
                        dadr.track = dadr.sector / (unsigned int)(drv->image_sectors) + 1;
                        dadr.sector = dadr.sector % (unsigned int)(drv->image_sectors);

                        for (j = 0; j < (1 << drv->sector_size); j += 2) {
#if 0
                            /* FD2000 and FD4000 drives will expect to read back a formatted
                               track 81 on a D81 image, but this is beyond the image spec.
                               We will track if the data wasn't all zeros here, and error
                               out in the read process (below) later. */
                            if (dadr.track >= 81 && drv->image->type == DISK_IMAGE_TYPE_D81
                                && !drv->write_beyond) {
                                for (c = 0; c < 256 ; c++ ) {
                                    if (data[j * 128 + c]) {
                                        drv->write_beyond = 1;
                                        break;
                                    }
                                }
                            } else {
                                disk_image_write_sector(drv->image, data + j * 128, &dadr);
                                drv->write_beyond = 0;
                            }
#endif
                            disk_image_write_sector(drv->image, data + j * 128, &dadr);
                            dadr.sector = (dadr.sector + 1) % (unsigned int)(drv->image_sectors);
                            if (!dadr.sector) {
                                dadr.track++;
                            }
                        }
                        i = drv->raw.size * 2;
                    default:
                        /* NOP */
                        break;
                }
                step = 0;
            }
        }
        lib_free(data);
    }
}

static void fdd_update_raw(fd_drive_t *drv)
{
    int i, j, s, p, res;
    uint8_t buffer[256];
    uint16_t crc;
    disk_addr_t dadr;

    if (drv->track * 2 + drv->head == drv->raw.track_head) {
        return;
    }
    if (drv->raw.dirty) {
        fdd_flush_raw(drv);
    }
    drv->raw.track_head = drv->track * 2 + drv->head;

    memset(drv->raw.data, 0x4e, (size_t)(drv->raw.size));
    memset(drv->raw.sync, 0, (size_t)((drv->raw.size + 7) >> 3));

    if (drv->track < drv->tracks && drv->image) {
        i = (drv->track * 2 + (drv->head ^ drv->head_invert)) * drv->sectors;
        i <<= drv->sector_size - 1;
        dadr.track = (unsigned int)(i / drv->image_sectors + 1);
        dadr.sector = (unsigned int)(i % drv->image_sectors);

        if (drv->iso) {
            p = 32; /* GAP 4a */
        } else {
            p = 80;
            for (i = 0; i < 12; i++) { /* Sync */
                fdd_raw_write(0x00);
            }
            for (i = 0; i < 3; i++) {
                fdd_raw_write_sync(0xa1);
            }
            fdd_raw_write(0xfc); /* Index mark */
            for (i = 0; i < 50; i++) {
                fdd_raw_write(0x4e);
            }
        }
        for (s = 0; s < drv->sectors; s++) {
            for (i = 0; i < 12; i++) { /* Sync */
                fdd_raw_write(0x00);
            }
            for (i = 0; i < 3; i++) {
                fdd_raw_write_sync(0xa1);
            }
            fdd_raw_write(0xfe); /* ID mark */
            fdd_raw_write((uint8_t)(drv->track));
            crc = fdd_crc(0xb230, (uint8_t)drv->track);
            fdd_raw_write((uint8_t)(drv->head ^ drv->head_invert));
            crc = fdd_crc(crc, (uint8_t)(drv->head ^ drv->head_invert));
            fdd_raw_write((uint8_t)(s + 1));
            crc = fdd_crc(crc, (uint8_t)(s + 1));
            fdd_raw_write((uint8_t)(drv->sector_size));
            crc = fdd_crc(crc, (uint8_t)drv->sector_size);
            fdd_raw_write((uint8_t)(crc >> 8));
            fdd_raw_write((uint8_t)crc);
            for (i = 0; i < drv->gap2; i++) {
                fdd_raw_write(0x4e);
            }
            crc = 0xe295;
            for (j = 0; j < (1 << drv->sector_size); j += 2) {
#if 0
                /* FD2000 and FD4000 drives will expect to read back a formatted
                   track 81 on a D81 image, but this is beyond the image spec.
                   If any data other than zero was written, error out on the
                   track read. We do this by not creating the MFM structure for
                   the whole track. */
                if (dadr.track >= 81 && drv->image->type == DISK_IMAGE_TYPE_D81) {
                    if (drv->write_beyond) {
                        /* remove all MFM data from the track */
                        memset(drv->raw.data, 0x4e, (size_t)(drv->raw.size));
                        memset(drv->raw.sync, 0, (size_t)((drv->raw.size + 7) >> 3));
                        drv->write_beyond = 0;
                        return;
                    }
                    memset(buffer, 0, 256);
                    res = 0;
                } else {
                    res = disk_image_read_sector(drv->image, buffer, &dadr);
                }
#endif
                res = disk_image_read_sector(drv->image, buffer, &dadr);
                if (res < 0) {
                    return;
                }
                if (j == 0) {
                    for (i = 0; i < 12; i++) { /* Sync */
                        fdd_raw_write(0x00);
                    }
                    for (i = 0; i < 3; i++) {
                        fdd_raw_write_sync(0xa1);
                    }
                    fdd_raw_write(0xfb); /* Data mark */
                }
                for (i = 0; i < 256; i++) {
                    fdd_raw_write(buffer[i]);
                    crc = fdd_crc(crc, buffer[i]);
                }
                dadr.sector = (dadr.sector + 1) % (unsigned int)(drv->image_sectors);
                if (!dadr.sector) {
                    dadr.track++;
                }
            }
            fdd_raw_write((uint8_t)(crc >> 8));
            fdd_raw_write((uint8_t)(crc & 0xff));
            for (i = 0; i < drv->gap3; i++) {
                fdd_raw_write(0x4e); /* GAP 3 */
            }
        }
#ifdef FDD_DEBUG
        for (i = 0; i < drv->raw.size; i++) {
            if (!(i & 15)) {
                printf("%04x: ", i);
            }
            printf("%02x ", drv->raw.data[i]);
            if ((i & 15) == 15) {
                printf("\n");
            }
        }
#endif
    }
}

uint64_t fdd_rotate(fd_drive_t *drv, uint64_t bytes_in)
{
    uint64_t bytes;

    if (!drv || !drv->motor || !drv->image) {
        return bytes_in;
    }

    for (bytes = bytes_in; bytes > INT32_MAX; bytes -= INT32_MAX)
    {
        drv->index_count += (drv->raw.head + INT32_MAX) / drv->raw.size;
        drv->raw.head = (drv->raw.head + INT32_MAX) % drv->raw.size;
    }
    drv->index_count += (drv->raw.head + bytes) / drv->raw.size;
    drv->raw.head = (drv->raw.head + bytes) % drv->raw.size;

    return bytes_in;
}

int fdd_index(fd_drive_t *drv)
{
    if (!drv) {
        return 0;
    }
    return (drv->raw.head < INDEXLEN) ? 1 : 0;
}

void fdd_index_count_reset(fd_drive_t *drv)
{
    if (!drv) {
        return;
    }
    drv->index_count = 0;
}

unsigned int fdd_index_count(fd_drive_t *drv)
{
    if (!drv) {
        return 0;
    }
    return drv->index_count;
}

int fdd_track0(fd_drive_t *drv)
{
    if (!drv) {
        return 0;
    }
    return drv->track ? 0 : 1;
}

int fdd_write_protect(fd_drive_t *drv)
{
    if (!drv) {
        return 0;
    }
    return drv->write_protect;
}

int fdd_disk_change(fd_drive_t *drv)
{
    if (!drv) {
        return 0;
    }
    return drv->disk_change;
}

uint16_t fdd_read(fd_drive_t *drv)
{
    uint16_t data;
    int p;

    if (!drv || !drv->motor) {
        return 0;
    }
    p = drv->raw.head;
    if (drv->disk_rate == drv->rate) {
        fdd_update_raw(drv);

        data = (uint16_t)drv->raw.data[p];
        if (drv->raw.sync[p >> 3] & (0x80 >> (p & 7))) {
            data |= 0x100;
        }
    } else {
        data = 0;
    }
    p++;
    if (p >= drv->raw.size) {
        p = 0;
        drv->index_count++;
    }
    drv->raw.head = p;
    return data;
}

int fdd_write(fd_drive_t *drv, uint16_t data)
{
    int p;

    if (!drv || !drv->motor) {
        return -1;
    }
    fdd_update_raw(drv);

    p = drv->raw.head;
    if (drv->disk_rate == drv->rate) {
        drv->raw.data[p] = (uint8_t)data;
        if (data & 0x100) {
            drv->raw.sync[p >> 3] |= (uint8_t)(0x80 >> (p & 7));
        } else {
            drv->raw.sync[p >> 3] &= (uint8_t)(0xff7f >> (p & 7));
        }
        drv->raw.dirty = 1;
    }
    p++;
    if (p >= drv->raw.size) {
        p = 0;
        drv->index_count++;
    }
    drv->raw.head = p;
    return 0;
}

void fdd_flush(fd_drive_t *drv)
{
    if (!drv) {
        return;
    }
    fdd_flush_raw(drv);
}

void fdd_seek_pulse(fd_drive_t *drv, int dir)
{
    if (!drv) {
        return;
    }

    if (drv->motor) {
        drv->track += dir ? 1 : -1;
    }
    if (drv->image) {
        drv->disk_change = 0;
    }
    if (drv->track < 0) {
        drv->track = 0;
    }
    if (drv->track > FDD_MAX_TRACK) {
        drv->track = FDD_MAX_TRACK;
    }
    drv->drive->current_half_track = (drv->track + 1) * 2;
#if 0
    printf("track:%d (half:%d) img tracks: %u (max half:%u)\n",
           drv->track, drv->drive->current_half_track,
           drv->image->tracks, drv->image->max_half_tracks);
#endif
    if (drv->image) {
        if (drv->drive->current_half_track > (drv->image->tracks * 2)) {
            log_warning(LOG_DEFAULT, "disk image will get extended (%d tracks)",
                        drv->drive->current_half_track / 2);
            /* FIXME: actually extend the image here */
        }
    }
}

void fdd_select_head(fd_drive_t *drv, int head)
{
    if (!drv) {
        return;
    }
    drv->head = head & 1;
}

void fdd_set_motor(fd_drive_t *drv, int motor)
{
    if (!drv) {
        return;
    }
    drv->motor = motor & 1;
}

void fdd_set_rate(fd_drive_t *drv, int rate)
{
    if (!drv) {
        return;
    }
    drv->rate = rate & 3;
}

#define FDD_SNAP_MAJOR 1
#define FDD_SNAP_MINOR 0

int fdd_snapshot_write_module(fd_drive_t *drv, struct snapshot_s *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, drv->myname, FDD_SNAP_MAJOR, FDD_SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (uint8_t)drv->number) < 0
        || SMW_B(m, (uint8_t)drv->disk_change) < 0
        || SMW_B(m, (uint8_t)drv->write_protect) < 0
        || SMW_B(m, (uint8_t)drv->track) < 0
        || SMW_B(m, (uint8_t)drv->tracks) < 0
        || SMW_B(m, (uint8_t)drv->head) < 0
        || SMW_B(m, (uint8_t)drv->sectors) < 0
        || SMW_B(m, (uint8_t)drv->motor) < 0
        || SMW_B(m, (uint8_t)drv->rate) < 0
        || SMW_B(m, (uint8_t)drv->sector_size) < 0
        || SMW_B(m, (uint8_t)drv->iso) < 0
        || SMW_B(m, (uint8_t)drv->gap2) < 0
        || SMW_B(m, (uint8_t)drv->gap3) < 0
        || SMW_B(m, (uint8_t)drv->head_invert) < 0
        || SMW_B(m, (uint8_t)drv->disk_rate) < 0
        || SMW_DW(m, (uint32_t)drv->image_sectors) < 0
        || SMW_DW(m, (uint32_t)drv->index_count) < 0
        || SMW_DW(m, (uint8_t)drv->raw.head) < 0
        || SMW_B(m, (uint8_t)drv->raw.track_head) < 0
        || SMW_B(m, (uint8_t)drv->raw.dirty) < 0
        || SMW_BA(m, drv->raw.data, (unsigned int)drv->raw.size) < 0
        || SMW_BA(m, drv->raw.sync, (unsigned int)((drv->raw.size + 7) >> 3)) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    /* TODO: Disk image save */

    return snapshot_module_close(m);
}

int fdd_snapshot_read_module(fd_drive_t *drv, struct snapshot_s *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, drv->myname, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, FDD_SNAP_MAJOR, FDD_SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || SMR_B_INT(m, &drv->number) < 0
        || SMR_B_INT(m, &drv->disk_change) < 0
        || SMR_B_INT(m, &drv->write_protect) < 0
        || SMR_B_INT(m, &drv->track) < 0
        || SMR_B_INT(m, &drv->tracks) < 0
        || SMR_B_INT(m, &drv->head) < 0
        || SMR_B_INT(m, &drv->sectors) < 0
        || SMR_B_INT(m, &drv->motor) < 0
        || SMR_B_INT(m, &drv->rate) < 0
        || SMR_B_INT(m, &drv->sector_size) < 0
        || SMR_B_INT(m, &drv->iso) < 0
        || SMR_B_INT(m, &drv->gap2) < 0
        || SMR_B_INT(m, &drv->gap3) < 0
        || SMR_B_INT(m, &drv->head_invert) < 0
        || SMR_B_INT(m, &drv->disk_rate) < 0
        || SMR_DW_INT(m, &drv->image_sectors) < 0
        || SMR_DW_UINT(m, &drv->index_count) < 0
        || SMR_DW_INT(m, &drv->raw.head) < 0
        || SMR_B_INT(m, &drv->raw.track_head) < 0
        || SMR_B_INT(m, &drv->raw.dirty) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    if (drv->track < 0) {
        drv->track = 0;
    }
    if (drv->track > FDD_MAX_TRACK) {
        drv->track = FDD_MAX_TRACK;
    }
    if (drv->tracks < 0) {
        drv->tracks = 0;
    }
    if (drv->tracks > FDD_NUM_TRACKS) {
        drv->tracks = FDD_NUM_TRACKS;
    }
    drv->head &= 1;
    drv->motor &= 1;
    drv->rate &= 3;
    drv->sector_size &= 3;
    drv->disk_rate &= 3;

    drv->raw.size = 25 * fdd_data_rates[drv->disk_rate];
    drv->raw.head %= drv->raw.size;

    lib_free(drv->raw.data);
    drv->raw.data = lib_malloc((size_t)drv->raw.size);
    lib_free(drv->raw.sync);
    drv->raw.sync = lib_malloc((size_t)((drv->raw.size + 7) >> 3));

    if (0
        || SMR_BA(m, drv->raw.data, (unsigned int)drv->raw.size) < 0
        || SMR_BA(m, drv->raw.sync, (unsigned int)((drv->raw.size + 7) >> 3)) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}
