/*
 * tape.h - Tape unit emulation.
 *
 * Written by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#ifndef VICE_TAPE_H
#define VICE_TAPE_H

#include <stdio.h>

#include "types.h"


#define TAPE_TYPE_T64 0
#define TAPE_TYPE_TAP 1

#define TAPE_ENCODING_NONE      0
#define TAPE_ENCODING_CBM       1
#define TAPE_ENCODING_TURBOTAPE 2

#define TAPE_CAS_TYPE_BAS  1 /* Relocatable Program */
#define TAPE_CAS_TYPE_PRG  3 /* Binary Program */
#define TAPE_CAS_TYPE_DATA 4 /* Data Record */
#define TAPE_CAS_TYPE_EOF  5 /* End of Tape marker */

struct trap_s;

struct tape_image_s {
    char *name;
    unsigned int read_only;
    unsigned int type;
    void *data;
};
typedef struct tape_image_s tape_image_t;

struct tape_init_s {
    WORD buffer_pointer_addr;
    WORD st_addr;
    WORD verify_flag_addr;
    WORD irqtmp;
    int irqval;
    WORD stal_addr;
    WORD eal_addr;
    WORD kbd_buf_addr;
    WORD kbd_buf_pending_addr;
    const struct trap_s *trap_list;
    int pulse_short_min;
    int pulse_short_max;
    int pulse_middle_min;
    int pulse_middle_max;
    int pulse_long_min;
    int pulse_long_max;
};
typedef struct tape_init_s tape_init_t;

struct tape_file_record_s {
    BYTE name[17];
    BYTE type, encoding;
    WORD start_addr;
    WORD end_addr;
};
typedef struct tape_file_record_s tape_file_record_t;


extern tape_image_t *tape_image_dev1;

extern int tape_init(const tape_init_t *init);
extern int tape_reinit(const tape_init_t *init);
extern void tape_shutdown(void);
extern int tape_deinstall(void);
extern void tape_get_header(tape_image_t *tape_image, BYTE *name);
extern int tape_find_header_trap(void);
extern int tape_receive_trap(void);
extern int tape_find_header_trap_plus4(void);
extern int tape_receive_trap_plus4(void);
extern const char *tape_get_file_name(void);
extern int tape_tap_attached(void);

extern void tape_traps_install(void);
extern void tape_traps_deinstall(void);

extern tape_file_record_t *tape_get_current_file_record(tape_image_t *tape_image);
extern int tape_seek_start(tape_image_t *tape_image);
extern int tape_seek_to_file(tape_image_t *tape_image, unsigned int file_number);
extern int tape_seek_to_next_file(tape_image_t *tape_image, unsigned int allow_rewind);
extern int tape_read(tape_image_t *tape_image, BYTE *buf, size_t size);

extern int tape_internal_close_tape_image(tape_image_t *tape_image);
extern tape_image_t *tape_internal_open_tape_image(const char *name, unsigned int read_only);
/* External tape image interface.  */
extern int tape_image_detach(unsigned int unit);
extern int tape_image_detach_internal(unsigned int unit);
extern int tape_image_attach(unsigned int unit, const char *name);
extern int tape_image_open(tape_image_t *tape_image);
extern int tape_image_close(tape_image_t *tape_image);
extern int tape_image_create(const char *name, unsigned int type);
extern void tape_image_event_playback(unsigned int unit, const char *filename);

#endif
