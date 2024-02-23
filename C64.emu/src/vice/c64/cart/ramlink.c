/*
 * ramlink.c - Cartridge handling, CMD Ramlink
 *
 * Written by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#define CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOT0_API
#include "c64cart.h"
#include "c64mem.h"
#include "c64cartmem.h"
#include "c64pla.h"
#include "c64-generic.h"
#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "ramlink.h"
#include "monitor.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "crt.h"
#include "lib.h"
#include "machine.h"
#include "ram.h"
#include "resources.h"
#include "cmdline.h"
#include "maincpu.h"
#include "log.h"
#include "i8255a.h"
#include "rtc/rtc-72421.h"
#include "drive.h"
#include "maincpu.h"
#include "drive/iec/cmdhd.h"

/* #define RLLOG1 */
/* #define RLLOG2 */
/* #define RLDEBUGIO */
/* #define RLDEBUGMEM */

#define LOG LOG_DEFAULT
#define ERR LOG_ERR

#ifdef RLDEBUGMEM
#define MDBG(_x_) log_message _x_
#else
#define MDBG(_x_)
#endif

#ifdef RLDEBUGIO
#define IDBG(_x_) log_message _x_
#else
#define IDBG(_x_)
#endif

#ifdef RLLOG1
#define LOG1(_x_) log_message _x_
#else
#define LOG1(_x_)
#endif

#ifdef RLLOG2
#define LOG2(_x_) log_message _x_
#else
#define LOG2(_x_)
#endif

#define CRIT(_x_) log_message _x_

#if C64CART_ROM_LIMIT <= 65536
#error C64CART_ROM_LIMIT is too small; it should be at least 65536
#endif

/*

to make crt:

ramlink v1.40
cartconv -t rl -i ramlink1.bin -o ramlink1.crt -n "CMD RAMLINK 1.40"

ramlink v2.01
cartconv -t rl -i ramlink2.bin -o ramlink2.crt -n "CMD RAMLINK 2.01"

*/

extern unsigned int reg_pc;

/* resources */
static int rl_enabled = 0;
static int rl_write_image = 0;
static int rl_cardsizemb = 0;
static int rl_normal = 1; /* either 1=normal, 0=direct */
static int rl_rtcsave = 0;
static char *rl_filename = NULL;
static char *rl_bios_filename = NULL;

#define rl_kernbase64  (2*0x4000)
#define rl_kernbase128 (3*0x4000)

/* internal stuff */
static uint8_t rl_on = 0;
static uint8_t rl_dos = 0;
static uint8_t rl_mapped = 0;
static uint32_t rl_rombase = 0;
static uint32_t rl_kernbase = 0;
static uint32_t rl_rambase = 0;
static uint32_t rl_cardbase = 0;
static uint32_t rl_cardmask = 0;
static uint32_t rl_io1mode = 7; /* initially unused mode */
static rtc_72421_t *rl_rtc = NULL;
static i8255a_state rl_i8255a;
static uint8_t rl_i8255a_i[3];
static uint8_t rl_i8255a_o[3];
static uint8_t *rl_card = NULL;
static uint32_t rl_cardsize = 0;
static uint32_t rl_cardsize_old = 0;
static uint32_t rl_scanned = 0;
static uint32_t rl_reu_trap = 0;
static uint8_t *rl_ram = NULL;
static uint8_t *rl_rom = NULL;
static int rl_extexrom = 0;
static int rl_extgame = 0;

/* some prototypes are needed */
static uint8_t ramlink_io1_read(uint16_t addr);
static uint8_t ramlink_io1_peek(uint16_t addr);
static void ramlink_io1_store(uint16_t addr, uint8_t value);
static int ramlink_io1_dump(void);
static uint8_t ramlink_io2_peek(uint16_t addr);
static uint8_t ramlink_io2_40_43_read(uint16_t addr);
static uint8_t ramlink_io2_b0_bf_read(uint16_t addr);
static void ramlink_io2_20_22_store(uint16_t addr, uint8_t value);
static void ramlink_io2_40_43_store(uint16_t addr, uint8_t value);
static void ramlink_io2_60_60_store(uint16_t addr, uint8_t value);
static void ramlink_io2_70_70_store(uint16_t addr, uint8_t value);
static void ramlink_io2_7e_7f_store(uint16_t addr, uint8_t value);
static void ramlink_io2_80_9f_store(uint16_t addr, uint8_t value);
static void ramlink_io2_a0_a3_store(uint16_t addr, uint8_t value);
static void ramlink_io2_b0_bf_store(uint16_t addr, uint8_t value);
static void ramlink_io2_c0_c3_store(uint16_t addr, uint8_t value);
static int ramlink_io2_dump(void);

static io_source_t ramlink_io1_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xde00, 0xdeff, 0xff,     /* range for the device, regs:$de00-$deff */
    1,                        /* read is always valid */
    ramlink_io1_store,        /* store function */
    NULL,                     /* NO poke function */
    ramlink_io1_read,         /* read function */
    ramlink_io1_peek,         /* peek function */
    ramlink_io1_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* normal priority, device read needs to be checked for collisions */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_20_22_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf20, 0xdf22, 0xff,     /* range for the device, regs:$df20-$df22 */
    1,                        /* read is always valid */
    ramlink_io2_20_22_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_40_43_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf40, 0xdf43, 0xff,     /* range for the device, regs:$df40-$df43 */
    1,                        /* read is always valid */
    ramlink_io2_40_43_store,  /* store function */
    NULL,                     /* NO poke function */
    ramlink_io2_40_43_read,   /* read function */
    ramlink_io2_peek,         /* peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_60_60_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf60, 0xdf60, 0xff,     /* range for the device, regs:$df60-$df60 */
    1,                        /* read is always valid */
    ramlink_io2_60_60_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_70_70_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf70, 0xdf70, 0xff,     /* range for the device, regs:$df70-$df70 */
    1,                        /* read is always valid */
    ramlink_io2_70_70_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_7e_7f_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf7e, 0xdf7f, 0xff,     /* range for the device, regs:$df7e-$df7f */
    1,                        /* read is always valid */
    ramlink_io2_7e_7f_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_80_9f_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdf80, 0xdf9f, 0xff,     /* range for the device, regs:$df80-$df9f */
    1,                        /* read is always valid */
    ramlink_io2_80_9f_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_a0_a3_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdfa0, 0xdfa3, 0xff,     /* range for the device, regs:$dfa0-$dfa3 */
    1,                        /* read is always valid */
    ramlink_io2_a0_a3_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_b0_bf_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdfb0, 0xdfbf, 0xff,     /* range for the device, regs:$dfb0-$dfbf */
    1,                        /* read is always valid */
    ramlink_io2_b0_bf_store,  /* store function */
    NULL,                     /* NO poke function */
    ramlink_io2_b0_bf_read,   /* read function */
    ramlink_io2_peek,         /* peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

static io_source_t ramlink_io2_c0_c3_device = {
    CARTRIDGE_NAME_RAMLINK,   /* name of the device */
    IO_DETACH_CART,           /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,    /* does not use a resource for detach */
    0xdfc0, 0xdfc3, 0xff,     /* range for the device, regs:$dfc0-$dfc3 */
    1,                        /* read is always valid */
    ramlink_io2_c0_c3_store,  /* store function */
    NULL,                     /* NO poke function */
    NULL,                     /* NO read function */
    NULL,                     /* NO peek function */
    ramlink_io2_dump,         /* device state information dump function */
    CARTRIDGE_RAMLINK,        /* cartridge ID */
    IO_PRIO_HIGH,             /* high priority, device reads first */
    0,                        /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE            /* NO mirroring */
};

#define RAMLINKIOS 10

static io_source_list_t *ramlink_list_items[RAMLINKIOS] = { NULL };

static io_source_t *ramlink_source_items[RAMLINKIOS] = {
    &ramlink_io2_7e_7f_device, /* the order of the first two */
    &ramlink_io1_device,       /*  are important */
    &ramlink_io2_20_22_device,
    &ramlink_io2_40_43_device,
    &ramlink_io2_60_60_device,
    &ramlink_io2_70_70_device,
    &ramlink_io2_80_9f_device,
    &ramlink_io2_a0_a3_device,
    &ramlink_io2_b0_bf_device,
    &ramlink_io2_c0_c3_device
};

static uint8_t (*ramlink_source_read_items[RAMLINKIOS])(uint16_t address) = {
    NULL,                    /* the order of the first two */
    ramlink_io1_read,        /*  are important */
    NULL,
    ramlink_io2_40_43_read,
    NULL,
    NULL,
    NULL,
    NULL,
    ramlink_io2_b0_bf_read,
    NULL
};

static void (*ramlink_source_store_items[RAMLINKIOS])(uint16_t address, uint8_t data) = {
    ramlink_io2_7e_7f_store, /* the order of the first two */
    ramlink_io1_store,       /*  are important */
    ramlink_io2_20_22_store,
    ramlink_io2_40_43_store,
    ramlink_io2_60_60_store,
    ramlink_io2_70_70_store,
    ramlink_io2_80_9f_store,
    ramlink_io2_a0_a3_store,
    ramlink_io2_b0_bf_store,
    ramlink_io2_c0_c3_store
};

static const export_resource_t export_res_plus[RAMLINKIOS] = {
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_7e_7f_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, &ramlink_io1_device, NULL,       CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_20_22_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_40_43_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_60_60_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_70_70_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_80_9f_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_a0_a3_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_b0_bf_device, CARTRIDGE_RAMLINK},
    {CARTRIDGE_NAME_RAMLINK, 1, 1, NULL, &ramlink_io2_c0_c3_device, CARTRIDGE_RAMLINK}
};

#define RAMLINKOTHERIOS 20

static int ramlink_devices_io1_count;
static int ramlink_devices_io1_georam;
static int ramlink_devices_io2_count;
static int ramlink_devices_io2_reu;
static int ramlink_devices_io2_georam;
static io_source_t ramlink_devices_io1_entry[RAMLINKOTHERIOS];
static io_source_t *ramlink_devices_io1[RAMLINKOTHERIOS];
static io_source_t ramlink_devices_io2_entry[RAMLINKOTHERIOS];
static io_source_t *ramlink_devices_io2[RAMLINKOTHERIOS];

static void ramlink_scan_io(void)
{
    io_source_list_t *current;

    ramlink_devices_io1_georam = -1;
    ramlink_devices_io2_reu = -1;
    ramlink_devices_io2_georam = -1;

    /* find top of list for io1 */
    current = ramlink_list_items[1]->previous;
    while (current->previous) {
        current = current->previous;
    }

    /* go down list copying pointers and data from all devices */
    ramlink_devices_io1_count = 0;
    while (current) {
        if (current->device) {
            if (current->device->cart_id != CARTRIDGE_RAMLINK) {
                LOG2((LOG, "RAMLINK: Scanning IO1 %s", current->device->name));
                ramlink_devices_io1[ramlink_devices_io1_count] = current->device;
                memcpy(&(ramlink_devices_io1_entry[ramlink_devices_io1_count]),
                    current->device, sizeof(io_source_t));
                /* remember which is georam */
                if (current->device->cart_id == CARTRIDGE_GEORAM) {
                    ramlink_devices_io1_georam = ramlink_devices_io1_count;
                    LOG1((LOG, "RAMLINK: Found GEORAM IO1"));
                }
                ramlink_devices_io1_count++;
            }
        }
        current = current->next;
    }

    /* find top of list for io2 */
    current = ramlink_list_items[0]->previous;
    while (current->previous) {
        current = current->previous;
    }

    /* go down list copying pointers and data from all devices */
    ramlink_devices_io2_count = 0;
    while (current) {
        if (current->device) {
            if (current->device->cart_id != CARTRIDGE_RAMLINK) {
                LOG2((LOG, "RAMLINK: Scanning IO2 %s", current->device->name));
                ramlink_devices_io2[ramlink_devices_io2_count] = current->device;
                memcpy(&(ramlink_devices_io2_entry[ramlink_devices_io2_count]),
                    current->device, sizeof(io_source_t));
                /* remember which is georam */
                if (current->device->cart_id == CARTRIDGE_GEORAM) {
                    ramlink_devices_io2_georam = ramlink_devices_io2_count;
                    current->device->io_source_prio = IO_PRIO_LOW;
                    LOG1((LOG, "RAMLINK: Found GEORAM IO2"));
                }
                /* remember which is reu */
                if (current->device->cart_id == CARTRIDGE_REU) {
                    ramlink_devices_io2_reu = ramlink_devices_io2_count;
                    current->device->io_source_prio = IO_PRIO_LOW;
                    LOG1((LOG, "RAMLINK: Found REU IO2"));
                }
                ramlink_devices_io2_count++;
            }
        }
        current = current->next;
    }
    rl_scanned = 1;
}

/* restore any modified IO resourcs back to their original values */
static void ramlink_restore_io(void)
{
    int i;

    for (i = 0; i < ramlink_devices_io1_count; i++) {
        memcpy(ramlink_devices_io1[i], &(ramlink_devices_io1_entry[i]),
            sizeof(io_source_t));
    }
    ramlink_devices_io1_count = 0;
    for (i = 0; i < ramlink_devices_io2_count; i++) {
        memcpy(ramlink_devices_io2[i], &(ramlink_devices_io2_entry[i]),
            sizeof(io_source_t));
    }
    ramlink_devices_io2_count = 0;
    ramlink_devices_io1_georam = -1;
    ramlink_devices_io2_reu = -1;
    ramlink_devices_io2_georam = -1;
}

/* turn off any other IO1 resources */
static void ramlink_other1_off(void)
{
    int i;

    for (i = 0; i < ramlink_devices_io1_count; i++) {
        /* skip georam */
        if (i == ramlink_devices_io1_georam) continue;
        LOG2((LOG, "RAMLINK: OFF IO1 %s", ramlink_devices_io1[i]->name));
        ramlink_devices_io1[i]->read = NULL;
        ramlink_devices_io1[i]->store = NULL;
    }
}

/* turn on any other IO1 resources */
static void ramlink_other1_on(void)
{
    int i;

    for (i = 0; i < ramlink_devices_io1_count; i++) {
        /* skip georam */
        if (i == ramlink_devices_io1_georam) continue;
        LOG2((LOG, "RAMLINK: ON IO1 %s", ramlink_devices_io1[i]->name));
        ramlink_devices_io1[i]->read = ramlink_devices_io1_entry[i].read;
        ramlink_devices_io1[i]->store = ramlink_devices_io1_entry[i].store;
    }
}

/* turn off REU by removing read/store pointers */
static void ramlink_reu_off(void)
{
    if (ramlink_devices_io2_reu >= 0) {
        ramlink_devices_io2[ramlink_devices_io2_reu]->read = NULL;
        ramlink_devices_io2[ramlink_devices_io2_reu]->store = NULL;
        IDBG((LOG, "RAMLINK: REU off"));
    }
}

/* turn on REU by restoring read/store pointers */
static void ramlink_reu_on(void)
{
    if (ramlink_devices_io2_reu >= 0) {
        ramlink_devices_io2[ramlink_devices_io2_reu]->read =
            ramlink_devices_io2_entry[ramlink_devices_io2_reu].read;
        ramlink_devices_io2[ramlink_devices_io2_reu]->store =
            ramlink_devices_io2_entry[ramlink_devices_io2_reu].store;
        IDBG((LOG, "RAMLINK: REU on"));
    }
}

/* turn off GEORAM IO1 by removing read/store pointers */
static void ramlink_georam1_off(void)
{
    if (ramlink_devices_io1_georam >= 0) {
        ramlink_devices_io1[ramlink_devices_io1_georam]->read = NULL;
        ramlink_devices_io1[ramlink_devices_io1_georam]->store = NULL;
        IDBG((LOG, "RAMLINK: GEORAM1 off"));
    }
}

/* turn off GEORAM IO2 by removing read/store pointers */
static void ramlink_georam2_off(void)
{
    if (ramlink_devices_io2_georam >= 0) {
        ramlink_devices_io2[ramlink_devices_io2_georam]->read = NULL;
        ramlink_devices_io2[ramlink_devices_io2_georam]->store = NULL;
        IDBG((LOG, "RAMLINK: GEORAM2 off"));
    }
}

/* turn on GEORAM IO1 by restoring read/store pointers */
static void ramlink_georam1_on(void)
{
    if (ramlink_devices_io1_georam >= 0) {
        ramlink_devices_io1[ramlink_devices_io1_georam]->read =
            ramlink_devices_io1_entry[ramlink_devices_io1_georam].read;
        ramlink_devices_io1[ramlink_devices_io1_georam]->store =
            ramlink_devices_io1_entry[ramlink_devices_io1_georam].store;
        IDBG((LOG, "RAMLINK: GEORAM2 on"));
    }
}

/* turn on GEORAM IO2 by restoring read/store pointers */
static void ramlink_georam2_on(void)
{
    if (ramlink_devices_io2_georam >= 0) {
        ramlink_devices_io2[ramlink_devices_io2_georam]->read =
            ramlink_devices_io2_entry[ramlink_devices_io2_georam].read;
        ramlink_devices_io2[ramlink_devices_io2_georam]->store =
            ramlink_devices_io2_entry[ramlink_devices_io2_georam].store;
        IDBG((LOG, "RAMLINK: GEORAM2 on"));
    }
}

/* turn off anything in RAMPORT */
static void ramlink_ramport_off(void)
{
    ramlink_georam1_off();
}

/* turn on anything in RAMPORT */
static void ramlink_ramport_on(void)
{
    ramlink_georam1_on();
}

/* turn off RL IO1 */
static void ramlink_io1_off(void)
{
    ramlink_source_items[1]->read = NULL;
    ramlink_source_items[1]->store = NULL;
}

/* turn on RL IO1 */
static void ramlink_io1_on(void)
{
    ramlink_source_items[1]->read = ramlink_source_read_items[1];
    ramlink_source_items[1]->store = ramlink_source_store_items[1];
}

/* turn off RL IO2 except $DF7E-$DF7F */
static void ramlink_io2_off(void)
{
    int i;

    for (i=2;i<RAMLINKIOS;i++) {
        ramlink_source_items[i]->read = NULL;
        ramlink_source_items[i]->store = NULL;
    }
}

/* turn on RL IO2 */
static void ramlink_io2_on(void)
{
    int i;

    for (i=2;i<RAMLINKIOS;i++) {
        ramlink_source_items[i]->read = ramlink_source_read_items[i];
        ramlink_source_items[i]->store = ramlink_source_store_items[i];
    }
}

/* Turn off RL except $DF7E-$DF7F */
static void ramlink_off(void)
{
    ramlink_io1_off();
    ramlink_io2_off();
    if (rl_normal) {
        ramlink_ramport_off();
        ramlink_reu_off();
        ramlink_georam2_off();
    } else {
        ramlink_ramport_on();
        ramlink_reu_on();
        ramlink_georam2_on();
    }
    ramlink_other1_on();
    rl_on = 0;
    rl_dos = 0;
    rl_reu_trap = 0;
    cart_port_config_changed_slot0();
}

/* Turn on RL */
static void ramlink_on(void)
{
    ramlink_io1_on();
    ramlink_io2_on();
    ramlink_other1_off();
    ramlink_reu_on();
    ramlink_georam2_on();
    rl_on = 1;
    cart_port_config_changed_slot0();
}

/* ---------------------------------------------------------------------*/
static int ramlink_registerio(void)
{
    int i;

    LOG2((LOG, "RAMLINK: registerio"));

    /* register anything that hasn't been registered */
    for (i = 0; i < RAMLINKIOS ; i++ ) {
        if (!ramlink_list_items[i]) {
            if (export_add(&export_res_plus[i]) < 0) {
                return -1;
            }
            ramlink_list_items[i] = io_source_register(ramlink_source_items[i]);
        }
    }

    return 0;
}

static void ramlink_unregisterio(void)
{
    int i;

    LOG2((LOG, "RAMLINK: unregisterio"));

    /* unregister anything that has been registered */
    for (i = 0; i < RAMLINKIOS ; i++ ) {
        if (ramlink_list_items[i]) {
            export_remove(&export_res_plus[i]);
            io_source_unregister(ramlink_list_items[i]);
            ramlink_list_items[i] = NULL;
        }
    }
}

int ramlink_ram_save(const char *filename)
{
    if (rl_card == NULL) {
        return -1;
    }

    if (filename == NULL) {
        return -1;
    }

    if (!util_check_null_string(filename)) {
        LOG1((LOG, "RAMLINK: Writing RAMLINK memory image %s.", filename));
        if (util_file_save(filename, rl_card, rl_cardsize) < 0) {
            CRIT((ERR, "RAMLINK: Writing RAMLINK memory image %s failed.",
                filename));
            return -1;
        }
    }

    return 0;
}

int ramlink_can_save_ram_image(void)
{
    if (rl_filename == NULL) {
        return 0;
    }
    return 1;
}

int ramlink_flush_ram_image(void)
{
    if (ramlink_ram_save(rl_filename) < 0) {
        return -1;
    }
    return 0;
}

int ramlink_can_flush_ram_image(void)
{
    if (rl_filename == NULL) {
        return 0;
    }
    return 1;
}

/* save RAMCard image file if set to by resource */
static int ramlink_save_image(void)
{
    if (rl_write_image) {
        return ramlink_flush_ram_image();
    }

    return 0;
}

/* load RAMCard image file */
static int ramlink_load_image(void)
{
    if (rl_card == NULL) {
        return -1;
    }

    if (rl_filename == NULL) {
        return -1;
    }

    if (!util_check_null_string(rl_filename)) {
        if (util_file_load(rl_filename, rl_card, (size_t)rl_cardsize,
            UTIL_FILE_LOAD_RAW) < 0) {
            CRIT((ERR, "RAMLINK: Reading RAMLINK memory image %s failed.",
                rl_filename));
            /* only create a new file if no file exists, so we dont accidently
                overwrite any files */
            if (!util_file_exists(rl_filename)) {
                if (util_file_save(rl_filename, rl_card, rl_cardsize) < 0) {
                    CRIT((ERR, "RAMLINK: Creating RAMLINK memory image %s failed.",
                        rl_filename));
                    return -1;
                }
                LOG1((LOG, "RAMLINK: Creating RAMLINK memory image %s.", rl_filename));
            }
            return 0;
        }
        LOG1((LOG, "RAMLINK: Reading RAMLINK memory image %s.", rl_filename));
    }

    return 0;
}

/* FIXME: this still needs to be tweaked to match the hardware */
static RAMINITPARAM ramparam = {
    .start_value = 255,
    .value_invert = 2,
    .value_offset = 1,

    .pattern_invert = 0x100,
    .pattern_invert_value = 255,

    .random_start = 0,
    .random_repeat = 0,
    .random_chance = 0,
};

static int ramlink_activate(void)
{
    i8255a_reset(&rl_i8255a);

    if (!rl_rtc) {
        rl_rtc = rtc72421_init("RAMLINKRTC");
    }

    if (!rl_cardsize) {
        rl_card = NULL;
        return 0;
    }

    rl_card = lib_realloc(rl_card, rl_cardsize);

    /* Clear newly allocated RAM.  */
    if (rl_cardsize > rl_cardsize_old) {
        /* memset(rl_card, 0, (size_t)(rl_cardsize - rl_cardsize_old)); */
        ram_init_with_pattern(&rl_card[rl_cardsize_old],
                              (unsigned int)(rl_cardsize - rl_cardsize_old), &ramparam);
    }

    rl_cardsize_old = rl_cardsize;

    LOG1((LOG, "RAMLINK: %dMiB unit installed.", rl_cardsizemb));

    return ramlink_load_image();
}

static int ramlink_deactivate(void)
{
    int res = 0;

    if (rl_rtc) {
        rtc72421_destroy(rl_rtc, rl_rtcsave);
        rl_rtc = NULL;
    }

    if (rl_card == NULL) {
        return 0;
    }

    res = ramlink_save_image();

    lib_free(rl_card);
    rl_card = NULL;

    return res;
}

static int set_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    LOG2((LOG, "RAMLINK: set_enabled %d", val));

    if ((!val) && (rl_enabled)) {
        cart_power_off();
        if (ramlink_deactivate() < 0) {
            return -1;
        }
        ramlink_restore_io();
        ramlink_unregisterio();
        rl_enabled = 0;
        cart_port_config_changed_slot0();
    } else if ((val) && (!rl_enabled)) {
        /* activate ramlink */
        if (param) {
            /* if the param is != NULL, then we should load the default image file */
            LOG1((LOG, "RAMLINK: set_enabled(1) '%s'", rl_bios_filename));
            if (rl_bios_filename) {
                if (*rl_bios_filename) {
                    /* try .crt image first */
                    if ((cartridge_attach_image(CARTRIDGE_CRT, rl_bios_filename) < 0) &&
                        (cartridge_attach_image(CARTRIDGE_RAMLINK, rl_bios_filename) < 0)) {
                        LOG1((LOG, "RAMLINK: set_enabled(1) did not register"));
                        return -1;
                    }
                    /* rl_enabled = 1; */ /* cartridge_attach_image will end up calling set_enabled again */
                    return 0;
                }
            }
        } else {
            cart_power_off();
            if (ramlink_activate() < 0) {
                return -1;
            }
            ramlink_registerio();
            rl_enabled = 1;
            rl_scanned = 0;
            cart_port_config_changed_slot0();
        }
    }
    return 0;
}

static int set_size(int size, void *param)
{
    if (size < 0 || size > 16) {
        return -1;
    }

    if (rl_enabled) {
        ramlink_deactivate();
    }

    rl_cardsizemb = size;
    rl_cardsize = rl_cardsizemb << 20;
    rl_cardmask = rl_cardsize - 1;

    if (rl_enabled) {
        ramlink_activate();
    }

    LOG1((LOG, "RAMLINK: size = %d MiB", size));

    return 0;
}

static int set_mode(int value, void *param)
{
    if (value) {
        rl_normal = RL_MODE_NORMAL;
    } else {
        rl_normal = RL_MODE_DIRECT; /* direct mode */
    }

    LOG1((LOG, "RAMLINK: mode = %s", rl_normal ? "Normal" : "Direct" ));

    return 0;
}

static int set_rtcsave(int val, void *param)
{
    rl_rtcsave = val ? 1 : 0;

    return 0;
}

static int set_image_write(int val, void *param)
{
    rl_write_image = val ? 1 : 0;

    return 0;
}

static int set_bios_filename(const char *name, void *param)
{
    int enabled;

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }
    LOG1((LOG, "RAMLINK: set_bios_filename: %d '%s'", rl_enabled, rl_bios_filename));

    util_string_set(&rl_bios_filename, name);
    resources_get_int("RAMLINK", &enabled);

    if (set_enabled(enabled, (void*)1) < 0) {
        lib_free(rl_bios_filename);
        rl_bios_filename = NULL;
        LOG1((LOG, "RAMLINK: set_bios_filename done: %d 'NULL'", rl_enabled));
        return -1;
    }
    LOG1((LOG, "RAMLINK: set_bios_filename done: %d '%s'", rl_enabled, rl_bios_filename));

    return 0;
}

static int set_filename(const char *name, void *param)
{
    if (rl_filename != NULL && name != NULL && strcmp(name, rl_filename) == 0) {
        return 0;
    }

    if (name != NULL && *name != '\0') {
        if (util_check_filename_access(name) < 0) {
            return -1;
        }
    }

    if (rl_enabled) {
        ramlink_deactivate();
    }
    util_string_set(&rl_filename, name);

    if (rl_enabled) {
        ramlink_activate();
    }

    LOG1((LOG, "RAMLINK: filename = '%s'", name));

    return 0;
}

static const resource_string_t resources_string[] = {
    { "RAMLINKBIOSfilename", "", RES_EVENT_NO, NULL,
      &rl_bios_filename, set_bios_filename, NULL },
    { "RAMLINKfilename", "", RES_EVENT_NO, NULL,
      &rl_filename, set_filename, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "RAMLINKImageWrite", 0, RES_EVENT_NO, NULL,
      &rl_write_image, set_image_write, NULL },
    { "RAMLINKsize", 16, RES_EVENT_NO, NULL,
      &rl_cardsizemb, set_size, 0 },
    { "RAMLINKmode", RL_MODE_NORMAL, RES_EVENT_NO, NULL,
      &rl_normal, set_mode, 0 },
    { "RAMLINKRTCSave", 0, RES_EVENT_NO, NULL,
      &rl_rtcsave, set_rtcsave, 0 },
    /* keeping "enable" resource last prevents unnecessary (re)init when loading config file */
    { "RAMLINK", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &rl_enabled, set_enabled, (void *)1 },
    RESOURCE_INT_LIST_END
};

static const cmdline_option_t cmdline_options[] =
{
    { "-ramlink", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINK", (resource_value_t)1,
      NULL, "Enable the " CARTRIDGE_NAME_RAMLINK " Unit" },
    { "+ramlink", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINK", (resource_value_t)0,
      NULL, "Disable the " CARTRIDGE_NAME_RAMLINK " Unit" },
    { "-ramlinkbios", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMLINKBIOSfilename", NULL,
      "<Name>", "Specify name of " CARTRIDGE_NAME_RAMLINK " BIOS image" },
    { "-ramlinkmode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMLINKmode", NULL,
      "<Mode>", "RAMPort Mode (1=Normal, 0=Direct)" },
    { "-ramlinkrtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINKrtcsave", (resource_value_t)1,
      NULL, "Enable saving of the RTC data when changed." },
    { "+ramlinkrtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINKrtcsave", (resource_value_t)0,
      NULL, "Disable saving of the RTC data when changed." },
    { "-ramlinksize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMLINKsize", NULL,
      "<value>", CARTRIDGE_NAME_RAMLINK " RAMCARD size in MiB (0=Disabled)" },
    { "-ramlinkimage", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RAMLINKfilename", NULL,
      "<Name>", "Specify name of " CARTRIDGE_NAME_RAMLINK " image" },
    { "-ramlinkimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINKImageWrite", (resource_value_t)1,
      NULL, "Allow writing to " CARTRIDGE_NAME_RAMLINK " image" },
    { "+ramlinkimagerw", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RAMLINKImageWrite", (resource_value_t)0,
      NULL, "Do not write to " CARTRIDGE_NAME_RAMLINK " image" },
    CMDLINE_LIST_END
};

int ramlink_cmdline_options_init(void)
{
    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    return 0;
}

int ramlink_resources_init(void)
{
    LOG2((LOG, "RAMLINK: resource init"));

    rl_filename = NULL;

    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    if (!rl_ram) {
        rl_ram = lib_malloc(0x2000);
    }

    if (!rl_rom) {
        rl_rom = lib_malloc(0x10000);
    }

    return 0;
}

int ramlink_resources_shutdown(void)
{
    LOG2((LOG, "RAMLINK: resource shutdown"));

    if (rl_filename) {
        lib_free(rl_filename);
        rl_filename = NULL;
    }

    if (rl_bios_filename) {
        lib_free(rl_bios_filename);
        rl_bios_filename = NULL;
    }

    if (rl_card) {
        lib_free(rl_card);
    }

    if (rl_ram) {
        lib_free(rl_ram);
        rl_ram = NULL;
    }

    if (rl_rom) {
        lib_free(rl_rom);
        rl_rom = NULL;
    }

    return 0;
}

const char *ramlink_get_ram_file_name(void)
{
    return rl_filename;
}

int ramlink_cart_enabled(void)
{
    return rl_enabled;
}

/* ---------------------------------------------------------------------*/

/* Makes sure all drive cpus are synced up with main CPU */
/* The timing for the parallel transfers are pretty tight */
static void ramlink_sync_cpus(void) {
    if (maincpu_clk) {
        drive_cpu_execute_all(maincpu_clk);
    }
}

/* i8255a interfacing */
/* This is mostly for the parallel port */
/* Port A is for CMD Parallel bus, input/output, data only */
static void set_pa(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
#ifdef RLLOG2
    static uint32_t calls=0;

    calls++;
    if (!(calls & 0xffff)) {
        LOG2((LOG,"RAMLINK: Total parallel writes = %u",calls));
    }
#endif
    ramlink_sync_cpus();
    cmdbus.cpu_data = byte;
    cmdbus_update();
}

static uint8_t get_pa(struct _i8255a_state *ctx, int8_t reg)
{
    uint8_t data;
#ifdef RLLOG2
    static uint32_t calls=0;

    calls++;
    if (!(calls & 0xffff)) {
        LOG2((LOG,"RAMLINK: Total parallel reads = %u",calls));
    }
#endif
    ramlink_sync_cpus();
    if (reg == 0) {
        /* if reg is 0, it is an actual read from the register */
        data = cmdbus.data;
    } else {
        /* otherwise it is a bus change; physically it is pulled up */
        data = 0xff;
    }

    return data;
}

/* Port B is for CMD Parallel bus (output only):
   PB7 is /PREADY
   PB6 is /PCLK
   PB5 is /PATN
   PB4 is ACTIVE LED (1=on)
   PB3 is ERROR LED (1=on)
   PB2 is SWAP9 button control
   PB1 is SWAP8 button control
   PB0 is system mode (1=128, 0=64)
*/
static void set_pb(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
    int new, old;

    ramlink_sync_cpus();

    /* see if bit 0 toggles */
    if ((rl_i8255a_o[1] ^ byte) & 0x01) {
        /* use older value to avoid inversion */
        c128ramlink_switch_mode(rl_i8255a_o[1] & 0x01);
    }

    /* check for patn change */
    old = rl_i8255a_o[1] & 0x20 ? 0 : 1;
    new = byte & 0x20 ? 0 : 1;

    rl_i8255a_o[1] = byte;
    cmdbus.cpu_bus = ( rl_i8255a_o[1] & 0xe0 ) | 0x1f;
    cmdbus_patn_changed(new, old);

    cmdbus_update();
}

static uint8_t get_pb(struct _i8255a_state *ctx, int8_t reg)
{
    uint8_t data = 0xff;

    data = rl_i8255a_i[1];

    return data;
}

/* Port C is for CMD Parallel bus, and ROM addressing:
   PC7 is PREADY (input)
   PC6 is PCLK (input)
   PC5 is SWAP9 control (input)
   PC4 is SWAP8 control (input)
   PC3 is ??? (output)
   PC2 is ROM Bank (A2) (output) - not used
   PC1 is ROM Bank (A1) (output)
   PC0 is ROM Bank (A0) (output)
*/
static void set_pc(struct _i8255a_state *ctx, uint8_t byte, int8_t reg)
{
    rl_i8255a_o[2] = byte;

    rl_rombase = ( rl_i8255a_o[2] & 3 ) * 0x4000;
    cart_port_config_changed_slot0();
}

/* FIXME: SWAP8 and SWAP9 are ignored for now */
static uint8_t get_pc(struct _i8255a_state *ctx, int8_t reg)
{
    uint8_t data = 0xff;

    ramlink_sync_cpus();
    data = ((cmdbus.bus ^ 0xff) & 0xc0) | (rl_i8255a_i[2] & 0x3f);
    return data;
}

static uint8_t ramlink_io1_read(uint16_t addr)
{
    uint8_t val = 0;

    if (rl_io1mode == 0) {
        val = rl_ram[rl_rambase | (addr & 0xff)];
        IDBG((LOG, "RAMLINK: io1 r ram[%04x] = %02x at 0x%04x", rl_rambase |
            (addr & 0xff), val, reg_pc));
        return val;
    } else if (rl_io1mode == 1) {
        val = rl_card[rl_cardbase | (addr & 0xff)];
        IDBG((LOG, "RAMLINK: io1 r card[%06x] = %02x at 0x%04x", rl_cardbase |
            (addr & 0xff), val, reg_pc));
        return val;
    }

    IDBG((LOG, "RAMLINK: unhandled io1 r %04x at 0x%04x", addr, reg_pc));

    return 255;
}

static uint8_t ramlink_io2_40_43_read(uint16_t addr)
{
    uint8_t val = 0;

    IDBG((LOG, "--------------------"));

    val = i8255a_read(&rl_i8255a, addr & 3);

    IDBG((LOG, "RAMLINK: io2 r %04x = %02x at 0x%04x", addr, val, reg_pc));

    return val;
}


static uint8_t ramlink_io2_b0_bf_read(uint16_t addr)
{
    uint8_t val = 0;

    IDBG((LOG, "--------------------"));

    if (rl_rtc) {
        val = rtc72421_read(rl_rtc, addr & 15);
    }

    IDBG((LOG, "RAMLINK: io2 r %04x = %02x at 0x%04x", addr, val, reg_pc));

    return val;
}

static uint8_t ramlink_io1_peek(uint16_t addr)
{
    return 0;
}

static uint8_t ramlink_io2_peek(uint16_t addr)
{
    uint8_t val = 0;

    IDBG((LOG, "RAMLINK: io2 p %04x = %02x at 0x%04x", addr, val, reg_pc));

    return val;
}

static void ramlink_io1_store(uint16_t addr, uint8_t value)
{
#ifdef RLDEBUGIO
    uint8_t old_val;
#endif
    if (rl_io1mode == 0) {
#ifdef RLDEBUGIO
        old_val = rl_ram[rl_rambase | (addr & 0xff)];
#endif
        rl_ram[rl_rambase | (addr & 0xff)] = value;
        IDBG((LOG, "RAMLINK: io1 w ram[%04x] < %02x (%02x) at 0x%04x", rl_rambase |
            (addr & 0xff), value, old_val, reg_pc));
        return;
    } else if (rl_io1mode == 1) {
#ifdef RLDEBUGIO
        old_val = rl_card[rl_cardbase | (addr & 0xff)];
#endif
        rl_card[rl_cardbase | (addr & 0xff)] = value;
        IDBG((LOG, "RAMLINK: io1 w card[%06x] < %02x (%02x) at 0x%04x", rl_cardbase |
            (addr & 0xff), value, old_val, reg_pc));
        return;
    }

    IDBG((LOG, "RAMLINK: unhandled io1 w %04x (%02x) at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_20_22_store(uint16_t addr, uint8_t value)
{

    IDBG((LOG, "--------------------"));

    switch (addr & 0x03) {
        case 0:
            rl_reu_trap = 0;
            break;
        case 1:
            if (rl_reu_trap && (ramlink_devices_io2_reu >= 0)) {
                ramlink_devices_io2_entry[ramlink_devices_io2_reu].store(1, value);
            }
            break;
        case 2:
            rl_reu_trap = 1;
            break;
    }

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_40_43_store(uint16_t addr, uint8_t value)
{

    IDBG((LOG, "--------------------"));

    i8255a_store(&rl_i8255a, addr & 3, value);

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_60_60_store(uint16_t addr, uint8_t value)
{
    rl_dos = 1;
    cart_port_config_changed_slot0();

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_70_70_store(uint16_t addr, uint8_t value)
{
    rl_dos = 0;
    cart_port_config_changed_slot0();

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_7e_7f_store(uint16_t addr, uint8_t value)
{
    if (rl_on && (addr == 0x7f)) {
        ramlink_off();
    } else if (addr == 0x7e) {
/*
        if (!rl_scanned) {
            ramlink_scan_io();
        }
*/
        ramlink_on();
    }

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_80_9f_store(uint16_t addr, uint8_t value)
{
    rl_rambase = (addr & 0x1f) << 8;

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_a0_a3_store(uint16_t addr, uint8_t value)
{
    switch (addr & 3) {
        case 0:
            rl_cardbase = ((rl_cardbase & 0xff0000) | (value << 8)) & rl_cardmask;
            break;
        case 1:
            rl_cardbase = ((rl_cardbase & 0x00ff00) | (value << 16)) & rl_cardmask;
            break;
    }

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_b0_bf_store(uint16_t addr, uint8_t value)
{
    IDBG((LOG, "--------------------"));

    if (rl_rtc) {
        rtc72421_write(rl_rtc, addr & 15, value);
    }

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

static void ramlink_io2_c0_c3_store(uint16_t addr, uint8_t value)
{
    rl_io1mode = addr & 0x3;
    if (rl_io1mode == 0) { /* internal RAM */
        ramlink_other1_off();
        ramlink_ramport_off();
        ramlink_io1_on();
    } else if ((rl_io1mode == 1) && (rl_cardsizemb != 0) && rl_card &&
        rl_enabled) { /* RAMCard */
        ramlink_other1_off();
        ramlink_ramport_off();
        ramlink_io1_on();
    } else if (rl_io1mode == 2) { /* GEORAM or RAMDRIVE */
        ramlink_other1_off();
        ramlink_io1_off();
        ramlink_ramport_on();
    } else { /* PASSTHRU */
        ramlink_ramport_off();
        ramlink_io1_off();
        ramlink_other1_on();
    }

    IDBG((LOG, "RAMLINK: io2 w %04x < %02x at 0x%04x", addr, value, reg_pc));
}

/* no dump here, it is just a window to variable memory sources */
static int ramlink_io1_dump(void)
{
    return 0;
}

static int ramlink_io2_dump(void)
{
    mon_out("IO mapped?: %s\n", rl_on ? "Yes" : "No");
    mon_out("DOS mapped?: %s\n", rl_dos ? "Yes" : "No");
    mon_out("Mode: %s\n", rl_normal ? "Normal" : "Direct");
    mon_out("RAMCard Size: %d MiB\n", rl_cardsizemb);
    mon_out("I8255A at $DF40\n");
    i8255a_dump(&rl_i8255a);

    return 0;
}

/* ---------------------------------------------------------------------*/
int c128ramlink_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit, int mem_config)
{
#if 0
/* for no-mmu testing */
    *base = 0;
    *start = 0;
    *limit = 0;
    return 1;
#endif

    /* unlike the c64 mmu_translate, here we only apply what we can and move
       on. return a 1 if we did, or 0 if we didn't apply anything. */
    if (!rl_mapped || !rl_enabled) {
        return 0;
    }

    if (addr >= 0x8000 && addr <= 0x9fff && ((mem_config & 0x0c) == 0x08)) {
        if (rl_dos) {
            *base = rl_rom + rl_rombase - 0x8000;
            *start = 0x8000;
            *limit = 0x9ffd;
            return 1;
        }
    } else if (addr >= 0xa000 && addr <= 0xbfff && ((mem_config & 0x0c) == 0x08)) {
        if (rl_dos) {
            *base = rl_rom + rl_rombase + 0x2000 - 0xa000;
            *start = 0xa000;
            *limit = 0xbffd;
            return 1;
        }
    } else if (addr >= 0xe000 && ((mem_config & 0x30) == 0x00)) {
        if (rl_on) {
            *base = rl_rom + rl_kernbase128 - 0xe000;
            *start = 0xe000;
            *limit = 0xfffd;
        /* on the 128, RL doesn't map $fd00-$ff0f to avoid dealing with
           international keyboard differences in all the ROMs */
        } else if (addr < 0xfd00) {
            *base = rl_rom + rl_kernbase128 + 0x2000 - 0xe000;
            *start = 0xe000;
            *limit = 0xfcfd;
        } else if (addr >= 0xff10) {
            *base = rl_rom + rl_kernbase128 + 0x2000 - 0xe000;
            *start = 0xff10;
            *limit = 0xfffd;
        } else {
            return 0;
        }
        return 1;
    } else if (addr >= 0xe000 && ((mem_config & 0x30) == 0x20)) {
        if (rl_on) {
    /* switched kernal */
    /* for $e000-$ffff, ramlink exposes the switched kernal, but there are a couple holes:
       $ff05-$ff0f, $fff0-$ffff */
            *base = rl_rom + rl_kernbase128 - 0xe000;
            if (addr < 0xff00) {
                *start = 0xe000;
                *limit = 0xff00 - 3;
            } else if (addr >= 0xff10 && addr < 0xfff0 ) {
                *start = 0xff10;
                *limit = 0xfff0 - 3;
            } else {
                return 0;
            }
        } else {
    /* main kernal */
    /* for $e000-$ffff, ramlink exposes the main kernal, but there are a few holes:
       $eb00-$ecff, $f7a0-$f7af, $fd00-$feff, $ff05-$ff0f, $ff50-$ff5f, $fff0-$ffff */
            *base = rl_rom + rl_kernbase128 + 0x2000 - 0xe000;
            if        (addr >= 0xe000 && addr < 0xeb00) {
                *start = 0xe000;
                *limit = 0xeb00 - 3;
            } else if (addr >= 0xed00 && addr < 0xf7a0) {
                *start = 0xed00;
                *limit = 0xf7a0 - 3;
            } else if (addr >= 0xf7b0 && addr < 0xfd00) {
                *start = 0xf7b0;
                *limit = 0xfd00 - 3;
            } else if (addr >= 0xff10 && addr < 0xff50) {
                *start = 0xff10;
                *limit = 0xff50 - 3;
            } else if (addr >= 0xff60 && addr < 0xfff0) {
                *start = 0xff60;
                *limit = 0xfff0 - 3;
            } else {
                return 0;
            }
        }
        return 1;
    }

    return 0;
}

uint8_t c128ramlink_hi_read(uint16_t addr, uint8_t *value)
{
    /* check IO if not done already */
    if (!rl_scanned) {
        ramlink_scan_io();
        ramlink_off();
    }

    if (rl_mapped) {
        /* other wise pull from one of the ROMS */
        if (!rl_enabled) {
            return 0;
        } else if (rl_on) {
            *value = rl_rom[rl_kernbase128 | (addr & 0x1fff)];
        /* on the 128, RL doesn't map $fd00-$ff0f to avoid dealing with
           international keyboard differences in all the ROMs */
        } else if (addr < 0xfd00 || addr >= 0xff10) {
            *value = rl_rom[rl_kernbase128 | 0x2000 | (addr & 0x1fff)];
        } else {
            return 0;
        }
        MDBG((LOG, "RAMLINK: c128ramlink_hi_read %04x = %02x", (int)addr, (int)*value));
        return 1;
    }

    return 0;
}

uint8_t c128ramlink_roml_read(uint16_t addr, uint8_t *value)
{
    /* check IO if not done already */
    if (!rl_scanned) {
        ramlink_scan_io();
        ramlink_off();
    }

    /* only expose ramlink banks if function rom is enabled */
    if (addr >= 0x8000 && addr <= 0xbfff && rl_mapped && rl_dos && rl_enabled) {
        *value = rl_rom[rl_rombase | (addr & 0x3fff)];
        MDBG((LOG, "RAMLINK: c128ramlink_roml_read %04x = %02x", addr, *value));
        return 1;
    }

    if (rl_mapped && rl_enabled && addr >= 0xe000) {
        if (rl_on) {
    /* switched kernal */
    /* for $e000-$ffff, ramlink exposes the switched kernal, but there are a couple holes:
       $ff05-$ff0f, $fff0-$ffff */
            if ((addr >= 0xff00 && addr <= 0xff0f ) ||
                (addr >= 0xfff0 )) {
                return 0;
            }
            *value = rl_rom[rl_kernbase128 | (addr & 0x1fff)];
            return 1;
        } else {
    /* main kernal */
    /* for $e000-$ffff, ramlink exposes the main kernal, but there are a few holes:
       $eb00-$ecff, $f7a0-$f7af, $fd00-$feff, $ff05-$ff0f, $ff50-$ff5f, $fff0-$ffff */
            if ((addr >= 0xeb00 && addr <= 0xecff ) ||
                (addr >= 0xf7a0 && addr <= 0xf7af ) ||
                (addr >= 0xfd00 && addr <= 0xfeff ) ||
                (addr >= 0xff00 && addr <= 0xff0f ) ||
                (addr >= 0xff50 && addr <= 0xff5f ) ||
                (addr >= 0xfff0 )) {
                return 0;
            }
            *value = rl_rom[rl_kernbase128 | 0x2000 | (addr & 0x1fff)];
            return 1;
        }
    }

    return 0;
}

void c128ramlink_switch_mode(int mode)
{
    LOG2((LOG, "RAMLINK: switch mode %d", mode));

    if ( mode ) {
        /* reconfigure for c64 mode */
        cart_config_changed_slot0(CMODE_RAM, CMODE_ULTIMAX, CMODE_READ);
    } else {
        /* reconfigure for c128 mode */
        cart_config_changed_slot0(CMODE_RAM, CMODE_RAM, CMODE_READ);
    }
}

/* read 8000-9fff */
int ramlink_roml_read(uint16_t addr, uint8_t *value)
{
    /* check IO if not done already */
    if (!rl_scanned) {
        ramlink_scan_io();
        ramlink_off();
    }

    /* do not map this for super cpu */
    if (machine_class == VICE_MACHINE_SCPU64) {
        return CART_READ_THROUGH;
    }

    if (rl_mapped && rl_dos && rl_enabled) {
        *value = rl_rom[rl_rombase | (addr & 0x1fff)];
        MDBG((LOG, "RAMLINK: roml_read %04x = %02x", addr, *value));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

/* read e000-efff */
int ramlink_romh_read(uint16_t addr, uint8_t *value)
{
    /* check IO if not done already */
    if (!rl_scanned) {
        ramlink_scan_io();
        ramlink_off();
    }

    /* It seems that rl_on has higher priority over $1, but NOT on addresses
       0xff00 - 0xff10 and 0xfff0 - 0xffff. */
    if (rl_mapped) {
        int p = (pport.dir & pport.data) | (~pport.dir & 7);
        /* other wise pull from one of the ROMS */
        if (!rl_enabled) {
            return CART_READ_THROUGH;
        } else if (rl_on && (addr < 0xff00 || (addr >= 0xff10 && addr < 0xfff0))) {
            *value = rl_rom[rl_kernbase64 | (addr & 0x1fff)];
        } else if (p & 2) {
            *value = rl_rom[rl_kernbase64 | 0x2000 | (addr & 0x1fff)];
        } else {
            return CART_READ_THROUGH;
        }
        MDBG((LOG, "RAMLINK: romh_read %04x = %02x pport=%02x",
            (unsigned int)addr, (unsigned int)*value, (unsigned int)(~pport.dir | pport.data)));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

/* read a000-bfff */
int ramlink_a000_bfff_read(uint16_t addr, uint8_t *value)
{
    /* check IO if not done already */
    if (!rl_scanned) {
        ramlink_scan_io();
        ramlink_off();
    }

    /* do not map this for super cpu */
    if (machine_class == VICE_MACHINE_SCPU64) {
        return CART_READ_THROUGH;
    }

    if (rl_mapped && rl_dos && rl_enabled) {
        *value = rl_rom[rl_rombase | 0x2000 | (addr & 0x1fff)];
        MDBG((LOG, "RAMLINK: rom_a000_read %04x = %02x",
            (int)addr, (int)*value));
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

int ramlink_peek_mem(uint16_t addr, uint8_t *value)
{
    if (machine_class == VICE_MACHINE_SCPU64) {
        return CART_READ_THROUGH;
    }

    if (!rl_mapped || !rl_enabled) {
        return CART_READ_THROUGH;
    }

    if (addr >= 0x8000 && addr <= 0x9fff) {
        if (rl_dos) {
            *value = rl_rom[rl_rombase | (addr & 0x3fff)];
            return CART_READ_VALID;
        }
    } else if (addr >= 0xa000 && addr <= 0xbfff) {
        if (rl_dos) {
            *value = rl_rom[rl_rombase | (addr & 0x3fff)];
            return CART_READ_VALID;
        }
    /* It seems that rl_on has higher priority over $1 */
    } else if (addr >= 0xe000) {
        if (rl_on) {
            *value = rl_rom[rl_kernbase64 | (addr & 0x1fff)];
        } else if ((~pport.dir | pport.data) & 2) {
            *value = rl_rom[rl_kernbase64 | 0x2000 | (addr & 0x1fff)];
        } else {
            return CART_READ_THROUGH;
        }
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

/* ---------------------------------------------------------------------*/

int ramlink_romh_phi1_read(uint16_t addr, uint8_t *value)
{
    return CART_READ_C64MEM;
}

int ramlink_romh_phi2_read(uint16_t addr, uint8_t *value)
{
    return ramlink_romh_phi1_read(addr, value);
}

/* ---------------------------------------------------------------------*/

int ramlink_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
/*
    LOG2((LOG, "RAMLINK: mmu translate: addr=%04x (mapped=%d,on=%d,dos=%d,rombase=%04x)", addr,rl_mapped,rl_on,rl_dos,rl_rombase));
*/

    if (!rl_mapped || !rl_enabled) {
        return CART_READ_THROUGH;
    }

    if (addr >= 0x8000 && addr <= 0x9fff) {
        if (rl_dos) {
            *base = rl_rom + rl_rombase - 0x8000;
            *start = 0x8000;
            *limit = 0xa000 - 3;
            return CART_READ_VALID;
        }
    } else if (addr >= 0xa000 && addr <= 0xbfff) {
        if (rl_dos) {
            *base = rl_rom + rl_rombase + 0x2000 - 0xa000;
            *start = 0xa000;
            *limit = 0xc000 - 3;
            return CART_READ_VALID;
        }
    /* It seems that rl_on has higher priority over $1, but NOT on addresses
       0xff00 - 0xff10 and 0xfff0 - 0xffff. */
    } else if (addr >= 0xe000) {
        int p = (pport.dir & pport.data) | (~pport.dir & 7);
        if (rl_on && (addr < 0xff00 || (addr >= 0xff10 && addr < 0xfff0))) {
            *base = rl_rom + rl_kernbase64 - 0xe000;
            if (addr < 0xff00) {
                *start = 0xe000;
                *limit = 0xff00 - 3;
            } else {
                *start = 0xff10;
                *limit = 0xfff0 - 3;
            }
        } else if (p & 2) {
            *base = rl_rom + rl_kernbase64 + 0x2000 - 0xe000;
            *start = 0xe000;
            *limit = 0x10000 - 3;
        } else {
            return CART_READ_THROUGH;
        }
        return CART_READ_VALID;
    }

    return CART_READ_THROUGH;
}

void ramlink_passthrough_changed(export_t *ex)
{
    rl_extexrom = ex->exrom;
    rl_extgame = ex->game;

    if (!rl_extexrom && rl_extgame) {
        /* make sure passthough carts with ultimax get priority */
        rl_mapped = 0;
    } else {
        /* everything else stays in ultimax mode and we handle it later */
        rl_mapped = 1;
        cart_set_port_exrom_slot0(0);
        cart_set_port_game_slot0(1);
    }
    cart_port_config_changed_slot0();
}

/* used by c64cartmem.c to determine the original intended mode */
int ramlink_cart_mode(void)
{
    return ( rl_extgame << 4 ) | ( rl_extexrom << 3 ) |
       ( ( ~pport.dir | pport.data ) & 7 );
}

void ramlink_config_init(export_t *ex)
{
    int32_t i;
    LOG2((LOG, "RAMLINK: config init"));

    rl_extexrom = ex->exrom;
    rl_extgame = ex->game;

    /* set default cart mode depending on machine type */
    if ( machine_class == VICE_MACHINE_C128 ) {
        c128ramlink_switch_mode(0);
    } else {
        /* everything else */
        c128ramlink_switch_mode(1);
    }

    for (i = 0; i < 0x2000; i++) {
        rl_ram[i] = (i >> 8);
    }

    /* "pull-ups" */
    rl_i8255a_i[0] = 0xff;
    rl_i8255a_i[1] = 0xff;
    rl_i8255a_i[2] = 0xff;

    /* reset "previous" values */
    rl_i8255a_o[0] = 0xff;
    rl_i8255a_o[1] = 0xff;
    rl_i8255a_o[2] = 0xff;

    /* setup I8255A */
    rl_i8255a.set_pa = set_pa;
    rl_i8255a.set_pb = set_pb;
    rl_i8255a.set_pc = set_pc;
    rl_i8255a.get_pa = get_pa;
    rl_i8255a.get_pb = get_pb;
    rl_i8255a.get_pc = get_pc;

    i8255a_reset(&rl_i8255a);

    /* make sure the parallel bus is updated */
    cmdbus_update();
}

void ramlink_config_setup(uint8_t *rawcart)
{
    LOG2((LOG, "RAMLINK: config setup"));

    /* copy supplied ROM image to memory */
    memcpy(rl_rom, rawcart, 0x10000);

    /* set default cart mode depending on machine type */
    if ( machine_class == VICE_MACHINE_C128 ) {
        c128ramlink_switch_mode(0);
    } else {
        /* everything else */
        c128ramlink_switch_mode(1);
    }
}

/* ---------------------------------------------------------------------*/

static int ramlink_common_attach(void)
{
    LOG2((LOG, "RAMLINK: common attach"));

    if (ramlink_registerio() < 0) {
        return -1;
    }
    set_enabled(1, NULL);
    return 0;
}

int ramlink_crt_attach(FILE *fd, uint8_t *rawcart, const char *filename)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 8; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            break;
        }

        if (chip.bank > 7 || chip.size != 0x2000) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 13, &chip, fd)) {
            return -1;
        }
    }
    set_bios_filename(filename, NULL); /* set the resource */
    return ramlink_common_attach();
}

int ramlink_bin_attach(const char *filename, uint8_t *rawcart)
{
    LOG2((LOG, "RAMLINK: bin attach"));

    /* just load the full 64 KiB */
    if (util_file_load(filename, rawcart, 0x10000,
        UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
        return -1;
    }

    set_bios_filename(filename, NULL); /* set the resource */
    return ramlink_common_attach();
}

void ramlink_detach(void)
{
    LOG2((LOG, "RAMLINK: detach"));

    set_enabled(0, NULL);
}

int ramlink_enable(void)
{
    return set_enabled(1, (void*)1);
}

int ramlink_disable(void)
{
    return set_enabled(0, (void*)1);
}

/* ---------------------------------------------------------------------*/

/* CARTRAMLINK snapshot module format:

   type   | name           | description
   -------------------------------------------------
   INT32  | cardsizemb     | rl_cardsizemb
   UINT32 | rombase        | rl_rombase
   UINT32 | kernbase       | rl_kernbase
   UINT32 | rambase        | rl_rambase
   UINT32 | cardbase       | rl_cardbase
   UINT32 | io1mode        | rl_io1mode
   UINT32 | reu_trap       | rl_reu_trap
   BYTE   | on             | rl_on
   BYTE   | dos            | rl_dos
   ARRAY  | i8255a_i       | 3 internal inputs for I8255A
   ARRAY  | i8255a_o       | 3 internal outputs for I8255A
   ARRAY  | rom            | rl_rom 64 KiB for firmware
   ARRAY  | ram            | rl_ram 8 KiB for SRAM
   I8255A | SNAPSHOTI8255A | rl_8255
   ARRAY  | rl_card        | Contents of RAMCARD (cardsibemb) in MiB
*/

static const char snap_module_name[] = "CARTRAMLINK";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ramlink_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    LOG2((LOG, "RAMLINK: snapshot_write"));

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || (SMW_DW(m, rl_cardsizemb) < 0)
        || (SMW_DW(m, rl_rombase) < 0)
        || (SMW_DW(m, rl_kernbase) < 0)
        || (SMW_DW(m, rl_rambase) < 0)
        || (SMW_DW(m, rl_cardbase) < 0)
        || (SMW_DW(m, rl_io1mode) < 0)
        || (SMW_DW(m, rl_reu_trap) < 0)
        || (SMW_B(m, rl_on) < 0)
        || (SMW_B(m, rl_dos) < 0)
        || (SMW_BA(m, rl_i8255a_i, 3) < 0)
        || (SMW_BA(m, rl_i8255a_o, 3) < 0)
        || (SMW_BA(m, rl_rom, 0x10000) < 0)
        || (SMW_BA(m, rl_ram, 0x2000) < 0)) {
        goto fail;
    }

    if (i8255a_snapshot_write_data(&rl_i8255a, m) < 0) {
        goto fail;
    }

    if (SMW_BA(m, rl_card, rl_cardsize) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}

int ramlink_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    uint32_t size;

    LOG2((LOG, "RAMLINK: snapshot_read"));

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || (SMR_DW(m, &size) < 0)
        || (SMR_DW(m, &rl_rombase) < 0)
        || (SMR_DW(m, &rl_kernbase) < 0)
        || (SMR_DW(m, &rl_rambase) < 0)
        || (SMR_DW(m, &rl_cardbase) < 0)
        || (SMR_DW(m, &rl_io1mode) < 0)
        || (SMR_DW(m, &rl_reu_trap) < 0)
        || (SMR_B(m, &rl_on) < 0)
        || (SMR_B(m, &rl_dos) < 0)
        || (SMR_BA(m, rl_i8255a_i, 3) < 0)
        || (SMR_BA(m, rl_i8255a_o, 3) < 0)
        || (SMR_BA(m, rl_rom, 0x10000) < 0)
        || (SMR_BA(m, rl_ram, 0x2000) < 0)) {
        goto fail;
    }

    if (i8255a_snapshot_read_data(&rl_i8255a, m) < 0) {
        goto fail;
    }

    set_enabled(1, NULL);

    set_size(size, NULL);

    if (SMR_BA(m, rl_card, rl_cardsize) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
