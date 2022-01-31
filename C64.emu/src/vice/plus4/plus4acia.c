/*
 * plus4acia.c - Definitions for a 6551 ACIA interface
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Groepaz <groepaz@gmx.net>
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

#include "plus4acia.h"

#define mycpu           maincpu
#define myclk           maincpu_clk
#define mycpu_rmw_flag  maincpu_rmw_flag

#define myacia acia

/* resource defaults */
#define MYACIA   "Acia1"
#define MyDevice 0
#define MyIrq    IK_IRQ

#define myaciadev acia1dev

/* prototypes to make modern compilers happy */
int _acia_resources_init(void);
int _acia_cmdline_options_init(void);
int _acia_snapshot_read_module(struct snapshot_s *);
int _acia_snapshot_write_module(struct snapshot_s *);

#define myacia_init acia_init
#define myacia_init_cmdline_options _acia_cmdline_options_init
#define myacia_init_resources _acia_resources_init
#define myacia_snapshot_read_module _acia_snapshot_read_module
#define myacia_snapshot_write_module _acia_snapshot_write_module
#define myacia_peek acia_peek
#define myacia_read acia_read
#define myacia_reset acia_reset
#define myacia_store acia_store

/* no set mode */
#define myacia_set_mode(x) 0

#include "maincpu.h"

#define mycpu_alarm_context maincpu_alarm_context
#define mycpu_set_irq maincpu_set_irq
#define mycpu_set_nmi maincpu_set_nmi
#define mycpu_set_int_noclk maincpu_set_int

#include "acia.h"
#include "cartio.h"

#define ACIA_MODE_HIGHEST   ACIA_MODE_NORMAL

#include "aciacore.c"

/* Flag: Do we enable the ACIA RS232 interface emulation?  */
static int _acia_enabled = 0;

/* ------------------------------------------------------------------------- */

static io_source_t acia_device = {
    "ACIA",               /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "Acia1Enable",        /* resource to set to '0' */
    0xfd00, 0xfd0f, 0x03, /* range for the device, regs:$fd00-$fd03, mirrors:$df04-$fd0f */
    1,                    /* read is always valid */
    acia_store,           /* store function */
    NULL,                 /* NO poke function */
    acia_read,            /* read function */
    acia_peek,            /* peek function */
    acia_dump,            /* device state information dump function */
    IO_CART_ID_NONE,      /* not a cartridge */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0                     /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *acia_list_item = NULL;

int acia_enabled(void)
{
    return _acia_enabled;
}

static int acia_enable(void)
{
    if (!_acia_enabled) {
        acia_list_item = io_source_register(&acia_device);
    }
    return 0;
}

static void acia_disable(void)
{
    if (_acia_enabled) {
        io_source_unregister(acia_list_item);
        acia_list_item = NULL;
    }
}

static int set_acia_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((val) && (!_acia_enabled)) {
        if (acia_enable() < 0) {
            return -1;
        }
        _acia_enabled = 1;
        acia_reset();
    } else if ((!val) && (_acia_enabled)) {
        acia_disable();
        _acia_enabled = 0;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

static const resource_int_t resources_i[] = {
    { "Acia1Enable", 1, RES_EVENT_STRICT, NULL,
      &_acia_enabled, set_acia_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int acia_resources_init(void)
{
    if (_acia_resources_init() < 0) {
        return -1;
    }
    return resources_register_int(resources_i);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t acia_cmdline_options[] =
{
    { "-acia", SET_RESOURCE, 0,
      NULL, NULL, "Acia1Enable", (void *)1,
      NULL, "Enable the ACIA emulation" },
    { "+acia", SET_RESOURCE, 0,
      NULL, NULL, "Acia1Enable", (void *)0,
      NULL, "Disable the ACIA emulation" },
    CMDLINE_LIST_END
};

int acia_cmdline_options_init(void)
{
    if (_acia_cmdline_options_init() < 0) {
        return -1;
    }
    return cmdline_register_options(acia_cmdline_options);
}

/* ------------------------------------------------------------------------- */

int acia_snapshot_write_module(struct snapshot_s *p)
{
    if (_acia_snapshot_write_module(p) < 0) {
        return -1;
    }
    return 0;
}

int acia_snapshot_read_module(struct snapshot_s *p)
{
    if (_acia_snapshot_read_module(p) < 0) {
        _acia_enabled = 0;
        return -1;
    }
    /* FIXME: Why do we need to do so???  */
    if (acia_enable() == 0) {
        acia_reset();          /* Clear interrupts.  */
        _acia_enabled = 1;
    }
    return 0;
}
