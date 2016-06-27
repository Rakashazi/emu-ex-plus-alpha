/*
 * tapeimage.c - Common low-level tape image access.
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
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "t64.h"
#include "tap.h"
#include "tape.h"
#include "tapeimage.h"


int tape_image_close(tape_image_t *tape_image)
{
    int retval = 0;

    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            retval = t64_close((t64_t *)tape_image->data);
            break;
        case TAPE_TYPE_TAP:
            retval = tap_close((tap_t *)tape_image->data);
            break;
    }

    lib_free(tape_image->name);
    tape_image->name = NULL;

    return retval;
}

int tape_image_open(tape_image_t *tape_image)
{
    t64_t *new_t64_tape;
    tap_t *new_tap_tape;
    int initial_read_only = tape_image->read_only;

    new_t64_tape = t64_open(tape_image->name, &tape_image->read_only);
    if (new_t64_tape != NULL) {
        tape_image->data = (void *)new_t64_tape;
        tape_image->type = TAPE_TYPE_T64;
        return 0;
    }

    tape_image->read_only = initial_read_only;
    new_tap_tape = tap_open(tape_image->name, &tape_image->read_only);
    if (new_tap_tape != NULL) {
        tape_image->data = (void *)new_tap_tape;
        tape_image->type = TAPE_TYPE_TAP;
        return 0;
    }

    return -1;
}
/* ------------------------------------------------------------------------- */

int tape_image_create(const char *name, unsigned int type)
{
    return tap_create(name);
}

/* ------------------------------------------------------------------------- */

void tape_get_header(tape_image_t *tape_image, BYTE *name)
{
    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            t64_get_header((t64_t *)tape_image->data, name);
            break;
        case TAPE_TYPE_TAP:
            tap_get_header((tap_t *)tape_image->data, name);
            break;
    }
}

tape_file_record_t *tape_get_current_file_record(tape_image_t *tape_image)
{
    static tape_file_record_t rec;

    memset(rec.name, 0, 17);

    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            {
                t64_file_record_t *t64_rec;

                t64_rec = t64_get_current_file_record((t64_t *)tape_image->data);
                memcpy(rec.name, t64_rec->cbm_name, 16);
                rec.type = (t64_rec->entry_type == T64_FILE_RECORD_FREE) ? 0 : 1;
                rec.encoding = TAPE_ENCODING_NONE;
                rec.start_addr = t64_rec->start_addr;
                rec.end_addr = t64_rec->end_addr;
                break;
            }
        case TAPE_TYPE_TAP:
            {
                tape_file_record_t *tape_rec;

                tape_rec = tap_get_current_file_record((tap_t *)tape_image->data);
                memcpy(rec.name, tape_rec->name, 16);
                rec.type = tape_rec->type;
                rec.encoding = tape_rec->encoding;
                rec.start_addr = tape_rec->start_addr;
                rec.end_addr = tape_rec->end_addr;
                break;
            }
    }
    return &rec;
}

int tape_seek_start(tape_image_t *tape_image)
{
    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            return t64_seek_start((t64_t *)tape_image->data);
        case TAPE_TYPE_TAP:
            return tap_seek_start((tap_t *)tape_image->data);
    }
    return -1;
}

int tape_seek_to_file(tape_image_t *tape_image, unsigned int file_number)
{
    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            return t64_seek_to_file((t64_t *)tape_image->data, file_number);
        case TAPE_TYPE_TAP:
            return tap_seek_to_file((tap_t *)tape_image->data, file_number);
    }
    return -1;
}

int tape_seek_to_next_file(tape_image_t *tape_image, unsigned int allow_rewind)
{
    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            return t64_seek_to_next_file((t64_t *)tape_image->data, allow_rewind);
        case TAPE_TYPE_TAP:
            return tap_seek_to_next_file((tap_t *)tape_image->data, allow_rewind);
    }
    return -1;
}

int tape_read(tape_image_t *tape_image, BYTE *buf, size_t size)
{
    switch (tape_image->type) {
        case TAPE_TYPE_T64:
            return t64_read((t64_t *)tape_image->data, buf, size);
        case TAPE_TYPE_TAP:
            return tap_read((tap_t *)tape_image->data, buf, size);
    }
    return -1;
}

/* ------------------------------------------------------------------------- */

void tape_image_init(void)
{
}
