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

#include "snapshot.h"
#include "tap.h"
#include "tape.h"
#include "tapeport.h"

/*******************************************************************************
    tape
*******************************************************************************/

tape_image_t *tape_image_dev1 = NULL;

int tape_image_attach(unsigned int unit, const char *name)
{
    return 0;
}

void tape_shutdown(void)
{
}

int tape_tap_attached(void)
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

void tape_image_event_playback(unsigned int unit, const char *filename)
{
}

int tape_image_detach(unsigned int unit)
{
    return 0;
}

int tap_seek_start(tap_t *tap)
{
    return 0;
}

int tape_image_create(const char *name, unsigned int type)
{
    return 0;
}

int tape_snapshot_write_module(snapshot_t *s, int save_image)
{
    return 0;
}

int tape_snapshot_read_module(snapshot_t *s)
{
    return 0;
}

int tape_read(tape_image_t *tape_image, BYTE *buf, size_t size)
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

void tape_get_header(tape_image_t *tape_image, BYTE *name)
{
}

const char *tape_get_file_name(void)
{
    return NULL;
}

BYTE colorram_read(WORD addr)
{
    return 0;
}

void colorram_store(WORD addr, BYTE value)
{
}

tapeport_device_list_t *tapeport_device_register(tapeport_device_t *device)
{
    return NULL;
}

void tapeport_snapshot_register(tapeport_snapshot_t *snapshot)
{
}

void tapeport_device_unregister(tapeport_device_list_t *device)
{
}

void tapeport_trigger_flux_change(unsigned int on, int id)
{
}

void tapeport_set_tape_sense(int sense, int id)
{
}
