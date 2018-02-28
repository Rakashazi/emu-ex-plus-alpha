/** \file   src/imagecontents.c
 * \brief   Extract the directory listing from disk/tape images
 *
 * Used in c1541, autostart, mon_file and the 'attach disk image' dialogs in
 * various ports
 */

/*
 * imagecontents.c - Extract the directory from disk/tape images.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *  Tibor Biczo <crown@mail.matav.hu>
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

#include "charset.h"
#include "diskcontents.h"
#include "imagecontents.h"
#include "lib.h"
#include "tapecontents.h"
#include "types.h"
#include "util.h"


/* ------------------------------------------------------------------------- */


/** \brief  Create a new directory listing object
 *
 * This allocates memory on the heap, so the pointer returned should be passed
 * to image_contents_destroy() when done.
 *
 * \return  image contents object
 */
image_contents_t *image_contents_new(void)
{
    image_contents_t *newimg;

    newimg = lib_calloc(1, sizeof(image_contents_t));

    newimg->blocks_free = -1;
    newimg->file_list = NULL;

    return newimg;
}

/** \brief  Free memory used by \a contents and its members
 *
 * \param[in/out]   contents    image contents object
 */
void image_contents_destroy(image_contents_t *contents)
{
    image_contents_file_list_t *node = contents->file_list;

    while (node != NULL) {
        image_contents_file_list_t *next = node->next;
        lib_free(node);
        node = next;
    }
    lib_free(contents);
}


/** \brief  Free memory used by image contents as a list of screen codes
 *
 * \param[in,out]   c   screencode contents object
 */
void image_contents_screencode_destroy(image_contents_screencode_t *c)
{
    image_contents_screencode_t *h;

    while (c != NULL) {
        h = c->next;
        lib_free(c->line);
        lib_free(c);
        c = h;
    }
}


/** \brief  Write out directory listing as a list of screencodes
 *
 * This function allocates memory on the heap, so after use call
 * image_contents_screen_code_destroy() to clean up.
 *
 * \param[in]   constents   image contents object
 *
 * \return  image contents as a list of screencode strings
 */
image_contents_screencode_t *image_contents_to_screencode(image_contents_t
                                                          *contents)
{
    BYTE *buf;
    BYTE rawline[64];   /* FIXME: shouldn't be an integer literal */
    unsigned int len, i;
    image_contents_screencode_t *image_contents_screencode, *screencode_ptr;
    image_contents_file_list_t *p;

    image_contents_screencode = lib_malloc(sizeof(image_contents_screencode_t));

    screencode_ptr = image_contents_screencode;

    sprintf((char *)rawline, "0 \"%s\" %s", contents->name, contents->id);
    charset_petcii_to_screencode_line(rawline, &buf, &len);
    screencode_ptr->line = buf;
    screencode_ptr->length = len;
    screencode_ptr->next = NULL;

    /*
     I removed this for OS/2 because I want to have an output
     which looks like a directory listing which you can load in
     the emulator.
     */
#ifndef __OS2__
    if (contents->file_list == NULL) {
        charset_petcii_to_screencode_line((BYTE *)"(eMPTY IMAGE.)", &buf, &len);
        screencode_ptr->next = lib_malloc(sizeof(image_contents_screencode_t));
        screencode_ptr = screencode_ptr->next;

        screencode_ptr->line = buf;
        screencode_ptr->length = len;
        screencode_ptr->next = NULL;
    }
#endif

    for (p = contents->file_list; p != NULL; p = p->next) {
        sprintf((char *)rawline, "%-5d \"                  ", p->size);
        memcpy(&rawline[7], p->name, IMAGE_CONTENTS_FILE_NAME_LEN);

        for (i = 0; i < IMAGE_CONTENTS_FILE_NAME_LEN; i++) {
            if (rawline[7 + i] == 0xa0) {
                rawline[7 + i] = '"';
                break;
            }
        }

        if (i == IMAGE_CONTENTS_FILE_NAME_LEN) {
            rawline[7 + IMAGE_CONTENTS_FILE_NAME_LEN] = '"';
        }

        memcpy(&rawline[7 + IMAGE_CONTENTS_FILE_NAME_LEN + 2], p->type, 5);
        charset_petcii_to_screencode_line(rawline, &buf, &len);

        screencode_ptr->next = lib_malloc(sizeof(image_contents_screencode_t));
        screencode_ptr = screencode_ptr->next;

        screencode_ptr->line = buf;
        screencode_ptr->length = len;
        screencode_ptr->next = NULL;
    }

    if (contents->blocks_free >= 0) {
        sprintf((char *)rawline, "%d BLOCKS FREE.", contents->blocks_free);
        charset_petcii_to_screencode_line(rawline, &buf, &len);

        screencode_ptr->next = lib_malloc(sizeof(image_contents_screencode_t));
        screencode_ptr = screencode_ptr->next;

        screencode_ptr->line = buf;
        screencode_ptr->length = len;
        screencode_ptr->next = NULL;
    }

    return image_contents_screencode;
}


/** \brief  Generate the first line of a directory listing (0 name id)
 *
 * \param[in]   contents            image contents object
 * \param[in]   convert_to_ascii    convert to ASCII (boolean)
 *
 * \return  heap allocated string, free with lib_free()
 */
char *image_contents_to_string(image_contents_t * contents,
                               char convert_to_ascii)
{
    char *string = lib_msprintf("0 \"%s\" %s", contents->name, contents->id);
    if (convert_to_ascii) {
        charset_petconvstring((unsigned char *)string, 1);
    }

    return string;
}


/** \brief  Convert filename in \a p to '\"<filename>\"'
 *
 * \param[in]   p   image contents file list
 *
 * \return  converted string, _not_ allocated on the heap, _not_ MT safe
 */
static char *image_contents_get_filename(image_contents_file_list_t * p)
{
    int i;
    static char print_name[IMAGE_CONTENTS_FILE_NAME_LEN + 3] = { 0 };
    char encountered_a0 = 0;

    memset(print_name, 0x20, sizeof(print_name) - 1); /* redundant? better safe than sorry */
    print_name[0] = '\"';

    for (i = 0; i < IMAGE_CONTENTS_FILE_NAME_LEN; i++) {
        if (p->name[i] == 0) { /* a 0x00 would mess a dir on real thing anyway */
            print_name[i + 1] = '?'; /* better than showing a reversed @ */
        } else if (p->name[i] == 0xa0) {
            encountered_a0++;
            if (encountered_a0 == 1) {
                print_name[i + 1] = '\"';
            } else {
                print_name[i + 1] = 0x20;
            }
        } else {
            print_name[i + 1] = (char)p->name[i];
        }
    }

    if (!encountered_a0) {
        print_name[i + 1] = '\"';
    }

    return print_name;
}


/** \brief  Get filename for directory listing, optionally converting to ASCII
 *
 * \param[in]   p   image contents file list
 * \param[in]   convert_to_ascii    convert string to ASCII (boolean)
 *
 * \return  allocated string, free with lib_free()
 */
char *image_contents_filename_to_string(image_contents_file_list_t * p,
                                        char convert_to_ascii)
{
    char *print_name;
    char *string;

    print_name = image_contents_get_filename(p);
    string = lib_stralloc(print_name);

    if (convert_to_ascii) {
        charset_petconvstring((unsigned char *)string, 1);
    }

    return string;
}


/** \brief  Get the file type and flags for a file
 *
 * \param[in]   p                   image contents file entry
 * \param[in]   convert_to_ascii    convert string to ASCII from PETSCII
 *
 * \return  5-character string: 'SFFFL', where S is the scratched '*' char or
 *          a space, FFF is the file type (PRG etc) and L is the locked '<'
 *          char or space.
 */
char *image_contents_filetype_to_string(image_contents_file_list_t *p,
                                        char convert_to_ascii)
{
    char *type = lib_stralloc((const char *)(p->type));
    if (convert_to_ascii) {
        charset_petconvstring((BYTE *)type, 1);
    }
    return type;
}


/** \brief  Generate a '<blocks> "<filename>" *prg<' line from \a p
 *
 * \param[in]   p                   image contents file list
 * \param[in]   convert_to_ascii    convert to ASCII (boolean)
 *
 * \return  allocated string, free with lib_free()
 */
char *image_contents_file_to_string(image_contents_file_list_t *p,
                                    char convert_to_ascii)
{
    char *print_name;
    char *string;

    print_name = image_contents_get_filename(p);
    string = lib_msprintf("%-5d %s %s", p->size, print_name, p->type);

    if (convert_to_ascii) {
        charset_petconvstring((unsigned char *)string, 1);
    }

    return string;
}


/** \brief  Get image contents file name by index
 *
 * \param[in]   contents    image contents object
 * \param[in]   file_index  index in \a contents' file list
 *
 * \return  allocated string, free with lib_free(), or `NULL` when
 *          \a file_index was too large
 */
char *image_contents_filename_by_number(image_contents_t *contents,
                                        unsigned int file_index)
{
    image_contents_file_list_t *current;
    char *s = NULL;

    if (contents == NULL) {
        return NULL;
    }

    if (file_index != 0) {
        current = contents->file_list;
        file_index--;
        while ((file_index != 0) && (current != NULL)) {
            current = current->next;
            file_index--;
        }
        if (current != NULL) {
            s = lib_stralloc((char *)(current->name));
        }
    }
    return s;
}

