/*
 * diskcontents-iec.c - Read directory from IEC device.
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
#include <string.h>

#include "diskcontents-iec.h"
#include "imagecontents.h"
#include "lib.h"
#include "serial.h"


#define STATE_LOAD_ADDRESS 0
#define STATE_LINK_ADDRESS 1
#define STATE_FILE_SIZE    2
#define STATE_FIND_MARK1   3
#define STATE_FIND_MARK2   4
#define STATE_FIND_TYPE    5
#define STATE_LINE_END     6
#define STATE_PARSE_END    7
#define STATE_ERROR        100


static image_contents_t *contents;
static image_contents_file_list_t currfl, *last_list;
static unsigned int image_contents_find_name;


static int getbuf(BYTE *buf, int length, int *index, BYTE *data)
{
    if (length == *index) {
        return -1;
    }

    *data = buf[*index];

    *index += 1;

    return 0;
}

static int state_load_address(BYTE *buf, int length, int *index)
{
    BYTE data;

    if (getbuf(buf, length, index, &data) < 0) {
        return STATE_ERROR;
    }
    if (getbuf(buf, length, index, &data) < 0) {
        return STATE_ERROR;
    }

    return STATE_LINK_ADDRESS;
}

static int state_link_address(BYTE *buf, int length, int *index)
{
    BYTE data1, data2;

    if (getbuf(buf, length, index, &data1) < 0) {
        return STATE_ERROR;
    }
    if (getbuf(buf, length, index, &data2) < 0) {
        return STATE_ERROR;
    }

    if (data1 == 0 && data2 == 0) {
        return STATE_PARSE_END;
    }

    return STATE_FILE_SIZE;
}

static int state_file_size(BYTE *buf, int length, int *index)
{
    BYTE data1, data2;

    if (getbuf(buf, length, index, &data1) < 0) {
        return STATE_ERROR;
    }
    if (getbuf(buf, length, index, &data2) < 0) {
        return STATE_ERROR;
    }

    currfl.size = data1 | (data2 << 8);

    return STATE_FIND_MARK1;
}

static int state_find_mark1(BYTE *buf, int length, int *index)
{
    BYTE data;

    while (1) {
        if (getbuf(buf, length, index, &data) < 0) {
            return STATE_ERROR;
        }
        if (data == '"') {
            break;
        }
        if (data == 0) {
            contents->blocks_free = currfl.size;
            return STATE_PARSE_END;
        }
    }

    return STATE_FIND_MARK2;
}

static int state_find_mark2(BYTE *buf, int length, int *index)
{
    BYTE data, name[16];
    unsigned int count = 0;

    memset(&(currfl.name), 0, IMAGE_CONTENTS_FILE_NAME_LEN + 1);

    while (1) {
        if (getbuf(buf, length, index, &data) < 0) {
            return STATE_ERROR;
        }
        if (data == '"') {
            break;
        }
        if (count >= IMAGE_CONTENTS_NAME_LEN) {
            return STATE_ERROR;
        }
        name[count] = data;
        count++;
    }

    memcpy(&(currfl.name), name, count);

    return STATE_FIND_TYPE;
}

static int state_find_type(BYTE *buf, int length, int *index)
{
    BYTE data, scratch[40];
    unsigned int count = 0;
    /*unsigned int copynum = 0;*/

    memset(scratch, 0, sizeof(scratch));
    memset(&(currfl.type), 0, IMAGE_CONTENTS_TYPE_LEN + 1);

    while (1) {
        if (getbuf(buf, length, index, &data) < 0) {
            return STATE_ERROR;
        }
        if (data == 0) {
            break;
        }
        if (count >= 40) {
            return STATE_ERROR;
        }
        scratch[count] = data;
        count++;
    }

    if (image_contents_find_name) {
        memcpy(contents->name, &(currfl.name),
               IMAGE_CONTENTS_FILE_NAME_LEN + 1);
        image_contents_find_name = 0;
        return STATE_LINK_ADDRESS;
    }

    return STATE_LINE_END;
}

static int state_line_end(BYTE *buf, int length, int *index)
{
    image_contents_file_list_t *new_list;

    new_list = lib_malloc(sizeof(image_contents_file_list_t));

    memcpy(new_list, &currfl, sizeof(image_contents_file_list_t));

    new_list->next = NULL;

    if (last_list == NULL) {
        new_list->prev = NULL;
        contents->file_list = new_list;
        last_list = contents->file_list;
    } else {
        new_list->prev = last_list;
        last_list->next = new_list;
        last_list = new_list;
    }

    return STATE_LINK_ADDRESS;
}

static image_contents_t *parse_directory(BYTE *buf, int length)
{
    int index = 0, state, loop = 1;

    contents = image_contents_new();

    state = STATE_LOAD_ADDRESS;
    image_contents_find_name = 1;

    contents->file_list = NULL;
    last_list = NULL;

    while (loop) {
        switch (state) {
            case STATE_LOAD_ADDRESS:
                state = state_load_address(buf, length, &index);
                break;
            case STATE_LINK_ADDRESS:
                state = state_link_address(buf, length, &index);
                break;
            case STATE_FILE_SIZE:
                state = state_file_size(buf, length, &index);
                break;
            case STATE_FIND_MARK1:
                state = state_find_mark1(buf, length, &index);
                break;
            case STATE_FIND_MARK2:
                state = state_find_mark2(buf, length, &index);
                break;
            case STATE_FIND_TYPE:
                state = state_find_type(buf, length, &index);
                break;
            case STATE_LINE_END:
                state = state_line_end(buf, length, &index);
                break;
            case STATE_PARSE_END:
                loop = 0;
                break;
            case STATE_ERROR:
                /* FIXME */
                loop = 0;
                break;
        }
    }

    return contents;
}

image_contents_t *diskcontents_iec_read(unsigned int unit)
{
    int length;
    BYTE *buf = NULL;

    length = serial_iec_lib_directory(unit, "$", &buf);

    if (length <= 0) {
        return NULL;
    }

    return parse_directory(buf, length);
}
