/*
 * vdrive.h - Virtual disk-drive implementation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VDRIVE_H
#define VICE_VDRIVE_H

#include "types.h"
#include "vdrive-dir.h"

/* temporarily until the 4000 support is not safe */
#define VDRIVE_IMAGE_FORMAT_4000_TEST (vdrive->image_format == VDRIVE_IMAGE_FORMAT_4000)

/* High level disk formats.
   They can be different than the disk image type.  */
#define VDRIVE_IMAGE_FORMAT_1541 0
#define VDRIVE_IMAGE_FORMAT_1571 1
#define VDRIVE_IMAGE_FORMAT_1581 2
#define VDRIVE_IMAGE_FORMAT_8050 3 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_8250 4 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_2040 5 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_4000 6

#define BUFFER_NOT_IN_USE      0
#define BUFFER_DIRECTORY_READ  1
#define BUFFER_SEQUENTIAL      2
#define BUFFER_MEMORY_BUFFER   3
#define BUFFER_RELATIVE        4
#define BUFFER_COMMAND_CHANNEL 5

#define WRITE_BLOCK 512

#define SET_LO_HI(p, val)               \
    do {                                \
        *((p)++) = (val) & 0xff;        \
        *((p)++) = ((val) >> 8) & 0xff; \
    } while (0)

#define DRIVE_RAMSIZE           0x400

#define BAM_MAXSIZE (33 * 256)

/* Serial Error Codes. */
#define SERIAL_OK               0
#define SERIAL_WRITE_TIMEOUT    1
#define SERIAL_READ_TIMEOUT     2
#define SERIAL_FILE_NOT_FOUND   64
#define SERIAL_NO_DEVICE        128

#define SERIAL_ERROR            (2)
#define SERIAL_EOF              (64)

typedef struct bufferinfo_s {
    unsigned int mode;     /* Mode on this buffer */
    unsigned int readmode; /* Is this channel for reading or writing */
    BYTE *buffer;          /* Use this to save data */
    BYTE *slot;            /* Save data for directory-slot */
    unsigned int bufptr;   /* Use this to save/read data to disk */
    unsigned int track;    /* which track is allocated for this sector */
    unsigned int sector;   /*   (for write files only) */
    unsigned int length;   /* Directory-read length */
    unsigned int record;   /* Current record */

    /* REL file information stored in buffers since we can have more than
        one open */
    BYTE *side_sector;
    /* location of the side sectors */
    BYTE *side_sector_track;
    BYTE *side_sector_sector;

    BYTE *super_side_sector;
    /* location of the super side sector */
    BYTE super_side_sector_track;
    BYTE super_side_sector_sector;

    BYTE *buffer_next;          /* next buffer for rel file */
    unsigned int track_next;    /* track for the next sector */
    unsigned int sector_next;   /* sector for the next sector */

    unsigned int record_max;  /* Max rel file record, inclusive */
    unsigned int record_next; /* Buffer pointer to beginning of next record */
    BYTE needsupdate;         /* true if the current sector needs to be
                                  written (from REL write) */
    BYTE super_side_sector_needsupdate; /* similar to above */
    BYTE *side_sector_needsupdate;

    vdrive_dir_context_t dir; /* directory listing context or directory entry */
} bufferinfo_t;

struct disk_image_s;

/* Run-time data struct for each drive. */
typedef struct vdrive_s {
    struct disk_image_s *image;

    /* Current image file */
    unsigned int mode;         /* Read/Write */
    unsigned int image_format; /* 1541/71/81 */
    unsigned int unit;

    unsigned int Bam_Track;
    unsigned int Bam_Sector;
    unsigned int bam_name;     /* Offset from start of BAM to disk name.   */
    unsigned int bam_id;       /* Offset from start of BAM to disk ID.  */
    unsigned int Header_Track; /* Directory header location */
    unsigned int Header_Sector;
    unsigned int Dir_Track;    /* First directory sector location */
    unsigned int Dir_Sector;
    unsigned int num_tracks;
    /* CBM partition first and last track (1581)
     * Part_Start is 1, Part_End = num_tracks if no partition is used
     */
    unsigned int Part_Start, Part_End;

    unsigned int bam_size;
    BYTE *bam;
    bufferinfo_t buffers[16];

    /* Memory read command buffer.  */
    BYTE mem_buf[256];
    unsigned int mem_length;

    /* removed side sector data and placed it in buffer structure */
    /* BYTE *side_sector; */

    BYTE ram[0x800];
} vdrive_t;

/* Actually, serial-code errors ... */

#define FLOPPY_COMMAND_OK       0
#define FLOPPY_ERROR            2


/* Return values used around. */

#define FD_OK           0
#define FD_EXIT         1       /* -1,0, 1 are fixed values */

#define FD_NOTREADY     -2
#define FD_CHANGED      -3      /* File has changed on disk */
#define FD_NOTRD        -4
#define FD_NOTWRT       -5
#define FD_WRTERR       -6
#define FD_RDERR        -7
#define FD_INCOMP       -8      /* DOS Format Mismatch */
#define FD_BADIMAGE     -9      /* ID mismatch (Disk or tape) */
#define FD_BADNAME      -10     /* Illegal filename */
#define FD_BADVAL       -11     /* Illegal value */
#define FD_BADDEV       -12
#define FD_BAD_TS       -13     /* Track or sector */
#define FD_BAD_TRKNUM   -14     /* Illegal track number */
#define FD_BAD_SECNUM   -15     /* Illegal sector number */


#define CHK_NUM         0
#define CHK_RDY         1

/* ------------------------------------------------------------------------- */

extern void vdrive_init(void);
extern int vdrive_device_setup(vdrive_t *vdrive, unsigned int unit);
extern void vdrive_device_shutdown(vdrive_t *vdrive);
extern int vdrive_attach_image(struct disk_image_s *image, unsigned int unit, vdrive_t *vdrive);
extern void vdrive_detach_image(struct disk_image_s *image, unsigned int unit, vdrive_t *vdrive);
extern void vdrive_close_all_channels(vdrive_t *vdrive);
extern int vdrive_get_max_sectors(vdrive_t *vdrive, unsigned int track);
extern void vdrive_get_last_read(unsigned int *track, unsigned int *sector, BYTE **buffer);
extern void vdrive_set_last_read(unsigned int track, unsigned int sector, BYTE *buffer);

extern void vdrive_alloc_buffer(struct bufferinfo_s *p, int mode);
extern void vdrive_free_buffer(struct bufferinfo_s *p);
extern void vdrive_set_disk_geometry(vdrive_t *vdrive);
extern int vdrive_read_sector(vdrive_t *vdrive, BYTE *buf, unsigned int track, unsigned int sector);
extern int vdrive_write_sector(vdrive_t *vdrive, const BYTE *buf, unsigned int track, unsigned int sector);

#endif
