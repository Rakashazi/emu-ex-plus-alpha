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
#include "export.h"
#include "machine.h"
#include "resources.h"
#include "snapshot.h"
#include "vic20-midi.h"

midi_interface_t midi_interface[] = {
    /* Electronics - Maplin magazine */
    { "Maplin", 0x9c00, 0, 0, 1, 1, 0xff, 2, 0, CARTRIDGE_MIDI_MAPLIN },
    MIDI_INFERFACE_LIST_END
};

/* ---------------------------------------------------------------------*/

static uint8_t vic20midi_read(uint16_t address)
{
    return midi_read(address);
}

static uint8_t vic20midi_peek(uint16_t address)
{
    return midi_peek(address);
}

/* ---------------------------------------------------------------------*/

static io_source_t midi_device = {
    CARTRIDGE_VIC20_NAME_MIDI, /* name of the device */
    IO_DETACH_RESOURCE,        /* use resource to detach the device when involved in a read-collision */
    "MIDIEnable",              /* resource to set to '0' */
    0x9c00, 0x9fff, 0x3ff,     /* range for the device, regs:$9c00-$9c01, mirrors:$9c02-$9fff */
    1,                         /* read is always valid */
    midi_store,                /* store function */
    NULL,                      /* NO poke function */
    vic20midi_read,            /* read function */
    vic20midi_peek,            /* peek function */
    NULL,                      /* TODO: device state information dump function */
    CARTRIDGE_MIDI_MAPLIN,     /* cartridge ID */
    IO_PRIO_NORMAL,            /* normal priority, device read needs to be checked for collisions */
    0                          /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *midi_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_VIC20_NAME_MIDI, 0, 0, NULL, &midi_device, CARTRIDGE_MIDI_MAPLIN
};

/* ---------------------------------------------------------------------*/

static int set_midi_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!midi_enabled && val) {
        if (export_add(&export_res) < 0) {
            return -1;
        }
        midi_list_item = io_source_register(&midi_device);
        midi_enabled = 1;
    } else if (midi_enabled && !val) {
        export_remove(&export_res);
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
    RESOURCE_INT_LIST_END
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

/* CARTMIDI snapshot module format:

   type  | name | description
   --------------------------
   BYTE  | mode | midi mode
 */

static char snap_module_name[] = "CARTMIDI";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int vic20_midi_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (SMW_B(m, (uint8_t)midi_mode) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return midi_snapshot_write_module(s);
}

int vic20_midi_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    int tmp_midi_mode;

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not allow versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (SMR_B_INT(m, &tmp_midi_mode) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    /* enable midi */
#if 0
    midi_set_vic20mode(tmp_midi_mode, NULL);
#endif
    set_midi_enabled(1, NULL);

    return midi_snapshot_read_module(s);

fail:
    snapshot_module_close(m);
    return -1;
}

/* ---------------------------------------------------------------------*/

void vic20_midi_detach(void)
{
    set_midi_enabled(0, NULL);
}
#endif
