/*
 * rawfile.c - Raw file handling.
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

#include "vice.h"

#include <stdio.h>

#include "archdep.h"
#include "fileio.h"
#include "ioutil.h"
#include "lib.h"
#include "rawfile.h"
#include "util.h"


struct rawfile_info_s {
    FILE *fd;
    char *name;
    char *path;
    unsigned int read_only;
};
typedef struct rawfile_info_s rawfile_info_t;


rawfile_info_t *rawfile_open(const char *file_name, const char *path,
                             unsigned int command)
{
    rawfile_info_t *info;
    char *complete;
    FILE *fd;
    const char *mode = NULL;
    unsigned int isdir, len;

    if (path == NULL) {
        complete = lib_stralloc(file_name);
    } else {
        complete = util_concat(path, FSDEV_DIR_SEP_STR, file_name, NULL);
    }

    switch (command) {
        case FILEIO_COMMAND_STAT:
        case FILEIO_COMMAND_READ:
            mode = MODE_READ;
            break;
        case FILEIO_COMMAND_WRITE:
            mode = MODE_WRITE;
            break;
        case FILEIO_COMMAND_APPEND:
            mode = MODE_APPEND;
            break;
        case FILEIO_COMMAND_APPEND_READ:
            mode = MODE_APPEND_READ_WRITE;
            break;
        default:
            return NULL;
    }

    if (ioutil_stat(complete, &len, &isdir) != 0) {
        /* if stat failed exit early, except in write mode
           (since opening a non existing file creates a new file) */
        if (command != FILEIO_COMMAND_WRITE) {
            lib_free(complete);
            return NULL;
        }
    }

    info = lib_malloc(sizeof(rawfile_info_t));
    if ((isdir) && (command == FILEIO_COMMAND_STAT)) {
        info->fd = NULL;
        info->read_only = 1;
    } else {
        fd = fopen(complete, mode);
        if (fd == NULL) {
            lib_free(complete);
            lib_free(info);
            return NULL;
        }
        info->fd = fd;
        info->read_only = 0;
    }

    util_fname_split(complete, &(info->path), &(info->name));

    lib_free(complete);

    return info;
}

void rawfile_destroy(rawfile_info_t *info)
{
    if (info != NULL) {
        if (info->fd) {
            fclose(info->fd);
        }
        lib_free(info->name);
        lib_free(info->path);
        lib_free(info);
    }
}

unsigned int rawfile_read(rawfile_info_t *info, BYTE *buf, unsigned int len)
{
    if (info->fd) {
        return (unsigned int)fread(buf, 1, len, info->fd);
    }
    return -1;
}

unsigned int rawfile_write(rawfile_info_t *info, BYTE *buf, unsigned int len)
{
    if (info->fd) {
        return (unsigned int)fwrite(buf, 1, len, info->fd);
    }
    return -1;
}

int rawfile_seek_set(rawfile_info_t *info, int offset)
{
    return fseek(info->fd, offset, SEEK_SET);
}

unsigned int rawfile_get_bytes_left(struct rawfile_info_s *info)
{
    unsigned int old_pos = ftell(info->fd);
    unsigned int size;
    fseek(info->fd, 0, SEEK_END);
    size = ftell(info->fd);
    fseek(info->fd, old_pos, SEEK_SET);
    return size - old_pos;
}

unsigned int rawfile_ferror(rawfile_info_t *info)
{
    return (unsigned int)ferror(info->fd);
}

unsigned int rawfile_rename(const char *src_name, const char *dst_name,
                            const char *path)
{
    char *complete_src, *complete_dst;
    int rc;

    if (path == NULL) {
        complete_src = lib_stralloc(src_name);
        complete_dst = lib_stralloc(dst_name);
    } else {
        complete_src = util_concat(path, FSDEV_DIR_SEP_STR, src_name, NULL);
        complete_dst = util_concat(path, FSDEV_DIR_SEP_STR, dst_name, NULL);
    }

    /*ioutil_remove(dst_name);*/
    rc = ioutil_rename(complete_src, complete_dst);

    lib_free(complete_src);
    lib_free(complete_dst);

    if (rc < 0) {
        if (ioutil_errno(IOUTIL_ERRNO_EPERM)) {
            return FILEIO_FILE_PERMISSION;
        }
        return FILEIO_FILE_NOT_FOUND;
    }

    return FILEIO_FILE_OK;
}

unsigned int rawfile_remove(const char *src_name, const char *path)
{
    char *complete_src;
    int rc;

    if (path == NULL) {
        complete_src = lib_stralloc(src_name);
    } else {
        complete_src = util_concat(path, FSDEV_DIR_SEP_STR, src_name, NULL);
    }

    rc = ioutil_remove(complete_src);

    lib_free(complete_src);

    if (rc < 0) {
        return FILEIO_FILE_NOT_FOUND;
    }

    return FILEIO_FILE_SCRATCHED;
}
