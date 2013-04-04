/*
 * tap.h - TAP file support.
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

#ifndef VICE_TAP_H
#define VICE_TAP_H

#include <stdio.h>

#include "types.h"


#define TAP_HDR_SIZE         20
#define TAP_HDR_MAGIC_OFFSET 0
#define TAP_HDR_VERSION      12
#define TAP_HDR_SYSTEM       13
#define TAP_HDR_LEN          16


struct tape_init_s;
struct tape_file_record_s;

typedef struct tap_s {
    /* File name.  */
    char *file_name;

    /* File descriptor.  */
    FILE *fd;

    /* Size of the image.  */
    int size;

    /* The TAP version byte.  */
    BYTE version;

    /* System the image is made for.  */
    BYTE system;

    /* Tape name.  */
    BYTE name[12];

    /* Current file number.  */
    int current_file_number;

    /* Position in the current file.  */
    int current_file_seek_position;

    /* buffer for decoded content of current file */
    size_t current_file_data_pos;
    size_t current_file_size;
    BYTE * current_file_data;

    /* Header offset.  */
    int offset;

    /* Pointer to the current file record.  */
    struct tape_file_record_s *tap_file_record;

    /* Tape counter in machine-cycles/8 for even looong tapes */
    int cycle_counter;

    /* Tape length in machine-cycles/8 */
    int cycle_counter_total;

    /* Tape counter.  */
    int counter;

    /* Which mode is activated?  */
    int mode;

    /* Is the file opened read only?  */
    int read_only;

    /* Has the tap changed? We correct the size then.  */
    int has_changed;
} tap_t;

extern void tap_init(const struct tape_init_s *init);
extern tap_t *tap_open(const char *name, unsigned int *read_only);
extern int tap_close(tap_t *tap);
extern int tap_create(const char *name);

extern int tap_seek_start(tap_t *tap);
extern int tap_seek_to_file(tap_t *tap, unsigned int file_number);
extern int tap_seek_to_next_file(tap_t *tap, unsigned int allow_rewind);
extern void tap_get_header(tap_t *tap, BYTE *name);
extern struct tape_file_record_s *tap_get_current_file_record(tap_t *tap);

extern int tap_read(tap_t *tap, BYTE *buf, size_t size);

#endif
