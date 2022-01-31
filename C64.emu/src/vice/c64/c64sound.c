/*
 * c64sound.c - C64/C128 sound emulation.
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

#include <stdio.h>
#include <string.h>

#include "cartio.h"
#include "cartridge.h"
#include "machine.h"
#include "sid.h"
#include "sid-resources.h"
#include "sound.h"
#include "types.h"

#define MACHINE_SIDx_RFUNC(fname, func)  \
    static uint8_t fname(uint16_t addr) \
    {                                   \
        return func(addr);              \
    }

MACHINE_SIDx_RFUNC(machine_sid2_read, sid2_read)
MACHINE_SIDx_RFUNC(machine_sid3_read, sid3_read)
MACHINE_SIDx_RFUNC(machine_sid4_read, sid4_read)
MACHINE_SIDx_RFUNC(machine_sid5_read, sid5_read)
MACHINE_SIDx_RFUNC(machine_sid6_read, sid6_read)
MACHINE_SIDx_RFUNC(machine_sid7_read, sid7_read)
MACHINE_SIDx_RFUNC(machine_sid8_read, sid8_read)

MACHINE_SIDx_RFUNC(machine_sid2_peek, sid2_peek)
MACHINE_SIDx_RFUNC(machine_sid3_peek, sid3_peek)
MACHINE_SIDx_RFUNC(machine_sid4_peek, sid4_peek)
MACHINE_SIDx_RFUNC(machine_sid5_peek, sid5_peek)
MACHINE_SIDx_RFUNC(machine_sid6_peek, sid6_peek)
MACHINE_SIDx_RFUNC(machine_sid7_peek, sid7_peek)
MACHINE_SIDx_RFUNC(machine_sid8_peek, sid8_peek)

#define MACHINE_SIDx_STORE(fname, func)            \
    static void fname(uint16_t addr, uint8_t byte) \
    {                                              \
        func(addr, byte);                          \
    }

MACHINE_SIDx_STORE(machine_sid2_store, sid2_store)
MACHINE_SIDx_STORE(machine_sid3_store, sid3_store)
MACHINE_SIDx_STORE(machine_sid4_store, sid4_store)
MACHINE_SIDx_STORE(machine_sid5_store, sid5_store)
MACHINE_SIDx_STORE(machine_sid6_store, sid6_store)
MACHINE_SIDx_STORE(machine_sid7_store, sid7_store)
MACHINE_SIDx_STORE(machine_sid8_store, sid8_store)

/* ---------------------------------------------------------------------*/

/* 2nd SID, can be a cartridge or an internal board */
static io_source_t sid2_device = {
    "Stereo SID",         /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xde00, 0xde1f, 0x1f, /* range for the 2nd SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid2_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid2_read,    /* read function */
    machine_sid2_peek,    /* peek function */
    sid2_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 3rd SID, can be a cartridge or an internal board */
static io_source_t sid3_device = {
    "Triple SID",         /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xdf00, 0xdf1f, 0x1f, /* range for the 3rd SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid3_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid3_read,    /* read function */
    machine_sid3_peek,    /* peek function */
    sid3_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 4th SID, can be a cartridge or an internal board */
static io_source_t sid4_device = {
    "Quad SID",           /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xdf80, 0xdf9f, 0x1f, /* range for the 4th SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid4_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid4_read,    /* read function */
    machine_sid4_peek,    /* peek function */
    sid4_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 5th SID, can be a cartridge or an internal board */
static io_source_t sid5_device = {
    "Penta SID",          /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xde80, 0xde9f, 0x1f, /* range for the 5th SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid5_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid5_read,    /* read function */
    machine_sid5_peek,    /* peek function */
    sid5_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 6th SID, can be a cartridge or an internal board */
static io_source_t sid6_device = {
    "Hexa SID",           /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xdf40, 0xdf5f, 0x1f, /* range for the 6th SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid6_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid6_read,    /* read function */
    machine_sid6_peek,    /* peek function */
    sid6_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 7th SID, can be a cartridge or an internal board */
static io_source_t sid7_device = {
    "Hepta SID",          /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xde40, 0xde5f, 0x1f, /* range for the 6th SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid7_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid7_read,    /* read function */
    machine_sid7_peek,    /* peek function */
    sid7_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

/* 8th SID, can be a cartridge or an internal board */
static io_source_t sid8_device = {
    "Octa SID",           /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "SidStereo",          /* resource to set to '0' */
    0xdfc0, 0xdfdf, 0x1f, /* range for the 6th SID device, can be changed to other ranges */
    1,                    /* read is always valid */
    machine_sid8_store,   /* store function */
    NULL,                 /* NO poke function */
    machine_sid8_read,    /* read function */
    machine_sid8_peek,    /* peek function */
    sid8_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* none is used here, because it is an I/O only device */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *sid2_list_item = NULL;
static io_source_list_t *sid3_list_item = NULL;
static io_source_list_t *sid4_list_item = NULL;
static io_source_list_t *sid5_list_item = NULL;
static io_source_list_t *sid6_list_item = NULL;
static io_source_list_t *sid7_list_item = NULL;
static io_source_list_t *sid8_list_item = NULL;

/* ---------------------------------------------------------------------*/

/* C64 SID sound chip */
static sound_chip_t sid_sound_chip = {
    sid_sound_machine_open,              /* sound chip open function */
    sid_sound_machine_init,              /* sound chip init function */
    sid_sound_machine_close,             /* sound chip close function */
    sid_sound_machine_calculate_samples, /* sound chip calculate samples function */
    sid_sound_machine_store,             /* sound chip store function */
    sid_sound_machine_read,              /* sound chip read function */
    sid_sound_machine_reset,             /* sound chip reset function */
    sid_sound_machine_cycle_based,       /* sound chip 'is_cycle_based()' function, resid engine is cycle based, all other engines are not */
    sid_sound_machine_channels,          /* sound chip 'get_amount_of_channels()' function, the amount of channels depends on the extra amount of active SIDs */
    1                                    /* sound chip is always enabled */
};

static uint16_t sid_sound_chip_offset = 0;

void sid_sound_chip_init(void)
{
    sid_sound_chip_offset = sound_chip_register(&sid_sound_chip);
}

/* ---------------------------------------------------------------------*/

#define SIDx_CHECK_RANGE(sid_nr)                                                                        \
    int machine_sid##sid_nr##_check_range(unsigned int sid_adr)                                         \
    {                                                                                                   \
        if (machine_class == VICE_MACHINE_C128) {                                                       \
            if ((sid_adr >= 0xd400 && sid_adr <= 0xd4e0) || (sid_adr >= 0xd700 && sid_adr <= 0xdfe0)) { \
                sid##sid_nr##_address_start = sid_adr;                                                  \
                sid##sid_nr##_device.start_address = sid_adr;                                           \
                sid##sid_nr##_address_end = sid_adr + 0x1f;                                             \
                sid##sid_nr##_device.end_address = sid_adr + 0x1f;                                      \
                if (sid_adr >= 0xd400 && sid_adr <= 0xd4e0) {                                           \
                    sid##sid_nr##_device.io_source_prio = IO_PRIO_HIGH;                                 \
                } else {                                                                                \
                    sid##sid_nr##_device.io_source_prio = IO_PRIO_NORMAL;                               \
                }                                                                                       \
                if (sid##sid_nr##_list_item != NULL) {                                                  \
                    io_source_unregister(sid##sid_nr##_list_item);                                      \
                    sid##sid_nr##_list_item = io_source_register(&sid##sid_nr##_device);                \
                } else {                                                                                \
                    if (sid_stereo >= sid_nr - 1) {                                                     \
                        sid##sid_nr##_list_item = io_source_register(&sid##sid_nr##_device);            \
                    }                                                                                   \
                }                                                                                       \
                return 0;                                                                               \
            }                                                                                           \
        } else {                                                                                        \
            if (sid_adr >= 0xd400 && sid_adr <= 0xdfe0) {                                               \
                sid##sid_nr##_address_start = sid_adr;                                                  \
                sid##sid_nr##_device.start_address = sid_adr;                                           \
                sid##sid_nr##_address_end = sid_adr + 0x1f;                                             \
                sid##sid_nr##_device.end_address = sid_adr + 0x1f;                                      \
                if (sid_adr >= 0xd400 && sid_adr <= 0xd7e0) {                                           \
                    sid##sid_nr##_device.io_source_prio = IO_PRIO_HIGH;                                 \
                } else {                                                                                \
                    sid##sid_nr##_device.io_source_prio = IO_PRIO_NORMAL;                               \
                }                                                                                       \
                if (sid##sid_nr##_list_item != NULL) {                                                  \
                    io_source_unregister(sid##sid_nr##_list_item);                                      \
                    sid##sid_nr##_list_item = io_source_register(&sid##sid_nr##_device);                \
                } else {                                                                                \
                    if (sid_stereo >= sid_nr - 1) {                                                     \
                        sid##sid_nr##_list_item = io_source_register(&sid##sid_nr##_device);            \
                    }                                                                                   \
                }                                                                                       \
                return 0;                                                                               \
            }                                                                                           \
        }                                                                                               \
        return -1;                                                                                      \
    }

SIDx_CHECK_RANGE(2)
SIDx_CHECK_RANGE(3)
SIDx_CHECK_RANGE(4)
SIDx_CHECK_RANGE(5)
SIDx_CHECK_RANGE(6)
SIDx_CHECK_RANGE(7)
SIDx_CHECK_RANGE(8)

void machine_sid2_enable(int val)
{
    if (sid2_list_item != NULL) {
        io_source_unregister(sid2_list_item);
        sid2_list_item = NULL;
    }
    if (sid3_list_item != NULL) {
        io_source_unregister(sid3_list_item);
        sid3_list_item = NULL;
    }
    if (sid4_list_item != NULL) {
        io_source_unregister(sid4_list_item);
        sid4_list_item = NULL;
    }
    if (sid5_list_item != NULL) {
        io_source_unregister(sid5_list_item);
        sid5_list_item = NULL;
    }
    if (sid6_list_item != NULL) {
        io_source_unregister(sid6_list_item);
        sid6_list_item = NULL;
    }
    if (sid7_list_item != NULL) {
        io_source_unregister(sid7_list_item);
        sid7_list_item = NULL;
    }
    if (sid8_list_item != NULL) {
        io_source_unregister(sid8_list_item);
        sid8_list_item = NULL;
    }

    if (val >= 1) {
        sid2_list_item = io_source_register(&sid2_device);
    }
    if (val >= 2) {
        sid3_list_item = io_source_register(&sid3_device);
    }
    if (val >= 3) {
        sid4_list_item = io_source_register(&sid4_device);
    }
    if (val >= 4) {
        sid5_list_item = io_source_register(&sid5_device);
    }
    if (val >= 5) {
        sid6_list_item = io_source_register(&sid6_device);
    }
    if (val >= 6) {
        sid7_list_item = io_source_register(&sid7_device);
    }
    if (val >= 7) {
        sid8_list_item = io_source_register(&sid8_device);
    }
}

char *sound_machine_dump_state(sound_t *psid)
{
    return sid_sound_machine_dump_state(psid);
}

void sound_machine_enable(int enable)
{
    sid_sound_machine_enable(enable);
}
