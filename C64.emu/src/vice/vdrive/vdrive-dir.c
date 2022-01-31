/*
 * vdrive-dir.c - Virtual disk-drive implementation.
 *                Directory specific functions.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ingo Korb <ingo@akana.de>
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

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cbmdos.h"
#include "diskconstants.h"
#include "diskimage.h"
#include "lib.h"
#include "log.h"
#include "types.h"
#include "vdrive-bam.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive-rel.h"
#include "vdrive.h"
#include "diskconstants.h"



static log_t vdrive_dir_log = LOG_ERR;


void vdrive_dir_init(void)
{
    vdrive_dir_log = log_open("VDriveDIR");
}

/* Returns the interleave for directory sectors of a given image type */
static int vdrive_dir_get_interleave(unsigned int type)
{
    /* Note: Values for all drives determined empirically */
    switch (type) {
        case VDRIVE_IMAGE_FORMAT_1541:
        case VDRIVE_IMAGE_FORMAT_2040:
        case VDRIVE_IMAGE_FORMAT_1571:
        case VDRIVE_IMAGE_FORMAT_8050:
        case VDRIVE_IMAGE_FORMAT_8250:
        case VDRIVE_IMAGE_FORMAT_9000:
            return 3;
        case VDRIVE_IMAGE_FORMAT_1581:
        case VDRIVE_IMAGE_FORMAT_NP:
            return 1;
        default:
            log_error(LOG_ERR, "Unknown disk type %u.  Using interleave 3.", type);
            return 3;
    }
}

static unsigned int vdrive_dir_name_match(uint8_t *slot, uint8_t *nslot, int length, int type)
{
    if (length < 0) {
        if (slot[SLOT_TYPE_OFFSET]) {
            return 0;
        } else {
            return 1;
        }
    }

    if (!slot[SLOT_TYPE_OFFSET]) {
        return 0;
    }

    if (type != CBMDOS_FT_DEL && type != (slot[SLOT_TYPE_OFFSET] & 0x07)) {
        return 0;
    }

    return cbmdos_parse_wildcard_compare(nslot, &slot[SLOT_NAME_OFFSET]);
}

void vdrive_dir_free_chain(vdrive_t *vdrive, int t, int s)
{
    uint8_t buf[256];

    while (t) {
        /* Check for illegal track or sector.  */
        if (disk_image_check_sector(vdrive->image, t, s) < 0) {
            break;
        }

        /* Check if this sector is really allocated.  */
        if (!vdrive_bam_free_sector(vdrive, t, s)) {
            break;
        }

        /* FIXME: This seems to be redundant.  AB19981124  */
        vdrive_bam_free_sector(vdrive, t, s);
        vdrive_read_sector(vdrive, buf, t, s);
        t = (int)buf[0];
        s = (int)buf[1];
    }
}

/* Tries to allocate the given track/sector and link it */
/* to the current directory sector of vdrive.           */
/* Returns NULL if the allocation failed.               */
static uint8_t *find_next_directory_sector(vdrive_dir_context_t *dir,
                                           unsigned int track,
                                           unsigned int sector)
{
    vdrive_t *vdrive = dir->vdrive;

    if (vdrive_bam_allocate_sector(vdrive, track, sector)) {
        dir->buffer[0] = track;
        dir->buffer[1] = sector;
        vdrive_write_sector(vdrive, dir->buffer, dir->track, dir->sector);
#ifdef DEBUG_DRIVE
        log_debug("Found (%u %u) TR = %u SE = %u.",
                  track, sector, dir->track, dir->sector);
#endif
        dir->slot = 0;
        memset(dir->buffer, 0, 256);
        dir->buffer[1] = 0xff;
        dir->track = track;
        dir->sector = sector;
        return dir->buffer;
    }
    return NULL;
}


void vdrive_dir_create_slot(bufferinfo_t *p, uint8_t *realname,
                            int reallength, int filetype)
{
    p->slot = lib_calloc(1, 32);
    memset(p->slot + SLOT_NAME_OFFSET, 0xa0, 16);
    memcpy(p->slot + SLOT_NAME_OFFSET, realname, reallength);
#ifdef DEBUG_DRIVE
    log_debug("DIR: Created dir slot. Name (%d) '%s'\n", reallength, realname);
#endif
    p->slot[SLOT_TYPE_OFFSET] = filetype;       /* unclosed */

    vdrive_alloc_buffer(p, BUFFER_SEQUENTIAL);
    p->bufptr = 2;
    return;
}

/*
 * vdrive_dir_remove_slot() is called from ip_execute() for 'SCRATCH'.
 */

void vdrive_dir_remove_slot(vdrive_dir_context_t *dir)
{
    int t, s;
    vdrive_t *vdrive = dir->vdrive;

    /* Free all sector this file is using.  */
    t = (int) dir->buffer[dir->slot * 32 + SLOT_FIRST_TRACK];
    s = (int) dir->buffer[dir->slot * 32 + SLOT_FIRST_SECTOR];

    vdrive_dir_free_chain(vdrive, t, s);

    /* Free side sectors.  */
    t = (int) dir->buffer[dir->slot * 32 + SLOT_SIDE_TRACK];
    s = (int) dir->buffer[dir->slot * 32 + SLOT_SIDE_SECTOR];

    vdrive_rel_scratch(vdrive, t, s);

    /* Update bam */
    vdrive_bam_write_bam(vdrive);

    /* Update directory entry */
    dir->buffer[dir->slot * 32 + SLOT_TYPE_OFFSET] = 0;
    vdrive_write_sector(vdrive, dir->buffer, dir->track, dir->sector);
}

/*
   read first dir buffer into Dir_buffer
*/
void vdrive_dir_find_first_slot(vdrive_t *vdrive, const uint8_t *name,
                                int length, unsigned int type,
                                vdrive_dir_context_t *dir)
{
    if (length > 0) {
        uint8_t *nslot;

        nslot = cbmdos_dir_slot_create((char*)name, length);
        memcpy(dir->find_nslot, nslot, CBMDOS_SLOT_NAME_LENGTH);
        lib_free(nslot);
    }

    dir->vdrive = vdrive;
    dir->find_length = length;
    dir->find_type = type;

    dir->track = vdrive->Header_Track;
    dir->sector = vdrive->Header_Sector;
    dir->slot = 7;

    /* date comparisons; show everything */
    dir->time_low = 0;
    dir->time_high = 0xffffffff;

    vdrive_read_sector(vdrive, dir->buffer, dir->track, dir->sector);

    /* old drives may have needed this, but NP's keep their info correct */
    if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_NP) {
        dir->buffer[0] = vdrive->Dir_Track;
        dir->buffer[1] = vdrive->Dir_Sector;
    }
#ifdef DEBUG_DRIVE
    log_debug("DIR: vdrive_dir_find_first_slot (curr t:%u/s:%u dir t:%u/s:%u)",
              dir->track, dir->sector, vdrive->Dir_Track, vdrive->Dir_Sector);
#endif
}

/* convert date/time into a single 32-bit unsigned value */
static unsigned int date_to_int(int year, int month, int day, int hour, int minute)
{
    unsigned int a;

    /* 7 + 4 + 5 + 5 + 6 = 27 which is < 32 */
    a = ( 0 << 7 ) | year;
    a = ( a << 4 ) | month;
    a = ( a << 5 ) | day;
    a = ( a << 5 ) | hour;
    a = ( a << 6 ) | minute;

    return a;
}

uint8_t *vdrive_dir_find_next_slot(vdrive_dir_context_t *dir)
{
    static uint8_t return_slot[32];
    vdrive_t *vdrive = dir->vdrive;
    uint8_t *tmp;
    int j;
    unsigned int t, s, c;
    uint8_t *dirbuf = NULL;

#ifdef DEBUG_DRIVE
    log_debug("DIR: vdrive_dir_find_next_slot start (t:%u/s:%u) #%u",
            dir->track, dir->sector, dir->slot);
#endif
    /*
     * Loop all directory blocks starting from track 18, sector 1 (1541).
     */

    do {
        /*
         * Load next(first) directory block ?
         */

        dir->slot++;

        if (dir->slot >= 8) {
            int status;

            /* end of current directory? */
            if (dir->buffer[0] == 0) {
                break;
            }

            dir->slot = 0;
            dir->track = (unsigned int)dir->buffer[0];
            dir->sector = (unsigned int)dir->buffer[1];

            status = vdrive_read_sector(vdrive, dir->buffer, dir->track, dir->sector);
            if (status != 0) {
                return NULL; /* error */
            }
        }

        if (vdrive_dir_name_match(&dir->buffer[dir->slot * 32],
                                  dir->find_nslot, dir->find_length,
                                  dir->find_type)) {
            memcpy(return_slot, &dir->buffer[dir->slot * 32], 32);
            /* check date range; for DIR listings */
            t = date_to_int(return_slot[SLOT_GEOS_YEAR], return_slot[SLOT_GEOS_MONTH],
                return_slot[SLOT_GEOS_DATE], return_slot[SLOT_GEOS_HOUR],
                return_slot[SLOT_GEOS_MINUTE] );
            /* time_low is initially 0, and time_high is initially largest,
                so it should always match for most uses. */
            if (t >= dir->time_low && t <= dir->time_high)
                return return_slot;
        }
    } while (1);

#ifdef DEBUG_DRIVE
    log_debug("DIR: vdrive_dir_find_next_slot (t:%u/s:%u) #%u",
            dir->track, dir->sector, dir->slot);
#endif

    /*
     * If length < 0, create new directory-entry if possible
     */
    if (dir->find_length < 0) {
        int i, h, h2;
        unsigned int sector, max_sector, max_sector_all;

        max_sector = vdrive_get_max_sectors_per_head(vdrive, dir->track);
        max_sector_all = vdrive_get_max_sectors(vdrive, dir->track);
        h = (dir->sector / max_sector) * max_sector;
        sector = dir->sector % max_sector;
        sector += vdrive_dir_get_interleave(vdrive->image_format);
        if (sector >= max_sector) {
            sector -= max_sector;
            if (sector != 0) {
                sector--;
            }
        }
        /* go through all groups, 1 round for most CBM drives */
        for (h2 = 0; h2 < max_sector_all; h2 += max_sector) {
            for (i = 0; i < max_sector; i++) {
                dirbuf = find_next_directory_sector(dir, dir->track, sector + h);
                if (dirbuf != NULL) {
                    return dirbuf;
                }
                sector++;
                if (sector >= max_sector) {
                    sector = 0;
                }
            }
            /* for D9090/60 only move on to next track if we scanned all
                the sector groups */
            h += max_sector;
            if (h >= max_sector_all) {
                h = 0;
            }
        }
        /* D9090/60 and NP can go beyond the directory track and use any space */
        if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_NP ||
            vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
            /* restore inputs */
            t = dir->track;
            sector = dir->sector;
            /* look for a free sector */
            if (vdrive_bam_alloc_next_free_sector_interleave(vdrive, &t, &sector,
                vdrive_dir_get_interleave(vdrive->image_format))) {
                return NULL;
            }
            /* unallocate it as find_next_directory_sector allocates it */
            vdrive_bam_free_sector(vdrive, t, sector);
            /* allocate and fill it */
            dirbuf = find_next_directory_sector(dir, t, sector);
            /* it should never be NULL, but check anyways */
            if (dirbuf != NULL) {
                goto newnpsec;
            }
        }
    }
    return NULL;

newnpsec:
    /* D9090/60 don't support subdirs */
    if (vdrive->image_format == VDRIVE_IMAGE_FORMAT_9000) {
        return dirbuf;
    }

    /* have to update the block count when we add more sectors to a
        sub-directory on an NP */
    if (vdrive->Header_Track == BAM_TRACK_NP &&
        vdrive->Header_Sector == BAM_SECTOR_NP) {
        return dirbuf;
    }

    tmp = lib_malloc(256);

    /* find parent entries */
    if (vdrive_read_sector(vdrive, tmp, vdrive->Header_Track, vdrive->Header_Sector )) {
        goto out2;
    }

    /* get dir entry offset */
    t = tmp[0x24];
    s = tmp[0x25];
    j = tmp[0x26] - 2 + SLOT_NR_BLOCKS;

    /* read in dir entry; no real error checking here */
    if (vdrive_read_sector(vdrive, tmp, t, s )) {
        goto out2;
    }

    /* read it, and increment it */
    c = tmp[j] | (tmp[j + 1] << 8);
    c++;
    tmp[j] = c & 255;
    tmp[j + 1] = c >> 8;

    /* write dir entry; no real error checking here */
    if (vdrive_write_sector(vdrive, tmp, t, s )) {
        goto out2;
    }

    /* all good, leave */
    lib_free(tmp);
    return dirbuf;

out2:
    /* something bad, don't return anything good */
    lib_free(tmp);
    return NULL;

}

void vdrive_dir_no_a0_pads(uint8_t *ptr, int l)
{
    while (l--) {
        if (*ptr == 0xa0) {
            *ptr = 0x20;
        }
        ptr++;
    }
}

int vdrive_dir_filetype(const uint8_t *name, int length)
{
    int filetype = CBMDOS_FT_DEL;
    const uint8_t *ptr = name + length;

    while (--ptr != name && *ptr != '=');

    if (*ptr == '=') {
        switch (*++ptr) {
            case 'S':
                filetype = CBMDOS_FT_SEQ;
                break;
            case 'P':
                filetype = CBMDOS_FT_PRG;
                break;
            case 'U':
                filetype = CBMDOS_FT_USR;
                break;
            case 'R':
                filetype = CBMDOS_FT_REL;
                break;
            case 'C':
                filetype = CBMDOS_FT_CBM;
                break;
            case 'D':
                filetype = CBMDOS_FT_DIR;
                break;
        }
    }
    return filetype;
}


/* convert CMD ASCII date format to a single value; see above */
static unsigned int asciidate_to_int(const char *p, unsigned int def) {
/* MM/DD/YY HH:MM xM - : or . */
/* 01234567890123456 */
/*           1111111 */
    int year, month, day, hour, minute, ap;

    /* check basics */
    if ( p[2]!='/' || p[5]!='/' || p[8]!=' ' || p[14]!=' ' || p[16]!='M' ||
        (p[11]!=':' && p[11]!='.') || (p[15]!='A' && p[15]!='P') ) {
        return def;
    }

    if ( p[0]<'0' || p[0]>'9' ||
         p[1]<'0' || p[1]>'9' ||
         p[3]<'0' || p[3]>'9' ||
         p[4]<'0' || p[4]>'9' ||
         p[6]<'0' || p[6]>'9' ||
         p[7]<'0' || p[7]>'9' ||
         p[9]<'0' || p[9]>'9' ||
         p[10]<'0' || p[10]>'9' ||
         p[12]<'0' || p[12]>'9' ||
         p[13]<'0' || p[13]>'9' ) {
         return def;
    }

    month = (p[0]-'0') * 10 + p[1]-'0';
    day = (p[3]-'0') * 10 + p[4]-'0';
    year = (p[6]-'0') * 10 + p[7]-'0';
    hour = (p[9]-'0') * 10 + p[10]-'0';
    minute = (p[12]-'0') * 10 + p[13]-'0';
    ap = p[15]=='P' ? 1 : 0;

    /* allow zero inputs */
    if ( month > 12 || day > 31 || hour > 12 || minute > 59 ) {
        return def;
    }

    /* adjust am/pm to 24 hours clock */
    if (hour == 12 && !ap) {
        hour = 0;
    } else if (hour > 0 && ap) {
        hour += 12;
    }

    return date_to_int(year,month,day,hour,minute);
}

int vdrive_dir_next_directory(vdrive_t *vdrive, bufferinfo_t *b);

int vdrive_dir_first_directory(vdrive_t *vdrive,
                               cbmdos_cmd_parse_plus_t *cmd_parse,
                               bufferinfo_t *p)
{
    uint8_t *l;
    char *c, *limit;
    int filetype;
    int length;
    int newlen;
    char *name;

#ifdef DEBUG_VDRIVE
    log_debug("DIR: %s name: '%s', length: %d, filetype: %d",
            __func__, name, length, filetype);
#endif

    l = p->buffer;

    /* small (short) read used as flag for multi-drive dir */
    if (!p->small) {

        filetype = CBMDOS_FT_DEL;

        if (cmd_parse->file && cmd_parse->filelength > 0) {
            name = lib_strdup((const char *)cmd_parse->file);
            length = cmd_parse->filelength;
        } else if (cmd_parse->colon) {
            /* handle "$:" situations */
            name = lib_malloc(1);
            name[0] = 0;
            length = 1;
        } else {
            name = lib_msprintf("*");
            length = 1;
        }

        limit = &(name[length]);

        c = memchr(name, '=', length);
        newlen = length;
        if (c) {
            newlen = (int) (c - name);
        }

        vdrive_dir_find_first_slot(vdrive, (uint8_t *)name, newlen, filetype, &p->dir);

        if (c) {
            while (c < limit) {
                switch (*c) {
                    case ',':
                    case '=':
                        break;

                    case 'L':
                        p->timemode = 2;
                        break;

                    case 'N':
                        p->timemode = 0;
                        break;

                    case 'S':
                        filetype = CBMDOS_FT_SEQ;
                        break;

                    case 'P':
                        filetype = CBMDOS_FT_PRG;
                        break;

                    case 'U':
                        filetype = CBMDOS_FT_USR;
                        break;

                    case 'R':
                        filetype = CBMDOS_FT_REL;
                        break;

                    case 'C':
                        filetype = CBMDOS_FT_CBM;
                        break;

                    case 'D':
                        filetype = CBMDOS_FT_DIR;
                        break;

                    case '>':
                        if (c + 17 < limit) {
                            p->dir.time_low = asciidate_to_int(c + 1, 0);
                        }
                        c = memchr(c, ',', limit - c);
                        if (!c) {
                            c = limit;
                        }
                        break;

                    case '<':
                        if (c + 17 < limit) {
                            p->dir.time_high = asciidate_to_int(c + 1, 0xffffffff);
                        }
                        c = memchr(c, ',', limit - c);
                        if (!c) {
                            c = limit;
                        }
                        break;

                    default:
                        break;
                }
                c++;
            }
        }

        p->dir.find_type = filetype;

        /* start address */
        *l++ = 1;
        *l++ = 4;

    } else {

        p->dir.track = vdrive->Header_Track;
        p->dir.sector = vdrive->Header_Sector;
        p->dir.slot = 7;

        vdrive_read_sector(vdrive, p->dir.buffer, p->dir.track, p->dir.sector);

        /* old drives may have needed this, but NP's keep their info correct */
        if (vdrive->image_format != VDRIVE_IMAGE_FORMAT_NP) {
            p->dir.buffer[0] = vdrive->Dir_Track;
            p->dir.buffer[1] = vdrive->Dir_Sector;
        }

        /* no start address here */
    }

    /* line link */
    *l++ = 1;
    *l++ = 1;

    /* current partition or drive */
    *l++ = vdrive->current_part;
    *l++ = 0;

    *l++ = (uint8_t)0x12;          /* Reverse on */
    *l++ = '"';

    memcpy(l, &p->dir.buffer[vdrive->bam_name], 16);
    vdrive_dir_no_a0_pads(l, 16);
    l += 16;
    *l++ = '"';
    *l++ = ' ';
    memcpy(l, &p->dir.buffer[vdrive->bam_id], 5);
    vdrive_dir_no_a0_pads(l, 5);
    l += 5;
    /* add in extra two spaces lost to the starting address on the subsequent
        headers */
    if (p->small) {
        *l++ = ' ';
        *l++ = ' ';
    }
    *l++ = 0;

    p->bufptr = (unsigned int)(l - p->buffer);

    p->small = 1;

    return p->bufptr - 1;
}

int vdrive_dir_next_directory(vdrive_t *vdrive, bufferinfo_t *b)
{
    uint8_t *l, *p;
    int blocks, i;

    b->small = 0;

    while ((p = vdrive_dir_find_next_slot(&b->dir))) {
        if (p[SLOT_TYPE_OFFSET]) {
            l = b->buffer + b->bufptr;

            *l++ = 1;
            *l++ = 1;

            /*
             * Length and spaces
             */
            *l++ = p[SLOT_NR_BLOCKS];
            *l++ = p[SLOT_NR_BLOCKS + 1];

            memset(l, 32, 27);
            l[27] = 0;

            blocks = p[SLOT_NR_BLOCKS] + p[SLOT_NR_BLOCKS + 1] * 256;

            if (blocks < 10) {
                l++;
            }
            if (blocks < 100) {
                l++;
            }
            l++;

            *l++ = '"';

            memcpy(l, &p[SLOT_NAME_OFFSET], 16);

            for (i = 0; (i < 16) && (p[SLOT_NAME_OFFSET + i] != 0xa0); ) {
                i++;
            }

            vdrive_dir_no_a0_pads(l, 16);

            l[i] = '"';

            /*
             * Type + End
             * There are 3 spaces or < and 2 spaces after the filetype.
             * Well, not exactly - the whole directory entry is 32 byte long
             * (including nullbyte).
             * Depending on the file size, there are more or less spaces
             */

            l[17] = (p[SLOT_TYPE_OFFSET] & CBMDOS_FT_CLOSED) ? ' ' : '*';
            if (b->timemode) {
                /* short list w/ month/day hour.min A/P */
                char ap = 'A';
                int hour =  p[SLOT_GEOS_HOUR];
                if (hour == 0) hour=12;
                else if (hour == 12) ap = 'P';
                else if (hour > 12) {
                    ap = 'P';
                    hour -= 12;
                }
                b->small = 1;
                if (b->timemode==1) {
                    l[18] = *cbmdos_filetype_get(p[SLOT_TYPE_OFFSET] & 0x07);
                    sprintf((char*) l + 19, " %02d/%02d %02d.%02d %c",p[SLOT_GEOS_MONTH],
                        p[SLOT_GEOS_DATE], hour, p[SLOT_GEOS_MINUTE], ap);
                    /* CMD pads the entry with 1's; each entry is 42 bytes */
                    for (l+=33;l<(b->buffer + b->bufptr + 41);l++) *l = 1;
                } else { /* must be long */
                    memcpy(l + 18, cbmdos_filetype_get(p[SLOT_TYPE_OFFSET] & 0x07), 3);
                    l[21] = (p[SLOT_TYPE_OFFSET] & CBMDOS_FT_LOCKED) ? '<' : ' ';
                    sprintf((char *) l + 22, "  %02d/%02d/%02d   %02d.%02d %cM",p[SLOT_GEOS_MONTH],
                        p[SLOT_GEOS_DATE], p[SLOT_GEOS_YEAR] % 100, hour, p[SLOT_GEOS_MINUTE], ap);
                    /* CMD pads the entry with 1's; each entry is 64 bytes */
                    for (l+=43;l<(b->buffer + b->bufptr + 63);l++) *l = 1;
                }
                *l = 0;
                return (int)(l - (b->buffer + b->bufptr));
            } else {
                /* standard output */
                memcpy(l + 18, cbmdos_filetype_get(p[SLOT_TYPE_OFFSET] & 0x07), 3);
                l[21] = (p[SLOT_TYPE_OFFSET] & CBMDOS_FT_LOCKED) ? '<' : ' ';
                b->bufptr = (b->bufptr + 32) & 255;

                if (b->bufptr == 0) {
                    return 0;
                }
            }
        }
    }

    vdrive->dir_count--;

    blocks = vdrive_bam_free_block_count(vdrive);

    l = b->buffer + b->bufptr;
    *l++ = 1;
    *l++ = 1;
    *l++ = blocks;
    *l++ = blocks >> 8;
    memcpy(l, "BLOCKS FREE.", 12);
    memset(l + 12, ' ', 15);

    /* if we are doing a multi drive list, check other drive */
    if (vdrive->dir_count) {
        blocks = b->partition;
        b->partition = (vdrive->dir_part + 1) % NUM_DRIVES;
        /* switch to the other drive if possible */
        if (vdrive_iec_switch(vdrive, b)) {
            /* no, don't do it; follow through */
            vdrive->dir_count = 0;
        }
        /* switch back */
        b->partition = blocks;
        vdrive_iec_switch(vdrive, b);
    }

    if (vdrive->dir_count) {
        b->small = 1;
        vdrive->dir_part = (vdrive->dir_part + 1) % NUM_DRIVES;
        b->mode = BUFFER_DIRECTORY_MORE_READ;
    } else {
        l[25] = 0;
        l[26] = 0;
    }
    l[27] = 0;
    return b->bufptr + 31;
}

void vdrive_dir_updatetime(vdrive_t *vdrive, uint8_t *slot)
{
    time_t timep;
    struct tm *ts;

    time(&timep);
    ts = localtime(&timep);

/* CMDHD's don't like the year > 99, it will screw up the date/time output */

    slot[SLOT_GEOS_YEAR]   = ts->tm_year % 100;
    slot[SLOT_GEOS_MONTH]  = ts->tm_mon + 1;
    slot[SLOT_GEOS_DATE]   = ts->tm_mday;
    slot[SLOT_GEOS_HOUR]   = ts->tm_hour;
    slot[SLOT_GEOS_MINUTE] = ts->tm_min;

    return;
}

/* shamelessly duplicate dir code from above to do the partition listings */
void vdrive_dir_part_find_first_slot(vdrive_t *vdrive, const uint8_t *name,
                                int length, unsigned int type,
                                vdrive_dir_context_t *dir)
{
    if (length > 0) {
        uint8_t *nslot;

        nslot = cbmdos_dir_slot_create((char*)name, length);
        memcpy(dir->find_nslot, nslot, CBMDOS_SLOT_NAME_LENGTH);
        lib_free(nslot);
    }

    dir->vdrive = vdrive;
    dir->find_length = length;
    dir->find_type = type;

    dir->track = 1;
    dir->sector = 0;
    dir->slot = 7;

    dir->buffer[0] = 1;
    dir->buffer[1] = 0;
}

static unsigned int vdrive_dir_part_name_match(uint8_t *slot, uint8_t *nslot, int type)
{
    if (!slot[PSLOT_TYPE]) {
        return 0;
    }

    if (type != 0 && type != slot[PSLOT_TYPE]) {
        return 0;
    }

    return cbmdos_parse_wildcard_compare(nslot, &slot[PSLOT_NAME]);
}

uint8_t *vdrive_dir_part_find_next_slot(vdrive_dir_context_t *dir)
{
    static uint8_t return_slot[32];
    vdrive_t *vdrive = dir->vdrive;

#ifdef DEBUG_DRIVE
    log_debug("DIR: vdrive_dir_find_next_slot start (t:%u/s:%u) #%u", dir->track, dir->sector, dir->slot);
#endif
    /*
     * Loop all partition entrys starting at 1,0 on system partition
     */

    do {
        /*
         * Load next(first) directory block ?
         */

        dir->slot++;

        if (dir->slot >= 8) {
            int status;

            /* end of current directory? */
            if (dir->buffer[0] == 0) {
                break;
            }

            dir->slot = 0;
            dir->track = (unsigned int)dir->buffer[0];
            dir->sector = (unsigned int)dir->buffer[1];

            status = vdrive_read_sector(vdrive, dir->buffer, dir->track, dir->sector);
            if (status != 0) {
                return NULL; /* error */
            }
        }
        if (vdrive_dir_part_name_match(&dir->buffer[dir->slot * 32],
                                  dir->find_nslot,
                                  dir->find_type)) {
            memcpy(return_slot, &dir->buffer[dir->slot * 32], 32);
            return return_slot;
        }
    } while (1);

#ifdef DEBUG_DRIVE
    log_debug("DIR: vdrive_dir_find_next_slot (t:%u/s:%u) #%u", dir->track, dir->sector, dir->slot);
#endif

    return NULL;
}

int vdrive_dir_part_next_directory(vdrive_t *vdrive, bufferinfo_t *b)
{
    uint8_t *l, *p;
    int blocks, i;
    uint8_t ptypes[8][4] = {"SYS", "NAT", "41 ", "71 ", "81 ", "C81", "PRN", "FOR"};

    while ((p = vdrive_dir_part_find_next_slot(&b->dir))) {
        if (p[PSLOT_TYPE]) {
            l = b->buffer + b->bufptr;

            *l++ = 1;
            *l++ = 1;

            /*
             * Length and spaces
             */
            blocks = (b->dir.sector << 3) | b->dir.slot;

            *l++ = blocks & 255;
            *l++ = 0;

            memset(l, 32, 27);
            l[27] = 0;

            if (blocks < 10) {
                l++;
            }
            if (blocks < 100) {
                l++;
            }
            l++;

            *l++ = '"';

            memcpy(l, &p[PSLOT_NAME], 16);

            for (i = 0; (i < 16) && (p[PSLOT_NAME + i] != 0xa0); ) {
                i++;
            }

            vdrive_dir_no_a0_pads(l, 16);

            l[i] = '"';

            i = p[SLOT_TYPE_OFFSET];
            if (i==255) i = 0;
            memcpy(l + 18, ptypes[i], 3);
            b->bufptr = (b->bufptr + 32) & 255;

            if (b->bufptr == 0) {
                return 0;
            }
        }
    }

    l = b->buffer + b->bufptr;
    *l++ = 0;
    *l++ = 0;
    *l++ = 0;
    return b->bufptr + 3;
}

int vdrive_dir_part_first_directory(vdrive_t *vdrive, const uint8_t *name,
                               int length, bufferinfo_t *p)
{
    uint8_t *l;
    const uint8_t *c, *limit;
    int ptype;

#ifdef DEBUG_VDRIVE
    log_debug("DIRPART: %s name: '%s', length: %d, filetype: %d",
            __func__, name, length, filetype);
#endif

    ptype = 0;

    if (length < 1) {
        name = (uint8_t*)"*";
        length = 1;
    }

    limit = &(name[length]);

    c = memchr(name,'=',length);
    if (c) {
        length = (int) (c - name);
    }

    if (c && c+1<limit) {
        switch (c[1]) {
            case 'N':
                ptype = 1;
                break;

            case '4':
                ptype = 2;
                break;

            case '7':
                ptype = 3;
                break;

            case '8':
                ptype = 4;
                break;

            case 'C':
                ptype = 5;
                break;

        }
    }

    vdrive_dir_part_find_first_slot(vdrive, name, length, ptype, &p->dir);

    /*
     * Start Address, Line Link and Line number 0 or current partition
     */

    l = p->buffer;

    *l++ = 1;
    *l++ = 4;

    *l++ = 1;
    *l++ = 1;

    *l++ = 255;
    *l++ = 0;

    l[25] = 0;

    *l++ = (uint8_t)0x12;          /* Reverse on */
    *l++ = '"';

    if (VDRIVE_IS_HD(vdrive)) {
        memcpy(l, "CMD HD          ", 16);
    } else {
        memcpy(l, "CMD FD          ", 16);
    }
    vdrive_dir_no_a0_pads(l, 16);
    l += 16;
    *l++ = '"';
    *l++ = ' ';
    if (VDRIVE_IS_HD(vdrive)) {
        memcpy(l, "HD 1H", 5);
    } else {
        memcpy(l, "FD 1H", 5);
    }
    vdrive_dir_no_a0_pads(l, 5);

    p->bufptr = 32;

    return vdrive_dir_part_next_directory(vdrive, p);
}

