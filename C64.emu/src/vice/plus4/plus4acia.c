/*
 * plus4acia.c - Definitions for a 6551 ACIA interface
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Groepaz <groepaz@gmx.net>
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

#define myacia acia

/* resource defaults */
#define MYACIA   "Acia1"
#define MyDevice 0
#define MyIrq    IK_IRQ

#define myaciadev acia1dev

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

#define ACIA_MODE_HIGHEST   ACIA_MODE_NORMAL

#include "aciacore.c"

/* Flag: Do we enable the ACIA RS232 interface emulation?  */
static int _acia_enabled = 0;

/* ------------------------------------------------------------------------- */

int acia_enabled(void)
{
    return _acia_enabled;
}

static int acia_enable(void)
{
    /* FIXME: register i/o device */
    return 0;
}

static void acia_disable(void)
{
    /* FIXME: unregister i/o device */
}

static int set_acia_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((val) && (!_acia_enabled)) {
        if (acia_enable() < 0) {
            return -1;
        }
        _acia_enabled = 1;
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
    { NULL }
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
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_ACIA_EMU,
      NULL, NULL },
    { "+acia", SET_RESOURCE, 0,
      NULL, NULL, "Acia1Enable", (void *)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_ACIA_EMU,
      NULL, NULL },
    { NULL }
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
