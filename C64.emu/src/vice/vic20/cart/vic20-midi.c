/*
 * vic20-midi.c - VIC20 specific MIDI (6850 UART) emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
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

#ifdef HAVE_MIDI

#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "vic20-midi.h"

midi_interface_t midi_interface[] = {
    /* Electronics - Maplin magazine */
    { "Maplin", 0x9c00, 0, 0, 1, 1, 0xff, 2, 0, CARTRIDGE_MIDI_MAPLIN },
    { NULL }
};

/* ---------------------------------------------------------------------*/

static BYTE vic20midi_read(WORD address)
{
    return midi_read(address);
}

static BYTE vic20midi_peek(WORD address)
{
    return midi_peek(address);
}

/* ---------------------------------------------------------------------*/

static io_source_t midi_device = {
    "MIDI",
    IO_DETACH_RESOURCE,
    "MIDIEnable",
    0x9c00, 0x9fff, 0x3ff,
    1, /* read is always valid */
    midi_store,
    vic20midi_read,
    vic20midi_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_MIDI_MAPLIN,
    0,
    0
};

static io_source_list_t *midi_list_item = NULL;

/* ---------------------------------------------------------------------*/

static int set_midi_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!midi_enabled && val) {
        midi_list_item = io_source_register(&midi_device);
        midi_enabled = 1;
    } else if (midi_enabled && !val) {
        io_source_unregister(midi_list_item);
        midi_list_item = NULL;
        midi_enabled = 0;
    }
    midi_enabled = val;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "MIDIEnable", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &midi_enabled, set_midi_enabled, NULL },
#if 0
    /* currently only 1 mode is supported, so this resource is unneeded */
    { "MIDIMode", MIDI_MODE_MAPLIN, RES_EVENT_NO, NULL,
      &midi_mode, midi_set_mode, NULL },
#endif
    { NULL }
};

int vic20_midi_resources_init(void)
{
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

#if 1
    /* currently only 1 mode is supported */
    midi_set_mode(MIDI_MODE_MAPLIN, NULL);
#endif

    return midi_resources_init();
}

int vic20_midi_cmdline_options_init(void)
{
    return midi_cmdline_options_init();
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTMIDI"

/* FIXME: implement snapshot support */
int vic20_midi_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int vic20_midi_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

#endif
