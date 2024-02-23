/*
 * scpu64stubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdbool.h>

#include "c64mem.h"
#include "pet/petpia.h"
#include "snapshot.h"
#include "tap.h"
#include "tape.h"
#include "tapecart.h"
#include "tapeport.h"
#include "tape-snapshot.h"


/*******************************************************************************
    tape
*******************************************************************************/

tape_image_t *tape_image_dev[TAPEPORT_MAX_PORTS] =  { NULL };

int tape_image_attach(unsigned int unit, const char *name)
{
    return 0;
}

void tape_shutdown(void)
{
}

int tape_tap_attached(int port)
{
    return 0;
}

int tape_seek_start(tape_image_t *tape_image)
{
    return 0;
}

int tape_seek_to_file(tape_image_t *tape_image, unsigned int file_number)
{
    return 0;
}

int tape_seek_to_offset(tape_image_t *tape_image, unsigned long offset)
{
    return 0;
}

void tape_image_event_playback(unsigned int unit, const char *filename)
{
}

int tape_image_detach(unsigned int unit)
{
    return 0;
}

void tape_image_detach_all(void)
{
}

int tap_seek_start(tap_t *tap)
{
    return 0;
}

int tap_seek_to_offset(tap_t *tap, unsigned long offset)
{
    return 0;
}

int tape_image_create(const char *name, unsigned int type)
{
    return 0;
}

int tape_snapshot_write_module(int port, snapshot_t *s, int save_image)
{
    return 0;
}

int tape_snapshot_read_module(int port, snapshot_t *s)
{
    return 0;
}

int tape_read(tape_image_t *tape_image, uint8_t *buf, size_t size)
{
    return 0;
}

tape_file_record_t *tape_get_current_file_record(tape_image_t *tape_image)
{
    return NULL;
}

int tape_image_open(tape_image_t *tape_image)
{
    return -1;
}

int tape_image_close(tape_image_t *tape_image)
{
    return 0;
}

int tape_internal_close_tape_image(tape_image_t *tape_image)
{
    return 0;
}

tape_image_t *tape_internal_open_tape_image(const char *name, unsigned int read_only)
{
    return NULL;
}

int tape_seek_to_next_file(tape_image_t *tape_image, unsigned int allow_rewind)
{
    return 0;
}

void tape_get_header(tape_image_t *tape_image, uint8_t *name)
{
}

const char *tape_get_file_name(int port)
{
    return NULL;
}

uint8_t colorram_read(uint16_t addr)
{
    return 0;
}

void colorram_store(uint16_t addr, uint8_t value)
{
}

int tapeport_device_register(int id, tapeport_device_t *device)
{
    return 0;
}

void tapeport_trigger_flux_change(unsigned int on, int port)
{
}

void tapeport_set_tape_sense(int sense, int port)
{
}

tapeport_desc_t *tapeport_get_valid_devices(int port, int sort)
{
    return NULL;
}

const char *tapeport_get_device_type_desc(int type)
{
    return NULL;
}

int tapeport_valid_port(int port)
{
    return 0;
}

int tapecart_flush_tcrt(void)
{
    return -1;
}

bool pia1_get_diagnostic_pin(void)
{
    return false;
}

int tapeport_get_active_state(void)
{
    return 0;
}
