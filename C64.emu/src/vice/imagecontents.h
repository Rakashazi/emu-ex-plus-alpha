/** \file   src/imagecontents.h
 * \brief   Extract the directory listing from disk/tape images - header
 */

/*
 * imagecontents.h - Extract the directory from disk/tape images.
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

#ifndef VICE_IMAGE_CONTENTS_H
#define VICE_IMAGE_CONTENTS_H

#include "types.h"

#define IMAGE_CONTENTS_NAME_LEN         16  /**< standard length of container
                                                 names, such as D64 */
#define IMAGE_CONTENTS_NAME_T64_LEN     24  /**< length of a T64 container
                                                 name */
#define IMAGE_CONTENTS_ID_LEN           5   /**< length of a disk ID */
#define IMAGE_CONTENTS_FILE_NAME_LEN    16  /**< lenght of a PETSCII filename */
#define IMAGE_CONTENTS_TYPE_LEN         5   /**< file type and flags, eg:
                                                 '*PRG<' */

/** \brief  Object containing a directory listing entry
 */
struct image_contents_file_list_s {
    BYTE name[IMAGE_CONTENTS_FILE_NAME_LEN + 1];    /**< PETSCII file name */
    BYTE type[IMAGE_CONTENTS_TYPE_LEN + 1];         /**< PETSCII file type and
                                                         'flags' */

    unsigned int size;  /**< size of the file */

    struct image_contents_file_list_s *prev, *next; /**< linked list pointers */
};
typedef struct image_contents_file_list_s image_contents_file_list_t;


/** \brief  Image contents object
 *
 * Contains a disk/tape name, and ID, number of blocks free (if available)
 * and a list of directory entries
 */
struct image_contents_s {
    BYTE name[IMAGE_CONTENTS_NAME_T64_LEN + 1]; /**< disk/tape name in PETSCII */
    BYTE id[IMAGE_CONTENTS_ID_LEN + 1];         /**< disk ID */
    int blocks_free;   /**< blocks free: -1: No free space information.  */
    image_contents_file_list_t *file_list;  /**< list of directory entries */
};
typedef struct image_contents_s image_contents_t;
typedef image_contents_t *(*read_contents_func_type)(const char *);

/** \brief  Screencode object for displaying directory entries
 */
struct image_contents_screencode_s {
    BYTE *line; /**< screencode data */
    unsigned int length;    /**< length of the line */
    struct image_contents_screencode_s *next;   /**< pointer to next node */
};
typedef struct image_contents_screencode_s image_contents_screencode_t;

/* ------------------------------------------------------------------------- */

extern void image_contents_destroy(image_contents_t *contents);
extern image_contents_t *image_contents_new(void);

extern image_contents_screencode_t *image_contents_to_screencode (image_contents_t *contents);
extern void image_contents_screencode_destroy(image_contents_screencode_t *c);

extern char *image_contents_to_string(image_contents_t * contents, char convert_to_ascii);
extern char *image_contents_file_to_string(image_contents_file_list_t * p, char convert_to_ascii);
extern char *image_contents_filename_to_string(image_contents_file_list_t * p, char convert_to_ascii);
extern char *image_contents_filetype_to_string(image_contents_file_list_t * p, char convert_to_ascii);
extern char *image_contents_filename_by_number(image_contents_t *contents,
                                               unsigned int file_index);

extern image_contents_t *diskcontents_iec_read(unsigned int unit);

#endif
