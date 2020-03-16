/*
 * ethernetcart.c - Generic CS8900 based ethernet cartridge emulation.
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

#ifdef HAVE_RAWNET

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "cs8900.h"
#include "crc32.h"
#include "export.h"
#include "lib.h"
#include "machine.h"
#include "monitor.h"
#include "rawnet.h"
#include "resources.h"
#include "snapshot.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "cs8900io.h"
#include "ethernetcart.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    Generic ethernet cartridge emulation:

    - TFE (The Final Ethernet)
    - RRNET
    - 64NIC
    - NET64
    - FB-NET

    - for register documentation refer to the cs8900a datasheet
*/

/* ------------------------------------------------------------------------- */
/*    resources support functions                                            */

/* Some prototypes are needed */
static uint8_t ethernetcart_read(uint16_t io_address);
static uint8_t ethernetcart_peek(uint16_t io_address);
static void ethernetcart_store(uint16_t io_address, uint8_t byte);
static int ethernetcart_dump(void);

static io_source_t ethernetcart_device = {
    CARTRIDGE_NAME_ETHERNETCART, /* name of the device */
    IO_DETACH_RESOURCE,          /* use resource to detach the device when involved in a read-collision */
    "ETHERNETCART_ACTIVE",       /* resource to set to '0' */
    0xde00, 0xde0f, 0x0f,        /* range for the device, address start can be changed, range will be different for vic20 */
    0,                           /* read validity is determined by the device upon a read */
    ethernetcart_store,          /* store function */
    NULL,                        /* NO poke function */
    ethernetcart_read,           /* read function */
    ethernetcart_peek,           /* peek function */
    ethernetcart_dump,           /* device state information dump function */
    CARTRIDGE_TFE,               /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0                            /* insertion order, gets filled in by the registration function */
};

static export_resource_t export_res = {
    CARTRIDGE_NAME_ETHERNETCART, 0, 0, &ethernetcart_device, NULL, CARTRIDGE_TFE
};

/* current configurations */
static io_source_list_t *ethernetcart_list_item = NULL;

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/* Flag: Do we have the Ethernet Cart enabled?  */
static int ethernetcart_enabled = 0;

/* Base address of the Ethernet Cart */
static int ethernetcart_base = 0xde00;

/* Mode of the Ethernet Cart (0: normal [TFE], 1: RRNET) */
static int ethernetcart_mode = 0;

static char *ethernetcart_address_list = NULL;

/* ------------------------------------------------------------------------- */
/*    initialization and deinitialization functions                          */

void ethernetcart_reset(void)
{
    cs8900io_reset();
}

static int ethernetcart_activate(void)
{
    return cs8900io_enable("Ethernet Cart");
}

static int ethernetcart_deactivate(void)
{
    return cs8900io_disable();
}

void ethernetcart_init(void)
{
    cs8900io_init();
}

void ethernetcart_detach(void)
{
    cs8900io_disable();
    ethernetcart_enabled = 0;
}

/* ------------------------------------------------------------------------- */

/* ----- read byte from I/O range in VICE ----- */
static uint8_t ethernetcart_read(uint16_t io_address)
{
    ethernetcart_device.io_source_valid = 1;

    if (ethernetcart_mode == ETHERNETCART_MODE_RRNET) {
        if (io_address < 2) {
            return 0;
            ethernetcart_device.io_source_valid = 0;
        }
        io_address ^= 8;
    }
    return cs8900io_read(io_address);
}

/* ----- peek byte with no sideeffects from I/O range in VICE ----- */
static uint8_t ethernetcart_peek(uint16_t io_address)
{
    if (ethernetcart_mode == ETHERNETCART_MODE_RRNET) {
        if (io_address < 2) {
            return 0;
        }
        io_address ^= 8;
    }
    return cs8900io_peek(io_address);
}

/* ----- write byte to I/O range of VICE ----- */
static void ethernetcart_store(uint16_t io_address, uint8_t byte)
{
    if (ethernetcart_mode == ETHERNETCART_MODE_RRNET) {
        if (io_address < 2) {
            return;
        }
        io_address ^= 8;
    }
    cs8900io_store(io_address, byte);
}

static int ethernetcart_dump(void)
{
    mon_out("CS8900 mapped to $%04x ($%04x-$%04x), Mode: %s.\n",
            (unsigned int)(ethernetcart_device.start_address & ~ethernetcart_device.address_mask),
            ethernetcart_device.start_address + (ethernetcart_mode ? 2U : 0U),
            ethernetcart_device.end_address,
            ethernetcart_mode ? "RR-Net" : "TFE" );

    return cs8900io_dump();
}

int ethernetcart_cart_enabled(void)
{
    return ethernetcart_enabled;
}

static int set_ethernetcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!val) {
        /* Ethernet Cart should be deactived */
        if (ethernetcart_enabled) {
            ethernetcart_enabled = 0;
            if (ethernetcart_deactivate() < 0) {
                return -1;
            }
            io_source_unregister(ethernetcart_list_item);
            ethernetcart_list_item = NULL;
            export_remove(&export_res);
        }
    } else {
        if (!ethernetcart_enabled) {
            if (ethernetcart_activate() < 0) {
                return -1;
            }
            export_res.io1 = &ethernetcart_device;
            if (export_add(&export_res) < 0) {
                return -1;
            }
            ethernetcart_enabled = 1;
            ethernetcart_list_item = io_source_register(&ethernetcart_device);
        }
    }
    return 0;
}

static int set_ethernetcart_base(int val, void *param)
{
    int addr = val;
    int old = ethernetcart_enabled;

    if (val == ethernetcart_base) {
        return 0;
    }

    if (addr == 0xffff) {
        if (machine_class == VICE_MACHINE_VIC20) {
            addr = 0x9800;
        } else {
            addr = 0xde00;
        }
    }

    if (old) {
        set_ethernetcart_enabled(0, NULL);
    }

    switch (addr) {
        case 0xde00:
        case 0xde10:
        case 0xde20:
        case 0xde30:
        case 0xde40:
        case 0xde50:
        case 0xde60:
        case 0xde70:
        case 0xde80:
        case 0xde90:
        case 0xdea0:
        case 0xdeb0:
        case 0xdec0:
        case 0xded0:
        case 0xdee0:
        case 0xdef0:
            if (machine_class != VICE_MACHINE_VIC20) {
                ethernetcart_device.start_address = (uint16_t)addr;
                ethernetcart_device.end_address = (uint16_t)(addr + 0xf);
                export_res.io1 = &ethernetcart_device;
                export_res.io2 = NULL;
            } else {
                return -1;
            }
            break;
        case 0xdf00:
        case 0xdf10:
        case 0xdf20:
        case 0xdf30:
        case 0xdf40:
        case 0xdf50:
        case 0xdf60:
        case 0xdf70:
        case 0xdf80:
        case 0xdf90:
        case 0xdfa0:
        case 0xdfb0:
        case 0xdfc0:
        case 0xdfd0:
        case 0xdfe0:
        case 0xdff0:
            if (machine_class != VICE_MACHINE_VIC20) {
                ethernetcart_device.start_address = (uint16_t)addr;
                ethernetcart_device.end_address = (uint16_t)(addr + 0xf);
                export_res.io1 = NULL;
                export_res.io2 = &ethernetcart_device;
            } else {
                return -1;
            }
            break;
        case 0x9800:
        case 0x9810:
        case 0x9820:
        case 0x9830:
        case 0x9840:
        case 0x9850:
        case 0x9860:
        case 0x9870:
        case 0x9880:
        case 0x9890:
        case 0x98a0:
        case 0x98b0:
        case 0x98c0:
        case 0x98d0:
        case 0x98e0:
        case 0x98f0:
        case 0x9c00:
        case 0x9c10:
        case 0x9c20:
        case 0x9c30:
        case 0x9c40:
        case 0x9c50:
        case 0x9c60:
        case 0x9c70:
        case 0x9c80:
        case 0x9c90:
        case 0x9ca0:
        case 0x9cb0:
        case 0x9cc0:
        case 0x9cd0:
        case 0x9ce0:
        case 0x9cf0:
            if (machine_class == VICE_MACHINE_VIC20) {
                ethernetcart_device.start_address = (uint16_t)addr;
                ethernetcart_device.end_address = (uint16_t)(addr + 0xf);
            } else {
                return -1;
            }
            break;
        default:
            return -1;
    }
    ethernetcart_base = addr;

    if (old) {
        set_ethernetcart_enabled(1, NULL);
    }
    return 0;
}

static int set_ethernetcart_mode(int val, void *param)
{
    ethernetcart_mode = val ? 1 : 0;

    return 0;
}

int ethernetcart_enable(void)
{
    return resources_set_int("ETHERNETCART_ACTIVE", 1);
}


int ethernetcart_disable(void)
{
    return resources_set_int("ETHERNETCART_ACTIVE", 0);
}


static const resource_int_t resources_int[] = {
    { "ETHERNETCART_ACTIVE", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &ethernetcart_enabled, set_ethernetcart_enabled, NULL },
    { "ETHERNETCARTBase", 0xffff, RES_EVENT_STRICT, (resource_value_t)0,
      &ethernetcart_base, set_ethernetcart_base, NULL },
    { "ETHERNETCARTMode", ETHERNETCART_MODE_TFE, RES_EVENT_STRICT, (resource_value_t)0,
      &ethernetcart_mode, set_ethernetcart_mode, NULL },
    RESOURCE_INT_LIST_END
};

int ethernetcart_resources_init(void)
{
    if (cs8900io_resources_init() < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

void ethernetcart_resources_shutdown(void)
{
    if (ethernetcart_address_list) {
        lib_free(ethernetcart_address_list);
    }
    cs8900io_resources_shutdown();
}

/* ------------------------------------------------------------------------- */
/*    commandline support functions                                          */

static int set_tfe_enable(const char *value, void *extra_param)
{
    resources_set_int("ETHERNETCART_ACTIVE", 0);
    resources_set_int("ETHERNETCARTBase", 0xde00);
    resources_set_int("ETHERNETCARTMode", ETHERNETCART_MODE_TFE);

    return resources_set_int("ETHERNETCART_ACTIVE", 1);
}

static int set_rrnet_enable(const char *value, void *extra_param)
{
    resources_set_int("ETHERNETCART_ACTIVE", 0);
    resources_set_int("ETHERNETCARTBase", 0xde00);
    resources_set_int("ETHERNETCARTMode", ETHERNETCART_MODE_RRNET);

    return resources_set_int("ETHERNETCART_ACTIVE", 1);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-ethernetcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "ETHERNETCART_ACTIVE", (resource_value_t)1,
      NULL, "Enable the Ethernet Cartridge (TFE/RR-Net/64NIC/FB-NET)" },
    { "+ethernetcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "ETHERNETCART_ACTIVE", (resource_value_t)0,
      NULL, "Disable the Ethernet Cartridge (TFE/RR-Net/64NIC/FB-NET)" },
    { "-tfe", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_tfe_enable, NULL, NULL, NULL,
      NULL, "Enable the Ethernet Cartridge in TFE (\"The Final Ethernet\") compatible mode and set default I/O address" },
    { "-rrnet", CALL_FUNCTION, CMDLINE_ATTRIB_NONE,
      set_rrnet_enable, NULL, NULL, NULL,
      NULL, "Enable the Ethernet Cartridge in RR-Net compatible mode and set default I/O address" },
    { "-ethernetcartmode", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ETHERNETCARTMode", NULL,
      "<Mode>", "Mode of Ethernet Cartridge (0: TFE, 1: RR-Net)" },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-ethernetcartbase", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ETHERNETCARTBase", NULL,
      "<Base address>", NULL },
    CMDLINE_LIST_END
};

int ethernetcart_cmdline_options_init(void)
{
    char *temp1, *temp2;

    if (cs8900io_cmdline_options_init() < 0) {
        return -1;
    }

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        temp1 = util_gen_hex_address_list(0x9800, 0x9900, 0x10);
        temp2 = util_gen_hex_address_list(0x9c00, 0x9d00, 0x10);
        ethernetcart_address_list = util_concat("Base address of the Ethernet Cartridge. (", temp1, "/", temp2, ")", NULL);        
        lib_free(temp2);
    } else {
        temp1 = util_gen_hex_address_list(0xde00, 0xe000, 0x10);
        ethernetcart_address_list = util_concat("Base address of the Ethernet Cartridge. (", temp1, ")", NULL);
    }
    lib_free(temp1);

    base_cmdline_options[0].description = ethernetcart_address_list;

    return cmdline_register_options(base_cmdline_options);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTETHERNET"

/* FIXME: implement snapshot support */
int ethernetcart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    snapshot_set_error(SNAPSHOT_MODULE_NOT_IMPLEMENTED);
    return -1;
#if 0
    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int ethernetcart_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    uint8_t vmajor, vminor;
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

#endif /* #ifdef HAVE_RAWNET */
