/** \file   textfilereader.c
 * \brief   Stack-based text file reader
 *
 * Read text files that can recursively "include" other text files while using
 * a single file descriptor.
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
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
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "archdep.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "sysfile.h"
#include "util.h"

#include "textfilereader.h"


/* #define DEBUG_VHK */
#include "vhkdebug.h"


/** \brief  Initial size of the buffer of the textfile reader
 *
 * The buffer will be automatically doubled in size
 */
#define TEXTFILE_READER_BUFSIZE 1024


/** \brief  Create new textfile_entry_t instance
 *
 * Allocate memory for a textfile_entry_t instance and copy \a path.
 *
 * \param[in]   path    path to file
 *
 * \return  heap-allocated textfile_entry_t instance
 */
static textfile_entry_t *textfile_entry_new(const char *path)
{
    textfile_entry_t *entry = lib_malloc(sizeof *entry);

    entry->path    = lib_strdup(path);
    entry->pos     = 0;
    entry->linenum = 0;
    entry->next    = NULL;
    return entry;
}

/** \brief  Free textfile entry and its members
 *
 * Free \a entry and its members.
 *
 * \param[in]   entry   textfile entry instance
 */
static void textfile_entry_free(textfile_entry_t *entry)
{
    if (entry != NULL) {
        lib_free(entry->path);
        lib_free(entry);
    }
}

/** \brief  Free textfile entry and its members recusively
 *
 * Free \a entry, its members and recursively its siblings.
 *
 * \param[in]   entry   textfile entry instance
 */
static void textfile_entry_free_all(textfile_entry_t *entry)
{
    textfile_entry_t *next = entry->next;

    textfile_entry_free(entry);
    if (next != NULL) {
        textfile_entry_free_all(next);
    }
}


/** \brief  Initialize \a reader
 *
 * Initialize the \a reader for use by setting all members and allocating the
 * buffer to its initial size.
 *
 * \param[in,out]   reader  textfile reader
 */
void textfile_reader_init(textfile_reader_t *reader)
{
    reader->buffer  = lib_malloc(TEXTFILE_READER_BUFSIZE);
    reader->bufsize = TEXTFILE_READER_BUFSIZE;
    reader->buflen  = 0;
    reader->fp      = NULL;
    reader->entries = NULL;
}


/** \brief  Free all memory used by \a reader
 *
 * Free all memory used by \a reader and close any open file.
 *
 * \param[in,out]   reader  textfile reader instance
 */
void textfile_reader_free(textfile_reader_t *reader)
{
    lib_free(reader->buffer);
    if (reader->fp != NULL) {
        fclose(reader->fp);
        reader->fp = NULL;
    }
    /* unwind stack */
    if (reader->entries != NULL) {
        textfile_entry_free_all(reader->entries);
    }
}


/** \brief  Open a new file
 *
 * Open a new file and remember the position in the previous file, if any.
 *
 * If a previous file was open, close it and remember its stream position and
 * line number, open the new file and push it on top of the stack.
 *
 * \param[in]   reader  textfile reader
 * \param[in]   path    path to new file
 *
 * \return  bool
 */
bool textfile_reader_open(textfile_reader_t *reader, const char *path)
{
    textfile_entry_t *current;
    textfile_entry_t *new;

    /* get top of stack */
    current = reader->entries;
    if (current != NULL) {

        if (reader->fp == NULL) {
            debug_vhk("ERROR: file entry on stack, but no open FP.");
        } else {
            /* remember position in current file */
            current->pos = ftell(reader->fp);
            /* close current file */
            fclose(reader->fp);
            reader->fp = NULL;
        }
    }

    /* try to open new file */

    /* first try plain path (for -hotkeyfile) */
    reader->fp = fopen(path, "rb");
    if (reader->fp == NULL) {
        return false;
    }
    new = textfile_entry_new(path);
    new->next = current;
    reader->entries = new;

    return true;
}


/** \brief  Close current file, reopen any previous file
 *
 * Closes the current file and reopens and repositions any previous file.
 *
 * \param[in]   reader  textfile reader
 *
 * \return  true if a previous file was reopened
 */
bool textfile_reader_close(textfile_reader_t *reader)
{
    textfile_entry_t *current = reader->entries;
    textfile_entry_t *next;


    if (reader->fp != NULL) {
#if 0
        debug_vhk("Current file getting closed: '%s'.", current->path);
        debug_vhk("Closing reader->fp.");
        debug_vhk("FCLOSE(%p)", (void*)(reader->fp));
#endif
        fclose(reader->fp);
        reader->fp = NULL;
    } else {
        debug_vhk("CLOSE called when no file is open.");
        return false;
    }

    if (current != NULL) {

        /* close current file and free memory */
        next = current->next;
        textfile_entry_free(current);
        current = next;
        reader->entries = current;

        /* reopen previous file on stack, if any */
        if (current != NULL) {
            debug_vhk("reopening previous file '%s'.", current->path);
            reader->fp = fopen(current->path, "rb");
            if (reader->fp == NULL) {
                log_message(vhk_log,
                            "failed to open '%s'.",
                            current->path);
                return false;
            }
            /* reposition stream to the position it had when a new file was
             * opened */
            if (fseek(reader->fp, current->pos, SEEK_SET) != 0) {
                debug_vhk("FSEEK FAIL!");
                return false;
            }
            return true;
        }
        return false;
    }
    return false;
}


/** \brief  Read a single line of text
 *
 * Reads a single line of text from the file on top of the stack.
 *
 * \param[in]   reader  textfile reader
 *
 * \return  pointer to line buffer in \a reader or `NULL` when EOF
 *
 * \todo    Also returns `NULL` on I/O error, I'll need a way to differenciate
 *          between EOF and I/O errors, akin to feof(3).
 */
const char *textfile_reader_read(textfile_reader_t *reader)
{
    reader->buflen = 0;

    if (reader->entries == NULL) {
        debug_vhk("NO ENTRIES.");
        return NULL;
    }
    if (reader->fp == NULL) {
        debug_vhk("NO FP!");
        return NULL;
    }

    while (true) {
        int ch = fgetc(reader->fp);

        if (ch == EOF) {
            if (feof(reader->fp)) {
                /* end of file: close file and get previous one */
                if (!textfile_reader_close(reader)) {
                    /* no more files */
                    return NULL;
                }
            } else {
                /* error */
                log_error(LOG_ERR, "read error: %d: %s", errno, strerror(errno));
                return NULL;
            }
        } else {
            /* add character to buffer */
            if (reader->bufsize == reader->buflen) {
                /* resize buffer */
                reader->bufsize *= 2;
                reader->buffer = lib_realloc(reader->buffer, reader->bufsize);
            }
            reader->buffer[reader->buflen] = ch;

            /* newline? */
            if (ch == '\n') {
                reader->buffer[reader->buflen] = '\0';
                /* delete CR if present */
                if (reader->buflen > 0
                        && reader->buffer[reader->buflen - 1] == '\r') {
                    reader->buffer[--(reader->buflen)] = '\0';
                }
                /* update line number and return buffer */
                reader->entries->linenum++;
                return reader->buffer;
            }
            reader->buflen++;
        }
    }
}


/** \brief  Get line number of current file
 *
 * Get the line number of the currently opened file in \a reader.
 *
 * \param[in]   reader  textfile reader
 *
 * \return  line number or -1 when no file is open
 */
long textfile_reader_linenum(const textfile_reader_t *reader)
{
    if (reader->entries != NULL) {
        return reader->entries->linenum;
    } else {
        return -1;
    }
}


/** \brief  Get name of current file
 *
 * Get the path of the currently opened file of \a reader.
 *
 * \param[in]   reader  textfile reader
 *
 * \return  path or "<null>" when no file is open
 */
const char *textfile_reader_filename(const textfile_reader_t *reader)
{
    if (reader->entries != NULL) {
        return reader->entries->path;
    } else {
        return "<null>";
    }
}
