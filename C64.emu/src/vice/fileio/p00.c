/*
 * p00.c - Utility functions for P00 file support.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * Based on older code by
 *  Dan Fandrich <dan@fch.wimsey.bc.ca>
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cbmdos.h"
#include "fileio.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "p00.h"
#include "rawfile.h"
#include "types.h"
#include "util.h"


/* P00 Header structure:

   typedef struct {
       BYTE Magic[8];
       char CbmName[17];
       BYTE RecordSize;
   } X00HDR; */

#define P00_HDR_MAGIC_OFFSET      0
#define P00_HDR_MAGIC_LEN         8
#define P00_HDR_CBMNAME_OFFSET    8
#define P00_HDR_CBMNAME_LEN       17
#define P00_HDR_RECORDSIZE_OFFSET 25
#define P00_HDR_RECORDSIZE_LEN    1

#define P00_HDR_LEN               26

static const BYTE p00_hdr_magic_string[8] = "C64File";

/* FIXME: There should be an enum for file types.  */
static int p00_check_name(const char *name)
{
    int t = -1;
    char *p;

    if (name == NULL || (p = strrchr(name, '.')) == NULL || strlen(++p) != 3) {
        return -1;
    }

    if (!isdigit((int)p[1]) || !isdigit((int)p[2])) {
        return -1;
    }

    switch (util_toupper(*p)) {
        case 'D':
            t = CBMDOS_FT_DEL;
            break;
        case 'S':
            t = CBMDOS_FT_SEQ;
            break;
        case 'P':
            t = CBMDOS_FT_PRG;
            break;
        case 'U':
            t = CBMDOS_FT_USR;
            break;
        case 'R':
            t = CBMDOS_FT_REL;
            break;
    }

    return t;
}

static int p00_read_header(struct rawfile_info_s *info, BYTE *cbmname_return,
                           unsigned int *recsize_return)
{
    BYTE hdr[P00_HDR_LEN];

    if (rawfile_read(info, hdr, P00_HDR_LEN) != P00_HDR_LEN) {
        return -1;
    }

    if (memcmp(hdr + P00_HDR_MAGIC_OFFSET, p00_hdr_magic_string,
               P00_HDR_MAGIC_LEN) != 0) {
        return -1;
    }

    memcpy(cbmname_return, hdr + P00_HDR_CBMNAME_OFFSET, P00_HDR_CBMNAME_LEN);

    if (recsize_return != NULL) {
        *recsize_return = (unsigned int)hdr[P00_HDR_RECORDSIZE_OFFSET];
    }

    return 0;
}

static int p00_write_header(struct rawfile_info_s *info, const BYTE *cbmname,
                            BYTE recsize)
{
    BYTE hdr[P00_HDR_LEN];

    memset(hdr, 0, sizeof(hdr));

    memcpy(hdr + P00_HDR_MAGIC_OFFSET, p00_hdr_magic_string,
           P00_HDR_MAGIC_LEN);
    memcpy(hdr + P00_HDR_CBMNAME_OFFSET, cbmname, P00_HDR_CBMNAME_LEN);
    hdr[P00_HDR_RECORDSIZE_OFFSET] = (BYTE)recsize;

    if (rawfile_seek_set(info, 0) != 0) {
        return -1;
    }

    if (rawfile_write(info, hdr, P00_HDR_LEN) != P00_HDR_LEN) {
        return -1;
    }

    return 0;
}

static void p00_pad_a0(BYTE *slot)
{
    unsigned int index;

    for (index = 0; index < CBMDOS_SLOT_NAME_LENGTH; index++) {
        if (slot[index] == 0) {
            slot[index] = 0xa0;
        }
    }
}

static char *p00_file_find(const char *file_name, const char *path)
{
    struct ioutil_dir_s *ioutil_dir;
    struct rawfile_info_s *rawfile;
    BYTE p00_header_file_name[P00_HDR_CBMNAME_LEN];
    char *name, *alloc_name = NULL;
    int rc;

    ioutil_dir = ioutil_opendir(path);

    if (ioutil_dir == NULL) {
        return NULL;
    }

    while (1) {
        name = ioutil_readdir(ioutil_dir);

        if (name == NULL) {
            break;
        }

        if (p00_check_name(name) < 0) {
            continue;
        }

        rawfile = rawfile_open(name, path, FILEIO_COMMAND_READ);
        if (rawfile == NULL) {
            continue;
        }

        rc = p00_read_header(rawfile, (BYTE *)p00_header_file_name, NULL);

        if (rc >= 0) {
            BYTE *cname;
            unsigned int equal;

            p00_pad_a0(p00_header_file_name);
            cname = cbmdos_dir_slot_create(file_name, (unsigned int)strlen(file_name));
            equal = cbmdos_parse_wildcard_compare(cname, p00_header_file_name);
            lib_free(cname);

            if (equal > 0) {
                alloc_name = lib_stralloc(name);
            } else {
                rc = -1;
            }
        }

        rawfile_destroy(rawfile);

        if (rc >= 0) {
            break;
        }
    }

    ioutil_closedir(ioutil_dir);

    return alloc_name;
}

static size_t p00_eliminate_char_p00(char *filename, int pos)
{
    memcpy(&filename[pos], &filename[pos + 1], 16 - pos);
    return strlen(filename);
}

static int p00_reduce_filename_p00(char *filename, int len)
{
    int i, j;

    for (i = len - 1; i >= 0; i--) {
        if (filename[i] == '_') {
            if (p00_eliminate_char_p00(filename, i) <= 8) {
                return 8;
            }
        }
    }

    for (i = 0; i < len; i++) {
        if (strchr("AEIOU", filename[i]) != NULL) {
            break;
        }
    }

    for (j = len - 1; j >= i; j--) {
        if (strchr("AEIOU", filename[j]) != NULL) {
            if (p00_eliminate_char_p00(filename, j) <= 8) {
                return 8;
            }
        }
    }

    for (i = len - 1; i >= 0; i--) {
        if (isalpha((int) filename[i])) {
            if (p00_eliminate_char_p00(filename, i) <= 8) {
                return 8;
            }
        }
    }

    for (i = len - 1; i >= 0; i--) {
        if (p00_eliminate_char_p00(filename, i) <= 8) {
            return 8;
        }
    }

    return 1;
}

static char *p00_evaluate_name(const char *name, int length)
{
    int i, j;
    char *filename;

    filename = lib_calloc(1, 17);

    for (i = 0, j = 0; i < length; i++) {
        switch (name[i]) {
            case ' ':
            case '-':
                filename[j++] = '_';
                break;
            default:
                if (islower((int)name[i])) {
                    filename[j++] = util_toupper(name[i]);
                    break;
                }
                if (isalnum((int)name[i])) {
                    filename[j++] = name[i];
                    break;
                }
        }
    }
    if (j == 0) {
        strcpy(filename, "_");
        j++;
    }

    if (j > 8) {
        p00_reduce_filename_p00(filename, j);
    }

    return filename;
}

static char *p00_filename_create(const char *file_name, unsigned int type)
{
    char *p00name, *main_name;
    const char *typeext = NULL;
    int length;

    length = (int)strlen(file_name);

    if (length > 16) {
        length = 16;
    }

    main_name = p00_evaluate_name(file_name, length);

    switch (type) {
        case FILEIO_TYPE_DEL:
            typeext = "D";
            break;
        case FILEIO_TYPE_SEQ:
            typeext = "S";
            break;
        case FILEIO_TYPE_PRG:
            typeext = "P";
            break;
        case FILEIO_TYPE_USR:
            typeext = "U";
            break;
        case FILEIO_TYPE_REL:
            typeext = "R";
            break;
    }

    p00name = util_concat(main_name, FSDEV_EXT_SEP_STR, typeext, "00", NULL);
    lib_free(main_name);

    return p00name;
}

static char *p00_file_create(const char *file_name, const char *path,
                             unsigned int type)
{
    char *p00name;
    unsigned int i;

    p00name = p00_filename_create(file_name, type);

    for (i = 1; i < 100; i++) {
        if (util_file_exists(p00name) == 0) {
            break;
        }
        sprintf(&p00name[strlen(p00name) - 2], "%02i", i);
    }

    if (i >= 100) {
        return NULL;
    }

    return p00name;
}

fileio_info_t *p00_open(const char *file_name, const char *path,
                        unsigned int command, unsigned int open_type)
{
    char rname[20]; /* FIXME */
    fileio_info_t *info;
    struct rawfile_info_s *rawfile;
    char *fname = NULL;
    int type;

    if (command & FILEIO_COMMAND_FSNAME) {
        fname = lib_stralloc(file_name);
    } else {
        switch (command & FILEIO_COMMAND_MASK) {
            case FILEIO_COMMAND_STAT:
            case FILEIO_COMMAND_READ:
            case FILEIO_COMMAND_APPEND:
            case FILEIO_COMMAND_APPEND_READ:
                fname = p00_file_find(file_name, path);
                break;
            case FILEIO_COMMAND_WRITE:
                fname = p00_file_create(file_name, path, open_type);
                break;
        }
    }

    if (fname == NULL) {
        return NULL;
    }

    type = p00_check_name(fname);

    rawfile = rawfile_open(fname, path, command & FILEIO_COMMAND_MASK);
    lib_free(fname);

    if (rawfile == NULL) {
        return NULL;
    }

    switch (command & FILEIO_COMMAND_MASK) {
        case FILEIO_COMMAND_STAT:
        case FILEIO_COMMAND_READ:
            if (type < 0 || p00_read_header(rawfile, (BYTE *)rname, NULL) < 0) {
                rawfile_destroy(rawfile);
                return NULL;
            }
            break;
        case FILEIO_COMMAND_APPEND:
        case FILEIO_COMMAND_APPEND_READ:
            if (type < 0 || p00_read_header(rawfile, (BYTE *)rname, NULL) < 0) {
                rawfile_destroy(rawfile);
                return NULL;
            }
/*
        if (fseek((FILE *)(rawfile->fd), 0, SEEK_END) != 0)
            rawfile_destroy(rawfile);
            return NULL;
*/
            break;
        case FILEIO_COMMAND_WRITE:
            memset(rname, 0, sizeof(rname));
            strncpy(rname, file_name, 16);
            if (p00_write_header(rawfile, (BYTE *)rname, 0) < 0) {
                rawfile_destroy(rawfile);
                return NULL;
            }
            break;
    }

    info = lib_malloc(sizeof(fileio_info_t));
    info->name = (BYTE *)lib_stralloc(rname);
    info->length = (unsigned int)strlen((char *)(info->name));
    info->type = (unsigned int)type;
    info->format = FILEIO_FORMAT_P00;
    info->rawfile = rawfile;

    return info;
}

void p00_close(fileio_info_t *info)
{
    rawfile_destroy(info->rawfile);
}

unsigned int p00_read(fileio_info_t *info, BYTE *buf, unsigned int len)
{
    return rawfile_read(info->rawfile, buf, len);
}

unsigned int p00_write(fileio_info_t *info, BYTE *buf, unsigned int len)
{
    return rawfile_write(info->rawfile, buf, len);
}

unsigned int p00_ferror(fileio_info_t *info)
{
    return rawfile_ferror(info->rawfile);
}

unsigned int p00_rename(const char *src_name, const char *dst_name,
                        const char *path)
{
    char *p00_src, *p00_dst, rname[20];
    int type;
    struct rawfile_info_s *rawfile;
    unsigned int rc;

    p00_dst = p00_file_find(dst_name, path);

    if (p00_dst != NULL) {
        lib_free(p00_dst);
        return FILEIO_FILE_EXISTS;
    }

    p00_src = p00_file_find(src_name, path);

    if (p00_src == NULL) {
        return FILEIO_FILE_NOT_FOUND;
    }

    type = p00_check_name(p00_src);

    if (type < 0) {
        return FILEIO_FILE_NOT_FOUND;
    }

    rawfile = rawfile_open(p00_src, path, FILEIO_COMMAND_APPEND);

    if (rawfile == NULL) {
        lib_free(p00_src);
        return FILEIO_FILE_NOT_FOUND;
    }

    memset(rname, 0, sizeof(rname));
    strncpy(rname, dst_name, 16);

    if (p00_write_header(rawfile, (BYTE *)rname, 0) < 0) {
        rawfile_destroy(rawfile);
        lib_free(p00_src);
        return FILEIO_FILE_NOT_FOUND;
    }

    rawfile_destroy(rawfile);

    p00_dst = p00_file_create(dst_name, path, type);

    if (p00_dst == NULL) {
        lib_free(p00_src);
        return FILEIO_FILE_NOT_FOUND;
    }

    rc = rawfile_rename(p00_src, p00_dst, path);

    lib_free(p00_src);
    lib_free(p00_dst);

    return rc;
}

unsigned int p00_scratch(const char *file_name, const char *path)
{
    char *p00_src;
    unsigned int rc;

    p00_src = p00_file_find(file_name, path);

    if (p00_src == NULL) {
        return FILEIO_FILE_NOT_FOUND;
    }

    rc = rawfile_remove(p00_src, path);

    lib_free(p00_src);

    return rc;
}

unsigned int p00_get_bytes_left(struct fileio_info_s *info)
{
    return rawfile_get_bytes_left(info->rawfile);
}
