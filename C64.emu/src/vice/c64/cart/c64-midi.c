/*
 * c64-midi.c - C64 specific MIDI (6850 UART) emulation.
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

#include <string.h>

#include "types.h"

#include "c64-midi.h"
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"

/* the order must match the enum in c64-midi.h */
midi_interface_t midi_interface[] = {
    /* Sequential Circuits Inc. */
    { "Sequential", 0xde00, 0, 2, 1, 3, 0xff, 1, 1, CARTRIDGE_MIDI_SEQUENTIAL },
    /* Passport & Syntech */
    { "Passport", 0xde00, 8, 8, 9, 9, 0xff, 1, 1, CARTRIDGE_MIDI_PASSPORT },
    /* DATEL/Siel/JMS */
    { "DATEL", 0xde00, 4, 6, 5, 7, 0xff, 2, 1, CARTRIDGE_MIDI_DATEL },
    /* Namesoft */
    { "Namesoft", 0xde00, 0, 2, 1, 3, 0xff, 1, 2, CARTRIDGE_MIDI_NAMESOFT },
    /* Electronics - Maplin magazine */
    { "Maplin", 0xdf00, 0, 0, 1, 1, 0xff, 2, 0, CARTRIDGE_MIDI_MAPLIN },
    { NULL }
};

/* ---------------------------------------------------------------------*/

static BYTE c64midi_read(WORD address)
{
    return midi_read(address);
}

static BYTE c64midi_peek(WORD address)
{
    return midi_peek(address);
}

/* ---------------------------------------------------------------------*/

static io_source_t midi_device = {
    "MIDI",
    IO_DETACH_RESOURCE,
    "MIDIEnable",
    0xde00, 0xdeff, 0xff,
    1, /* read is always valid */
    midi_store,
    c64midi_read,
    c64midi_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_MIDI_SEQUENTIAL,
    0,
    0
};

static c64export_resource_t export_res = {
    "MIDI", 0, 0, &midi_device, NULL, CARTRIDGE_MIDI_SEQUENTIAL
};

static io_source_list_t *midi_list_item = NULL;

/* ---------------------------------------------------------------------*/

/*
    since different carts which have a different internal ID are emulated, we
    supply seperate hooks.
*/
int c64_midi_cart_enabled(void)
{
    return midi_enabled;
}

int c64_midi_seq_cart_enabled(void)
{
    return midi_enabled && (export_res.cartid == CARTRIDGE_MIDI_SEQUENTIAL);
}

int c64_midi_pp_cart_enabled(void)
{
    return midi_enabled && (export_res.cartid == CARTRIDGE_MIDI_PASSPORT);
}

int c64_midi_datel_cart_enabled(void)
{
    return midi_enabled && (export_res.cartid == CARTRIDGE_MIDI_DATEL);
}

int c64_midi_nsoft_cart_enabled(void)
{
    return midi_enabled && (export_res.cartid == CARTRIDGE_MIDI_NAMESOFT);
}

int c64_midi_maplin_cart_enabled(void)
{
    return midi_enabled && (export_res.cartid == CARTRIDGE_MIDI_MAPLIN);
}

static int set_midi_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!midi_enabled && val) {
        if (c64export_add(&export_res) < 0) {
            return -1;
        }
        midi_list_item = io_source_register(&midi_device);
        midi_enabled = 1;
    } else if (midi_enabled && !val) {
        c64export_remove(&export_res);
        io_source_unregister(midi_list_item);
        midi_list_item = NULL;
        midi_enabled = 0;
    }
    midi_enabled = val;
    return 0;
}

static int midi_set_c64mode(int new_mode, void *param)
{
    int old = midi_enabled;

    switch (new_mode) {
        case MIDI_MODE_SEQUENTIAL:
        case MIDI_MODE_PASSPORT:
        case MIDI_MODE_DATEL:
        case MIDI_MODE_NAMESOFT:
        case MIDI_MODE_MAPLIN:
            break;
        default:
            return -1;
    }

    if (midi_mode != new_mode) {
        set_midi_enabled(0, NULL);
        switch (new_mode) {
            case MIDI_MODE_MAPLIN:
                midi_device.start_address = 0xdf00;
                midi_device.end_address = 0xdfff;
                export_res.io1 = NULL;
                export_res.io2 = &midi_device;
                break;
            default:
                midi_device.start_address = 0xde00;
                midi_device.end_address = 0xdeff;
                export_res.io1 = &midi_device;
                export_res.io2 = NULL;
                break;
        }
        export_res.cartid = midi_interface[new_mode].cartid;
        /* export_res.name = midi_interface[new_mode].name; */
        set_midi_enabled(old, NULL);
    }
    return midi_set_mode(new_mode, param);
}

int c64_midi_enable(void)
{
    return resources_set_int("MIDIEnable", 1);
}

void c64_midi_detach(void)
{
    resources_set_int("MIDIEnable", 0);
}

/* ---------------------------------------------------------------------*/

static const resource_int_t resources_int[] = {
    { "MIDIMode", MIDI_MODE_SEQUENTIAL, RES_EVENT_NO, NULL,
      &midi_mode, midi_set_c64mode, NULL },
    { "MIDIEnable", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &midi_enabled, set_midi_enabled, NULL },
    { NULL }
};

int c64_midi_resources_init(void)
{
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    return midi_resources_init();
}

/* ---------------------------------------------------------------------*/

static const cmdline_option_t cmdline_options[] = {
    { "-miditype", SET_RESOURCE, 1,
      NULL, NULL, "MIDIMode", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SPECIFY_C64_MIDI_TYPE,
      "<0-4>", NULL },
    { NULL }
};

int c64_midi_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return midi_cmdline_options_init();
}

/* ---------------------------------------------------------------------*/

int c64_midi_base_de00(void)
{
    return (midi_interface[midi_mode].base_addr == 0xde00) ? 1 : 0;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTMIDI"

/* FIXME: implement snapshot support */
int c64_midi_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
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

int c64_midi_snapshot_read_module(snapshot_t *s)
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
