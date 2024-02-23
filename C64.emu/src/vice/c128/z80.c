/*
 * z80.c
 *
 * Written by
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
#include "daa.h"
#include "debug.h"
#include "interrupt.h"
#include "log.h"
#include "maincpu.h"
#include "monitor.h"
#include "types.h"
#include "z80.h"
#include "z80mem.h"
#include "z80regs.h"

/*#define DEBUG_Z80*/

#define CLK maincpu_clk

static int dma_request = 0;

static uint8_t *z80_bank_base;
static int z80_bank_limit;

static void z80core_reset(void);

void z80_trigger_dma(void)
{
    dma_request = 1;
}

void z80_reset(void)
{
    z80core_reset();
}

inline static uint8_t *z80mem_read_base(int addr)
{
    uint8_t *p = _z80mem_read_base_tab_ptr[addr >> 8];

    if (p == 0) {
        return p;
    }

    return p - (addr & 0xff00);
}

inline static int z80mem_read_limit(int addr)
{
    return z80mem_read_limit_tab_ptr[addr >> 8];
}

#define JUMP(addr)                                      \
    do {                                                \
        z80_reg_pc = (addr);                            \
        z80_bank_base = z80mem_read_base(z80_reg_pc);   \
        z80_bank_limit = z80mem_read_limit(z80_reg_pc); \
    } while (0)

#define LOAD(addr) ((uint32_t)(*_z80mem_read_tab_ptr[(addr) >> 8])((uint16_t)(addr)))

#define STORE(addr, value) (*_z80mem_write_tab_ptr[(addr) >> 8])((uint16_t)(addr), (uint8_t)(value))

/* undefine IN and OUT first for platforms that have them already defined as something else */
#undef IN
#define IN(addr) (io_read_tab[(addr) >> 8])((uint16_t)(addr))

#undef OUT
#define OUT(addr, value) (io_write_tab[(addr) >> 8])((uint16_t)(addr), (uint8_t)(value))

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
            tmp_clock++;
        } else {
            z80_half_cycle += left;
        }
    }

    return tmp_clock;
}

void z80_clock_stretch(void)
{
    CLK++;
    z80_half_cycle = 0;
}
#endif

/* ------------------------------------------------------------------------- */

#define Z80_SET_DMA_REQUEST(x) dma_request = x;
#define Z80_LOOP_COND !dma_request

#include "z80core.c"

void z80_resync_limits(void)
{
    z80_bank_base = z80mem_read_base(z80_reg_pc);
    z80_bank_limit = z80mem_read_limit(z80_reg_pc);
}

void z80_mainloop(interrupt_cpu_status_t *cpu_int_status, alarm_context_t *cpu_alarm_context)
{
    z80_maincpu_loop(cpu_int_status, cpu_alarm_context);
}
