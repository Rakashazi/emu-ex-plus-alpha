/** \file   src/tape/t64.h
 * \brief   T64 file support
 *
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

/** \brief  Size of a T64 header n bytes */
#define T64_HDR_SIZE                    64

/** \brief  Offset in header of the 'magic' bytes
 *
 * ASCII string padded with 0x00
 */
#define T64_HDR_MAGIC_OFFSET            0

/** \brief  Size of the 'magic' bytes */
#define T64_HDR_MAGIC_LEN               32

/** \brief  Offset in header of version number */
#define T64_HDR_VERSION_OFFSET          32

/** \brief  Size in bytes of the version number */
#define T64_HDR_VERSION_LEN             2

/** \brief  Offset in header of available file records
 *
 * This value includes file records in use. Highly unreliable.
 */
#define T64_HDR_NUMENTRIES_OFFSET       34

/** \brief  Size of file record count value in bytes */
#define T64_HDR_NUMENTRIES_LEN          2

/** \brief  Offset in header of the number of file records in use
 *
 * Highly unreliable, values of 0 not uncommon while the T64 actually contains
 * a single file.
 */
#define T64_HDR_NUMUSED_OFFSET          36

/** \brief  Size in bytes of the file records in use value
 */
#define T64_HDR_NUMUSED_LEN             2

/** \brief  Offset in header of the tape description string
 *
 * String is in PETSCII, padded with 0x20, not 0xa0, and doesn't contain a
 * terminating nul character
 */
#define T64_HDR_DESCRIPTION_OFFSET      40

/** \brief  Size in bytes of the tape description */
#define T64_HDR_DESCRIPTION_LEN         24


/** \brief  T64 header structure
 */
typedef struct t64_header_s {
    BYTE magic[T64_HDR_MAGIC_LEN];  /**< 'magic' bytes of the container */
    WORD version;                   /**< version number, usually either 0x100
                                         or 0x101 */
    WORD num_entries;               /**< total available file records */
    WORD num_used;                  /**< file records in use */
    BYTE description[T64_HDR_DESCRIPTION_LEN];  /**< tape description string */
} t64_header_t;


/*
 * Constants related to file records
 */

/** \brief  Size of a single file record
 */
#define T64_REC_SIZE              32

/** \brief  Offset to entry type */
#define T64_REC_ENTRYTYPE_OFFSET        0
/** \brief  Size of entry type in bytes */
#define T64_REC_ENTRYTYPE_LEN           1

/** \brief  Offset to C1541 file type */
#define T64_REC_CBMTYPE_OFFSET          1
/** \brief  Size of C1541 file type */
#define T64_REC_CBMTYPE_LEN             1

/** \brief  Offset to start address of file data */
#define T64_REC_STARTADDR_OFFSET        2
/** \brief  Size of start address in bytes */
#define T64_REC_STARTADDR_LEN           2

/** \brief  Offset to end adress of file data */
#define T64_REC_ENDADDR_OFFSET          4
/** \brief  Size of end adress in bytes */
#define T64_REC_ENDADDR_LEN             2

/** \brief  Offset to offset in the T64 file of the file data */
#define T64_REC_CONTENTS_OFFSET         8
/** \brief  Size of the offset in the T64 to the file data */
#define T64_REC_CONTENTS_LEN            4

/** \brief  Offset to PETSCII filename */
#define T64_REC_CBMNAME_OFFSET          16
/** \brief  Size of PETSCII filename */
#define T64_REC_CBMNAME_LEN             16


/** \brief  File record type enumerator
 */
enum t64_file_record_type_s {
    T64_FILE_RECORD_FREE,       /**< unused record */
    T64_FILE_RECORD_NORMAL,     /**< normal record */
    T64_FILE_RECORD_HEADER,
    T64_FILE_RECORD_SNAPSHOT,   /**< C64S snapshot file */
    T64_FILE_RECORD_BLOCK,
    T64_FILE_RECORD_STREAM
};
typedef enum t64_file_record_type_s t64_file_record_type_t;


/** \brief  T64 file record object
 *
 * Contains data on a single file record in the T64 file
 */
struct t64_file_record_s {
    t64_file_record_type_t entry_type;  /**< T64 entry type */
    BYTE cbm_name[T64_REC_CBMNAME_LEN]; /**< PETSCII filename */
    BYTE cbm_type;                      /**< C1541 file type */
    WORD start_addr;                    /**< start address on machine */
    WORD end_addr;                      /**< end address on machine */
    DWORD contents;                     /**< offset in T64 to file contents */
    int index;  /**< index of the record in the container, required to restore
                     the orginal record order after fixing end addresses */
};
typedef struct t64_file_record_s t64_file_record_t;



/** \brief  T64 container object
 *
 * Contains data on the T64 file: header, file records and position in the T64
 * and its current file record.
 */
struct t64 {
    /** \brief  file name on host OS */
    char *file_name;

    /** \brief  file descriptor */
    FILE *fd;

    /** \brief  parsed header of the T64 */
    t64_header_t header;

    /** \brief  file records
     *
     * Dynamically allocated array of file records.  Some of them can be
     * empty (`T64_FILE_RECORD_FREE')
     */
    t64_file_record_t *file_records;

    /** \brief  number of the current file
     *
     * -1 means that there is no current file
     */
    int current_file_number;

    /** \brief  position in the current file
     *
     * \todo    Should be long since fread(3) returns long, not int
     */
    int current_file_seek_position;
};
typedef struct t64 t64_t;


/* XXX: The following functions could be made static, they are only used in
 *      their own module
 *
 * t64_header_read()
 * t64_file_record_read()
 * t64_file_record_get_size()
 * t64_new()
 * t64_destroy()
 * t64_get_file_record()
 */

extern t64_t *t64_open(const char *name, unsigned int *read_only);
extern int t64_close(t64_t *t64);

extern int t64_seek_start(t64_t *t64);
extern int t64_seek_to_file(t64_t *t64, int file_number);
extern int t64_seek_to_next_file(t64_t *t64, unsigned int allow_rewind);
extern t64_file_record_t *t64_get_current_file_record(t64_t *t64);
extern int t64_read(t64_t *t64, BYTE *buf, size_t size);
extern void t64_get_header(t64_t *t64, BYTE *name);

#endif
