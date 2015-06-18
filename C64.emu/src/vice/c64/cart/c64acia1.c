/*
 * c64acia1.c - Definitions for a 6551 ACIA interface
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
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

#define mycpu           maincpu
#define myclk           maincpu_clk
#define mycpu_rmw_flag  maincpu_rmw_flag
#define mycpu_clk_guard maincpu_clk_guard

#define myacia acia1

/* resource defaults */
#define MYACIA          "Acia1"
#define MyDevice        0
#define MyIrq           IK_IRQ

#define myaciadev       acia1dev

#define myacia_init acia1_init
#define myacia_init_cmdline_options acia1_cmdline_options_init
#define myacia_init_resources acia1_resources_init
#define myacia_snapshot_read_module acia1_snapshot_read_module
#define myacia_snapshot_write_module acia1_snapshot_write_module
#define myacia_peek acia1_peek
#define myacia_reset acia1_reset
#define myacia_store acia1_store

extern int acia1_set_mode(int mode);

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#define myacia_set_mode(x) acia1_set_mode(x)
#else
#define myacia_set_mode(x) 0
#endif

#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"

#define mycpu_alarm_context maincpu_alarm_context
#define mycpu_set_irq maincpu_set_irq
#define mycpu_set_nmi maincpu_set_nmi
#define mycpu_set_int_noclk maincpu_set_int

#include "acia.h"

#define ACIA_MODE_HIGHEST   ACIA_MODE_TURBO232

#include "aciacore.c"

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
/* Flag: Do we enable the ACIA RS232 interface emulation?  */
static int acia_enabled = 0;

/* Base address of the ACIA RS232 interface */
static int acia_base = 0xde00;

static char *acia_base_list = NULL;

#endif

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
/* a prototype is needed */
static BYTE aciacart_read(WORD addr);
static BYTE aciacart_peek(WORD addr);

static io_source_t acia_device = {
    CARTRIDGE_NAME_ACIA,
    IO_DETACH_RESOURCE,
    "Acia1Enable",
    0xde00, 0xde07, 0x07,
    0,
    acia1_store,
    aciacart_read,
    aciacart_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_ACIA,
    0,
    0
};

static io_source_list_t *acia_list_item = NULL;

static const c64export_resource_t export_res = {
    CARTRIDGE_NAME_TURBO232, 0, 0, &acia_device, NULL, CARTRIDGE_TURBO232
};
#endif

/* ------------------------------------------------------------------------- */

int aciacart_cart_enabled(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    return acia_enabled;
#else
    return 0;
#endif
}

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static int acia1_enable(void)
{
    if (c64export_add(&export_res) < 0) {
        return -1;
    }
    acia_list_item = io_source_register(&acia_device);
    return 0;
}

static void acia1_disable(void)
{
    c64export_remove(&export_res);
    io_source_unregister(acia_list_item);
    acia_list_item = NULL;
}

static int set_io_source_base(int address)
{
    int adr = address;

    if (adr == 0xffff) {
        switch (machine_class) {
            case VICE_MACHINE_VIC20:
                adr = 0x9800;
                break;
            default:
                adr = 0xde00;
                break;
        }
    }

    switch (adr) {
        case 0xde00:
        case 0xdf00:
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
        case 0xd700:
            if (machine_class != VICE_MACHINE_C128) {
                return -1;
            }
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
        case 0x9800:
        case 0x9c00:
            if (machine_class != VICE_MACHINE_VIC20) {
                return -1;
            }
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
    }
    return -1;
}

static int set_acia_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((val) && (!acia_enabled)) {
        if (acia1_enable() < 0) {
            return -1;
        }
        acia_enabled = 1;
    } else if ((!val) && (acia_enabled)) {
        acia1_disable();
        acia_enabled = 0;
    }
    return 0;
}

static int set_acia_base(int val, void *param)
{
    int temp;

    if (acia_enabled) {
        set_acia_enabled(0, NULL);
        temp = set_io_source_base(val);
        set_acia_enabled(1, NULL);
    } else {
        temp = set_io_source_base(val);
    }
    return temp;
}

static void set_io_source_mode(int mode)
{
    switch (mode) {
        default:
        case ACIA_MODE_NORMAL:
            acia_device.name = CARTRIDGE_NAME_ACIA;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 3;
            acia_device.address_mask = 3;
            acia_device.cart_id = CARTRIDGE_TURBO232;
            break;
        case ACIA_MODE_SWIFTLINK:
            acia_device.name = CARTRIDGE_NAME_SWIFTLINK;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 3;
            acia_device.address_mask = 3;
            acia_device.cart_id = CARTRIDGE_SWIFTLINK;
            break;
        case ACIA_MODE_TURBO232:
            acia_device.name = CARTRIDGE_NAME_TURBO232;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 7;
            acia_device.address_mask = 7;
            acia_device.cart_id = CARTRIDGE_TURBO232;
            break;
    }
}

int acia1_set_mode(int mode)
{
    if (acia_enabled) {
        set_acia_enabled(0, NULL);
        set_io_source_mode(mode);
        set_acia_enabled(1, NULL);
    } else {
        set_io_source_mode(mode);
    }
    return 1;
}
#endif

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static const resource_int_t resources_i[] = {
    { "Acia1Enable", 0, RES_EVENT_STRICT, 0,
      &acia_enabled, set_acia_enabled, NULL },
    { "Acia1Irq", MyIrq, RES_EVENT_NO, NULL,
      &acia.irq_res, acia_set_irq, NULL },
    { "Acia1Mode", ACIA_MODE_NORMAL, RES_EVENT_NO, NULL,
      &acia.mode, acia_set_mode, NULL },
    { "Acia1Base", 0xffff, RES_EVENT_STRICT, int_to_void_ptr(0xffff),
      &acia_base, set_acia_base, NULL },
    { NULL }
};
#endif

int aciacart_resources_init(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia1_resources_init() < 0) {
        return -1;
    }
    return resources_register_int(resources_i);
#else
    return 0;
#endif
}

void aciacart_resources_shutdown(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia_base_list) {
        lib_free(acia_base_list);
    }
#endif
}

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static BYTE aciacart_read(WORD addr)
{
    acia_device.io_source_valid = 0;
    if (acia.mode == ACIA_MODE_TURBO232 && (addr & 7 ) > 3 && (addr & 7) != 7) {
        return 0;
    }
    acia_device.io_source_valid = 1;
    return myacia_read(addr);
}

static BYTE aciacart_peek(WORD addr)
{
    if (acia.mode == ACIA_MODE_TURBO232 && (addr & 7 ) > 3 && (addr & 7) != 7) {
        return 0;
    }
    return acia1_peek(addr);
}
#endif

void aciacart_reset(void)
{
    acia1_reset();
}

void aciacart_init(void)
{
    acia1_init();
}

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static const cmdline_option_t cart_cmdline_options[] =
{
    { "-acia1irq", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Irq", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_IRQ, IDCLS_SET_ACIA_IRQ,
      NULL, NULL },
    { "-acia1mode", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Mode", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_MODE, IDCLS_SET_ACIA_MODE,
      NULL, NULL },
    { NULL }
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-acia1base", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Base", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_COMBO,
      IDCLS_P_BASE_ADDRESS, IDCLS_SET_ACIA_BASE,
      NULL, NULL },
    { NULL }
};
#endif

int aciacart_cmdline_options_init(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (machine_class == VICE_MACHINE_C128) {
        acia_base_list = lib_stralloc(". (0xD700, 0xDE00, 0xDF00)");
    } else if (machine_class == VICE_MACHINE_VIC20) {
        acia_base_list = lib_stralloc(". (0x9800, 0x9C00)");
    } else {
        acia_base_list = lib_stralloc(". (0xDE00, 0xDF00)");
    }

    base_cmdline_options[0].description = acia_base_list;

    if (cmdline_register_options(base_cmdline_options) < 0) {
          return -1;
    }

    if (cmdline_register_options(cart_cmdline_options) < 0) {
          return -1;
    }
#endif

    return acia1_cmdline_options_init();
}

/* ------------------------------------------------------------------------- */

void aciacart_detach(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    set_acia_enabled(0, NULL);
#endif
}

int aciacart_enable(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    return set_acia_enabled(1, NULL);
#else
    return 0;
#endif
}

/* ------------------------------------------------------------------------- */

int aciacart_snapshot_write_module(struct snapshot_s *p)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia1_snapshot_write_module(p) < 0) {
        return -1;
    }
#endif
    return 0;
}
int aciacart_snapshot_read_module(struct snapshot_s *p)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia1_snapshot_read_module(p) < 0) {
        acia_enabled = 0;
        return -1;
    }
    /* FIXME: Why do we need to do so???  */
    if (acia1_enable() == 0) {
        aciacart_reset();          /* Clear interrupts.  */
        acia_enabled = 1;
    }
#endif
    return 0;
}
