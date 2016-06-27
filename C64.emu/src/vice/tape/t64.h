/*
 * t64.h - T64 file support.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_T64_H
#define VICE_T64_H

#include <stdio.h>

#include "types.h"

#define T64_HDR_SIZE                    64

#define T64_HDR_MAGIC_OFFSET            0
#define T64_HDR_MAGIC_LEN               32
#define T64_HDR_VERSION_OFFSET          32
#define T64_HDR_VERSION_LEN             2
#define T64_HDR_NUMENTRIES_OFFSET       34
#define T64_HDR_NUMENTRIES_LEN          2
#define T64_HDR_NUMUSED_OFFSET          36
#define T64_HDR_NUMUSED_LEN             2
#define T64_HDR_DESCRIPTION_OFFSET      40
#define T64_HDR_DESCRIPTION_LEN         24

struct t64_header_s {
    BYTE magic[T64_HDR_MAGIC_LEN];
    WORD version;
    WORD num_entries;
    WORD num_used;
    BYTE description[T64_HDR_DESCRIPTION_LEN];
};
typedef struct t64_header_s t64_header_t;

#define T64_REC_SIZE              32

#define T64_REC_ENTRYTYPE_OFFSET        0
#define T64_REC_ENTRYTYPE_LEN           1
#define T64_REC_CBMTYPE_OFFSET          1
#define T64_REC_CBMTYPE_LEN             1
#define T64_REC_STARTADDR_OFFSET        2
#define T64_REC_STARTADDR_LEN           2
#define T64_REC_ENDADDR_OFFSET          4
#define T64_REC_ENDADDR_LEN             2
#define T64_REC_CONTENTS_OFFSET         8
#define T64_REC_CONTENTS_LEN            4
#define T64_REC_CBMNAME_OFFSET          16
#define T64_REC_CBMNAME_LEN             16

enum t64_file_record_type_s {
    T64_FILE_RECORD_FREE,
    T64_FILE_RECORD_NORMAL,
    T64_FILE_RECORD_HEADER,
    T64_FILE_RECORD_SNAPSHOT,
    T64_FILE_RECORD_BLOCK,
    T64_FILE_RECORD_STREAM
};
typedef enum t64_file_record_type_s t64_file_record_type_t;

struct t64_file_record_s {
    t64_file_record_type_t entry_type;
    BYTE cbm_name[T64_REC_CBMNAME_LEN];
    BYTE cbm_type;
    WORD start_addr;
    WORD end_addr;
    DWORD contents;
};
typedef struct t64_file_record_s t64_file_record_t;

struct t64 {
    /* File name.  */
    char *file_name;

    /* File descriptor.  */
    FILE *fd;

    /* The T64 header.  */
    t64_header_t header;

    /* Dynamically allocated array of file records.  Some of them can be
       empty (`T64_FILE_RECORD_FREE').  */
    t64_file_record_t *file_records;

    /* Number of the current file.  `-1' means that there is no current
       file.  */
    int current_file_number;

    /* Position in the current file.  */
    int current_file_seek_position;
};
typedef struct t64 t64_t;

extern int t64_header_read(t64_header_t *hdr, FILE *fd);
extern int t64_file_record_read(t64_file_record_t *rec, FILE *fd);
extern int t64_file_record_get_size(t64_file_record_t *rec);

extern t64_t *t64_new(void);
extern void t64_destroy(t64_t *t64);

extern t64_t *t64_open(const char *name, unsigned int *read_only);
extern int t64_close(t64_t *t64);

extern int t64_seek_start(t64_t *t64);
extern int t64_seek_to_file(t64_t *t64, int file_number);
extern int t64_seek_to_next_file(t64_t *t64, unsigned int allow_rewind);
extern t64_file_record_t *t64_get_file_record(t64_t *t64, unsigned int num);
extern t64_file_record_t *t64_get_current_file_record(t64_t *t64);
extern int t64_read(t64_t *t64, BYTE *buf, size_t size);
extern void t64_get_header(t64_t *t64, BYTE *name);

#endif
