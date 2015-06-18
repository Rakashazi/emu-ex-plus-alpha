/*
 * tape.c - Tape unit emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Based on older code by
 *  Jouko Valta <jopi@stekt.oulu.fi>
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

#include "datasette.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mem.h"
#include "network.h"
#include "t64.h"
#include "tap.h"
#include "tape-internal.h"
#include "tape.h"
#include "tapeimage.h"
#include "traps.h"
#include "types.h"
#include "uiapi.h"
#include "vice-event.h"

/* #define DEBUG_TAPE */

/* Cassette Format Constants */
#define CAS_TYPE_OFFSET 0
#define CAS_STAD_OFFSET 1       /* start address */
#define CAS_ENAD_OFFSET 3       /* end address */
#define CAS_NAME_OFFSET 5       /* filename */

/* CPU addresses for tape routine variables.  */
static WORD buffer_pointer_addr;
static WORD st_addr;
static WORD verify_flag_addr;
static WORD stal_addr;
static WORD eal_addr;
static WORD kbd_buf_addr;
static WORD kbd_buf_pending_addr;
static int irqval;
static WORD irqtmp;

/* Flag: has tape been initialized?  */
static int tape_is_initialized = 0;

/* Tape traps to be installed.  */
static const trap_t *tape_traps;

/* Logging goes here.  */
static log_t tape_log = LOG_ERR;

/* The tape image for device 1. */
tape_image_t *tape_image_dev1 = NULL;

/* ------------------------------------------------------------------------- */

static inline void set_st(BYTE b)
{
    mem_store(st_addr, (BYTE)(mem_read(st_addr) | b));
}

/* ------------------------------------------------------------------------- */

void tape_traps_install(void)
{
    const trap_t *p;

    if (tape_traps != NULL) {
        for (p = tape_traps; p->func != NULL; p++) {
            traps_add(p);
        }
    }
}

void tape_traps_deinstall(void)
{
    const trap_t *p;

    if (tape_traps != NULL) {
        for (p = tape_traps; p->func != NULL; p++) {
            traps_remove(p);
        }
    }
}

static void tape_init_vars(const tape_init_t *init)
{
    /* Set addresses of tape routine variables.  */
    st_addr = init->st_addr;
    buffer_pointer_addr = init->buffer_pointer_addr;
    verify_flag_addr = init->verify_flag_addr;
    irqtmp = init->irqtmp;
    irqval = init->irqval;
    stal_addr = init->stal_addr;
    eal_addr = init->eal_addr;

    kbd_buf_addr = init->kbd_buf_addr;
    kbd_buf_pending_addr = init->kbd_buf_pending_addr;

    tape_traps = init->trap_list;
}

/* Initialize the tape emulation, using the traps in `trap_list'.  */
/* FIXME: This should be passed through a struct.  */
int tape_init(const tape_init_t *init)
{
    if (tape_log == LOG_ERR) {
        tape_log = log_open("Tape");
    }

    tape_internal_init();
    tape_image_init();

    lib_free(tape_image_dev1);
    tape_image_dev1 = lib_calloc(1, sizeof(tape_image_t));

    tap_init(init);

    tape_init_vars(init);
    tape_traps_install();

    tape_is_initialized = 1;
    return 0;
}

/* re-init the traps, needed to switch between different configurations in x128 */
int tape_reinit(const tape_init_t *init)
{
    if (tape_is_initialized == 0) {
        return -1;
    }

    tape_traps_deinstall();
    tape_traps = NULL;

    tape_init_vars(init);
    tape_traps_install();

    return 0;
}

void tape_shutdown(void)
{
    lib_free(tape_image_dev1);
}

int tape_deinstall(void)
{
    if (!tape_is_initialized) {
        return -1;
    }

    if (tape_image_dev1->name != NULL &&
        tape_image_dev1->type == TAPE_TYPE_T64) {
        tape_image_detach_internal(1);
    }

    tape_traps_deinstall();

    tape_traps = NULL;

    tape_is_initialized = 0;

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Tape traps.  These functions implement the standard kernal replacements
   for the tape functions.  Every emulator can either use these traps, or
   install its own ones, by passing an appropriate `trap_list' to
   `tape_init()'.  */

/* Find the next Tape Header and load it onto the Tape Buffer.  */
int tape_find_header_trap(void)
{
    int err;
    BYTE *cassette_buffer;

    cassette_buffer = mem_ram + (mem_read(buffer_pointer_addr) | (mem_read((WORD)(buffer_pointer_addr + 1)) << 8));

    if (tape_image_dev1->name == NULL
        || tape_image_dev1->type != TAPE_TYPE_T64) {
        err = 1;
    } else {
        t64_t *t64;
        t64_file_record_t *rec;

        t64 = (t64_t *)tape_image_dev1->data;
        rec = NULL;

        err = 0;
        do {
            if (t64_seek_to_next_file(t64, 1) < 0) {
                err = 1;
                break;
            }

            rec = t64_get_current_file_record(t64);
        } while (rec->entry_type != T64_FILE_RECORD_NORMAL);

        if (!err) {
            cassette_buffer[CAS_TYPE_OFFSET] = machine_tape_type_default();
            cassette_buffer[CAS_STAD_OFFSET] = rec->start_addr & 0xff;
            cassette_buffer[CAS_STAD_OFFSET + 1] = rec->start_addr >> 8;
            cassette_buffer[CAS_ENAD_OFFSET] = rec->end_addr & 0xff;
            cassette_buffer[CAS_ENAD_OFFSET + 1] = rec->end_addr >> 8;
            memcpy(cassette_buffer + CAS_NAME_OFFSET,
                   rec->cbm_name, T64_REC_CBMNAME_LEN);
        }
    }

    if (err) {
        cassette_buffer[CAS_TYPE_OFFSET] = TAPE_CAS_TYPE_EOF;
    }

    mem_store(st_addr, 0);      /* Clear the STATUS word.  */
    mem_store(verify_flag_addr, 0);

    if (irqtmp) {
        mem_store(irqtmp, (BYTE)(irqval & 0xff));
        mem_store((WORD)(irqtmp + 1), (BYTE)((irqval >> 8) & 0xff));
    }

    /* Check if STOP has been pressed.  */
    {
        int i, n = mem_read(kbd_buf_pending_addr);

        maincpu_set_carry(0);
        for (i = 0; i < n; i++) {
            if (mem_read((WORD)(kbd_buf_addr + i)) == 0x3) {
                maincpu_set_carry(1);
                break;
            }
        }
    }

    maincpu_set_zero(1);
    return 1;
}

int tape_find_header_trap_plus4(void)
{
    int err;
    BYTE *cassette_buffer;

    cassette_buffer = mem_ram + buffer_pointer_addr;

    if (tape_image_dev1->name == NULL
        || tape_image_dev1->type != TAPE_TYPE_T64) {
        err = 1;
    } else {
        t64_t *t64;
        t64_file_record_t *rec;

        t64 = (t64_t *)tape_image_dev1->data;
        rec = NULL;

        err = 0;
        do {
            if (t64_seek_to_next_file(t64, 1) < 0) {
                err = 1;
                break;
            }

            rec = t64_get_current_file_record(t64);
        } while (rec->entry_type != T64_FILE_RECORD_NORMAL);

        if (!err) {
            mem_store(0xF8, TAPE_CAS_TYPE_BAS);
            cassette_buffer[CAS_STAD_OFFSET - 1] = rec->start_addr & 0xff;
            cassette_buffer[CAS_STAD_OFFSET] = rec->start_addr >> 8;
            cassette_buffer[CAS_ENAD_OFFSET - 1] = rec->end_addr & 0xff;
            cassette_buffer[CAS_ENAD_OFFSET] = rec->end_addr >> 8;
            memcpy(cassette_buffer + CAS_NAME_OFFSET - 1,
                   rec->cbm_name, T64_REC_CBMNAME_LEN);
        }
    }

    if (err) {
        mem_store(0xF8, TAPE_CAS_TYPE_EOF);
    }

    mem_store(0xb6, 0x33);
    mem_store(0xb7, 0x03);

    mem_store(st_addr, 0);      /* Clear the STATUS word.  */
    mem_store(verify_flag_addr, 0);

    /* Check if STOP has been pressed.  */
    {
        int i, n = mem_read(kbd_buf_pending_addr);

        maincpu_set_carry(0);
        for (i = 0; i < n; i++) {
            if (mem_read((WORD)(kbd_buf_addr + i)) == 0x3) {
                maincpu_set_carry(1);
                break;
            }
        }
    }

    maincpu_set_zero(1);
    return 1;
}

/* Cassette Data transfer trap.

   XR flags the function to be performed on IRQ:

   08   Write tape
   0a   Write tape leader
   0c   Normal keyscan
   0e   Read tape

   Luckily enough, these values are valid for all the machines.  */
int tape_receive_trap(void)
{
    int len;
    WORD start, end;
    BYTE st;

    start = (mem_read(stal_addr) | (mem_read((WORD)(stal_addr + 1)) << 8));
    end = (mem_read(eal_addr) | (mem_read((WORD)(eal_addr + 1)) << 8));

    switch (maincpu_get_x()) {
        case 0x0e:
            {
                int amount;

                len = (int)(end - start);
                amount = t64_read((t64_t *)tape_image_dev1->data, mem_ram + (int)start, len);
                if (amount == len) {
                    st = 0x40;  /* EOF */
                } else {
                    st = 0x10;

                    log_warning(tape_log,
                                "Unexpected end of tape: file may be truncated.");
                }
            }
            break;
        default:
            log_error(tape_log, "Kernal command %x not supported.",
                      maincpu_get_x());
            st = 0x40;
            break;
    }

    /* Set registers and flags like the Kernal routine does.  */

    if (irqtmp) {
        mem_store(irqtmp, (BYTE)(irqval & 0xff));
        mem_store((WORD)(irqtmp + 1), (BYTE)((irqval >> 8) & 0xff));
    }

    set_st(st);                 /* EOF and possible errors */

    maincpu_set_carry(0);
    maincpu_set_interrupt(0);
    return 1;
}

int tape_receive_trap_plus4(void)
{
    WORD start, end, len;
    BYTE st;

    start = (mem_read(stal_addr) | (mem_read((WORD)(stal_addr + 1)) << 8));
    end = (mem_read(eal_addr) | (mem_read((WORD)(eal_addr + 1)) << 8));

    /* Read block.  */
    len = end - start;

    if (t64_read((t64_t *)tape_image_dev1->data,
                 mem_ram + (int) start, (int)len) == (int) len) {
        st = 0x40;      /* EOF */
    } else {
        st = 0x10;

        log_warning(tape_log,
                    "Unexpected end of tape: file may be truncated.");
    }

    /* Set registers and flags like the Kernal routine does.  */


    set_st(st);                 /* EOF and possible errors */
    return 1;
}

const char *tape_get_file_name(void)
{
    if (tape_image_dev1 == NULL) {
        return "";
    }

    return tape_image_dev1->name;
}

int tape_tap_attached(void)
{
    if (tape_image_dev1->name != NULL
        && tape_image_dev1->type == TAPE_TYPE_TAP) {
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Detach.  */
int tape_image_detach_internal(unsigned int unit)
{
    int retval = 0;
    char event_data[2];

    if (unit != 1) {
        return -1;
    }

    if (tape_image_dev1 == NULL || tape_image_dev1->name == NULL) {
        return 0;
    }

    switch (tape_image_dev1->type) {
        case TAPE_TYPE_T64:
            log_message(tape_log,
                        "Detaching T64 image `%s'.", tape_image_dev1->name);
            /* Tape detached: release play button.  */
            datasette_set_tape_sense(0);
            break;
        case TAPE_TYPE_TAP:
            log_message(tape_log,
                        "Detaching TAP image `%s'.", tape_image_dev1->name);
            datasette_set_tape_image(NULL);

            tape_traps_install();
            break;
        default:
            log_error(tape_log, "Unknown tape type %i.",
                      tape_image_dev1->type);
    }

    retval = tape_image_close(tape_image_dev1);

    ui_display_tape_current_image("");

    event_data[0] = (char)unit;
    event_data[1] = 0;

    event_record(EVENT_ATTACHTAPE, (void *)event_data, 2);

    return retval;
}

int tape_image_detach(unsigned int unit)
{
    char event_data[2];

    if (unit != 1) {
        return -1;
    }

    event_data[0] = (char)unit;
    event_data[1] = 0;

    if (event_playback_active()) {
        return -1;
    }

    if (network_connected()) {
        network_event_record(EVENT_ATTACHTAPE, (void *)event_data, 2);
        return 0;
    }

    return tape_image_detach_internal(unit);
}

/* Attach.  */
static int tape_image_attach_internal(unsigned int unit, const char *name)
{
    tape_image_t tape_image;

    if (unit != 1) {
        return -1;
    }

    if (!name || !*name) {
        return -1;
    }

    tape_image.name = lib_stralloc(name);
    tape_image.read_only = 0;

    if (tape_image_open(&tape_image) < 0) {
        lib_free(tape_image.name);
        log_error(tape_log, "Cannot open file `%s'", name);
        return -1;
    }

    tape_image_detach_internal(unit);

    memcpy(tape_image_dev1, &tape_image, sizeof(tape_image_t));

    ui_display_tape_current_image(tape_image_dev1->name);

    switch (tape_image_dev1->type) {
        case TAPE_TYPE_T64:
            log_message(tape_log, "T64 image '%s' attached.", name);
            /* Tape attached: press play button.  */
            datasette_set_tape_sense(1);
            break;
        case TAPE_TYPE_TAP:
            datasette_set_tape_image((tap_t *)tape_image_dev1->data);
            log_message(tape_log, "TAP image '%s' attached.", name);
            log_message(tape_log, "TAP image version: %i, system: %i.",
                        ((tap_t *)tape_image_dev1->data)->version,
                        ((tap_t *)tape_image_dev1->data)->system);
            tape_traps_deinstall();
            break;
        default:
            log_error(tape_log, "Unknown tape type %i.",
                      tape_image_dev1->type);
            return -1;
    }

    event_record_attach_image(unit, name, tape_image.read_only);

    return 0;
}

int tape_image_attach(unsigned int unit, const char *name)
{
    if (event_playback_active()) {
        return -1;
    }

    if (network_connected()) {
        network_attach_image(unit, name);
        return 0;
    }

    return tape_image_attach_internal(unit, name);
}

void tape_image_event_playback(unsigned int unit, const char *filename)
{
    if (filename == NULL || filename[0] == 0) {
        tape_image_detach_internal(unit);
    } else {
        tape_image_attach_internal(unit, filename);
    }
}
