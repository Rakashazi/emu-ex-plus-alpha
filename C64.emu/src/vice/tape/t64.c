/** \file   src/tape/t64.c
 * \brief   T64 file support
 *
 * t64.c - T64 file support.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Compyx <b.wassink@ziggo.nl>
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "t64.h"
#include "types.h"
#include "zfile.h"


/** \brief  magic bytes found at the start of a possible T64 file
 */
static const char *magic_headers[] = {
    "C64 tape image file",
    "C64S tape file",
    "C64S tape image file",
    NULL
};


/* --------------------------- Static functions -----------------------------*/

/** \brief  Parse \a n bytes of \a p for a little endian unsigned integer
 *
 * \param[in]   p   pointer to memory containing value
 * \param[in]   n   number of bytes to use from \a p
 *
 * \return  unsigned integer
 */
static DWORD get_number(const BYTE *p, unsigned int n)
{
    DWORD retval = 0;
    unsigned int weight = 1;
    unsigned int i;

    for (i = 0; i < n; i++, p++) {
        retval |= (DWORD)(*p * weight);
        weight <<= 8;
    }

    return retval;
}


/** \brief  Check \a hdr for a valid magic sequence
 *
 * \param[in]   hdr     T64 header
 *
 * \return  boolean
 */
static int check_magic(t64_header_t *hdr)
{
    const char **p;

    for (p = magic_headers; *p != NULL; p++) {
        if (memcmp(*p, hdr->magic, strlen(*p)) == 0) {
            return 1;
        }
    }

    return 0;
}


/** \brief  `compar` argument to qsort(3) call in t64_open()
 *
 * Compares offsets in T64 container of \a p1 and \p2 and returns a value like
 * strcmp(). Used to sort records on their data offset, which is required to
 * fix invalid end addresses of records.
 *
 * \param[in]   p1  T64 file record
 * \param[in]    p2  T64 file record
 *
 * \return  0 if equal, <0 if p1 < p2, >0 if p1 > p2
 */
static int comp_content(const void *p1, const void *p2)
{
    const t64_file_record_t *r1 = p1;
    const t64_file_record_t *r2 = p2;

    if (r1->contents < r2->contents) {
        return -1;
    } else if (r1->contents > r2->contents) {
        return 1;
    } else {
        return 0;
    }
}


/** \brief  `compar` argument to qsort(3) call in t64_open()
 *
 * Compares original index of \a p1 and \a p2 in T64 container and returns a
 * value like strcmp(). Required to restore the original order of records
 * after sorting on data offset.
 *
 * \param[in]   p1  T64 file record
 * \param[in]   p2  T64 file record
 *
 * \return  0 if equal, <0 if p1 < p2, >0 if p1 > p2
 */
static int comp_index(const void *p1, const void *p2)
{
    const t64_file_record_t *r1 = p1;
    const t64_file_record_t *r2 = p2;

    if (r1->index < r2->index) {
        return -1;
    } else if (r1->index > r2->index) {
        return 1;
    } else {
        return 0;
    }
}


/** \brief  Read and parse a file record
 *
 * \param[out]  rec     T64 file record
 * \param[in]   fd      file descriptor
 *
 * \return  0 on success, -1 on failure
 */
static int t64_file_record_read(t64_file_record_t *rec, FILE *fd)
{
    BYTE buf[T64_REC_SIZE];

    if (fread(buf, T64_REC_SIZE, 1, fd) != 1) {
        return -1;
    }

    rec->entry_type = buf[T64_REC_ENTRYTYPE_OFFSET];
    memcpy(rec->cbm_name, buf + T64_REC_CBMNAME_OFFSET, T64_REC_CBMNAME_LEN);
    rec->cbm_type = buf[T64_REC_CBMTYPE_OFFSET];
    rec->start_addr = (WORD)get_number(buf + T64_REC_STARTADDR_OFFSET,
            (unsigned int)T64_REC_STARTADDR_LEN);
    rec->end_addr = (WORD)get_number(buf + T64_REC_ENDADDR_OFFSET,
            (unsigned int)T64_REC_ENDADDR_LEN);
    rec->contents = get_number(buf +
            T64_REC_CONTENTS_OFFSET, T64_REC_CONTENTS_LEN);

    return 0;
}


/** \brief  Calculate file size for \a rec
 *
 * \return  file size in bytes
 */
static int t64_file_record_get_size(t64_file_record_t *rec)
{
    return (int)(rec->end_addr - rec->start_addr);
}


/** \brief  Get pointer to file record at index \a num in \a t64
 *
 * \param[in]   t64     T64 container
 * \param[in]   num     index in directory of \a t64
 *
 * \return  file record or `NULL` on failure
 */
static t64_file_record_t *t64_get_file_record(t64_t *t64, unsigned int num)
{
    if (num >= t64->header.num_entries) {
        return NULL;
    }

    return t64->file_records + num;
}


/** \brief  Read and parse T64 header into \a hdr from file \a fd
 *
 * \param[out]  hdr     T64 header
 * \param[in]   fd      file descriptor
 *
 * \return  0 on success, -1 on failure
 */
static int t64_header_read(t64_header_t *hdr, FILE *fd)
{
    BYTE buf[T64_HDR_SIZE];

    if (fread(buf, T64_HDR_SIZE, 1, fd) != 1) {
        return -1;
    }

    memcpy(hdr->magic, buf + T64_HDR_MAGIC_OFFSET, T64_HDR_MAGIC_LEN);
    if (!check_magic(hdr)) {
        return -1;
    }

    hdr->version = (WORD)get_number(buf + T64_HDR_VERSION_OFFSET,
                                    (unsigned int)T64_HDR_VERSION_LEN);

    /* We could make a version check, but there are way too many images with
       broken version number out there for us to trust it.  */
#if 0
    if (hdr->version != 0x100) {
        return -1;
    }
#endif

    hdr->num_entries = (WORD)get_number(buf + T64_HDR_NUMENTRIES_OFFSET,
                                        (unsigned int)T64_HDR_NUMENTRIES_LEN);
    if (hdr->num_entries == 0) {
        /* XXX: The correct behavior here would be to reject it, but there
           are so many broken T64 images out there, that it's better if we
           silently suffer.  */
        log_warning(LOG_DEFAULT,
                "t64 image reports 0 max entries, adjusting to 1");
        hdr->num_entries = 1;
    }

    hdr->num_used = (WORD)get_number(buf + T64_HDR_NUMUSED_OFFSET,
                                     (unsigned int)T64_HDR_NUMUSED_LEN);
    if (hdr->num_used == 0) {
        /* corrupt tape image, too many of those out there, so just adust to
         * 1 and log a warning */
        log_warning(LOG_DEFAULT,
                "t64 image reports 0 used entries, adjusting to 1");
        hdr->num_used = 1;
    }

    if (hdr->num_used > hdr->num_entries) {
        return -1;
    }

    memcpy(hdr->description, buf + T64_HDR_DESCRIPTION_OFFSET,
           T64_HDR_DESCRIPTION_LEN);
    return 0;
}



/** \brief  Allocate memory for new T64 container and initialize members
 *
 * \return  new T64 object
 */
static t64_t *t64_new(void)
{
    t64_t *new64;

    new64 = lib_calloc(1, sizeof(t64_t));

    new64->file_name = NULL;
    new64->fd = NULL;
    new64->file_records = NULL;
    new64->current_file_number = -1;
    new64->current_file_seek_position = 0;

    return new64;
}


/** \brief  Free memory and close file used by T64 container \a t64
 *
 * \param[in,out]   t64 T64 container
 */
static void t64_destroy(t64_t *t64)
{
    if (t64->fd != NULL) {
        zfile_fclose(t64->fd);
    }
    lib_free(t64->file_name);
    lib_free(t64->file_records);
    lib_free(t64);
}



/* --------------------------- Public functions -----------------------------*/

/* ------------------------- File record functions --------------------------*/


/** \brief  Move file index in \a t64 to the first file record
 *
 * \param[in]   t64 T64 container
 *
 * \return  0 on success, -1 on failure
 */
int t64_seek_start(t64_t *t64)
{
    return t64_seek_to_file(t64, 0);
}


/** \brief  Move file index in \a t64 to \a file_number
 *
 * \param[in,out]   t64         T64 container
 * \param[in]       file_number index in directory of \a t64
 *
 * \return  0 on success, -1 on failure
 */
int t64_seek_to_file(t64_t *t64, int file_number)
{
    if (t64 == NULL || file_number < 0
            || file_number >= t64->header.num_entries) {
        return -1;
    }

    t64->current_file_number = file_number;
    t64->current_file_seek_position = 0;

    return 0;
}


/** \brief  Move file index record in \a t64 to next record
 *
 * Moves the directory index one record forward, if no such record exists AND
 * \a allow_rewind is true, move to the first record.
 *
 * \param[in,out]   t64             T64 container
 * \param[in]       allow_rewind    allow rewinding the 'tape' once
 *
 * \return  0 on success, -1 on failure
 */
int t64_seek_to_next_file(t64_t *t64, unsigned int allow_rewind)
{
    int n;

    if (t64 == NULL) {
        return -1;
    }

    if (t64->current_file_number < 0) {
        n = -1;
    } else {
        n = t64->current_file_number;
    }

    while (1) {
        t64_file_record_t *rec;

        n++;
        if (n >= t64->header.num_entries) {
            if (allow_rewind) {
                n = 0;
                allow_rewind = 0; /* Do not let this happen again.  */
            } else {
                return -1;
            }
        }

        rec = t64->file_records + n;
        if (rec->entry_type == T64_FILE_RECORD_NORMAL) {
            t64->current_file_number = n;
            t64->current_file_seek_position = 0;
            return t64->current_file_number;
        }
    }
}


/** \brief  Get pointer to current file record
 *
 * \param[in]   t64     T64 container
 *
 * \return  file record on `NULL` on failure
 */
t64_file_record_t *t64_get_current_file_record(t64_t *t64)
{
    if (t64->current_file_number < 0) {
        log_error(LOG_ERR, "T64: Negative file number.");
        return NULL;
    }
    return t64_get_file_record(t64, (unsigned int)(t64->current_file_number));
}


/** \brief  Read \a size bytes of the current file record in \a t64 into \a buf
 *
 * \param[in,out]   t64     T64 container
 * \param[out]      buf     buffer to store file data
 * \param[in]       size    number of bytes to read
 *
 * \return      number of bytes read or -1 on failure
 *
 * \todo        should return `long`, fread(3) returns `long`, not `int`
 */
int t64_read(t64_t *t64, BYTE *buf, size_t size)
{
    t64_file_record_t *rec;
    int recsize;
    long offset;
    long amount;

    if (t64 == NULL || t64->fd == NULL || t64->current_file_number < 0
            || size == 0) {
        return -1;
    }

    rec = t64->file_records + t64->current_file_number;
    recsize = t64_file_record_get_size(rec);
    offset = rec->contents + t64->current_file_seek_position;

    if (fseek(t64->fd, offset, SEEK_SET) != 0) {
        return -1;
    }

    /* limit size of the block that is to be read to not exceed the end of the
     * T64 container */
    if (recsize < (int)(t64->current_file_seek_position + size)) {
        if (t64->current_file_seek_position > recsize) {
            return -1;
        }
        size = (size_t)(recsize - t64->current_file_seek_position);
    }

    amount = fread(buf, 1, size, t64->fd);
    if (amount != (long)size) {
        return -1;
    }
    t64->current_file_seek_position += (int)amount;

    return amount;
}


/** \brief  Copy tape description in \a t64 to \a name
 *
 * Copies the tape description string in \a t64 to \a name. Doesn't terminate
 * string with nul if the string is 24 bytes.
 *
 * \param[in]   t64     T64 container
 * \param[out]  name    destination for description, should be at least 24
 *                      bytes
 */
void t64_get_header(t64_t *t64, BYTE *name)
{
    memcpy(name, t64->header.description, T64_HDR_DESCRIPTION_LEN);
}



/* ----------------------- T64 container functions --------------------------*/

/** \brief  Try to open file \a name as a T64 container
 *
 * \param[in]   name        filename/path to T64 file
 * \param[out]  read_only   read-only status of container
 *
 * \return  T64 container on success or `NULL` on failure
 */
t64_t *t64_open(const char *name, unsigned int *read_only)
{
    FILE *fd;
    t64_t *new;
    int i;
    long tapesize;   /* file size in bytes */

    WORD actual_size;
    WORD reported_size;

    fd = zfile_fopen(name, MODE_READ);
    if (fd == NULL) {
        return NULL;
    }

    *read_only = 1;

    new = t64_new();
    new->fd = fd;

    if (t64_header_read(&new->header, fd) < 0) {
        t64_destroy(new);
        return NULL;
    }

    new->file_records = lib_malloc(sizeof(t64_file_record_t)
                                   * new->header.num_entries);

    /* FIXME: why does this read all entries and not just the entries
     * actually used?*/
    for (i = 0; i < new->header.num_entries; i++) {
        if (t64_file_record_read(new->file_records + i, fd) < 0) {
            t64_destroy(new);
            return NULL;
        }
        /* keep track of the original index in the container */
        new->file_records[i].index = i;
    }

    /* Attempt to fix bugged end addresses by sorting the records on their
     * content member and using that information (and the size of the
     * container) to determine the actual end address */

    /* get file size */
    if (fseek(fd, 0L, SEEK_END) != 0) {
        t64_destroy(new);
        return NULL;
    }
    tapesize = ftell(fd);
    if (tapesize < 0) {
        t64_destroy(new);
        return NULL;
    }

    /* sort records on content member */
    qsort(new->file_records, (size_t)(new->header.num_used),
            sizeof *(new->file_records), comp_content);

    /* set end addresses based on the content member of the next entry */
    for (i = 0; i < new->header.num_used - 1; i++) {
        actual_size = (WORD)(new->file_records[i + 1].contents
                - new->file_records[i].contents);
        reported_size = new->file_records[i].end_addr
            - new->file_records[i].start_addr;

        if (reported_size != actual_size) {
            log_warning(LOG_DEFAULT,
                    "invalid file size for record %d in t64 image: $%04x, "
                    "should be $%04x, fixing",
                    new->file_records[i].index, reported_size, actual_size);

            new->file_records[i].end_addr = new->file_records[i].start_addr
                + actual_size;
        }
    }
    /* use the file size of the container to set the correct end address for
     * the last file record */
    reported_size = new->file_records[i].end_addr -
        new->file_records[i].start_addr;
    actual_size = (WORD)(tapesize - new->file_records[i].contents);
    /* warn and fix if sizes mismatch (only adjust size if it would 'overrun'
     * the T64's data, seems some T64's have extra unused data after the last
     * file) */
    if (reported_size > actual_size) {
            log_warning(LOG_DEFAULT,
                    "invalid file size for record %d in t64 image: $%04x, "
                    "should be $%04x, fixing",
                    new->file_records[i].index, reported_size, actual_size);


        new->file_records[i].end_addr = new->file_records[i].start_addr +
            actual_size;
    }
    /* restore original order of records */
    qsort(new->file_records, (size_t)(new->header.num_used),
            sizeof *(new->file_records), comp_index);


    new->file_name = lib_stralloc(name);

    return new;
}


/** \brief  Close T64 container \a t64
 *
 * \param[in,out]   t64 T64 container
 *
 * \return  0 on success, -1 on failure
 */
int t64_close(t64_t *t64)
{
    int retval = 0;

    if (t64->fd != NULL) {
        retval = zfile_fclose(t64->fd);
        t64->fd = NULL;
    }
    t64_destroy(t64);
    return retval;
}


