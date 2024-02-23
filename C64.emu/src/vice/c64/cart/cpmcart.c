/*
 * cpmcart.c
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include <stdlib.h>

#include "6510core.h"
#include "alarm.h"
#include "c64cia.h"
#include "c64mem.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "cpmcart.h"
#include "daa.h"
#include "export.h"
#include "interrupt.h"
#include "log.h"
#include "maincpu.h"
#include "mem.h"
#include "monitor.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "z80regs.h"

#define CLK maincpu_clk

z80_regs_t z80_regs;

static int z80_started = 0;
static int cpmcart_enabled = 0;

static uint8_t cpmcart_wrap_read(uint16_t addr)
{
    uint32_t address = ((uint32_t)addr + 0x1000) & 0xffff;
    return mem_dma_read((uint16_t)address);
}

static void cpmcart_wrap_store(uint16_t addr, uint8_t value)
{
    uint32_t address = ((uint32_t)addr + 0x1000) & 0xffff;

    mem_dma_store((uint16_t)address, value);
}

static void cpmcart_io_store(uint16_t addr, uint8_t byte)
{
    int val = byte & 1;

    if (!z80_started && !val) {
        z80_started = 1;
    } else if (z80_started && val) {
        z80_started = 0;
    }
}

int cpmcart_cart_enabled(void)
{
    return cpmcart_enabled;
}

static int cpmcart_dump(void)
{
    mon_out("Active CPU: %s\n", z80_started ? "Z80" : "6510");
    return 0;
}

static io_source_t cpmcart_device = {
    "CP/M Cartridge",     /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "CPMCart",            /* resource to set to '0' */
    0xde00, 0xdeff, 0xff, /* range of the device, address is ignored, reg:$de00, mirrors:$de01-$deff */
    0,                    /* read is never valid, device is write only */
    cpmcart_io_store,     /* store function */
    NULL,                 /* NO poke function */
    NULL,                 /* NO read function */
    NULL,                 /* NO peek function */
    cpmcart_dump,         /* device state information dump function */
    CARTRIDGE_CPM,        /* cartridge ID */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0,                    /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE        /* NO mirroring */
};

static io_source_list_t *cpmcart_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_CPM, 0, 0, &cpmcart_device, NULL, CARTRIDGE_CPM
};

static int set_cpmcart_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (cpmcart_enabled != val) {
        if (val) {
            if (export_add(&export_res) < 0) {
                return -1;
            }
            cpmcart_list_item = io_source_register(&cpmcart_device);
            cpmcart_enabled = 1;
        } else {
            export_remove(&export_res);
            io_source_unregister(cpmcart_list_item);
            cpmcart_list_item = NULL;
            cpmcart_enabled = 0;
            z80_started = 0;
        }
    }
    return 0;
}

static const resource_int_t resources_int[] = {
    { "CPMCart", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &cpmcart_enabled, set_cpmcart_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int cpmcart_resources_init(void)
{
    return resources_register_int(resources_int);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-cpmcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPMCart", (resource_value_t)1,
      NULL, "Enable the CP/M cartridge" },
    { "+cpmcart", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPMCart", (resource_value_t)0,
      NULL, "Disable the CP/M cartridge" },
    CMDLINE_LIST_END
};

int cpmcart_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

struct cpmcart_ba_s {
    cpmcart_ba_check_callback_t *check;
    cpmcart_ba_steal_callback_t *steal;
    int *cpu_ba;
    int cpu_ba_mask;
    int enabled;
};

static struct cpmcart_ba_s cpmcart_ba = {
    NULL, NULL, NULL, 0, 0
};

void cpmcart_ba_register(cpmcart_ba_check_callback_t *ba_check,
                         cpmcart_ba_steal_callback_t *ba_steal,
                         int *ba_var, int ba_mask)
{
    cpmcart_ba.check = ba_check;
    cpmcart_ba.steal = ba_steal;
    cpmcart_ba.cpu_ba = ba_var;
    cpmcart_ba.cpu_ba_mask = ba_mask;
    cpmcart_ba.enabled = 1;
}

static void z80core_reset(void);

void cpmcart_reset(void)
{
    z80core_reset();
}

#define JUMP(addr)                                      \
    do {                                                \
        z80_reg_pc = (addr);                            \
    } while (0)

#define LOAD(addr) (cpmcart_wrap_read((uint16_t)(addr)))

#define STORE(addr, value) (cpmcart_wrap_store((uint16_t)(addr), (uint8_t)(value)))

/* undefine IN and OUT first for platforms that have them already defined as something else */
#undef IN
#define IN(addr) LOAD(addr)

#undef OUT
#define OUT(addr, value) STORE(addr, value)

#ifdef Z80_4MHZ
static int z80_half_cycle = 0;

inline static CLOCK z80cpu_clock_add(CLOCK clock, int amount)
{
    CLOCK tmp_clock = clock;
    int left = amount;

    while (left > 0) {
        if (left >= (2 - z80_half_cycle)) {
            left -= (2 - z80_half_cycle);
            z80_half_cycle = 0;
            if (cpmcart_ba.enabled) {
                tmp_clock++;
                if (cpmcart_ba.check()) {
                    cpmcart_ba.steal();
                }
            } else {
                tmp_clock++;
            }
        } else {
            z80_half_cycle += left;
        }
    }

    return tmp_clock;
}

void cpmcart_clock_stretch(void)
{
    z80_half_cycle = 0;
    if (cpmcart_ba.enabled) {
        CLK++;
        if (cpmcart_ba.check()) {
            cpmcart_ba.steal();
        }
    } else {
        CLK++;
    }
}
#endif

/* ------------------------------------------------------------------------- */

#define Z80_SET_DMA_REQUEST(x)
#define Z80_LOOP_COND z80_started


#include "z80core.c"

static void cpmcart_mainloop(interrupt_cpu_status_t *cpu_int_status, alarm_context_t *cpu_alarm_context)
{
    z80_maincpu_loop(cpu_int_status, cpu_alarm_context);
}

void cpmcart_check_and_run_z80(void)
{
    if (z80_started) {
        cpmcart_mainloop(maincpu_int_status, maincpu_alarm_context);
    }
}

/* ------------------------------------------------------------------------- */

/* CPMCART snapshot module format:

   type  | name           | description
   ------------------------------------
   DWORD | CLK            | main CPU clock
   BYTE  | A              | A register
   BYTE  | B              | B register
   BYTE  | C              | C register
   BYTE  | D              | D register
   BYTE  | E              | E register
   BYTE  | F              | F register
   BYTE  | H              | H register
   BYTE  | L              | L register
   BYTE  | IXH            | IXH register
   BYTE  | IXL            | IXL register
   BYTE  | IYH            | IYH register
   BYTE  | IYL            | IYL register
   WORD  | SP             | stack pointer register
   DWORD | PC             | program counter register
   BYTE  | I              | I register
   BYTE  | R              | R register
   BYTE  | IFF1           | IFF1 register
   BYTE  | IFF2           | IFF2 register
   BYTE  | im mode        | im mode flag
   BYTE  | A2             | A2 register
   BYTE  | B2             | B2 register
   BYTE  | C2             | C2 register
   BYTE  | D2             | D2 register
   BYTE  | E2             | E2 register
   BYTE  | F2             | F2 register
   BYTE  | H2             | H2 register
   BYTE  | L2             | L2 register
   DWORD | opcode info    | last opcode info
   DWORD | opcode address | last opcode address
 */

static const char snap_module_name[] = "CPMCART";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0

int cpmcart_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_CLOCK(m, maincpu_clk) < 0
        || SMW_B(m, reg_a) < 0
        || SMW_B(m, reg_b) < 0
        || SMW_B(m, reg_c) < 0
        || SMW_B(m, reg_d) < 0
        || SMW_B(m, reg_e) < 0
        || SMW_B(m, reg_f) < 0
        || SMW_B(m, reg_h) < 0
        || SMW_B(m, reg_l) < 0
        || SMW_B(m, reg_ixh) < 0
        || SMW_B(m, reg_ixl) < 0
        || SMW_B(m, reg_iyh) < 0
        || SMW_B(m, reg_iyl) < 0
        || SMW_W(m, reg_sp) < 0
        || SMW_DW(m, z80_reg_pc) < 0
        || SMW_B(m, reg_i) < 0
        || SMW_B(m, reg_r) < 0
        || SMW_B(m, iff1) < 0
        || SMW_B(m, iff2) < 0
        || SMW_B(m, im_mode) < 0
        || SMW_B(m, reg_a2) < 0
        || SMW_B(m, reg_b2) < 0
        || SMW_B(m, reg_c2) < 0
        || SMW_B(m, reg_d2) < 0
        || SMW_B(m, reg_e2) < 0
        || SMW_B(m, reg_f2) < 0
        || SMW_B(m, reg_h2) < 0
        || SMW_B(m, reg_l2) < 0
        || SMW_B(m, (uint8_t)z80_started) < 0
        || SMW_DW(m, (uint32_t)z80_last_opcode_info) < 0
        || SMW_DW(m, (uint32_t)z80_last_opcode_addr) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int cpmcart_snapshot_read_module(snapshot_t *s)
{
    uint8_t major, minor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, snap_module_name, &major, &minor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(major, minor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    /* FIXME: This is a mighty kludge to prevent VIC-II from stealing the
       wrong number of cycles.  */
    maincpu_rmw_flag = 0;

    if (0
        || SMR_CLOCK(m, &maincpu_clk) < 0
        || SMR_B(m, &reg_a) < 0
        || SMR_B(m, &reg_b) < 0
        || SMR_B(m, &reg_c) < 0
        || SMR_B(m, &reg_d) < 0
        || SMR_B(m, &reg_e) < 0
        || SMR_B(m, &reg_f) < 0
        || SMR_B(m, &reg_h) < 0
        || SMR_B(m, &reg_l) < 0
        || SMR_B(m, &reg_ixh) < 0
        || SMR_B(m, &reg_ixl) < 0
        || SMR_B(m, &reg_iyh) < 0
        || SMR_B(m, &reg_iyl) < 0
        || SMR_W(m, &reg_sp) < 0
        || SMR_DW(m, &z80_reg_pc) < 0
        || SMR_B(m, &reg_i) < 0
        || SMR_B(m, &reg_r) < 0
        || SMR_B(m, &iff1) < 0
        || SMR_B(m, &iff2) < 0
        || SMR_B(m, &im_mode) < 0
        || SMR_B(m, &reg_a2) < 0
        || SMR_B(m, &reg_b2) < 0
        || SMR_B(m, &reg_c2) < 0
        || SMR_B(m, &reg_d2) < 0
        || SMR_B(m, &reg_e2) < 0
        || SMR_B(m, &reg_f2) < 0
        || SMR_B(m, &reg_h2) < 0
        || SMR_B(m, &reg_l2) < 0
        || SMR_B_INT(m, &z80_started) < 0
        || SMR_DW_UINT(m, &z80_last_opcode_info) < 0
        || SMR_DW_UINT(m, &z80_last_opcode_addr) < 0) {
        goto fail;
    }

    set_cpmcart_enabled(1, NULL);

    return snapshot_module_close(m);

fail:
    snapshot_module_close(m);
    return -1;
}
