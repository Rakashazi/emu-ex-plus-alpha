/*
 * tpicore.c - TPI 6525 template
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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

#include <stdio.h>

#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "snapshot.h"
#include "tpi.h"
#include "types.h"


#define irq_active      (tpi_context->c_tpi[TPI_AIR])
#define irq_latches     (tpi_context->c_tpi[TPI_PC])
#define irq_mask        (tpi_context->c_tpi[TPI_DDPC])
#define irq_mode        (tpi_context->c_tpi[TPI_CREG] & 1)
#define irq_priority    (tpi_context->c_tpi[TPI_CREG] & 2)

#define IS_CA_MODE()            ((tpi_context->c_tpi[TPI_CREG] & 0x20) == 0x00)
#define IS_CA_PULSE_MODE()      ((tpi_context->c_tpi[TPI_CREG] & 0x30) == 0x10)
#define IS_CA_TOGGLE_MODE()     ((tpi_context->c_tpi[TPI_CREG] & 0x30) == 0x00)
#define IS_CB_MODE()            ((tpi_context->c_tpi[TPI_CREG] & 0x80) == 0x00)
#define IS_CB_PULSE_MODE()      ((tpi_context->c_tpi[TPI_CREG] & 0xc0) == 0x40)
#define IS_CB_TOGGLE_MODE()     ((tpi_context->c_tpi[TPI_CREG] & 0xc0) == 0x00)


static const BYTE pow2[] = { 1, 2, 4, 8, 16 };

static int mytpi_debug = 0;

/*------------------------------------------------------------------------*/
/* Handle irq stack etc */

static void set_latch_bit(tpi_context_t *tpi_context, int bit)
{
    if (mytpi_debug && !(bit & irq_latches)) {
        log_message(tpi_context->log, "set_latch_bit(%02x, mask=%02x)",
                    bit, irq_mask);
    }

    irq_latches |= bit;

    if (!(irq_mask & bit)) {
        return;
    }

    /* if one IRQ is already active put on stack, if not, trigger CPU int */
    if (irq_priority) {
        if (bit > (irq_latches & ~bit)) {
            irq_active = bit;
            (tpi_context->set_int)(tpi_context->tpi_int_num,
                                   tpi_context->irq_line);
        }
    } else {
        if (!irq_active) {
            irq_active = bit;
            (tpi_context->set_int)(tpi_context->tpi_int_num,
                                   tpi_context->irq_line);
        }
    }
    tpi_context->irq_stack |= bit;
}

static void pop_irq_state(tpi_context_t *tpi_context)
{
    if (mytpi_debug) {
        log_message(tpi_context->log,
                    "pop_irq_state(latches=%02x, stack=%02x, active=%02x)",
                    (int)irq_latches, (int)(tpi_context->irq_stack),
                    (int)irq_active);
    }
    if (irq_priority) {
        if (tpi_context->irq_stack) {
            int i;
            for (i = 4; i >= 0; i--) {
                if (tpi_context->irq_stack & pow2[i]) {
                    irq_active = pow2[i];
                    break;
                }
            }
        }
    }
    (tpi_context->set_int)(tpi_context->tpi_int_num, irq_active
                           ? tpi_context->irq_line : 0);
}

static BYTE push_irq_state(tpi_context_t *tpi_context)
{
    int old_active;

    old_active = irq_active;

    if (mytpi_debug) {
        log_message(tpi_context->log,
                    "push_irq_state(latches=%02x, act=%02x, stack=%02x mask=%02x).",
                    (int)irq_latches, (int)irq_active,
                    (int)(tpi_context->irq_stack), (int)irq_mask);
    }

    irq_latches &= ~irq_active;
    tpi_context->irq_stack &= ~irq_active;
    irq_active = 0;

    if (!irq_priority) {
        irq_active = tpi_context->irq_stack;
        tpi_context->irq_stack = 0;
    }
    (tpi_context->set_int)(tpi_context->tpi_int_num, irq_active
                           ? tpi_context->irq_line : 0);
    return old_active;
}

/*------------------------------------------------------------------------*/

void tpicore_reset(tpi_context_t *tpi_context)
{
    unsigned int i;

    for (i = 0; i < 8; i++) {
        tpi_context->c_tpi[i] = 0;
    }

    irq_mask = 0;
    irq_latches = 0;
    tpi_context->irq_previous = 0xff;
    tpi_context->irq_stack = 0;
    irq_active = 0;
    (tpi_context->set_int)(tpi_context->tpi_int_num, 0);

    tpi_context->oldpa = 0xff;
    tpi_context->oldpb = 0xff;
    tpi_context->oldpc = 0xff;

    (tpi_context->set_ca)(tpi_context, 0);
    (tpi_context->set_cb)(tpi_context, 0);
    tpi_context->ca_state = 0;
    tpi_context->cb_state = 0;

    (tpi_context->reset)(tpi_context);
}

void tpicore_store(tpi_context_t *tpi_context, WORD addr, BYTE byte)
{
    if (tpi_context->rmw_flag) {
        (*(tpi_context->clk_ptr))--;
        tpi_context->rmw_flag = 0;
        tpicore_store(tpi_context, addr, tpi_context->tpi_last_read);
        (*(tpi_context->clk_ptr))++;
    }

    addr &= 0x07;

    switch (addr) {
        case TPI_PA:
        case TPI_DDPA:
            tpi_context->c_tpi[addr] = byte;
            byte = tpi_context->c_tpi[TPI_PA] | ~(tpi_context->c_tpi[TPI_DDPA]);
            (tpi_context->store_pa)(tpi_context, byte);
            tpi_context->oldpa = byte;
            return;
        case TPI_PB:
        case TPI_DDPB:
            tpi_context->c_tpi[addr] = byte;
            byte = tpi_context->c_tpi[TPI_PB] | ~(tpi_context->c_tpi[TPI_DDPB]);
            (tpi_context->store_pb)(tpi_context, byte);
            tpi_context->oldpb = byte;
            if (IS_CB_MODE()) {
                tpi_context->cb_state = 0;
                (tpi_context->set_cb)(tpi_context, 0);
                if (IS_CB_PULSE_MODE()) {
                    tpi_context->cb_state = 1;
                    (tpi_context->set_cb)(tpi_context, 1);
                }
            }
            return;
        case TPI_PC:
        case TPI_DDPC:
            tpi_context->c_tpi[addr] = byte;
            if (irq_mode) {
                if (addr == TPI_PC) {
                    irq_latches &= byte;
                } else {
                    int i;

                    for (i = 4; i >= 0; i--) {
                        if (irq_mask & irq_latches & pow2[i]) {
                            set_latch_bit(tpi_context, pow2[i]);
                        }
                    }
                }
            } else {
                byte = tpi_context->c_tpi[TPI_PC] | ~(tpi_context->c_tpi[TPI_DDPC]);
                (tpi_context->store_pc)(tpi_context, byte);
                tpi_context->oldpc = byte;
            }
            return;
        case TPI_CREG:
            tpi_context->c_tpi[addr] = byte;
            if (mytpi_debug) {
                log_message(tpi_context->log, "write %02x to CREG", byte);
            }
            if (tpi_context->c_tpi[TPI_CREG] & 0x20) {
                tpi_context->ca_state = (tpi_context->c_tpi[TPI_CREG] & 0x10);
                (tpi_context->set_ca)(tpi_context, tpi_context->ca_state);
            } else {
                if (tpi_context->c_tpi[TPI_CREG] & 0x10) {
                    tpi_context->ca_state = 1;
                    (tpi_context->set_ca)(tpi_context, 1);
                }
            }
            if (tpi_context->c_tpi[TPI_CREG] & 0x80) {
                tpi_context->cb_state = (tpi_context->c_tpi[TPI_CREG] & 0x40);
                (tpi_context->set_cb)(tpi_context, tpi_context->cb_state);
            } else {
                if (tpi_context->c_tpi[TPI_CREG] & 0x40) {
                    tpi_context->cb_state = 1;
                    (tpi_context->set_cb)(tpi_context, 1);
                }
            }
            return;
        case TPI_AIR:
            pop_irq_state(tpi_context);
            return;
    }
    tpi_context->c_tpi[addr] = byte;
}

BYTE tpicore_read(tpi_context_t *tpi_context, WORD addr)
{
    BYTE byte = 0xff;

    addr &= 0x07;

    switch (addr) {
        case TPI_PA:
            byte = (tpi_context->read_pa)(tpi_context);
            if (IS_CA_MODE()) {
                tpi_context->ca_state = 0;
                (tpi_context->set_ca)(tpi_context, 0);
                if (IS_CA_PULSE_MODE()) {
                    tpi_context->ca_state = 1;
                    (tpi_context->set_ca)(tpi_context, 1);
                }
            }
            tpi_context->tpi_last_read = byte;
            return byte;
        case TPI_PB:
            byte = (tpi_context->read_pb)(tpi_context);
            tpi_context->tpi_last_read = byte;
            return byte;
        case TPI_PC:
            if (irq_mode) {
                byte = (irq_latches & 0x1f) | (irq_active ? 0x20 : 0) | 0xc0;
            } else {
                byte = (tpi_context->read_pc)(tpi_context);
            }
            tpi_context->tpi_last_read = byte;
            return byte;
        case TPI_AIR:
            tpi_context->tpi_last_read = push_irq_state(tpi_context);
            return tpi_context->tpi_last_read;
        default:
            tpi_context->tpi_last_read = tpi_context->c_tpi[addr];
            return tpi_context->tpi_last_read;
    }
}

/* FIXME: peek into register without any side effect */
BYTE tpicore_peek(tpi_context_t *tpi_context, WORD addr)
{
    BYTE byte = 0xff;
    addr &= 0x07;

    switch (addr) {
        case TPI_PC:
            if (irq_mode) {
                byte = (irq_latches & 0x1f) | (irq_active ? 0x20 : 0) | 0xc0;
            } else {
                byte = tpi_context->c_tpi[addr];
            }
            break;
        default:
            byte = tpi_context->c_tpi[addr];
            break;
    }

    return byte;
}

/* Port C can be setup as interrupt input - this collects connected IRQ states
 * and sets IRQ if necessary
 * Beware: An IRQ line is active low, but for active irqs we here get
 * a state parameter != 0 */
void tpicore_set_int(tpi_context_t *tpi_context, int bit, int state)
{
    if (bit >= 5) {
        return;
    }

    bit = pow2[bit];

    state = !state;

    /* check low-high transition */
    if (state && !(tpi_context->irq_previous & bit)) {
        /* on those two lines the transition can be selected. */
        if ((bit & 0x18) && ((bit >> 1) & tpi_context->c_tpi[TPI_CREG])) {
            set_latch_bit(tpi_context, bit);
            if ((bit & 0x08) && IS_CA_TOGGLE_MODE()) {
                tpi_context->ca_state = 1;
                (tpi_context->set_ca)(tpi_context, 1);
            }
            if ((bit & 0x10) && IS_CB_TOGGLE_MODE()) {
                tpi_context->cb_state = 1;
                (tpi_context->set_cb)(tpi_context, 1);
            }
        }
        tpi_context->irq_previous |= bit;
    } else
    /* check high-low transition */
    if ((!state) && (tpi_context->irq_previous & bit)) {
        /* on those two lines the transition can be selected. */
        if ((bit & 0x18) && !((bit >> 1) & tpi_context->c_tpi[TPI_CREG])) {
            set_latch_bit(tpi_context, bit);
            if ((bit & 0x08) && IS_CA_TOGGLE_MODE()) {
                tpi_context->ca_state = 1;
                (tpi_context->set_ca)(tpi_context, 1);
            }
            if ((bit & 0x10) && IS_CB_TOGGLE_MODE()) {
                tpi_context->cb_state = 1;
                (tpi_context->set_cb)(tpi_context, 1);
            }
        }
        /* those three always trigger at high-low */
        if (bit & 0x07) {
            set_latch_bit(tpi_context, bit);
        }
        tpi_context->irq_previous &= ~bit;
    }
}

void tpicore_restore_int(tpi_context_t *tpi_context, int bit, int state)
{
    if (bit >= 5) {
        return;
    }

    bit = pow2[bit];

    if (state) {
        tpi_context->irq_previous |= bit;
    } else {
        tpi_context->irq_previous &= ~bit;
    }
}

void tpicore_setup_context(tpi_context_t *tpi_context)
{
    tpi_context->irq_previous = 0;
    tpi_context->irq_stack = 0;
    tpi_context->tpi_last_read = 0;
}

void tpicore_shutdown(tpi_context_t *tpi_context)
{
    lib_free(tpi_context->myname);
    lib_free(tpi_context->prv);
    lib_free(tpi_context);
}

/* -------------------------------------------------------------------------- */
/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 0/0
 */

#define TPI_DUMP_VER_MAJOR      1
#define TPI_DUMP_VER_MINOR      0

/*
 * The dump data:
 *
 * UBYTE        PRA     port A output register
 * UBYTE        PRB
 * UBYTE        PRC
 * UBYTE        DDRA    data register A
 * UBYTE        DDRB
 * UBYTE        DDRC
 * UBYTE        CR
 * UBYTE        AIR
 *
 * UBYTE        STACK   irq sources saved on stack
 * UBYTE        CABSTATE state of CA and CB pins
 */

/* FIXME!!!  Error check.  */
int tpicore_snapshot_write_module(tpi_context_t *tpi_context, snapshot_t *p)
{
    snapshot_module_t *m;

    m = snapshot_module_create(p, tpi_context->myname, TPI_DUMP_VER_MAJOR, TPI_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, tpi_context->c_tpi[TPI_PA]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_PB]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_PC]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_DDPA]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_DDPB]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_DDPC]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_CREG]) < 0
        || SMW_B(m, tpi_context->c_tpi[TPI_AIR]) < 0
        || SMW_B(m, tpi_context->irq_stack) < 0
        || SMW_B(m, (BYTE)((tpi_context->ca_state ? 0x80 : 0) | (tpi_context->cb_state ? 0x40 : 0))) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    return snapshot_module_close(m);
}

int tpicore_snapshot_read_module(tpi_context_t *tpi_context, snapshot_t *p)
{
    BYTE vmajor, vminor;
    BYTE byte;
    snapshot_module_t *m;

    (tpi_context->restore_int)(tpi_context->tpi_int_num, 0); /* just in case */

    m = snapshot_module_open(p, tpi_context->myname, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (vmajor > TPI_DUMP_VER_MAJOR || vminor > TPI_DUMP_VER_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_PA])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_PB])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_PC])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_DDPA])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_DDPB])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_DDPC])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_CREG])) < 0
        || SMR_B(m, &(tpi_context->c_tpi[TPI_AIR])) < 0
        || SMR_B(m, &(tpi_context->irq_stack)) < 0
        || SMR_B(m, &byte) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    tpi_context->ca_state = byte & 0x80;
    tpi_context->cb_state = byte & 0x40;

    byte = tpi_context->c_tpi[TPI_PA] | ~(tpi_context->c_tpi[TPI_DDPA]);
    (tpi_context->undump_pa)(tpi_context, byte);
    tpi_context->oldpa = byte;

    byte = tpi_context->c_tpi[TPI_PB] | ~(tpi_context->c_tpi)[TPI_DDPB];
    (tpi_context->undump_pb)(tpi_context, byte);
    tpi_context->oldpb = byte;

    if (!irq_mode) {
        byte = tpi_context->c_tpi[TPI_PC] | ~(tpi_context->c_tpi)[TPI_DDPC];
        (tpi_context->undump_pc)(tpi_context, byte);
        tpi_context->oldpc = byte;
    }

    (tpi_context->set_ca)(tpi_context, tpi_context->ca_state);
    (tpi_context->set_cb)(tpi_context, tpi_context->cb_state);

    (tpi_context->restore_int)(tpi_context->tpi_int_num, irq_active
                               ? tpi_context->irq_line : 0);

    return snapshot_module_close(m);
}

int tpicore_dump(tpi_context_t *tpi_context)
{
    const char *ctrlmodes[4] = {
        "irq", "pulse", "low", "high"
    };
    int mode = tpi_context->c_tpi[TPI_CREG] & 1;

    mon_out("Mode:               %d\n", mode);
    mon_out("Interrupt Priority: %s\n", ((tpi_context->c_tpi[TPI_CREG] >> 1) & 1) ? "enabled" : "disabled");
    mon_out("IRQ 3 Edge Select:  %s\n", ((tpi_context->c_tpi[TPI_CREG] >> 2) & 1) ? "enabled" : "disabled");
    mon_out("IRQ 4 Edge Select:  %s\n", ((tpi_context->c_tpi[TPI_CREG] >> 3) & 1) ? "enabled" : "disabled");
    mon_out("CA Control Mode:    %s\n", ctrlmodes[(tpi_context->c_tpi[TPI_CREG] >> 4) & 3]);
    mon_out("CB Control Mode:    %s\n", ctrlmodes[(tpi_context->c_tpi[TPI_CREG] >> 6) & 3]);
    if (mode) {
        mon_out("Port A:             %02x\n", tpi_context->c_tpi[TPI_PA]);
        mon_out("Port B:             %02x\n", tpi_context->c_tpi[TPI_PB]);
        mon_out("Port Direction A:   %02x\n", tpi_context->c_tpi[TPI_DDPA]);
        mon_out("Port Direction B:   %02x\n", tpi_context->c_tpi[TPI_DDPB]);
        mon_out("Interrupt latch:    %02x\n", irq_latches & 0x1f);
        mon_out("Interrupt active:   %s\n", irq_active ? "yes" : "no");
        mon_out("Active Interrupt:   %02x\n", tpi_context->c_tpi[TPI_AIR]);
    } else {
        mon_out("Port Register A:    %02x\n", tpi_context->c_tpi[TPI_PA]);
        mon_out("Port Register B:    %02x\n", tpi_context->c_tpi[TPI_PB]);
        mon_out("Port Register C:    %02x\n", tpi_context->c_tpi[TPI_PC]);
        mon_out("Port Direction A:   %02x\n", tpi_context->c_tpi[TPI_DDPA]);
        mon_out("Port Direction B:   %02x\n", tpi_context->c_tpi[TPI_DDPB]);
        mon_out("Port Direction C:   %02x\n", tpi_context->c_tpi[TPI_DDPC]);
        mon_out("Active Interrupt:   %02x\n", tpi_context->c_tpi[TPI_AIR]);
    }
    return 0;
}
