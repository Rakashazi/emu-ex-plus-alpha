/*
 * vdrive.h - Virtual disk-drive implementation.
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

#ifndef VICE_VDRIVE_H
#define VICE_VDRIVE_H

#include "types.h"
#include "vdrive-dir.h"

#define NUM_DRIVES              2

#define VDRIVE_PART_SUPPORTED(a) (vdrive->ptype[a]>=1 && vdrive->ptype[a]<=4)
#define VDRIVE_IS_FD(a) (a->image && (a->image->type == DISK_IMAGE_TYPE_D1M || a->image->type == DISK_IMAGE_TYPE_D2M || a->image->type == DISK_IMAGE_TYPE_D4M ))
#define VDRIVE_IS_HD(a) (a->image && (a->image->type == DISK_IMAGE_TYPE_DHD))
#define VDRIVE_IS_READONLY(a) (a->image_mode > 0)

/* High level disk formats.
   They can be different than the disk image type.  */
#define VDRIVE_IMAGE_FORMAT_1541 0
#define VDRIVE_IMAGE_FORMAT_1571 1
#define VDRIVE_IMAGE_FORMAT_1581 2
#define VDRIVE_IMAGE_FORMAT_8050 3 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_8250 4 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_2040 5 /* Dual Disk Drive */
#define VDRIVE_IMAGE_FORMAT_NP   6
#define VDRIVE_IMAGE_FORMAT_SYS  7
#define VDRIVE_IMAGE_FORMAT_9000 8
#define VDRIVE_IMAGE_FORMAT_NONE 10

#define BUFFER_NOT_IN_USE      0
#define BUFFER_DIRECTORY_READ  1
#define BUFFER_SEQUENTIAL      2
#define BUFFER_MEMORY_BUFFER   3
#define BUFFER_RELATIVE        4
#define BUFFER_COMMAND_CHANNEL 5
#define BUFFER_PARTITION_READ  6
#define BUFFER_DIRECTORY_MORE_READ  7

#define VDRIVE_BAM_MAX_STATES    33

#define WRITE_BLOCK 512

#define SET_LO_HI(p, val)               \
    do {                                \
        *((p)++) = (val) & 0xff;        \
        *((p)++) = ((val) >> 8) & 0xff; \
    } while (0)

#define DRIVE_RAMSIZE           0x400

#define BAM_MAXSIZE (VDRIVE_BAM_MAX_STATES * 256)

/* Serial Error Codes. */
#define SERIAL_OK               0
#define SERIAL_WRITE_TIMEOUT    1
#define SERIAL_READ_TIMEOUT     2
#define SERIAL_FILE_NOT_FOUND   64      /* EOF */
#define SERIAL_NO_DEVICE        128

#define SERIAL_ERROR                    (2)
#define SERIAL_EOF                      (0x40)
#define SERIAL_DEVICE_NOT_PRESENT       (0x80)

typedef struct bufferinfo_s {
    unsigned int mode;     /* Mode on this buffer */
    unsigned int readmode; /* Is this channel for reading or writing */
    uint8_t *buffer;          /* Use this to save data */
    uint8_t *slot;            /* Save data for directory-slot */
    unsigned int bufptr;   /* Use this to save/read data to disk */
    unsigned int track;    /* which track is allocated for this sector */
    unsigned int sector;   /*   (for write files only) */
    unsigned int length;   /* Directory-read length */
    unsigned int record;   /* Current record */
    unsigned int partition;/* Current partition */
    unsigned int partstart;  /* for 1581's: partition (and BAM) can be different for open files */
    unsigned int partend;
    int small;               /* flag to enable small buffer reads on seq type files */
    int timemode;            /* flag to enable time output on dir listings */

    vdrive_dir_context_t dir; /* directory listing context or directory entry */

    /* REL file information stored in buffers since we can have more than
        one open */
    uint8_t *side_sector;
    /* location of the side sectors */
    uint8_t *side_sector_track;
    uint8_t *side_sector_sector;

    uint8_t *super_side_sector;
    /* location of the super side sector */
    uint8_t super_side_sector_track;
    uint8_t super_side_sector_sector;

    uint8_t *buffer_next;          /* next buffer for rel file */
    unsigned int track_next;    /* track for the next sector */
    unsigned int sector_next;   /* sector for the next sector */

    unsigned int record_max;  /* Max rel file record, inclusive */
    unsigned int record_next; /* Buffer pointer to beginning of next record */
    uint8_t *side_sector_needsupdate;
    uint8_t needsupdate;         /* true if the current sector needs to be
                                  written (from REL write) */
    uint8_t super_side_sector_needsupdate; /* similar to above */

} bufferinfo_t;

struct disk_image_s;

/* Run-time data struct for each drive. */
typedef struct vdrive_s {
    unsigned int unit;         /* IEC bus device number */

    struct disk_image_s *images[2]; /* for dual drives */

    /* Current image file */
    struct disk_image_s *image;
    int image_mode;            /* -1 no disk, 0 is write/read, 1 is read */
    unsigned int image_format; /* 1541/71/81 */

    unsigned int Bam_Track;
    unsigned int Bam_Sector;
    unsigned int bam_name;     /* Offset from start of BAM to disk name.   */
    unsigned int bam_id;       /* Offset from start of BAM to disk ID.  */
    int bam_state[VDRIVE_BAM_MAX_STATES];
    int bam_tracks[VDRIVE_BAM_MAX_STATES];
    int bam_sectors[VDRIVE_BAM_MAX_STATES];
                               /* bam cache dirty (1 char per sector)  */
                               /* -1 = not in mem, 0 = in mem, 1 = dirty */
    unsigned int Header_Track; /* Directory header location */
    unsigned int Header_Sector;
    unsigned int Dir_Track;    /* First directory sector location */
    unsigned int Dir_Sector;
    unsigned int num_tracks;
    /* CBM partition first and last track (1581)
     * Part_Start is 1, Part_End = num_tracks if no partition is used
     */
    unsigned int Part_Start, Part_End;
    /* CMD paritions */
    unsigned int current_offset;
    unsigned int sys_offset;
    signed int current_part; /* current part; one matching the bam loaded */
    signed int selected_part; /* the "currently selected" partition, with CP command */
    signed int default_part; /* the default one that is to be used on startup */
    uint8_t ptype[256];
    uint32_t poffset[256];
    uint32_t psize[256];
    unsigned int cheadertrack[256];    /* current header track and sector for each partition */
    unsigned int cheadersector[256];
    unsigned int cdirtrack[256];    /* current directory track and sector for each partition */
    unsigned int cdirsector[256];
    unsigned int cpartstart[256];   /* current partition start end and for each partition for 1581 */
    unsigned int cpartend[256];

    unsigned int d90toggle;    /* D9090/60 new sector toggle */
    int haspt;                 /* whether the connected disk supports partitions (not drives) */
    int dir_part;              /* which drive to show first when doing group dirs */
    int dir_count;             /* how many drives are left when doing group dirs */
    int last_code;             /* for command channel status string */

    unsigned int bam_size;
    uint8_t *bam;              /* Disk header blk (if any) followed by BAM blocks */
    bufferinfo_t buffers[16];

    /* Memory read command buffer.  */
    uint8_t mem_buf[256];
    unsigned int mem_length;

    /* removed side sector data and placed it in buffer structure */
    /* BYTE *side_sector; */

    uint8_t ram[0x8000];
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
extern int vdrive_attach_image(struct disk_image_s *image, unsigned int unit, unsigned int drive, vdrive_t *vdrive);
extern void vdrive_detach_image(struct disk_image_s *image, unsigned int unit, unsigned int drive, vdrive_t *vdrive);
extern void vdrive_close_all_channels(vdrive_t *vdrive);
extern void vdrive_close_all_channels_partition(vdrive_t *vdrive, int part);
extern int vdrive_get_max_sectors(vdrive_t *vdrive, unsigned int track);
extern int vdrive_get_max_sectors_per_head(vdrive_t *vdrive, unsigned int track);
extern void vdrive_get_last_read(unsigned int *track, unsigned int *sector, uint8_t **buffer);
extern void vdrive_set_last_read(unsigned int track, unsigned int sector, uint8_t *buffer);

extern void vdrive_alloc_buffer(struct bufferinfo_s *p, int mode);
extern void vdrive_free_buffer(struct bufferinfo_s *p);
extern void vdrive_set_disk_geometry(vdrive_t *vdrive);
extern int vdrive_read_sector(vdrive_t *vdrive, uint8_t *buf, unsigned int track, unsigned int sector);
extern int vdrive_write_sector(vdrive_t *vdrive, const uint8_t *buf, unsigned int track, unsigned int sector);
extern int vdrive_read_sector_physical(vdrive_t *vdrive, uint8_t *buf, unsigned int track, unsigned int sector);
extern int vdrive_write_sector_physical(vdrive_t *vdrive, const uint8_t *buf, unsigned int track, unsigned int sector);

extern struct disk_image_s *vdrive_get_image(vdrive_t *vdrive, unsigned int drive);

extern void vdrive_refresh(unsigned int unit);
extern void vdrive_flush(unsigned int unit);
extern int vdrive_find_sys(vdrive_t *vdrive);
extern int vdrive_read_partition_table(vdrive_t *vdrive);
extern int vdrive_ispartvalid(vdrive_t *vdrive, int part);
extern int vdrive_write_partition_table(vdrive_t *vdrive);
extern int vdrive_pack_parts(vdrive_t *vdrive);
extern int vdrive_switch(vdrive_t *vdrive, int part);
extern int vdrive_realpart(vdrive_t *vdrive, int part);

extern int vdrive_ext_read_sector(vdrive_t *vdrive, int drive, uint8_t *buf,
    unsigned int track, unsigned int sector);
extern int vdrive_ext_write_sector(vdrive_t *vdrive, int drive, const uint8_t *buf,
    unsigned int track, unsigned int sector);


#endif
